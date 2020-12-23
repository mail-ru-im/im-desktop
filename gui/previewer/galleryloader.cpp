#include "stdafx.h"

#include "../core_dispatcher.h"
#include "utils/InterConnector.h"
#include "utils/UrlParser.h"
#include "utils/medialoader.h"
#include "utils/utils.h"
#ifndef STRIP_AV_MEDIA
#include "main_window/mplayer/VideoPlayer.h"
#endif // !STRIP_AV_MEDIA

#include "ImageViewerWidget.h"

#include "galleryloader.h"

#include "../core_dispatcher.h"

using namespace Previewer;

GalleryLoader::GalleryLoader(const Utils::GalleryData& _data)
    : aimId_(_data.aimId_)
    , frontLoadSeq_(-1)
    , backLoadSeq_(-1)
    , isFirstItemInitialized_(false)
    , exhaustedFront_(false)
    , exhaustedBack_(false)
    , index_(-1)
    , total_(-1)
{
    connect(Ui::GetDispatcher(), &Ui::core_dispatcher::dialogGalleryResult, this, &GalleryLoader::dialogGalleryResult);
    connect(Ui::GetDispatcher(), &Ui::core_dispatcher::dialogGalleryByMsgResult, this, &GalleryLoader::dialogGalleryResultByMsg);
    connect(Ui::GetDispatcher(), &Ui::core_dispatcher::dialogGalleryState, this, &GalleryLoader::dialogGalleryState);
    connect(Ui::GetDispatcher(), &Ui::core_dispatcher::dialogGalleryUpdate, this, &GalleryLoader::dialogGalleryUpdate);
    connect(Ui::GetDispatcher(), &Ui::core_dispatcher::dialogGalleryInit, this, &GalleryLoader::dialogGalleryInit);
    connect(Ui::GetDispatcher(), &Ui::core_dispatcher::dialogGalleryHolesDownloaded, this, &GalleryLoader::dialogGalleryHoles);
    connect(Ui::GetDispatcher(), &Ui::core_dispatcher::dialogGalleryHolesDownloading, this, &GalleryLoader::dialogGalleryHoles);
    connect(Ui::GetDispatcher(), &Ui::core_dispatcher::dialogGalleryIndex, this, &GalleryLoader::dialogGalleryIndex);

    items_.push_back(createItem(_data.link_, _data.msgId_, 0, 0, QString(), QString(), _data.attachedPlayer_, _data.preview_, _data.originalSize_));
    current_ = 0;

    firstItemSeq_ = Ui::GetDispatcher()->getDialogGalleryByMsg(aimId_, { qsl("image"), qsl("video") }, _data.msgId_);
    initialLink_ = _data.link_;

    frontLoadSeq_ = -1;
    backLoadSeq_ = -1;

    Ui::GetDispatcher()->requestDialogGalleryState(aimId_);
}

void GalleryLoader::dialogGalleryResult(const int64_t _seq, const QVector<Data::DialogGalleryEntry> &_entries, bool _exhausted)
{
    if(_seq == frontLoadSeq_ || _seq == backLoadSeq_)
    {
        auto count = 0;
        for (auto & entry : _entries)
        {
            if (_seq == frontLoadSeq_)
            {
                if (!items_.empty())
                {
                    const auto& front = items_.front();
                    if (front.get()->msg() == entry.msg_id_ && front.get()->seq() == entry.seq_)
                        continue;
                }
                items_.push_front(createItem(entry.url_, entry.msg_id_, entry.seq_, entry.time_, entry.sender_, entry.caption_));
            }
            if (_seq == backLoadSeq_)
            {
                if (!items_.empty())
                {
                    const auto& back = items_.back();
                    if (back.get()->msg() == entry.msg_id_ && back.get()->seq() == entry.seq_)
                        continue;
                }
                items_.push_back(createItem(entry.url_, entry.msg_id_, entry.seq_, entry.time_, entry.sender_, entry.caption_));
            }

            count++;
        }

        if (_seq == frontLoadSeq_)
        {
            exhaustedFront_ = _exhausted;
            current_ += count;
            frontLoadSeq_ = -1;
        }
        else
        {
            exhaustedBack_ = _exhausted;
            backLoadSeq_ = -1;
        }

        Q_EMIT contentUpdated();
    }
}

