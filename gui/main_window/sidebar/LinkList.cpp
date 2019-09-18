#include "stdafx.h"

#include "LinkList.h"
#include "SidebarUtils.h"
#include "../../core_dispatcher.h"
#include "../../controls/TextUnit.h"
#include "../../controls/ContextMenu.h"
#include "../../fonts.h"
#include "../../utils/utils.h"
#include "utils/stat_utils.h"
#include "../../my_info.h"
#include "../GroupChatOperations.h"
#include "../friendly/FriendlyContainer.h"
#include "../contact_list/ContactListModel.h"
#include "../../styles/ThemeParameters.h"

namespace
{
    const int VER_OFFSET = 12;
    const int HOR_OFFSET = 16;
    const int RIGHT_OFFSET = 40;
    const int MORE_BUTTON_SIZE = 20;
    const int PREVIEW_WIDTH = 48;
    const int PREVIEW_HEIGHT = 48;
    const int PREVIEW_RIGHT_OFFSET = 12;
    const int TITLE_OFFSET = 4;
    const int DESC_OFFSET = 8;
    const int DATE_OFFSET = 4;
    const int DATE_TOP_OFFSET = 12;
    const int DATE_BOTTOM_OFFSET = 8;
    const int LOADING_RANGE = 100;
    const int DATE_LEFT_OFFSET = 4;

    const std::string MediaTypeName("Links");

    QPixmap rounded(const QPixmap& _source)
    {
        if (_source.isNull())
            return _source;

        QPixmap result(_source.size());
        result.fill(Qt::transparent);

        QPainter p(&result);
        p.setRenderHint(QPainter::Antialiasing);

        const qreal radius(Utils::scale_bitmap_with_value(4));
        QPainterPath path;
        path.addRoundedRect(result.rect(), radius, radius);
        p.setClipPath(path);

        p.drawPixmap(QPoint(), _source);

        return result;
    }

    QPixmap scaled(const QPixmap& _source, int _size)
    {
        if (_source.width() == _source.height())
            return _source.scaled(QSize(_size, _size), Qt::IgnoreAspectRatio, Qt::SmoothTransformation);

        auto p = _source;
        if (_source.width() > _source.height())
            p = p.scaledToHeight(_size, Qt::SmoothTransformation);
        else
            p = p.scaledToWidth(_size, Qt::SmoothTransformation);

        return p.copy((p.width() - _size) / 2, (p.height() - _size) / 2, _size, _size);
    }
}

namespace Ui
{
    DateLinkItem::DateLinkItem(time_t _time, qint64 _msg, qint64 _seq)
        : time_(_time)
        , msg_(_msg)
        , seq_(_seq)
    {
        auto dt = QDateTime::fromTime_t(_time);
        auto date = QLocale().standaloneMonthName(dt.date().month());

        date_ = Ui::TextRendering::MakeTextUnit(date.toUpper());
        date_->init(Fonts::appFontScaled(11, Fonts::FontWeight::SemiBold), Styling::getParameters().getColor(Styling::StyleVariable::BASE_PRIMARY));
        date_->evaluateDesiredSize();
    }


    qint64 DateLinkItem::getMsg() const
    {
        return msg_;
    }

    qint64 DateLinkItem::getSeq() const
    {
        return seq_;
    }

    time_t DateLinkItem::time() const
    {
        return time_;
    }

    void DateLinkItem::draw(QPainter& _p, const QRect& _rect)
    {
        date_->setOffsets(Utils::scale_value(HOR_OFFSET), Utils::scale_value(DATE_TOP_OFFSET) + _rect.y());
        date_->draw(_p);
    }

    int DateLinkItem::getHeight() const
    {
        return Utils::scale_value(DATE_BOTTOM_OFFSET + DATE_TOP_OFFSET) + date_->cachedSize().height();
    }

