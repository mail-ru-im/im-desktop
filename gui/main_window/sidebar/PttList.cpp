#include "stdafx.h"

#include "PttList.h"
#include "SidebarUtils.h"
#include "../../core_dispatcher.h"
#include "../../controls/TextUnit.h"
#include "../../controls/ContextMenu.h"
#include "../../fonts.h"
#include "../../utils/utils.h"
#include "../../my_info.h"
#include "../GroupChatOperations.h"
#include "../friendly/FriendlyContainer.h"
#include "../contact_list/ContactListModel.h"
#include "../history_control/complex_message/FileSharingUtils.h"
#include "../friendly/FriendlyContainer.h"
#include "../sounds/SoundsManager.h"
#include "../../utils/translator.h"
#include "utils/stat_utils.h"
#include "../../styles/ThemeParameters.h"
#include "../input_widget/InputWidgetUtils.h"

namespace
{
    const int ICON_VER_OFFSET = 12;
    const int VER_OFFSET = 8;
    const int HOR_OFFSET = 12;
    const int RIGHT_OFFSET = 40;
    const int MORE_BUTTON_SIZE = 20;
    const int BUTTON_SIZE = 40;
    const int SUB_BUTTON_SIZE = 20;
    const int PREVIEW_RIGHT_OFFSET = 12;
    const int NAME_OFFSET = 8;
    const int DATE_TOP_OFFSET = 12;
    const int DATE_BOTTOM_OFFSET = 8;
    const int DATE_OFFSET = 8;
    const int PROGRESS_OFFSET = 8;
    const int TEXT_OFFSET = 16;
    const int TEXT_HOR_OFFSET = 28;

    QString formatDuration(const int32_t _seconds)
    {
        assert(_seconds >= 0);

        const auto minutes = (_seconds / 60);

        const auto seconds = (_seconds % 60);

        return qsl("%1:%2")
            .arg(minutes, 2, 10, ql1c('0'))
            .arg(seconds, 2, 10, ql1c('0'));
    }

    const std::string MediaTypeName("PTT");
}

namespace Ui
{
    DatePttItem::DatePttItem(time_t _time, qint64 _msg, qint64 _seq)
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

    qint64 DatePttItem::getMsg() const
    {
        return msg_;
    }

    qint64 DatePttItem::getSeq() const
    {
        return seq_;
    }

    time_t DatePttItem::time() const
    {
        return time_;
    }

    void DatePttItem::draw(QPainter& _p, const QRect& _rect)
    {
        date_->setOffsets(Utils::scale_value(HOR_OFFSET), Utils::scale_value(DATE_TOP_OFFSET) + _rect.y());
        date_->draw(_p);
    }

    int DatePttItem::getHeight() const
    {
        return Utils::scale_value(DATE_BOTTOM_OFFSET + DATE_TOP_OFFSET) + date_->cachedSize().height();
    }