void GalleryLoader::dialogGalleryResultByMsg(const int64_t _seq, const QVector<Data::DialogGalleryEntry> &_entries, int _index, int _total)
{
    if (_seq != firstItemSeq_ || _entries.empty())
        return;

    index_ = _index;
    total_ = _total;
    auto & firstItem = items_.front();
    for (const auto& e : _entries)
    {
        ++index_;
        if (e.msg_id_ == firstItem->msg() && e.url_ == firstItem->link())
        {
            firstItem->setSeq(e.seq_);
            firstItem->setTime(e.time_);
            firstItem->setSender(e.sender_);
            firstItem->setCaption(e.caption_);
            isFirstItemInitialized_ = true;
            break;
        }
    }

    loadMore();

    Q_EMIT itemUpdated();
    Q_EMIT contentUpdated();
}

void GalleryLoader::dialogGalleryState(const QString& _aimId, const Data::DialogGalleryState& _state)
{
    if (aimId_ != _aimId)
        return;

    Q_EMIT itemUpdated();
    Q_EMIT contentUpdated();
}

void GalleryLoader::dialogGalleryUpdate(const QString& _aimId, const QVector<Data::DialogGalleryEntry>& _entries)
{
    if (aimId_ != _aimId)
        return;

    auto current = currentItem();
    if (!current)
        return;

    auto currentMsgId = current->msg();
    for (const auto& e : _entries)
    {
        if (e.msg_id_ == currentMsgId && e.action_ == u"add" && e.url_ == current->link())
        {
            auto galleryItem = qobject_cast<GalleryItem*>(current);
            if (galleryItem)
            {
                galleryItem->setSeq(e.seq_);
                galleryItem->setTime(e.time_);
                galleryItem->setSender(e.sender_);
                galleryItem->setCaption(e.caption_);

                if (galleryItem == items_.front().get())
                {
                    assert(!isFirstItemInitialized_);
                    isFirstItemInitialized_ = true;
                }
            }
            continue;
        }

        auto iter = items_.begin();
        while (iter != items_.end())
        {
            if (!iter->get() || ((e.action_ == u"del" || e.action_ == u"edit") && iter->get()->msg() == e.msg_id_ && hasPrev()))
            {
                if (e.msg_id_ <= currentMsgId)
                {
                    --current_;
                    --index_;
                }

                iter = items_.erase(iter);
            }
            else
            {
                ++iter;
            }
        }
    }
    exhaustedFront_ = false;
    exhaustedBack_ = false;
    loadMore();

    Q_EMIT itemUpdated();
    Q_EMIT contentUpdated();
}

void GalleryLoader::dialogGalleryInit(const QString& _aimId)
{
    if (aimId_ != _aimId)
        return;

    if (items_.size() == 1)
    {
        firstItemSeq_ = Ui::GetDispatcher()->getDialogGalleryByMsg(aimId_, { qsl("image"), qsl("video") }, items_.front()->msg());
        return;
    }

    loadMore();

    Q_EMIT itemUpdated();
    Q_EMIT contentUpdated();
}

void GalleryLoader::dialogGalleryHoles(const QString& _aimId)
{
    if (aimId_ != _aimId)
        return;

    auto current = currentItem();
    if (current)
    {
        if (!isFirstItemInitialized_ && current == items_.front().get())
        {
            Ui::GetDispatcher()->getDialogGalleryByMsg(_aimId, { qsl("image"), qsl("video") }, current->msg());
        }
        Ui::GetDispatcher()->getDialogGalleryIndex(_aimId, { qsl("image"), qsl("video") }, current->msg(), current->seq());
    }
}