    LinkItem::LinkItem(const QString& _url, const QString& _date, qint64 _msg, qint64 _seq, bool _outgoing, const QString& _sender, time_t _time, int _width)
        : url_(_url)
        , height_(0)
        , width_(0)
        , msg_(_msg)
        , seq_(_seq)
        , outgoing_(_outgoing)
        , reqId_(-1)
        , sender_(_sender)
        , time_(_time)
        , loaded_(false)
    {
        title_ = Ui::TextRendering::MakeTextUnit(QString(), Data::MentionMap(), TextRendering::LinksVisible::DONT_SHOW_LINKS);
        title_->init(Fonts::appFontScaled(14, Fonts::FontWeight::SemiBold), Styling::getParameters().getColor(Styling::StyleVariable::TEXT_SOLID), QColor(), QColor(), QColor(), TextRendering::HorAligment::LEFT, 1);

        desc_ = Ui::TextRendering::MakeTextUnit(QString(), Data::MentionMap(), TextRendering::LinksVisible::DONT_SHOW_LINKS);
        desc_->init(Fonts::appFontScaled(14), Styling::getParameters().getColor(Styling::StyleVariable::TEXT_SOLID), QColor(), QColor(), QColor(), TextRendering::HorAligment::LEFT, 3);

        link_ = Ui::TextRendering::MakeTextUnit(url_, Data::MentionMap(), TextRendering::LinksVisible::DONT_SHOW_LINKS);
        link_->init(Fonts::appFontScaled(14), Styling::getParameters().getColor(Styling::StyleVariable::BASE_PRIMARY), QColor(), QColor(), QColor(), TextRendering::HorAligment::LEFT, 1);
        height_ = link_->getHeight(_width - Utils::scale_value(HOR_OFFSET + RIGHT_OFFSET) - Utils::scale_value(PREVIEW_WIDTH) - Utils::scale_value(PREVIEW_RIGHT_OFFSET));

        date_ = Ui::TextRendering::MakeTextUnit(_date, Data::MentionMap(), TextRendering::LinksVisible::DONT_SHOW_LINKS);
        date_->init(Fonts::appFontScaled(13), Styling::getParameters().getColor(Styling::StyleVariable::BASE_SECONDARY));
        date_->evaluateDesiredSize();

        friedly_ = Ui::TextRendering::MakeTextUnit(Logic::GetFriendlyContainer()->getFriendly(sender_), Data::MentionMap(), TextRendering::LinksVisible::DONT_SHOW_LINKS);
        friedly_->init(Fonts::appFontScaled(13), Styling::getParameters().getColor(Styling::StyleVariable::BASE_SECONDARY));
        friedly_->evaluateDesiredSize();

        height_ += date_->cachedSize().height();
        height_ += Utils::scale_value(DATE_OFFSET);
        height_ += Utils::scale_value(VER_OFFSET) * 2;

        height_ = std::max(height_, Utils::scale_value(PREVIEW_HEIGHT + VER_OFFSET * 2));

        static auto icon = Utils::renderSvgLayered(qsl(":/gallery/placeholder_link"),
                                {
                                    { qsl("bg"), Styling::getParameters().getColor(Styling::StyleVariable::BASE_BRIGHT) },
                                    { qsl("icon"), Styling::getParameters().getColor(Styling::StyleVariable::BASE_SECONDARY) },
                                },
                                Utils::scale_value(QSize(PREVIEW_WIDTH, PREVIEW_WIDTH)));
        icon.setDevicePixelRatio(1);
        preview_ = rounded(scaled(icon, Utils::scale_value(PREVIEW_WIDTH) * Utils::scale_bitmap(1)));
        Utils::check_pixel_ratio(preview_);

        setMoreButtonState(ButtonState::HIDDEN);
    }

