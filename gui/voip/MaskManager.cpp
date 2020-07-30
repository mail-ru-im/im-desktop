#include "stdafx.h"

#include "../core_dispatcher.h"
#include "../utils/gui_coll_helper.h"
#include "types/masks.h"

#include "MaskManager.h"

Q_LOGGING_CATEGORY(maskManager, "maskManager")

namespace
{
    constexpr std::chrono::milliseconds updateTimeout = std::chrono::hours(24);
    constexpr std::chrono::milliseconds retryUpdateTimeout = std::chrono::seconds(1);
}

voip_masks::MaskManager::MaskManager(Ui::core_dispatcher& _dispatcher, QObject* _parent)
    : QObject(_parent)
    , dispatcher_(_dispatcher)
{
    connect(&dispatcher_, &Ui::core_dispatcher::im_created, this, &MaskManager::onImCreated, Qt::QueuedConnection);
    connect(&dispatcher_, &Ui::core_dispatcher::maskListLoaded, this, &MaskManager::onMaskListLoaded, Qt::QueuedConnection);
    connect(&dispatcher_, &Ui::core_dispatcher::maskPreviewLoaded, this, &MaskManager::onPreviewLoaded, Qt::QueuedConnection);
    connect(&dispatcher_, &Ui::core_dispatcher::maskModelLoaded, this, &MaskManager::onModelLoaded, Qt::QueuedConnection);
    connect(&dispatcher_, &Ui::core_dispatcher::maskLoaded, this, &MaskManager::onLoaded, Qt::QueuedConnection);
    connect(&dispatcher_, &Ui::core_dispatcher::maskRetryUpdate, this, &MaskManager::onRetryUpdate, Qt::QueuedConnection);
    connect(&dispatcher_, &Ui::core_dispatcher::existentMasksLoaded, this, &MaskManager::onExistentMasksLoaded, Qt::QueuedConnection);

    updateTimer_ = new QTimer(this);
    updateTimer_->setTimerType(Qt::VeryCoarseTimer);
    connect(updateTimer_, &QTimer::timeout, this, &MaskManager::onUpdateTimeout);
    connect(updateTimer_, &QTimer::timeout, updateTimer_, &QTimer::stop);
}

const voip_masks::MaskList& voip_masks::MaskManager::getAvailableMasks() const
{
    return masks_;
}

void voip_masks::MaskManager::loadMask(const QString &_id)
{
    Mask* mask = masksIndex_[_id];

    if (!mask)
        return;

    const auto seq = postMessageToCore("masks/get", _id);
    mask->setProgressSeq(seq);
    maskSeqList_[seq] = mask;
}

void voip_masks::MaskManager::onImCreated()
{
    loadMaskList();
}

void voip_masks::MaskManager::onMaskListLoaded(const QVector<QString>& maskList)
{
    MaskList tmp;
    for (const auto& id :maskList)
    {
        auto mask = new Mask(id, this);
        connect(&dispatcher_, &Ui::core_dispatcher::maskLoadingProgress, mask, &Mask::onLoadingProgress, Qt::QueuedConnection);
        tmp.push_back(mask);

        masksIndex_[id] = mask;
    }
    masks_.swap(tmp);
    currentDownloadPreviewList = masks_;

    qDeleteAll(tmp.begin(), tmp.end());

    previewSeqList_.clear();
    maskSeqList_.clear();

    loadPreviews();
}

void voip_masks::MaskManager::onPreviewLoaded(qint64 _seq, const QString& _localPath)
{
    qCDebug(maskManager) << "load preview " << _localPath;
    const auto it = previewSeqList_.find(_seq);
    if (it != previewSeqList_.end())
    {
        if (*it)
            (*it)->setPreview(_localPath);
        previewSeqList_.erase(it);
    }

    loadNextPreview();

    if (previewSeqList_.empty())
        loadModel();
}

void voip_masks::MaskManager::onModelLoaded()
{
    if (!existentListLoaded_)
        loadExistentMasks();

    updateTimer_->start(updateTimeout);
}

void voip_masks::MaskManager::onLoaded(qint64 _seq, const QString& _localPath)
{
    qCDebug(maskManager) << "load mask " << _localPath;
    const auto it = maskSeqList_.find(_seq);
    if (it != maskSeqList_.end())
    {
        if (*it)
            (*it)->setJsonPath(_localPath);
        maskSeqList_.erase(it);
    }
}

void voip_masks::MaskManager::onUpdateTimeout()
{
    loadMaskList();
}

void voip_masks::MaskManager::onRetryUpdate()
{
    qDeleteAll(masks_.begin(), masks_.end());
    masks_.clear();
    masksIndex_.clear();
    currentDownloadPreviewList.clear();

    previewSeqList_.clear();
    maskSeqList_.clear();

    updateTimer_->start(retryUpdateTimeout);
}

void voip_masks::MaskManager::onExistentMasksLoaded(const std::vector<Data::Mask> &_masks)
{
    for (const auto& m : _masks)
    {
        auto & mask = masksIndex_[m.name_];
        if (mask)
            mask->setJsonPath(m.json_path_);
    }
    existentListLoaded_ = true;
}

void voip_masks::MaskManager::loadMaskList()
{
    qCDebug(maskManager) << "load mask list";
    Ui::gui_coll_helper collection(dispatcher_.create_collection(), true);
    dispatcher_.post_message_to_core("masks/get_id_list", collection.get());
}

void voip_masks::MaskManager::loadPreviews()
{
    loadNextPreview();
}

void voip_masks::MaskManager::loadModel()
{
    Ui::gui_coll_helper collection(dispatcher_.create_collection(), true);
    dispatcher_.post_message_to_core("masks/model/get", collection.get());
}

void voip_masks::MaskManager::loadExistentMasks()
{
    Ui::gui_coll_helper collection(dispatcher_.create_collection(), true);
    dispatcher_.post_message_to_core("masks/get/existent", collection.get());
}

void voip_masks::MaskManager::loadNextPreview()
{
    qCDebug(maskManager) << "try to load preview. list size " << currentDownloadPreviewList.size();
    if (currentDownloadPreviewList.empty())
        return;

    const auto seq = postMessageToCore("masks/preview/get", currentDownloadPreviewList.back()->id());
    previewSeqList_[seq] = currentDownloadPreviewList.back();
    currentDownloadPreviewList.pop_back();
}

qint64 voip_masks::MaskManager::postMessageToCore(std::string_view _message, const QString& _maskId) const
{
    Ui::gui_coll_helper collection(dispatcher_.create_collection(), true);
    collection.set_value_as_qstring("mask_id", _maskId);
    return dispatcher_.post_message_to_core(_message, collection.get());
}
