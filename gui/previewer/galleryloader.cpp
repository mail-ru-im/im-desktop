#include "stdafx.h"

#include "../core_dispatcher.h"
#include "utils/UrlParser.h"
#include "utils/medialoader.h"
#include "utils/utils.h"
#include "main_window/mplayer/VideoPlayer.h"

#include "ImageViewerWidget.h"

#include "galleryloader.h"

#include "../core_dispatcher.h"

using namespace Previewer;

GalleryLoader::GalleryLoader(const QString &_aimId, const QString &_link, int64_t _msgId, Ui::DialogPlayer* _attachedPlayer)
    : aimId_(_aimId)
    , frontLoadSeq_(-1)
    , backLoadSeq_(-1)
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

    items_.clear();

    aimId_ = _aimId;

    items_.push_back(createItem(_link, _msgId, 0, 0, QString(), QString(), _attachedPlayer));
    current_ = 0;

    firstItemSeq_ = Ui::GetDispatcher()->getDialogGalleryByMsg(aimId_, QStringList() << qsl("image") << qsl("video"), _msgId);
    initialLink_ = _link;

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
                items_.push_front(createItem(entry.url_, entry.msg_id_, entry.seq_, entry.time_, entry.sender_, entry.caption_));
            if (_seq == backLoadSeq_)
                items_.push_back(createItem(entry.url_, entry.msg_id_, entry.seq_, entry.time_, entry.sender_, entry.caption_));

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

        emit contentUpdated();
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
            break;
        }
    }

    loadMore();

    emit itemUpdated();
    emit contentUpdated();
}

void GalleryLoader::dialogGalleryState(const QString& _aimId, const Data::DialogGalleryState& _state)
{
    if (aimId_ != _aimId)
        return;

    emit itemUpdated();
    emit contentUpdated();
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
        if (e.msg_id_ == currentMsgId && e.action_ == ql1s("add") && e.url_ == current->link())
        {
            auto galleryItem = qobject_cast<GalleryItem*>(current);
            if (galleryItem)
            {
                galleryItem->setSeq(e.seq_);
                galleryItem->setTime(e.time_);
                galleryItem->setSender(e.sender_);
                galleryItem->setCaption(e.caption_);
            }
            continue;
        }

        auto iter = items_.begin();
        while (iter != items_.end())
        {
            if (!iter->get() || ((e.action_ == qsl("del") || e.action_ == qsl("edit")) && iter->get()->msg() == e.msg_id_))
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

    emit itemUpdated();
    emit contentUpdated();
}

void GalleryLoader::dialogGalleryInit(const QString& _aimId)
{
    if (aimId_ != _aimId)
        return;

    if (items_.size() == 1)
    {
        firstItemSeq_ = Ui::GetDispatcher()->getDialogGalleryByMsg(aimId_, QStringList() << qsl("image") << qsl("video"), items_.front()->msg());
        return;
    }

    loadMore();

    emit itemUpdated();
    emit contentUpdated();
}

void GalleryLoader::dialogGalleryHoles(const QString& _aimId)
{
    if (aimId_ != _aimId)
        return;

    auto current = currentItem();
    if (current)
        Ui::GetDispatcher()->getDialogGalleryIndex(_aimId, QStringList() << qsl("image") << qsl("video"), current->msg(), current->seq());
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
            emit itemUpdated();
            emit contentUpdated();
        }
        else
        {
            Ui::GetDispatcher()->getDialogGalleryIndex(_aimId, QStringList() << qsl("image") << qsl("video"), current->msg(), current->seq());
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
        emit mediaLoaded();
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
        emit previewLoaded();
    }
}