    void LinkItem::draw(QPainter& _p, const QRect& _rect)
    {
        _p.drawPixmap(Utils::scale_value(HOR_OFFSET), Utils::scale_value(VER_OFFSET) + _rect.y(), preview_);
        if (!more_.isNull())
            _p.drawPixmap(width_ - Utils::scale_value(RIGHT_OFFSET) / 2 - Utils::scale_value(MORE_BUTTON_SIZE) / 2, height_ / 2 - Utils::scale_value(MORE_BUTTON_SIZE) / 2 + _rect.y(), more_);
        auto offset = Utils::scale_value(VER_OFFSET) - Utils::scale_value(2);

        title_->setOffsets(Utils::scale_value(HOR_OFFSET + PREVIEW_WIDTH + PREVIEW_RIGHT_OFFSET), offset + _rect.y());
        title_->draw(_p);

        desc_->setOffsets(Utils::scale_value(HOR_OFFSET + PREVIEW_WIDTH + PREVIEW_RIGHT_OFFSET), Utils::scale_value(TITLE_OFFSET) + offset + title_->cachedSize().height() + _rect.y());
        desc_->draw(_p);

        offset -= Utils::scale_value(2);
        auto link_offset = offset + _rect.y();
        if (!title_->getText().isEmpty())
            link_offset += Utils::scale_value(TITLE_OFFSET) + title_->cachedSize().height();
        if (!desc_->getText().isEmpty())
            link_offset += Utils::scale_value(DESC_OFFSET) + desc_->cachedSize().height();

        link_->setOffsets(Utils::scale_value(HOR_OFFSET + PREVIEW_WIDTH + PREVIEW_RIGHT_OFFSET), link_offset);
        link_->draw(_p);

        auto date_offset = link_offset + link_->cachedSize().height() + Utils::scale_value(DATE_OFFSET);

        friedly_->setOffsets(Utils::scale_value(HOR_OFFSET + PREVIEW_WIDTH + PREVIEW_RIGHT_OFFSET), date_offset);
        friedly_->draw(_p);

        date_->setOffsets(width_ - Utils::scale_value(RIGHT_OFFSET) - date_->cachedSize().width(), date_offset);
        date_->draw(_p);
    }

    int LinkItem::getHeight() const
    {
        return height_;
    }

    void LinkItem::setWidth(int _width)
    {
        height_ = title_->getHeight(_width - Utils::scale_value(HOR_OFFSET + RIGHT_OFFSET) - Utils::scale_value(PREVIEW_WIDTH) - Utils::scale_value(PREVIEW_RIGHT_OFFSET));
        height_ += desc_->getHeight(_width - Utils::scale_value(HOR_OFFSET + RIGHT_OFFSET) - Utils::scale_value(PREVIEW_WIDTH) - Utils::scale_value(PREVIEW_RIGHT_OFFSET));
        height_ += link_->getHeight(_width - Utils::scale_value(HOR_OFFSET + RIGHT_OFFSET) - Utils::scale_value(PREVIEW_WIDTH) - Utils::scale_value(PREVIEW_RIGHT_OFFSET));
        height_ += date_->cachedSize().height();
        if (!title_->getText().isEmpty())
            height_ += Utils::scale_value(TITLE_OFFSET);
        if (!desc_->getText().isEmpty())
            height_ += Utils::scale_value(DESC_OFFSET);
        height_ -= Utils::scale_value(4);
        height_ += Utils::scale_value(DATE_OFFSET);

        height_ = std::max(height_, Utils::scale_value(PREVIEW_HEIGHT));
        height_ += Utils::scale_value(VER_OFFSET) * 2;
        width_ = _width;

        if (friedly_)
            friedly_->elide(width_ - Utils::scale_value(HOR_OFFSET + RIGHT_OFFSET + PREVIEW_WIDTH + PREVIEW_RIGHT_OFFSET + DATE_LEFT_OFFSET) - date_->cachedSize().width());
    }

    void LinkItem::setReqId(qint64 _id)
    {
        reqId_ = _id;
    }

    qint64 LinkItem::reqId() const
    {
        return reqId_;
    }

    QString LinkItem::getLink() const
    {
        return url_;
    }

    qint64 LinkItem::getMsg() const
    {
        return msg_;
    }

    qint64 LinkItem::getSeq() const
    {
        return seq_;
    }

    QString LinkItem::sender() const
    {
        return sender_;
    }

