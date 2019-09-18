#include "stdafx.h"

#include "GalleryList.h"
#include "../../core_dispatcher.h"
#include "utils/utils.h"
#include "utils/stat_utils.h"
#include "utils/UrlParser.h"
#include "../history_control/complex_message/FileSharingUtils.h"
#include "utils/InterConnector.h"
#include "main_window/MainWindow.h"
#include "fonts.h"
#include "styles/ThemeParameters.h"

#include "ImageVideoList.h"

namespace
{
    int preferredItemSize()
    {
        return Utils::scale_value(96);
    }

    int maxPreviewSize()
    {
        return Utils::scale_bitmap(4. / 3. * preferredItemSize());
    }
}

using namespace Ui;

const auto visitorInterval = 50;

ImageVideoList::ImageVideoList(QWidget* _parent, MediaContentWidget::Type _type, const std::string& _mediaType)
    : MediaContentWidget(_type, _parent)
    , visitor_(new ImageVideoItemVisitor())
    , mediaType_(_mediaType)
{
    setMouseTracking(true);

    visitorTimer_.setSingleShot(true);
    visitorTimer_.setInterval(visitorInterval);
    connect(&visitorTimer_, &QTimer::timeout, this, &ImageVideoList::onVisitorTimeout);

    updateTimer_.setSingleShot(true);
    updateTimer_.setInterval(50);
    connect(&updateTimer_, &QTimer::timeout, this, [this](){ update(updateRegion_); updateRegion_ = QRegion(); });
}

ImageVideoList::~ImageVideoList()
{
}

void ImageVideoList::processItems(const QVector<Data::DialogGalleryEntry>& _entries)
{
    auto lastBlock = blocks_.empty() ? nullptr : blocks_.back().get();

    auto block = lastBlock;

    for (const auto& e : _entries)
    {
        auto messageDate = QDateTime::fromTime_t(e.time_).date();

        if (!block || block->date().month() != messageDate.month() || block->date().year() != messageDate.year())
        {
            if (block && block != lastBlock)
                addBlock(block);

            block = new ImageVideoBlock(this, aimId_);
            block->setDate(messageDate);
        }

        block->addItem(e.url_, e.msg_id_, e.seq_, e.outgoing_, e.sender_, e.time_);
    }

    if (block && block != lastBlock)
        addBlock(block);

    updateHeight();
    update();
}

void ImageVideoList::processUpdates(const QVector<Data::DialogGalleryEntry>& _entries)
{
    for (const auto& e : _entries)
    {
        if (e.action_ == qsl("del"))
        {
            for (auto & b : blocks_)
                b->removeItemByMsg(e.msg_id_);

            auto iter = blocks_.begin();
            while (iter != blocks_.end())
            {
                if (iter->get()->count() == 0)
                    iter = blocks_.erase(iter);
                else
                    ++iter;
            }

            continue;
        }

        ImageVideoBlock* block = nullptr;
        bool found = false;
        auto messageDate = QDateTime::fromTime_t(e.time_).date();
        for (auto& b : blocks_)
        {
            if (b->date().month() != messageDate.month() || b->date().year() != messageDate.year())
                continue;

            block = b.get();
            found = true;
            break;
        }

        if (!block && e.action_ == qsl("add"))
        {
            block = new ImageVideoBlock(this, aimId_);
            block->setDate(messageDate);
        }

        if ((e.type_ == qsl("image") || e.type_ == qsl("video")))
        {
            block->addItem(e.url_, e.msg_id_, e.seq_, e.outgoing_, e.sender_, e.time_);
        }

        if (!found)
            addBlock(block);
    }

    visitor_->setForceLoad();

    updateHeight();
    update();
}

void ImageVideoList::clear()
{
    // TODO: clean memory
    blocks_.clear();
    visitor_->reset();
}

void ImageVideoList::initFor(const QString& _aimId)
{
    aimId_ = _aimId;
    clear();
}