void GalleryLoader::dialogGalleryIndex(const QString& _aimId, qint64 _msg, qint64 _seq, int _index, int _total)
{
    if (aimId_ != _aimId)
        return;

    auto current = currentItem();
    if (current)
    {
        if (current->msg() == _msg && current->seq() == _seq)
        {
            index_ = _index;
            total_ = _total;
            Q_EMIT itemUpdated();
            Q_EMIT contentUpdated();
        }
        else
        {
            Ui::GetDispatcher()->getDialogGalleryIndex(_aimId, { qsl("image"), qsl("video") }, current->msg(), current->seq());
        }
    }
}

void GalleryLoader::next()
{
    move(Direction::Next);
}

void GalleryLoader::prev()
{
    move(Direction::Previous);
}

bool GalleryLoader::hasNext()
{
    return items_.size() - current_ > 1;
}

bool GalleryLoader::hasPrev()
{
    return current_ > 0;
}

ContentItem *Previewer::GalleryLoader::currentItem()
{
    if (!indexValid())
        return nullptr;

    return items_[current_].get();
}

int GalleryLoader::currentIndex() const
{
    return index_;
}

int GalleryLoader::totalCount() const
{
    return total_;
}

void GalleryLoader::cancelCurrentItemLoading()
{
    auto item = static_cast<GalleryItem*>(currentItem());

    if (!item)
        return;

    item->cancelLoading();
}

void GalleryLoader::startCurrentItemLoading()
{
    auto item = static_cast<GalleryItem*>(currentItem());

    if (!item)
        return;

    item->loadFullMedia();
}

void GalleryLoader::clear()
{
    items_.clear();
    firstItemSeq_ = -1;
    backLoadSeq_ = -1;
    frontLoadSeq_ = -1;
}

void GalleryLoader::onItemLoaded(const QString& _link, int64_t _msgId)
{
    auto item = static_cast<GalleryItem*>(currentItem());

    if (!item)
        return;

    if (item->link() == _link && item->msg() == _msgId)
    {
        Q_EMIT mediaLoaded();
    }
}

void GalleryLoader::onPreviewLoaded(const QString &_link, int64_t _msgId)
{
    auto item = static_cast<GalleryItem*>(currentItem());

    if (!item)
        return;

    if (item->link() == _link && item->msg() == _msgId)
    {
        item->loadFullMedia();
        Q_EMIT previewLoaded();
    }
}

void GalleryLoader::onItemError(const QString &_link, int64_t _msgId)
{
    auto item = static_cast<GalleryItem*>(currentItem());

    if (!item)
        return;

    if (item->link() == _link && item->msg() == _msgId)
    {
        Q_EMIT error();
    }
}

void GalleryLoader::move(Previewer::GalleryLoader::Direction _direction)
{
    if (_direction == Direction::Previous)
    {
        if (!hasPrev())
            return;

        current_--;
        index_--;
    }
    else
    {
        if (!hasNext())
            return;

        current_++;
        index_++;
    }

    if (auto current = static_cast<GalleryItem*>(currentItem()))
    {
        if (!current->path().isEmpty())
        {
            Q_EMIT mediaLoaded();
        }
        else if (!current->preview().isNull())
        {
            Q_EMIT previewLoaded();
            current->loadFullMedia();
        }
        else
        {
            current->load();
        }
    }

    loadMore();
}

void GalleryLoader::loadMore()
{
    if (items_.empty())
    {
        frontLoadSeq_ = loadItems(0, 0, loadDistance());
        return;
    }

    if (current_ < loadDistance() && frontLoadSeq_ == -1 && !exhaustedFront_)
        frontLoadSeq_ = loadItems(items_.front()->msg(), items_.front()->seq(), loadDistance());

    if (!indexValid())
        return;

    if (items_.size() - current_ < loadDistance() && backLoadSeq_ == -1 && !exhaustedBack_)
        backLoadSeq_ = loadItems(items_.back()->msg(), items_.back()->seq(), -int(loadDistance()));

    unload();
}