    time_t LinkItem::time() const
    {
        return time_;
    }

    void LinkItem::setPreview(const QPixmap& _preview)
    {
        preview_ = rounded(scaled(_preview, Utils::scale_value(PREVIEW_WIDTH) * Utils::scale_bitmap(1)));
        Utils::check_pixel_ratio(preview_);
    }

    bool LinkItem::isOverLink(const QPoint& _pos, int _totalHeight) const
    {
        auto r = QRect(Utils::scale_value(HOR_OFFSET), Utils::scale_value(VER_OFFSET) + _totalHeight, Utils::scale_value(PREVIEW_WIDTH), Utils::scale_value(PREVIEW_HEIGHT));
        return isOverLink(_pos) || title_->contains(_pos) || desc_->contains(_pos) || r.contains(_pos);
    }

    bool LinkItem::isOverLink(const QPoint& _pos) const
    {
        return link_->contains(_pos);
    }

    void LinkItem::underline(bool _enabled)
    {
        link_->setUnderline(_enabled);
    }

    bool LinkItem::isOverDate(const QPoint& _pos) const
    {
        return date_->contains(_pos);
    }

    void LinkItem::setDateState(bool _hover, bool _active)
    {
        if (_hover)
            date_->setColor(Styling::getParameters().getColor(Styling::StyleVariable::BASE_SECONDARY_HOVER));
        else if(_active)
            date_->setColor(Styling::getParameters().getColor(Styling::StyleVariable::BASE_SECONDARY_ACTIVE));
        else
            date_->setColor(Styling::getParameters().getColor(Styling::StyleVariable::BASE_SECONDARY));
    }

    void LinkItem::setTitle(const QString& _title)
    {
        title_->setText(_title);
        setWidth(width_);
        loaded_ = true;
    }

    void LinkItem::setDesc(const QString& _desc)
    {
        desc_->setText(_desc);
        setWidth(width_);
    }

    bool LinkItem::isOutgoing() const
    {
        return outgoing_;
    }

    bool LinkItem::loaded() const
    {
        return loaded_;
    }

    void LinkItem::setMoreButtonState(const ButtonState& _state)
    {
        moreState_ = _state;
        if (_state == ButtonState::HIDDEN)
        {
            more_ = QPixmap();
            return;
        }

        QColor color;
        switch (_state)
        {
        case ButtonState::NORMAL:
            color = Styling::getParameters().getColor(Styling::StyleVariable::BASE_SECONDARY);
            break;
        case ButtonState::HOVERED:
            color = Styling::getParameters().getColor(Styling::StyleVariable::BASE_SECONDARY_HOVER);
            break;
        case ButtonState::PRESSED:
            color = Styling::getParameters().getColor(Styling::StyleVariable::BASE_SECONDARY_ACTIVE);
            break;
        default:
            break;
        }

        more_ = Utils::renderSvgScaled(qsl(":/controls/more_vertical"), QSize(MORE_BUTTON_SIZE, MORE_BUTTON_SIZE), color);
    }

    bool LinkItem::isOverMoreButton(const QPoint& _pos, int _h) const
    {
        auto r = QRect(width_ - Utils::scale_value(RIGHT_OFFSET) / 2 - Utils::scale_value(MORE_BUTTON_SIZE) / 2, _h + height_ / 2 - Utils::scale_value(MORE_BUTTON_SIZE) / 2, Utils::scale_value(MORE_BUTTON_SIZE), Utils::scale_value(MORE_BUTTON_SIZE));
        return r.contains(_pos);
    }

    ButtonState LinkItem::moreButtonState() const
    {
        return moreState_;
    }

    LinkList::LinkList(QWidget* _parent)
        : MediaContentWidget(Type::Links, _parent)
        , timer_(new QTimer(this))
    {
        connect(Ui::GetDispatcher(), &core_dispatcher::linkMetainfoImageDownloaded, this, &LinkList::onImageDownloaded);
        connect(Ui::GetDispatcher(), &core_dispatcher::linkMetainfoMetaDownloaded, this, &LinkList::onMetaDownloaded);
        setMouseTracking(true);

        timer_->setSingleShot(true);
        timer_->setInterval(5000);
        connect(timer_, &QTimer::timeout, this, &LinkList::onTimer);
    }