void ImageVideoList::scrolled()
{
    accept(visitor_);
}

void ImageVideoList::accept(ImageVideoItemVisitor* _visitor)
{
    if (!visitorTimer_.isActive())
        visitorTimer_.start();
}

void ImageVideoList::paintEvent(QPaintEvent* _event)
{
    QPainter p(this);

    for (auto & block : blocks_)
    {
        if (block->rect().intersects(_event->rect()))
            block->drawInRect(p, _event->rect());
    }
}

void ImageVideoList::resizeEvent(QResizeEvent* _event)
{
    QWidget::resizeEvent(_event);

    updateHeight(true);
}

void ImageVideoList::mousePressEvent(QMouseEvent* _event)
{
    for (auto & block : blocks_)
    {
        if (block->rect().contains(_event->pos()))
            block->onMousePress(_event->pos(), mediaType_);
    }
    MediaContentWidget::mousePressEvent(_event);
}

void ImageVideoList::mouseReleaseEvent(QMouseEvent* _event)
{
    for (auto & block : blocks_)
    {
        block->onMouseRelease(_event->pos());
    }
    MediaContentWidget::mouseReleaseEvent(_event);
}

void ImageVideoList::mouseMoveEvent(QMouseEvent* _event)
{
    auto handCursor = false;
    for (auto & block : blocks_)
    {
        if (block->rect().contains(_event->pos()))
            handCursor |= block->onMouseMove(_event->pos());
    }

    setCursor(handCursor ? Qt::PointingHandCursor : Qt::ArrowCursor);
}

void ImageVideoList::leaveEvent(QEvent *_event)
{
    for (auto & block : blocks_)
        block->clearHovered();
}

MediaContentWidget::ItemData ImageVideoList::itemAt(const QPoint &_pos)
{
    ItemData data;

    for (auto & block : blocks_)
    {
        if (block->rect().contains(_pos))
        {
            data = block->itemAt(_pos);
            break;
        }
    }

    return data;
}

void ImageVideoList::onClicked(MediaContentWidget::ItemData _data)
{
    Ui::GetDispatcher()->post_stats_to_core(core::stats::stats_event_names::fullmediascr_view, { { "chat_type", Utils::chatTypeByAimId(aimId_) }, { "from", "gallery" }, { "media_type", _data.is_video_ ? "video" : (_data.is_gif_ ? "gif" : "photo") } });
    Utils::InterConnector::instance().getMainWindow()->openGallery(aimId_, _data.link_, _data.msg_);
}

void ImageVideoList::onVisitorTimeout()
{
    visitor_->visit(this);
}

void ImageVideoList::addBlock(ImageVideoBlock *block)
{
    if (block == nullptr)
        return;

    if (blocks_.empty())
    {
        blocks_.push_back(std::unique_ptr<ImageVideoBlock>(block));
    }
    else
    {
        auto iter = blocks_.begin();
        while (iter != blocks_.end())
        {
            auto dt = iter->get()->date();
            if ((dt.month() > block->date().month() && dt.year() == block->date().year()) || dt.year() > block->date().year())
            {
                ++iter;
                continue;
            }

            break;
        }

        blocks_.insert(iter, std::unique_ptr<ImageVideoBlock>(block));
    }

    connect(block, &ImageVideoBlock::needUpdate, this, [this](const QRect _rect)
    {
        updateRegion_ = updateRegion_.united(_rect);

        if (!updateTimer_.isActive())
            updateTimer_.start();
    });
}

void ImageVideoList::updateHeight(bool _force)
{
    int totalHeight = 0;
    for (auto & block : blocks_)
    {
        auto w = width();
        auto blockHeight = block->getHeight(w, _force);

        block->setRect(QRect(0, totalHeight, w, blockHeight));
        totalHeight += blockHeight;
    }

    setFixedHeight(totalHeight);

    accept(visitor_);
}

//////////////////////////////////////////////////////////////////////////