void GalleryLoader::onItemError(const QString &_link, int64_t _msgId)
{
    auto item = static_cast<GalleryItem*>(currentItem());

    if (!item)
        return;

    if (item->link() == _link && item->msg() == _msgId)
    {
        emit error();
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

    auto current = static_cast<GalleryItem*>(currentItem());
    if (current && !current->path().isEmpty())
    {
        emit mediaLoaded();
    }
    else if (current && !current->preview().isNull())
    {
        emit previewLoaded();
        current->loadFullMedia();
    }
    else
    {
        current->load();
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
    return Ui::GetDispatcher()->getDialogGallery(aimId_, QStringList() << qsl("image") << qsl("video"), _after, _seq, _count);
}

void GalleryLoader::unload()
{
    if (current_ > 2 * loadDistance())
    {
        items_.erase(items_.begin(), items_.begin() + current_ - 2 * loadDistance());
        current_ -= current_ - 2 * loadDistance();
        exhaustedFront_ = false;
    }

    if (items_.size() - current_ - 1 > 2 * loadDistance())
    {
        items_.erase(items_.end() - (items_.size() - current_ - 1 - 2 * loadDistance()), items_.end());
        exhaustedBack_ = false;
    }
}

std::unique_ptr<GalleryItem> GalleryLoader::createItem(const QString &_link, qint64 _msg, qint64 _seq, time_t _time, const QString& _sender, const QString& _caption, Ui::DialogPlayer* _attachedPlayer)
{
    auto item = std::make_unique<GalleryItem>(_link, _msg, _seq, _time, _sender, aimId_, _caption, _attachedPlayer);
    if (_attachedPlayer != nullptr)
        item->setPath(_attachedPlayer->mediaPath());

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
    , attachedPlayer_(_attachedPlayer)
    , attachedPlayerShown_(false)
    , aimid_(_aimid)
    , caption_(_caption)
{
    Utils::UrlParser parser;
    parser.process(QStringRef(&_link));

    auto loadMetaType = attachedPlayer_ ? Utils::MediaLoader::LoadMeta::BeforePreview : Utils::MediaLoader::LoadMeta::BeforeMedia;

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

QString GalleryItem::fileName() const
{
    auto fileName = loader_->fileName();
    if (!fileName.isEmpty())
        return fileName;

    Utils::UrlParser parser;
    parser.process(QStringRef(&link_));
    if (!parser.getUrl().is_filesharing())
        return QUrl(link_).fileName();

    auto filePath = path_;

    if (attachedPlayer_)
        filePath = attachedPlayer_->mediaPath();

    return QFileInfo(filePath).fileName();
}

bool GalleryItem::isVideo() const
{
    if (loader_)
        return loader_->isVideo() || loader_->isGif();;

    return false;
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

    _viewer->showMedia(mediaData);
}

void GalleryItem::showPreview(Previewer::ImageViewerWidget *_viewer)
{
    _viewer->showPixmap(preview_, originSize_, isVideo());
}

void GalleryItem::save(const QString &_path)
{
    const QFileInfo info(_path);

    const auto suffix = info.suffix();

    const auto dot = suffix.isEmpty() ? QString() : qsl(".");

    const auto name = QStringRef(&_path).left(_path.size() - suffix.size() - (suffix.isEmpty() ? 0 : 1));

    auto resultPath = _path;

    int counter = 1;
    while (QFileInfo::exists(resultPath))
    {
        resultPath = name % qsl("(%1)").arg(counter) % dot % suffix;

        if (++counter == std::numeric_limits<int>::max())
            break;
    }

    Utils::UrlParser parser;
    parser.process(QStringRef(&link_));

    auto saver = new Utils::FileSaver(this);
    saver->save([this, resultPath](bool _success)
    {
        if (_success)
            emit saved(resultPath);
        else
            emit saveError();
    }, link_, resultPath);
}

void GalleryItem::copyToClipboard()
{
    Utils::copyFileToClipboard(path_);
}

void GalleryItem::onFileLoaded(const QString& _path)
{
    path_ = _path;
    emit loaded(link_, msg_);
}

void GalleryItem::onPreviewLoaded(const QPixmap& _preview, const QSize& _originSize)
{
    preview_ = _preview;
    originSize_ = _originSize;
    if (!attachedPlayer_)
        emit previewLoaded(link_, msg_);
}

void GalleryItem::onItemError()
{
    if (preview_.isNull())
        emit error(link_, msg_);
}