    void LinkList::initFor(const QString& _aimId)
    {
        MediaContentWidget::initFor(_aimId);
        Items_.clear();
        RequestIds_.clear();
        setFixedHeight(0);
    }

    void LinkList::processItems(const QVector<Data::DialogGalleryEntry>& _entries)
    {
        auto h = height();
        for (const auto& e : _entries)
        {
            auto time = QDateTime::fromTime_t(e.time_);
            auto item = std::make_unique<LinkItem>(e.url_, time.toString(qsl("d MMM, ")) + time.time().toString(qsl("hh:mm")), e.msg_id_, e.seq_, e.outgoing_, e.sender_, e.time_, width());
            h += item->getHeight();

            auto reqId = GetDispatcher()->downloadLinkMetainfo(aimId_, e.url_, 0, 0);
            item->setReqId(reqId);
            RequestIds_.push_back(reqId);
            RequestedUrls_.push_back(e.url_);
            Items_.push_back(std::move(item));
        }
        setFixedHeight(h);
        validateDates();
    }

    void LinkList::processUpdates(const QVector<Data::DialogGalleryEntry>& _entries)
    {
        auto h = height();
        for (const auto& e : _entries)
        {
            if (e.action_ == qsl("del"))
            {
                Items_.erase(std::remove_if(Items_.begin(), Items_.end(), [e](const auto& i) { return i->getMsg() == e.msg_id_; }), Items_.end());
                h = 0;
                for (const auto& i : Items_)
                    h += i->getHeight();

                continue;
            }

            if (e.type_ != qsl("link"))
            {
                continue;
            }

            auto time = QDateTime::fromTime_t(e.time_);

            auto iter = Items_.cbegin();
            for (; iter != Items_.cend(); ++iter)
            {
                if (iter->get()->isDateItem())
                {
                    auto dt = QDateTime::fromTime_t(iter->get()->time());
                    if (dt.date().month() == time.date().month() && dt.date().year() == time.date().year())
                        continue;
                }

                auto curMsg = iter->get()->getMsg();
                auto curSeq = iter->get()->getSeq();
                if (curMsg > e.msg_id_ || (curMsg == e.msg_id_ && curSeq > e.seq_))
                    continue;

                break;
            }

            auto reqId = GetDispatcher()->downloadLinkMetainfo(aimId_, e.url_, 0, 0);
            auto item = std::make_unique<LinkItem>(e.url_, time.toString(qsl("d MMM, ")) + time.time().toString(qsl("hh:mm")), e.msg_id_, e.seq_, e.outgoing_, e.sender_, e.time_, width());
            h += item->getHeight();

            RequestIds_.push_back(reqId);
            item->setReqId(reqId);

            Items_.insert(iter, std::move(item));
        }
        setFixedHeight(h);
        validateDates();
    }

    void LinkList::scrolled()
    {
        if (!timer_->isActive())
            timer_->start();
    }

    void LinkList::paintEvent(QPaintEvent* e)
    {
        QPainter p(this);
        auto h = 0;

        const auto visibleRect = visibleRegion().boundingRect();
        for (const auto& i : Items_)
        {
            auto r = QRect(0, h, width(), i->getHeight());
            if (visibleRect.intersects(r))
                i->draw(p, r);

            h += i->getHeight();
        }
    }

    MediaContentWidget::ItemData LinkList::itemAt(const QPoint& _pos)
    {
        auto h = 0;
        for (const auto& i : Items_)
        {
            auto r = QRect(0, h, width(), i->getHeight());
            if (r.contains(_pos))
            {
                return ItemData(i->getLink(), i->getMsg(), i->sender(), i->time());
            }
            h += i->getHeight();
        }

        return ItemData();
    }