ImageVideoBlock::ImageVideoBlock(QObject* _parent, const QString& _aimid)
    : QObject(_parent)
    , aimid_(_aimid)
{
    static auto dateFont(Fonts::appFontScaled(11, Fonts::FontWeight::SemiBold));
    auto dateLabelUnit = TextRendering::MakeTextUnit(QString());
    dateLabelUnit->init(dateFont, Styling::getParameters().getColor(Styling::StyleVariable::BASE_PRIMARY), QColor(), QColor(), QColor(), TextRendering::HorAligment::LEFT, 1);

    dateLabel_ = std::make_unique<Label>();
    dateLabel_->setTextUnit(std::move(dateLabelUnit));
}

ImageVideoBlock::~ImageVideoBlock()
{

}

void ImageVideoBlock::setDate(const QDate &_date)
{
    date_ = _date;
    auto dateStr = QLocale().standaloneMonthName(date_.month()).toUpper();

    if (date_.year() != QDate::currentDate().year())
        dateStr.append(ql1c(' ') % QString::number(date_.year()));

    dateLabel_->setText(dateStr);
}

QDate ImageVideoBlock::date()
{
    return date_;
}

void ImageVideoBlock::addItem(const QString &_link, qint64 _msg, qint64 _seq, bool _outgoing, const QString& _sender, time_t _time)
{
    auto item = new ImageVideoItem(nullptr, _link, _msg, _seq, _outgoing, _sender, _time);

    connect(item, &ImageVideoItem::needUpdate, this, [this](const QRect& _rect)
    {
        emit needUpdate(_rect);
    });

    auto found = std::find_if(items_.begin(), items_.end(), [_msg, _seq](const auto& i) { return i->getMsg() == _msg && i->getSeq() == _seq; });
    if (found != items_.end())
        return;

    auto iter = items_.cbegin();
    for (; iter != items_.cend(); ++iter)
    {
        auto curMsg = iter->get()->getMsg();
        auto curSeq = iter->get()->getSeq();
        if (curMsg > _msg || (curMsg == _msg && curSeq > _seq))
            continue;

        break;
    }

    items_.insert(iter, std::unique_ptr<ImageVideoItem>(item));

    positionsValid_ = false;
    heightValid_ = false;
}

void ImageVideoBlock::removeItem(qint64 _msg, qint64 _seq)
{
    items_.erase(std::remove_if(items_.begin(), items_.end(), [_msg, _seq](const auto& i) { return i->getMsg() == _msg && i->getSeq() == _seq; }), items_.end());

    positionsValid_ = false;
    heightValid_ = false;
}

void ImageVideoBlock::removeItemByMsg(qint64 _msg)
{
    items_.erase(std::remove_if(items_.begin(), items_.end(), [_msg](const auto& i) { return i->getMsg() == _msg; }), items_.end());

    positionsValid_ = false;
    heightValid_ = false;
}

int ImageVideoBlock::count()
{
    return items_.size();
}

int ImageVideoBlock::getHeight(int _width, bool _force)
{
    if (!heightValid_ || _force)
    {
        auto rowSize = calcRowSize(_width);
        auto nRows = (items_.size() / rowSize + (items_.size() % rowSize == 0 ? 0 : 1));

        cachedHeight_ = nRows * (calcItemSize(_width) + spacing()) + 2 * vMargin() + Utils::scale_value(20);
        heightValid_ = true;
    }
    return cachedHeight_;
}

void ImageVideoBlock::setRect(const QRect &_rect)
{
    if (_rect != rect_)
    {
        Drawable::setRect(_rect);
        positionsValid_ = false;
        heightValid_ = _rect.height() == cachedHeight_;
    }
}

void ImageVideoBlock::drawInRect(QPainter &_p, const QRect& _rect)
{
    if (!positionsValid_)
        updateItemsPositions();

    if (dateLabel_->rect().intersects(_rect))
        dateLabel_->draw(_p);

    for (auto & item : items_)
    {
        if (item->rect().intersects(_rect))
            item->draw(_p);
    }
}