    PttItem::PttItem(const QString& _link, const QString& _date, qint64 _msg, qint64 _seq, int _width, const QString& _sender, bool _outgoing, time_t _time)
        : link_(_link)
        , height_(0)
        , width_(_width)
        , msg_(_msg)
        , seq_(_seq)
        , progress_(0)
        , playing_(false)
        , pause_(false)
        , textVisible_(false)
        , playState_(ButtonState::NORMAL)
        , outgoing_(_outgoing)
        , reqId_(-1)
        , sender_(_sender)
        , timet_(_time)
    {
        height_ = QFontMetrics(Fonts::appFontScaled(16)).height();
        height_ += QFontMetrics(Fonts::appFontScaled(12)).height();

        date_ = Ui::TextRendering::MakeTextUnit(_date, Data::MentionMap(), TextRendering::LinksVisible::DONT_SHOW_LINKS);
        date_->init(Fonts::appFontScaled(13), Styling::getParameters().getColor(Styling::StyleVariable::BASE_SECONDARY));
        date_->evaluateDesiredSize();

        height_ += date_->cachedSize().height();
        height_ += Utils::scale_value(DATE_OFFSET);
        height_ += Utils::scale_value(VER_OFFSET);

        playIcon_ = Utils::renderSvgScaled(qsl(":/videoplayer/video_play"), QSize(SUB_BUTTON_SIZE, SUB_BUTTON_SIZE), Styling::getParameters().getColor(Styling::StyleVariable::TEXT_SOLID_PERMANENT));
        textIcon_ = Utils::renderSvgScaled(qsl(":/ptt/text_icon"), QSize(SUB_BUTTON_SIZE, SUB_BUTTON_SIZE), Styling::getParameters().getColor(Styling::StyleVariable::PRIMARY_INVERSE));

        name_ = TextRendering::MakeTextUnit(Logic::GetFriendlyContainer()->getFriendly(_sender), Data::MentionMap(), TextRendering::LinksVisible::DONT_SHOW_LINKS);
        name_->init(Fonts::appFontScaled(16), Styling::getParameters().getColor(Styling::StyleVariable::TEXT_SOLID));
        name_->evaluateDesiredSize();

        const QString id = Ui::ComplexMessage::extractIdFromFileSharingUri(_link);
        const auto durationSec = Ui::ComplexMessage::extractDurationFromFileSharingId(id);

        time_ = TextRendering::MakeTextUnit(formatDuration(durationSec));
        time_->init(Fonts::appFontScaled(13), Styling::getParameters().getColor(Styling::StyleVariable::PRIMARY));
        time_->evaluateDesiredSize();

        setMoreButtonState(ButtonState::HIDDEN);
    }