    void LinkList::mousePressEvent(QMouseEvent* _event)
    {
        auto h = 0;
        for (auto& i : Items_)
        {
            if (i->isOverDate(_event->pos()))
            {
                i->setDateState(false, true);
            }
            else
            {
                i->setDateState(false, false);
            }

            if (i->isOverMoreButton(_event->pos(), h))
            {
                i->setMoreButtonState(ButtonState::PRESSED);
            }

            h += i->getHeight();
        }

        update();
        pos_ = _event->pos();
        MediaContentWidget::mousePressEvent(_event);
    }

    void LinkList::mouseReleaseEvent(QMouseEvent* _event)
    {
        Utils::GalleryMediaActionCont cont(MediaTypeName, aimId_);

        if (_event->button() == Qt::RightButton)
        {
            cont.happened();
            return MediaContentWidget::mouseReleaseEvent(_event);
        }

        auto h = 0;
        for (const auto& i : Items_)
        {
            if (Utils::clicked(pos_, _event->pos()))
            {
                cont.happened();

                if (i->isOverLink(_event->pos(), h))
                {
                    Utils::UrlParser parser;
                    parser.process(i->getLink());
                    if (parser.hasUrl())
                    {
                        auto url = parser.getUrl();
                        if (url.is_email())
                            return Utils::openUrl(ql1s("mailto:") + QString::fromStdString(url.url_));

                        Utils::openUrl(QString::fromStdString(url.url_));
                        return;
                    }
                }
                if (i->isOverDate(_event->pos()))
                {
                    emit Logic::getContactListModel()->select(aimId_, i->getMsg(), i->getMsg(), Logic::UpdateChatSelection::No);
                    i->setDateState(true, false);
                }
                else
                {
                    i->setDateState(false, false);
                }
            }

            if (i->moreButtonState() == ButtonState::PRESSED)
            {
                cont.happened();

                if (i->isOverMoreButton(_event->pos(), h))
                    i->setMoreButtonState(ButtonState::HOVERED);
                else
                    i->setMoreButtonState(ButtonState::NORMAL);

                if (Utils::clicked(pos_, _event->pos()))
                    showContextMenu(ItemData(i->getLink(), i->getMsg(), i->sender(), i->time()), _event->pos(), true);
            }

            h += i->getHeight();
        }
    }

    void LinkList::resizeEvent(QResizeEvent* _event)
    {
        for (auto& i : Items_)
            i->setWidth(_event->size().width());

        MediaContentWidget::resizeEvent(_event);
    }

    void LinkList::mouseMoveEvent(QMouseEvent* _event)
    {
        auto point = false;
        auto h = 0;
        for (auto& i : Items_)
        {
            if (i->isOverLink(_event->pos(), h))
            {
                point = true;
                if (i->isOverLink(_event->pos()))
                    i->underline(true);
            }
            else
            {
                i->underline(false);
            }

            if (i->isOverDate(_event->pos()))
            {
                point = true;
                i->setDateState(true, false);
            }
            else
            {
                i->setDateState(false, false);
            }

            if (i->isOverMoreButton(_event->pos(), h))
            {
                i->setMoreButtonState(ButtonState::HOVERED);
                point = true;
            }
            else
            {
                auto r = QRect(0, h, width(), i->getHeight());
                if (r.contains(_event->pos()))
                    i->setMoreButtonState(ButtonState::NORMAL);
                else
                    i->setMoreButtonState(ButtonState::HIDDEN);
            }

            h += i->getHeight();
        }

        setCursor(point ? Qt::PointingHandCursor : Qt::ArrowCursor);
        update();

        MediaContentWidget::mouseMoveEvent(_event);
    }

    void LinkList::leaveEvent(QEvent* _event)
    {
        for (auto& i : Items_)
            i->setMoreButtonState(ButtonState::HIDDEN);

        update();
        MediaContentWidget::leaveEvent(_event);
    }