MediaContentWidget::ItemData ImageVideoBlock::itemAt(const QPoint &_pos)
{
    MediaContentWidget::ItemData data;

    for (auto & item : items_)
    {
        if (item->rect().contains(_pos))
        {
            data.link_ = item->getLink();
            data.msg_ = item->getMsg();
            data.sender_ = item->sender();
            data.time_ = item->time();
            data.is_video_ = item->isVideo();
            data.is_gif_ = item->isGif();
            break;
        }
    }

    return data;
}

void ImageVideoBlock::onMousePress(const QPoint& _pos, const std::string& _mediaTYpe)
{
    Utils::GalleryMediaActionCont cont(_mediaTYpe, aimid_);

    for (auto & item : items_)
    {
        const auto overItem = item->rect().contains(_pos);
        if (overItem)
        {
            cont.happened();
            item->setPressed(true);
            emit needUpdate(item->rect());
            break;
        }
    }
}

void ImageVideoBlock::onMouseRelease(const QPoint& _pos)
{
    for (auto & item : items_)
    {
        const auto pressed = item->pressed();
        item->setPressed(false);
        if (pressed)
            emit needUpdate(item->rect());
    }
}

bool ImageVideoBlock::onMouseMove(const QPoint &_pos)
{
    auto overAnyItem = false;
    for (auto & item : items_)
    {
        auto overItem = item->rect().contains(_pos);
        auto needUpdateItem = item->hovered() != overItem;
        item->setHovered(overItem);

        if (item->pressed() && !overItem)
            item->setPressed(false);

        overAnyItem |= overItem;

        if (needUpdateItem)
            emit needUpdate(item->rect());
    }

    return overAnyItem;
}

void ImageVideoBlock::clearHovered()
{
    for (auto & item : items_)
    {
        auto needUpdateItem = item->hovered();
        item->setHovered(false);

        if (needUpdateItem)
            emit needUpdate(item->rect());
    }
}

void ImageVideoBlock::accept(ImageVideoItemVisitor* _visitor)
{
    _visitor->visit(this);
}

void ImageVideoBlock::unload(const QRect& _rect, ViewportChangeDirection _direction)
{
    auto it = std::lower_bound(items_.begin(), items_.end(), _rect.top(), [_direction](const auto & _item, auto value)
    {
        if (_direction == ViewportChangeDirection::Up)
            return _item->rect().top() < value;
        else
            return _item->rect().bottom() < value;
    });

    while (it != items_.end())
    {
        auto isEnd = false;

        if (_direction == ViewportChangeDirection::Down)
            isEnd = (*it)->rect().bottom() > _rect.bottom();
        else
            isEnd = (*it)->rect().top() > _rect.bottom();

        if (isEnd) break;

        (*it)->unload();

        ++it;
    }
}

void ImageVideoBlock::load(QRect _rect, ViewportChangeDirection _direction)
{
    if (!positionsValid_)
        updateItemsPositions();

    if (_direction == ViewportChangeDirection::Down)
        _rect.setTop(std::max(0, _rect.top() - itemSize_));

    auto it = std::lower_bound(items_.begin(), items_.end(), _rect.top(), [](const auto & _item, auto value)
    {
        return _item->rect().bottom() < value;
    });

    while (it != items_.end())
    {
        if ((*it)->rect().top() > _rect.bottom()) break;

        (*it)->load();
        ++it;
    }
}

QRect ImageVideoBlock::itemRect(int _index)
{
    itemSize_ = calcItemSize(rect_.width());

    auto rowSize = calcRowSize(rect_.width());

    auto row = _index / rowSize;
    auto column = _index % rowSize;

    auto topMargin = dateRect().height() + vMargin();

    auto topleft = QPoint(column * itemSize_ + column * spacing() + hMargin(), row * itemSize_ + row * spacing() + topMargin);

    return QRect(topleft + rect_.topLeft(), QSize(itemSize_, itemSize_));
}