    void PttItem::draw(QPainter& _p, const QRect& _rect)
    {
        _p.setRenderHint(QPainter::Antialiasing);
        _p.setPen(Qt::NoPen);

        switch (playState_)
        {
            case ButtonState::HOVERED:
                _p.setBrush(Styling::getParameters().getColor(Styling::StyleVariable::PRIMARY_HOVER));
                break;

            case ButtonState::PRESSED:
                _p.setBrush(Styling::getParameters().getColor(Styling::StyleVariable::PRIMARY_ACTIVE));
                break;

            case ButtonState::NORMAL:
            default:
                _p.setBrush(Styling::getParameters().getColor(Styling::StyleVariable::PRIMARY));
                break;
        }

        if (!more_.isNull())
            _p.drawPixmap(width_ - Utils::scale_value(RIGHT_OFFSET) / 2 - Utils::scale_value(MORE_BUTTON_SIZE) / 2, Utils::scale_value(ICON_VER_OFFSET + BUTTON_SIZE / 2) - Utils::scale_value(MORE_BUTTON_SIZE) / 2 + _rect.y(), more_);

        _p.drawEllipse(QPoint(Utils::scale_value(HOR_OFFSET + BUTTON_SIZE / 2), Utils::scale_value(ICON_VER_OFFSET + BUTTON_SIZE / 2) + _rect.y()), Utils::scale_value(BUTTON_SIZE / 2), Utils::scale_value(BUTTON_SIZE / 2));
        _p.drawPixmap(Utils::scale_value(HOR_OFFSET + BUTTON_SIZE / 2 - SUB_BUTTON_SIZE / 2), Utils::scale_value(ICON_VER_OFFSET + BUTTON_SIZE / 2 - SUB_BUTTON_SIZE / 2) + _rect.y(), playIcon_);

        const auto showPttRecognized = !build::is_dit();
        if (showPttRecognized)
        {
            switch (textState_)
            {
            case ButtonState::HOVERED:
                _p.setBrush(Styling::getParameters().getColor(Styling::StyleVariable::BASE_BRIGHT_HOVER));
                break;

            case ButtonState::PRESSED:
                _p.setBrush(Styling::getParameters().getColor(Styling::StyleVariable::BASE_BRIGHT_ACTIVE));
                break;

            case ButtonState::NORMAL:
            default:
                _p.setBrush(Styling::getParameters().getColor(Styling::StyleVariable::BASE_BRIGHT));
                break;
            }

            _p.drawEllipse(QPoint(width_ - Utils::scale_value(HOR_OFFSET + BUTTON_SIZE / 2 + RIGHT_OFFSET - PREVIEW_RIGHT_OFFSET), Utils::scale_value(ICON_VER_OFFSET + BUTTON_SIZE / 2) + _rect.y()), Utils::scale_value(BUTTON_SIZE / 2), Utils::scale_value(BUTTON_SIZE / 2));

            QRect textIconRect(width_ - Utils::scale_value(HOR_OFFSET + BUTTON_SIZE / 2 + SUB_BUTTON_SIZE / 2 + RIGHT_OFFSET - PREVIEW_RIGHT_OFFSET), Utils::scale_value(ICON_VER_OFFSET + SUB_BUTTON_SIZE / 2) + _rect.y(), Utils::scale_value(SUB_BUTTON_SIZE), Utils::scale_value(SUB_BUTTON_SIZE));
            _p.drawPixmap(textIconRect, textIcon_);
        }

        const auto mult = showPttRecognized ? 2 : 1;
        const auto progressWidth = width_ - Utils::scale_value(HOR_OFFSET + RIGHT_OFFSET) - Utils::scale_value(PREVIEW_RIGHT_OFFSET) * mult - Utils::scale_value(BUTTON_SIZE) * mult;
        auto offset = 0;
        if (name_)
        {
            name_->elide(progressWidth);
            name_->setOffsets(Utils::scale_value(HOR_OFFSET + BUTTON_SIZE + PREVIEW_RIGHT_OFFSET), Utils::scale_value(VER_OFFSET) + _rect.y());
            name_->draw(_p);
            offset += name_->cachedSize().height();
            offset += Utils::scale_value(NAME_OFFSET);
        }

        const auto cur = progressWidth * progress_ / 100.0 + Utils::scale_value(HOR_OFFSET + BUTTON_SIZE + PREVIEW_RIGHT_OFFSET);

        if (progress_ != 0)
        {
            _p.setPen(QPen(Styling::getParameters().getColor(Styling::StyleVariable::PRIMARY), Utils::scale_value(2)));
            _p.drawLine(Utils::scale_value(HOR_OFFSET + BUTTON_SIZE + PREVIEW_RIGHT_OFFSET), Utils::scale_value(PROGRESS_OFFSET) + offset + _rect.y(), cur, Utils::scale_value(PROGRESS_OFFSET) + offset + _rect.y());
        }

        if (progress_ != 100)
        {
            _p.setPen(QPen(Styling::getParameters().getColor(Styling::StyleVariable::BASE_BRIGHT), Utils::scale_value(2)));
            _p.drawLine(cur, Utils::scale_value(PROGRESS_OFFSET) + offset + _rect.y(), width_ - Utils::scale_value(RIGHT_OFFSET + (showPttRecognized ? BUTTON_SIZE + PREVIEW_RIGHT_OFFSET : 0)), Utils::scale_value(PROGRESS_OFFSET) + offset + _rect.y());
        }

        if (time_)
        {
            time_->setOffsets(Utils::scale_value(HOR_OFFSET + BUTTON_SIZE + PREVIEW_RIGHT_OFFSET), Utils::scale_value(VER_OFFSET + PROGRESS_OFFSET) + offset + _rect.y());
            time_->draw(_p);
        }

        date_->setOffsets(Utils::scale_value(HOR_OFFSET + BUTTON_SIZE + PREVIEW_RIGHT_OFFSET) + progressWidth - date_->desiredWidth(), Utils::scale_value(VER_OFFSET + PROGRESS_OFFSET) + offset + _rect.y());
        date_->draw(_p);

        if (textControl_ && textVisible_)
        {
            offset += Utils::scale_value(14);
            _p.fillRect(QRect(0, offset + date_->cachedSize().height() + _rect.y() + Utils::scale_value(VER_OFFSET * 2), width_, textControl_->cachedSize().height() + Utils::scale_value(TEXT_OFFSET) * 3), Styling::getParameters().getColor(Styling::StyleVariable::BASE_BRIGHT_INVERSE));
            _p.setPen(QPen(Styling::getParameters().getColor(Styling::StyleVariable::PRIMARY_INVERSE), Utils::scale_value(4)));
            _p.drawLine(QPoint(Utils::scale_value(14), Utils::scale_value(TEXT_OFFSET + VER_OFFSET * 2) + _rect.y() + offset + date_->cachedSize().height() + Utils::scale_value(2)), QPoint(Utils::scale_value(14), Utils::scale_value(TEXT_OFFSET + VER_OFFSET * 2) + offset + date_->cachedSize().height() + _rect.y() + textControl_->cachedSize().height() - Utils::scale_value(2)));
            textControl_->setOffsets(Utils::scale_value(TEXT_HOR_OFFSET), Utils::scale_value(TEXT_OFFSET + VER_OFFSET * 2) + offset + date_->cachedSize().height() + _rect.y());
            textControl_->draw(_p);
        }
    }