    void LinkList::onImageDownloaded(int64_t _seq, bool _success, const QPixmap& _image)
    {
        if (!_success || std::find(RequestIds_.begin(), RequestIds_.end(),_seq) == RequestIds_.end())
            return;

        for (auto& i : Items_)
        {
            if (i->reqId() == _seq)
            {
                i->setPreview(_image);
                break;
            }
        }
        update();
    }

    void LinkList::onMetaDownloaded(int64_t _seq, bool _success, const Data::LinkMetadata& _meta)
    {
        if (!_success || std::find(RequestIds_.begin(), RequestIds_.end(), _seq) == RequestIds_.end())
            return;

        for (auto& i : Items_)
        {
            if (i->reqId() == _seq)
            {
                i->setTitle(_meta.getTitle());
                i->setDesc(_meta.getDescription());
                RequestedUrls_.erase(std::remove(RequestedUrls_.begin(), RequestedUrls_.end(), i->getLink()), RequestedUrls_.end());
                break;
            }
        }

        auto h = 0;
        for (const auto& i : Items_)
            h += i->getHeight();
        setFixedHeight(h);

        update();
    }

    void LinkList::onTimer()
    {
        auto loadingRect = visibleRegion().boundingRect();
        loadingRect.setY(std::max(0, loadingRect.y() - Utils::scale_value(LOADING_RANGE / 2)));
        loadingRect.setHeight(loadingRect.height() + Utils::scale_value(LOADING_RANGE));

        auto h = 0;
        for (auto& i : Items_)
        {
            auto url = i->getLink();
            auto r = QRect(0, h, width(), i->getHeight());
            if (!url.isEmpty() && !i->loaded())
            {
                if (!loadingRect.intersects(r))
                {
                    if (std::find(RequestedUrls_.begin(), RequestedUrls_.end(), url) != RequestedUrls_.end())
                    {
                        Ui::GetDispatcher()->cancelLoaderTask(url);
                        RequestedUrls_.erase(std::remove(RequestedUrls_.begin(), RequestedUrls_.end(), url), RequestedUrls_.end());
                    }
                }
                else
                {
                    if (std::find(RequestedUrls_.begin(), RequestedUrls_.end(), url) == RequestedUrls_.end())
                    {
                        auto reqId = GetDispatcher()->downloadLinkMetainfo(aimId_, url, 0, 0);
                        i->setReqId(reqId);
                        RequestedUrls_.push_back(url);
                        RequestIds_.push_back(reqId);
                    }
                }
            }
            h += i->getHeight();
        }
    }

    void LinkList::validateDates()
    {
        if (Items_.empty())
            return;

        auto h = height();

        if (!Items_.front()->isDateItem())
        {
            Items_.emplace(Items_.begin(), new DateLinkItem(Items_.front()->time(), Items_.front()->getMsg(), Items_.front()->getSeq()));
            h += Items_.front()->getHeight();
        }

        auto iter = Items_.begin();
        auto prevIsDate = iter->get()->isDateItem();
        auto prevDt = QDateTime::fromTime_t(iter->get()->time()).date();
        ++iter;

        while (iter != Items_.end())
        {
            auto isDate = iter->get()->isDateItem();
            auto dt = QDateTime::fromTime_t(iter->get()->time()).date();
            if ((dt.month() == prevDt.month() && dt.year() == prevDt.year()) || prevIsDate != isDate)
            {
                prevIsDate = isDate;
                prevDt = dt;
                ++iter;
                continue;
            }

            if (!prevIsDate && !isDate)
            {
                auto dateItem = std::make_unique<DateLinkItem>(iter->get()->time(), iter->get()->getMsg(), iter->get()->getSeq());
                h += dateItem->getHeight();

                iter = Items_.insert(iter, std::move(dateItem));
                prevIsDate = true;
                prevDt = dt;
                ++iter;
                continue;
            }

            auto prev = std::prev(iter);
            h -= prev->get()->getHeight();
            iter = Items_.erase(prev);
            prevIsDate = true;
            prevDt = dt;
            ++iter;
        }

        setFixedHeight(h);
    }
}