QRect ImageVideoBlock::dateRect()
{
    return QRect(rect_.topLeft() + QPoint(hMargin(), vMargin()), QSize(rect_.width() - 2 * hMargin(), Utils::scale_value(20)));
}

void ImageVideoBlock::updateItemsPositions()
{
    for (auto i = 0u; i < items_.size(); ++i)
        items_[i]->setRect(itemRect(i));

    dateLabel_->setRect(dateRect());

    positionsValid_ = true;
}

int ImageVideoBlock::calcItemSize(int _width)
{
    static const auto preferredSize = Utils::scale_value(96);

    auto n = calcRowSizeHelper(_width, preferredSize);

    return (_width - 2 * hMargin() - spacing() * (n - 1)) / n;
}

int ImageVideoBlock::calcRowSize(int _width)
{
    return calcRowSizeHelper(_width, calcItemSize(_width));
}

int ImageVideoBlock::calcRowSizeHelper(int _width, int _itemSize)
{
    return (_width - 2 * hMargin() + spacing())/ (_itemSize + spacing());
}

int ImageVideoBlock::calcHeight()
{
    return 0;
}

int ImageVideoBlock::hMargin()
{
    return Utils::scale_value(16);
}

int ImageVideoBlock::vMargin()
{
    return Utils::scale_value(8);
}

int ImageVideoBlock::spacing()
{
    return Utils::scale_value(4);
}

//////////////////////////////////////////////////////////////////////////

const auto previewLoadDelay = 100;

ImageVideoItem::ImageVideoItem(QObject* _parent, const QString& _link, qint64 _msg, qint64 _seq, bool _outgoing, const QString& _sender, time_t _time)
    : QObject(_parent)
    , msg_(_msg)
    , seq_(_seq)
    , link_(_link)
    , outgoing_(_outgoing)
    , sender_(_sender)
    , time_(_time)
{
    Utils::UrlParser parser;
    parser.process(QStringRef(&_link));

    filesharing_ = parser.hasUrl() && parser.getUrl().is_filesharing();

    if (filesharing_)
        loader_.reset(new Utils::FileSharingLoader(_link, Utils::MediaLoader::LoadMeta::OnlyForVideo));
    else
        loader_.reset(new Utils::CommonMediaLoader(_link, Utils::MediaLoader::LoadMeta::OnlyForVideo));

    connect(loader_.get(), &Utils::MediaLoader::previewLoaded, this, &ImageVideoItem::onPreviewLoaded);


    loadDelayTimer_.setSingleShot(true);
    loadDelayTimer_.setInterval(previewLoadDelay);

    connect(&loadDelayTimer_, &QTimer::timeout, this, [this](){ loader_->loadPreview(); });
}

ImageVideoItem::~ImageVideoItem()
{

}

void ImageVideoItem::load(LoadType _loadType)
{
    if (previewPtr_ || loadDelayTimer_.isActive())
        return;

    if (_loadType == LoadType::Immediate)
        loader_->loadPreview();
    else
        loadDelayTimer_.start();
}

void ImageVideoItem::unload()
{
    loadDelayTimer_.stop();
    if (loader_->isLoading())
        loader_->cancelPreview();

    auto task = new ResetPreviewTask(std::move(previewPtr_));
    QThreadPool::globalInstance()->start(task);
    previewPtr_ = nullptr;               // without async reset it takes up to 0.3ms
}