    int PttItem::getHeight() const
    {
        return height_;
    }

    void PttItem::setWidth(int _width)
    {
        width_ = _width;
    }

    void PttItem::setReqId(qint64 _reqId)
    {
        reqId_ = _reqId;
    }

    qint64 PttItem::reqId() const
    {
        return reqId_;
    }

    qint64 PttItem::getMsg() const
    {
        return msg_;
    }

    qint64 PttItem::getSeq() const
    {
        return seq_;
    }

    QString PttItem::getLink() const
    {
        return link_;
    }

    QString PttItem::getLocalPath() const
    {
        return localPath_;
    }

    bool PttItem::isPlaying() const
    {
        return playing_;
    }

    bool PttItem::hasText() const
    {
        return !text_.isEmpty();
    }

    QString PttItem::sender() const
    {
        return sender_;
    }

    time_t PttItem::time() const
    {
        return timet_;
    }

    void PttItem::setProgress(int _progress)
    {
        progress_ = _progress;
    }

    void PttItem::setPlaying(bool _playing)
    {
        pause_ = false;
        playing_ = _playing;
        if (!_playing)
            progress_ = 0;

        playIcon_ = Utils::renderSvgScaled(_playing ? qsl(":/videoplayer/video_pause") : qsl(":/videoplayer/video_play"), QSize(SUB_BUTTON_SIZE, SUB_BUTTON_SIZE), Styling::getParameters().getColor(Styling::StyleVariable::TEXT_SOLID_PERMANENT));
    }

    void PttItem::setLocalPath(const QString& _localPath)
    {
        localPath_ = _localPath;
    }

    void PttItem::setPause(bool _pause)
    {
        pause_ = _pause;
        playIcon_ = Utils::renderSvgScaled(_pause ? qsl(":/videoplayer/video_play") : qsl(":/videoplayer/video_pause"), QSize(SUB_BUTTON_SIZE, SUB_BUTTON_SIZE), Styling::getParameters().getColor(Styling::StyleVariable::TEXT_SOLID_PERMANENT));
    }

    void PttItem::toggleTextVisible()
    {
        textVisible_ = !textVisible_;
        auto offset = textControl_ ? textControl_->cachedSize().height() : 0;
        offset += Utils::scale_value(TEXT_OFFSET) * 3;

        if (textVisible_)
            height_ += offset;
        else
            height_ -= offset;
    }

    void PttItem::setText(const QString& _text)
    {
        text_ = _text;
        textControl_ = TextRendering::MakeTextUnit(text_);
        textControl_->init(Fonts::appFontScaled(15), Styling::getParameters().getColor(Styling::StyleVariable::TEXT_SOLID));
        textControl_->getHeight(width_ - Utils::scale_value(HOR_OFFSET + TEXT_HOR_OFFSET + RIGHT_OFFSET));
    }