int64_t GalleryLoader::loadItems(int64_t _after, int64_t _seq, int _count)
{
    return Ui::GetDispatcher()->getDialogGallery(aimId_, { qsl("image"), qsl("video") }, _after, _seq, _count, true);
}

void GalleryLoader::unload()
{
    if (current_ > 2 * loadDistance())
    {
        const auto unloadCount = current_ - 2 * loadDistance();
        items_.erase(items_.begin(), items_.begin() + unloadCount);
        current_ -= unloadCount;
        exhaustedFront_ = false;
    }

    if (items_.size() - current_ - 1 > 2 * loadDistance())
    {
        items_.erase(items_.end() - (items_.size() - current_ - 1 - 2 * loadDistance()), items_.end());
        exhaustedBack_ = false;
    }
}

std::unique_ptr<GalleryItem> GalleryLoader::createItem(const QString &_link, qint64 _msg, qint64 _seq, time_t _time, const QString& _sender, const QString& _caption, Ui::DialogPlayer* _attachedPlayer, QPixmap _preview, QSize _originalSize)
{
    auto item = std::make_unique<GalleryItem>(_link, _msg, _seq, _time, _sender, aimId_, _caption, _attachedPlayer);
#ifndef STRIP_AV_MEDIA
    if (_attachedPlayer != nullptr)
        item->setPath(_attachedPlayer->mediaPath());
#endif // !STRIP_AV_MEDIA
    if (!_preview.isNull())
        item->setPreview(std::move(_preview), _originalSize);

    connect(item.get(), &GalleryItem::loaded, this, &GalleryLoader::onItemLoaded);
    connect(item.get(), &GalleryItem::previewLoaded, this, &GalleryLoader::onPreviewLoaded);
    connect(item.get(), &GalleryItem::saved, this, &GalleryLoader::itemSaved);
    connect(item.get(), &GalleryItem::saveError, this, &GalleryLoader::itemSaveError);
    connect(item.get(), &GalleryItem::error, this, &GalleryLoader::onItemError);
    return item;
}

bool GalleryLoader::indexValid()
{
    return current_ < items_.size();
}

GalleryItem::GalleryItem(const QString &_link, qint64 _msg, qint64 _seq, time_t _time, const QString& _sender, const QString& _aimid, const QString& _caption, Ui::DialogPlayer* _attachedPlayer)
    : link_(_link)
    , msg_(_msg)
    , seq_(_seq)
    , time_(_time)
    , sender_(_sender)
#ifndef STRIP_AV_MEDIA
    , attachedPlayer_(_attachedPlayer)
#endif // !STRIP_AV_MEDIA
    , attachedPlayerShown_(false)
    , aimid_(_aimid)
    , caption_(_caption)
{
    Utils::UrlParser parser;
    parser.process(QStringRef(&_link));

    const auto loadMetaType =
#ifndef STRIP_AV_MEDIA
        attachedPlayer_ ? Utils::MediaLoader::LoadMeta::BeforePreview :
#endif // !STRIP_AV_MEDIA
        Utils::MediaLoader::LoadMeta::BeforeMedia;

    if (parser.hasUrl() && parser.getUrl().is_filesharing())
        loader_.reset(new Utils::FileSharingLoader(_link, loadMetaType));
    else
        loader_.reset(new Utils::CommonMediaLoader(_link, loadMetaType));

    connect(loader_.get(), &Utils::MediaLoader::fileLoaded, this, &GalleryItem::onFileLoaded);
    connect(loader_.get(), &Utils::MediaLoader::previewLoaded, this, &GalleryItem::onPreviewLoaded);
    connect(loader_.get(), &Utils::MediaLoader::error, this, &GalleryItem::onItemError);

    load();
}

void GalleryItem::load()
{
    loader_->loadPreview();
}

GalleryItem::~GalleryItem() = default;