void ImageVideoItem::draw(QPainter &_p)
{
    if (rect_.size() != cachedSize_)
    {
        updatePreview();
        updateDurationLabel();
    }

    Utils::PainterSaver ps(_p);
    _p.translate(rect_.topLeft());
    _p.setRenderHint(QPainter::Antialiasing);

    static const auto r = Utils::scale_value(4);

    auto localRect = QRect({0, 0}, rect_.size());

    QPainterPath path;
    path.addRoundedRect(localRect, r, r);

    _p.setClipPath(path);

    if (previewPtr_ && !previewPtr_->scaled_.isNull())
        _p.drawPixmap(localRect, previewPtr_->scaled_);

    if (pressed_)
        _p.fillPath(path, QColor(0, 0, 0, 0.12 * 255));
    else if (hovered_)
        _p.fillPath(path, QColor(0, 0, 0, 0.08 * 255));
    else if (previewPtr_ && !previewPtr_->scaled_.isNull())
        _p.fillPath(path, QColor(0, 0, 0, 0.03 * 255));
    else
        _p.fillPath(path, Styling::getParameters().getColor(Styling::StyleVariable::BASE_BRIGHT_INVERSE));

    if (loader_ && (loader_->isVideo() || loader_->isGif()) && previewPtr_ && !previewPtr_->scaled_.isNull())
    {
        static auto gradientHeight = Utils::scale_value(28);
        QLinearGradient gradient(0, rect_.height() - gradientHeight, 0, rect_.height());

        gradient.setColorAt(0, QColor(0, 0, 0, 0));
        gradient.setColorAt(1, QColor(0, 0, 0, 0.37 * 255));

        _p.fillRect(QRect(0, rect_.height() - gradientHeight, rect_.width(), gradientHeight), gradient);

        if (durationLabel_)
            durationLabel_->draw(_p);
    }
}

void ImageVideoItem::cancelLoading()
{
    loader_->cancelPreview();
}

void ImageVideoItem::onPreviewLoaded(const QPixmap& _preview)
{
    previewPtr_ = std::make_unique<Preview>();
    previewPtr_->source_ = cropPreview(_preview, maxPreviewSize());

    updatePreview();

    if (loader_ && (loader_->isVideo() || loader_->isGif()))
    {
        QString durationStr;
        if (loader_->isVideo())
        {
            QTime t(0, 0);
            t = t.addSecs(loader_->duration());
            if (t.hour())
                durationStr += QString::number(t.hour()) % ql1c(':');
            durationStr += QString::number(t.minute()) % ql1c(':');
            if (t.second() < 10)
                durationStr += ql1c('0');
            durationStr += QString::number(t.second());
        }
        else
        {
            durationStr = QT_TRANSLATE_NOOP("contact_list", "GIF");
        }

        const auto durationFont(Fonts::appFontScaled(13, Fonts::FontWeight::Normal));
        durationLabel_ = TextRendering::MakeTextUnit(durationStr);
        durationLabel_->init(durationFont, Styling::getParameters().getColor(Styling::StyleVariable::TEXT_SOLID_PERMANENT), QColor(), QColor(), QColor(), TextRendering::HorAligment::LEFT, 1);

        updateDurationLabel();
     }

    emit needUpdate(rect_);
}

QPixmap ImageVideoItem::cropPreview(const QPixmap &_source, const int _toSize)
{
    auto ret = _source;

    if (_source.width() > _source.height())
    {
        ret = _source.scaledToHeight(_toSize, Qt::SmoothTransformation);
        ret = ret.copy((ret.width() - _toSize) / 2, 0, _toSize, _toSize);
    }
    else
    {
        ret = _source.scaledToWidth(_toSize, Qt::SmoothTransformation).copy(0, 0, _toSize, _toSize);
    }

    return ret;
}

void ImageVideoItem::updatePreview()
{
    if (!previewPtr_)
        return;

    previewPtr_->scaled_ = previewPtr_->source_.scaledToHeight(Utils::scale_bitmap(rect_.height()), Qt::SmoothTransformation);
    cachedSize_ = rect_.size();
}

void ImageVideoItem::updateDurationLabel()
{
    if (!durationLabel_)
        return;

    int w = durationLabel_->desiredWidth();
    int h = durationLabel_->getHeight(w);
    static const auto rightOffset = Utils::scale_value(4);

    durationLabel_->setOffsets(rect_.width() - w - rightOffset, rect_.height() - h);
}