    bool PttItem::playClicked(const QPoint& _pos) const
    {
        static QRect r(Utils::scale_value(HOR_OFFSET), Utils::scale_value(VER_OFFSET), Utils::scale_value(BUTTON_SIZE), Utils::scale_value(BUTTON_SIZE));
        return r.contains(_pos) && (!playing_ || pause_);
    }

    bool PttItem::pauseClicked(const QPoint& _pos) const
    {
        static QRect r(Utils::scale_value(HOR_OFFSET), Utils::scale_value(VER_OFFSET), Utils::scale_value(BUTTON_SIZE), Utils::scale_value(BUTTON_SIZE));
        return r.contains(_pos) && playing_;
    }

    bool PttItem::textClicked(const QPoint& _pos) const
    {
        if (build::is_dit())
            return false;

        QRect r(width_ - Utils::scale_value(HOR_OFFSET + BUTTON_SIZE + RIGHT_OFFSET - PREVIEW_RIGHT_OFFSET), Utils::scale_value(VER_OFFSET), Utils::scale_value(BUTTON_SIZE), Utils::scale_value(BUTTON_SIZE));
        return r.contains(_pos);
    }

    void PttItem::setPlayState(const ButtonState& _state)
    {
        playState_ = _state;
    }

    void PttItem::setTextState(const ButtonState& _state)
    {
        textState_ = _state;
    }

    bool PttItem::isOverDate(const QPoint& _pos) const
    {
        return date_->contains(_pos);
    }

    void PttItem::setDateState(bool _hover, bool _active)
    {
        if (_hover)
            date_->setColor(Styling::getParameters().getColor(Styling::StyleVariable::BASE_SECONDARY_HOVER));
        else if (_active)
            date_->setColor(Styling::getParameters().getColor(Styling::StyleVariable::BASE_SECONDARY_ACTIVE));
        else
            date_->setColor(Styling::getParameters().getColor(Styling::StyleVariable::BASE_SECONDARY));
    }

    bool PttItem::isOutgoing() const
    {
        return outgoing_;
    }