QString GalleryItem::formatFileName(const QString& fileName) const
{
    const QString suffix = QFileInfo(fileName).suffix();
    if (suffix.isEmpty())
        return fileName % u'.' % (loader_->isGif() ? QStringView(u"gif") : loader_->fileFormat());
    return fileName;
}

QString GalleryItem::fileName() const
{
    auto fileName = loader_->fileName();
    if (!fileName.isEmpty())
    {
        fileName = formatFileName(fileName);
        return fileName;
    }

    const auto& link = loader_->downloadUri();

    Utils::UrlParser parser;
    parser.process(link);
    if (!parser.getUrl().is_filesharing())
        return formatFileName(QUrl(link).fileName());

    auto filePath = path_;
#ifndef STRIP_AV_MEDIA
    if (attachedPlayer_)
        filePath = attachedPlayer_->mediaPath();
#endif // !STRIP_AV_MEDIA
    return QFileInfo(filePath).fileName();
}

void GalleryItem::setPreview(QPixmap _preview, QSize _originalSize)
{
    preview_ = std::move(_preview);
    originSize_ = _originalSize;
}

bool GalleryItem::isVideo() const
{
    return loader_ && (loader_->isVideo() || loader_->isGif());
}

void GalleryItem::loadFullMedia()
{
    if (loader_)
        loader_->load();
}

void GalleryItem::cancelLoading()
{
    if (loader_)
        loader_->cancel();
}

void GalleryItem::showMedia(Previewer::ImageViewerWidget *_viewer)
{
    MediaData mediaData;
    mediaData.preview = preview_;
    mediaData.fileName = path_;
#ifndef STRIP_AV_MEDIA
    if (isVideo())
    {
        if (attachedPlayerShown_)
            attachedPlayer_ = nullptr;
        if (!attachedPlayer_)
            showPreview(_viewer);
        mediaData.attachedPlayer = attachedPlayer_;
        mediaData.gotAudio = loader_->gotAudio();
        attachedPlayerShown_ = true;
    }
#endif // !STRIP_AV_MEDIA

    _viewer->showMedia(mediaData);
}

void GalleryItem::showPreview(Previewer::ImageViewerWidget *_viewer)
{
    _viewer->showPixmap(preview_, originSize_, isVideo());
}

void GalleryItem::save(const QString &_path, bool _exportAsPng)
{
    const auto suffix = QFileInfo(_path).suffix();

    const auto dot = suffix.isEmpty() ? QStringView() : u".";

    const auto name = QStringRef(&_path).left(_path.size() - suffix.size() - (suffix.isEmpty() ? 0 : 1));

    auto resultPath = _path;

    int counter = 1;
    while (QFileInfo::exists(resultPath))
    {
        resultPath = name % qsl("(%1)").arg(counter) % dot % suffix;

        if (++counter == std::numeric_limits<int>::max())
            break;
    }

    const auto& resolvedLink = !QUrl(loader_->downloadUri()).isValid() ? link_ : loader_->downloadUri();

    auto saver = new Utils::FileSaver(this);
    saver->save([this, resultPath](bool _success, const QString& _savedPath)
    {
        Q_UNUSED(_savedPath)

        if (_success)
            Q_EMIT saved(resultPath);
        else
            Q_EMIT saveError();
    }, resolvedLink, resultPath, _exportAsPng);
}

void GalleryItem::copyToClipboard()
{
    Utils::copyFileToClipboard(path_);
}

void GalleryItem::onFileLoaded(const QString& _path)
{
    path_ = _path;
    Q_EMIT loaded(link_, msg_);
}

void GalleryItem::onPreviewLoaded(const QPixmap& _preview, const QSize& _originSize)
{
    preview_ = _preview;
    originSize_ = _originSize;
#ifndef STRIP_AV_MEDIA
    if (!attachedPlayer_)
#endif // !STRIP_AV_MEDIA
        Q_EMIT previewLoaded(link_, msg_);
}

void GalleryItem::onItemError()
{
    if (preview_.isNull())
        Q_EMIT error(link_, msg_);
}