ImageVideoItemVisitor::ImageVideoItemVisitor(QObject* _parent)
    : QObject(_parent)
{
}

const auto preloadOffset = 1.5;

void ImageVideoItemVisitor::visit(ImageVideoList* _list)
{
    prevHeight_ = height_;
    height_ = _list->height();
    prevVisibleRect_ = visibleRect_;
    visibleRect_ = _list->visibleRegion().boundingRect();

    int pageOffset = visibleRect_.height() * preloadOffset;

    visibleRect_.adjust(0, -pageOffset , 0,  pageOffset);
    visibleRect_.setBottom(std::min(visibleRect_.bottom(), _list->height()));
    visibleRect_.setTop(std::max(0, visibleRect_.top()));

    auto & blocks = _list->blocks();

    QRegion region;
    region = region.united(prevVisibleRect_);
    region = region.united(visibleRect_);

    for (auto & rect : region.rects())
    {
        auto firstBlock = std::lower_bound(blocks.begin(), blocks.end(), rect.top(), [](const auto & _block, auto value)
        {
            return _block->rect().bottom() < value;
        });

        if (firstBlock == blocks.end())
            return;

        auto it = firstBlock;

        while (it != blocks.end())
        {
            if ((*it)->rect().top() > rect.bottom())
                break;

            (*it)->accept(this);
            it++;
        }
    }

    forceLoad_ = false;
}

void ImageVideoItemVisitor::visit(ImageVideoBlock* _block)
{
    QRect loadRect = visibleRect_;
    QRect unloadRect = prevVisibleRect_;

    ImageVideoBlock::ViewportChangeDirection direction = ImageVideoBlock::ViewportChangeDirection::None;

    QRect intersection = visibleRect_.intersected(prevVisibleRect_);

    if (visibleRect_.width() != prevVisibleRect_.width())
    {
        auto heightDiff = prevHeight_ - height_;
        if (heightDiff > 0)
        {
            unloadRect = visibleRect_;
            unloadRect.setTop(visibleRect_.top() - heightDiff);
            unloadRect.setHeight(heightDiff);
            direction = ImageVideoBlock::ViewportChangeDirection::Down;
        }
        else
        {
            unloadRect = visibleRect_;
            unloadRect.setTop(visibleRect_.bottom());
            unloadRect.setHeight(-heightDiff);
            direction = ImageVideoBlock::ViewportChangeDirection::Up;
        }
    }
    else if (intersection.height() != 0)
    {
        QRegion intersectionRegion;
        intersectionRegion = intersectionRegion.united(intersection);

        QRegion loadRegion;
        loadRegion = loadRegion.united(loadRect);
        loadRegion = loadRegion.subtracted(intersectionRegion);

        QRegion unloadRegion;
        unloadRegion = unloadRegion.united(unloadRect);
        unloadRegion = unloadRegion.subtracted(intersectionRegion);

        if (!forceLoad_)
        {
            if (loadRegion.rectCount())
                loadRect = loadRegion.rects().first();
            else
                loadRect = QRect();
        }

        if (unloadRegion.rectCount())
            unloadRect = unloadRegion.rects().first();
        else
            unloadRect = QRect();

        if (visibleRect_.top() < prevVisibleRect_.top())
            direction = ImageVideoBlock::ViewportChangeDirection::Up;
        else
            direction = ImageVideoBlock::ViewportChangeDirection::Down;
    }

    if (unloadRect.height() > 0)
        _block->unload(unloadRect, direction);

    if (loadRect.height() > 0)
        _block->load(loadRect, direction);
}

void ImageVideoItemVisitor::reset()
{
    visibleRect_ = QRect();
    prevVisibleRect_ = QRect();
}

void ResetPreviewTask::run()
{
    preview_.reset();
}