    void PttItem::setMoreButtonState(const ButtonState& _state)
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
            color =Styling::getParameters().getColor(Styling::StyleVariable::BASE_SECONDARY_HOVER);
            break;
        case ButtonState::PRESSED:
            color = Styling::getParameters().getColor(Styling::StyleVariable::BASE_SECONDARY_ACTIVE);
            break;
        default:
            break;
        }

        more_ = Utils::renderSvgScaled(qsl(":/controls/more_vertical"), QSize(MORE_BUTTON_SIZE, MORE_BUTTON_SIZE), color);
    }

    bool PttItem::isOverMoreButton(const QPoint& _pos, int _h) const
    {
        auto r = QRect(width_ - Utils::scale_value(RIGHT_OFFSET) / 2 - Utils::scale_value(MORE_BUTTON_SIZE) / 2, _h + Utils::scale_value(ICON_VER_OFFSET + BUTTON_SIZE / 2) - Utils::scale_value(MORE_BUTTON_SIZE) / 2, Utils::scale_value(MORE_BUTTON_SIZE), Utils::scale_value(MORE_BUTTON_SIZE));
        return r.contains(_pos);
    }

    ButtonState PttItem::moreButtonState() const
    {
        return moreState_;
    }

    PttList::PttList(QWidget* _parent)
        : MediaContentWidget(Type::Files, _parent)
        , playingId_(-1)
        , playingIndex_(std::make_pair(-1, -1))
        , lastProgress_(0)
    {
        connect(Ui::GetDispatcher(), &core_dispatcher::fileSharingFileDownloaded, this, &PttList::onFileDownloaded);
        connect(Ui::GetDispatcher(), &core_dispatcher::speechToText, this, &PttList::onPttText);

        connect(GetSoundsManager(), &SoundsManager::pttPaused, this, &PttList::onPttPaused);
        connect(GetSoundsManager(), &SoundsManager::pttFinished, this, &PttList::onPttFinished);

        setMouseTracking(true);
    }

    void PttList::initFor(const QString& _aimId)
    {
        MediaContentWidget::initFor(_aimId);
        animation_.finish();
        Items_.clear();
        RequestIds_.clear();
        setFixedHeight(0);
    }

    void PttList::processItems(const QVector<Data::DialogGalleryEntry>& _entries)
    {
        auto h = height();
        for (const auto& e : _entries)
        {
            auto time = QDateTime::fromTime_t(e.time_);
            auto item = std::make_unique<PttItem>(e.url_, time.toString(qsl("d MMM, ")) + time.time().toString(qsl("hh:mm")), e.msg_id_, e.seq_, width(), e.sender_, e.outgoing_, e.time_);
            h += item->getHeight();
            Items_.push_back(std::move(item));
        }
        setFixedHeight(h);
        validateDates();
    }

    void PttList::processUpdates(const QVector<Data::DialogGalleryEntry>& _entries)
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

            Utils::UrlParser parser;
            parser.process(QStringRef(&e.url_));

            const auto isFilesharing = parser.hasUrl() && parser.getUrl().is_filesharing();
            if (!isFilesharing || e.type_ != qsl("ptt"))
                continue;

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

            auto item = std::make_unique<PttItem>(e.url_, time.toString(qsl("d MMM, ")) + time.time().toString(qsl("hh:mm")), e.msg_id_, e.seq_, width(), e.sender_, e.outgoing_, e.time_);
            h += item->getHeight();

            Items_.insert(iter, std::move(item));
        }
        setFixedHeight(h);
        validateDates();
    }

    void PttList::markClosed()
    {
        if (playingId_ != -1)
        {
            GetSoundsManager()->pausePtt(playingId_);
            playingId_ = -1;
        }
    }

    void PttList::paintEvent(QPaintEvent*)
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

    MediaContentWidget::ItemData PttList::itemAt(const QPoint& _pos)
    {
        auto h = 0;
        for (const auto& i : Items_)
        {
            auto r = QRect(0, h, width(), i->getHeight());
            h += i->getHeight();

            if (r.contains(_pos))
                return MediaContentWidget::ItemData(i->getLink(), i->getMsg(), i->sender(), i->time());
        }

        return MediaContentWidget::ItemData();
    }

    void PttList::mousePressEvent(QMouseEvent *_event)
    {
        auto h = 0;
        for (auto& i : Items_)
        {
            auto r = QRect(0, h, width(), i->getHeight());
            if (r.contains(_event->pos()))
            {
                auto p = _event->pos();
                p.setY(p.y() - h);
                if (i->playClicked(p) || i->pauseClicked(p))
                {
                    i->setPlayState(ButtonState::PRESSED);
                }
                else if (i->textClicked(p))
                {
                    i->setTextState(ButtonState::PRESSED);
                }
                else
                {
                    i->setPlayState(ButtonState::NORMAL);
                    i->setTextState(ButtonState::NORMAL);
                }
            }

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

    void PttList::mouseReleaseEvent(QMouseEvent *_event)
    {
        Utils::GalleryMediaActionCont cont(MediaTypeName, aimId_);

        if (_event->button() == Qt::RightButton)
        {
            cont.happened();
            return MediaContentWidget::mouseReleaseEvent(_event);
        }

        if (Utils::clicked(pos_, _event->pos()))
        {
            auto h = 0;
            for (auto& i : Items_)
            {
                auto r = QRect(0, h, width(), i->getHeight());
                if (r.contains(_event->pos()))
                {
                    cont.happened();

                    auto p = _event->pos();
                    p.setY(p.y() - h);
                    if (i->playClicked(p))
                    {
                        i->setPlaying(true);
                        auto localPath = i->getLocalPath();
                        if (localPath.isEmpty())
                        {
                            auto reqId = Ui::GetDispatcher()->downloadSharedFile(aimId_, i->getLink(), true, QString(), true);
                            i->setReqId(reqId);
                            RequestIds_.push_back(reqId);
                        }
                        else
                        {
                            play(i);
                        }
                        i->setPlayState(ButtonState::HOVERED);
                    }
                    else if (i->pauseClicked(p))
                    {
                        i->setPause(true);
                        GetSoundsManager()->pausePtt(playingId_);
                        lastProgress_ = animation_.state() == anim::State::Stopped ? 0 : animation_.current();
                        animation_.pause();
                        update();
                        i->setPlayState(ButtonState::HOVERED);
                    }
                    else if (i->textClicked(p))
                    {
                        if (i->hasText())
                        {
                            i->toggleTextVisible();
                            invalidateHeight();
                        }
                        else
                        {
                            auto reqId = Ui::GetDispatcher()->pttToText(i->getLink(), Utils::GetTranslator()->getLang());
                            i->setReqId(reqId);
                            RequestIds_.push_back(reqId);
                        }
                        i->setTextState(ButtonState::HOVERED);
                    }
                    else
                    {
                        i->setPlayState(ButtonState::NORMAL);
                        i->setTextState(ButtonState::NORMAL);
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
    }

    void PttList::mouseMoveEvent(QMouseEvent* _event)
    {
        auto h = 0;
        auto point = false;
        for (auto& i : Items_)
        {
            auto r = QRect(0, h, width(), i->getHeight());
            if (r.contains(_event->pos()))
            {
                auto p = _event->pos();
                p.setY(p.y() - h);
                if (i->playClicked(p) || i->pauseClicked(p))
                {
                    i->setPlayState(ButtonState::HOVERED);
                    point = true;
                }
                else if (i->textClicked(p))
                {
                    i->setTextState(ButtonState::HOVERED);
                    point = true;
                }
                else
                {
                    i->setPlayState(ButtonState::NORMAL);
                    i->setTextState(ButtonState::NORMAL);
                }
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
                if (r.contains(_event->pos()))
                    i->setMoreButtonState(ButtonState::NORMAL);
                else
                    i->setMoreButtonState(ButtonState::HIDDEN);
            }

            h += i->getHeight();
        }

        setCursor(point ? Qt::PointingHandCursor : Qt::ArrowCursor);
        update();
    }

    void PttList::leaveEvent(QEvent* _event)
    {
        for (auto& i : Items_)
            i->setMoreButtonState(ButtonState::HIDDEN);

        update();
        MediaContentWidget::leaveEvent(_event);
    }

    void PttList::resizeEvent(QResizeEvent* _event)
    {
        for (auto& i : Items_)
            i->setWidth(_event->size().width());

        MediaContentWidget::resizeEvent(_event);
    }

    void PttList::onFileDownloaded(qint64 _seq, const Data::FileSharingDownloadResult& _result)
    {
        if (std::find(RequestIds_.begin(), RequestIds_.end(), _seq) == RequestIds_.end())
            return;

        for (auto& i : Items_)
        {
            if (i->reqId() == _seq)
            {
                i->setLocalPath(_result.filename_);
                play(i);
                break;
            }
        }
    }

    void PttList::onPttText(qint64 _seq, int _error, QString _text, int _comeback)
    {
        if (std::find(RequestIds_.begin(), RequestIds_.end(), _seq) == RequestIds_.end())
            return;

        if (_comeback > 0)
        {
            const auto retryTimeoutMsec = ((_comeback + 1) * 1000);
            QTimer::singleShot(
                retryTimeoutMsec,
                Qt::VeryCoarseTimer,
                this,
                [this, _seq]
                {
                    for (auto& i : Items_)
                    {
                        if (i->reqId() == _seq)
                        {
                            auto reqId = Ui::GetDispatcher()->pttToText(i->getLink(), Utils::GetTranslator()->getLang());
                            i->setReqId(reqId);
                            RequestIds_.push_back(reqId);
                            break;
                        }
                    }
                });

            return;
        }

        const bool isError = _error != 0;

        const QString text = isError ? QT_TRANSLATE_NOOP("ptt_widget", "unclear message") : _text;

        Ui::GetDispatcher()->post_stats_to_core(core::stats::stats_event_names::chatscr_recognptt_action, {
            { "from_gallery", "yes" },
            { "chat_type", Ui::getStatsChatType() },
            { "result", isError ? "fail" : "success" } });

        for (auto& i : Items_)
        {
            if (i->reqId() == _seq)
            {
                i->setText(text);
                i->toggleTextVisible();
                break;
            }
        }

        invalidateHeight();
    }

    void PttList::finishPtt(int id)
    {
        if (id != -1 && id == playingId_)
        {
            for (auto& i : Items_)
            {
                if (!i->isDateItem() && i->getMsg() == playingIndex_.first && i->getSeq() == playingIndex_.second)
                {
                    animation_.finish();
                    i->setPlaying(false);
                    lastProgress_ = 0;
                    playingId_ = -1;
                    playingIndex_ = std::make_pair(-1, -1);
                    break;
                }
            }
        }
    }

    void PttList::play(std::unique_ptr<BasePttItem>& _item)
    {
        if (!_item->isPlaying() || _item->getLocalPath().isEmpty())
            return;

        if (_item->getMsg() != playingIndex_.first || _item->getSeq() != playingIndex_.second)
            finishPtt(playingId_);

        int duration = 0;
        playingId_ = GetSoundsManager()->playPtt(_item->getLocalPath(), playingId_, duration);
        Ui::GetDispatcher()->post_stats_to_core(core::stats::stats_event_names::chatscr_playptt_action, { { "from_gallery", "yes" },  { "chat_type", Ui::getStatsChatType() } });
        playingIndex_ = std::make_pair(_item->getMsg(), _item->getSeq());

        if (!animation_.isRunning())
        {
            auto from = 0;
            if (animation_.state() == anim::State::Paused)
            {
                from = lastProgress_;
                duration = duration - duration * lastProgress_ / 100;
            }

            animation_.finish();
            animation_.start([this]()
            {
                for (auto& i : Items_)
                {
                    if (!i->isDateItem() && i->getMsg() == playingIndex_.first && i->getSeq() == playingIndex_.second)
                    {
                        const auto cur = animation_.current();
                        i->setProgress(cur);
                        if (cur == 100)
                            QTimer::singleShot(200, this, [this]() { finishPtt(playingId_); update();});

                        update();
                        break;
                    }
                }
            }, from, 100, duration);
        }
    }

    void PttList::invalidateHeight()
    {
        auto h = 0;
        for (const auto& i : Items_)
            h += i->getHeight();

        setFixedHeight(h);
    }

    void PttList::validateDates()
    {
        if (Items_.empty())
            return;

        auto h = height();

        if (!Items_.front()->isDateItem())
        {
            Items_.emplace(Items_.begin(), new DatePttItem(Items_.front()->time(), Items_.front()->getMsg(), Items_.front()->getSeq()));
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
                auto dateItem = std::make_unique<DatePttItem>(iter->get()->time(), iter->get()->getMsg(), iter->get()->getSeq());
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

    void PttList::onPttFinished(int _id, bool /*_byPlay*/)
    {
        if (_id != -1 && playingId_ == _id)
            finishPtt(playingId_);
    }

    void PttList::onPttPaused(int _id)
    {
        if (_id != -1 && _id == playingId_)
        {
            for (auto& i : Items_)
            {
                if (!i->isDateItem() && i->getMsg() == playingIndex_.first && i->getSeq() == playingIndex_.second)
                {
                    i->setPause(true);
                    lastProgress_ = animation_.state() == anim::State::Stopped ? 0 : animation_.current();
                    animation_.pause();
                    update();
                    break;
                }
            }
        }
    }
}
