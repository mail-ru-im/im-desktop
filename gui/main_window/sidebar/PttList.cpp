#include "stdafx.h"

#include "PttList.h"
#include "SidebarUtils.h"
#include "../common.shared/config/config.h"
#include "../../core_dispatcher.h"
#include "../../controls/TextUnit.h"
#include "../../controls/ContextMenu.h"
#include "../../controls/TooltipWidget.h"
#include "../../fonts.h"
#include "../../utils/utils.h"
#include "../../utils/UrlParser.h"
#include "../../my_info.h"
#include "../GroupChatOperations.h"
#include "../containers/FriendlyContainer.h"
#include "../contact_list/ContactListModel.h"
#include "../history_control/complex_message/FileSharingUtils.h"
#include "../containers/FriendlyContainer.h"
#include "../../utils/translator.h"
#include "utils/stat_utils.h"
#include "../../styles/ThemeParameters.h"
#include "../input_widget/InputWidgetUtils.h"
#ifndef STRIP_AV_MEDIA
#include "../../media/ptt/AudioUtils.h"
#include "../sounds/SoundsManager.h"
#endif

namespace
{
    const int ICON_VER_OFFSET = 12;
    const int VER_OFFSET = 8;
    const int HOR_OFFSET = 12;
    const int RIGHT_OFFSET = 40;
    const int MORE_BUTTON_SIZE = 20;
    const int BUTTON_SIZE = 40;
    const int SUB_BUTTON_SIZE = 20;
    const int TEXT_BUTTON_SIZE = 32;
    const int PREVIEW_RIGHT_OFFSET = 12;
    const int NAME_OFFSET = 8;
    const int DATE_TOP_OFFSET = 12;
    const int DATE_BOTTOM_OFFSET = 8;
    const int DATE_OFFSET = 8;
    const int PROGRESS_OFFSET = 8;
    const int TEXT_OFFSET = 16;
    const int TEXT_HOR_OFFSET = 28;

    QString formatDuration(const int _duration, const int _progress)
    {
#ifndef STRIP_AV_MEDIA
        const auto time = (int)(_duration *(100. - _progress) / 100);
        return ptt::formatDuration(std::chrono::seconds(time));
#else
        return qsl("00:00");
#endif
    }

    const std::string MediaTypeName("PTT");

    int getBulletClickableRadius()
    {
        return Utils::scale_value(32);
    }

    int getBulletSize(bool _hovered)
    {
        return _hovered ? Utils::scale_value(10) : Utils::scale_value(8);
    }

    auto getButtonColor()
    {
        return Styling::getParameters().getColor(Styling::StyleVariable::BASE_GLOBALWHITE);
    }

    auto getPlayButtonIcon(bool _isPlaying = false)
    {
        static const auto iconPlay = Utils::renderSvgScaled(qsl(":/videoplayer/video_play"), QSize(SUB_BUTTON_SIZE, SUB_BUTTON_SIZE), getButtonColor());
        static const auto iconPause = Utils::renderSvgScaled(qsl(":/videoplayer/video_pause"), QSize(SUB_BUTTON_SIZE, SUB_BUTTON_SIZE), getButtonColor());
        return _isPlaying ? iconPause : iconPlay;
    }

    constexpr std::chrono::milliseconds tooltipShowDelay() noexcept { return std::chrono::milliseconds(400); }
}

namespace Ui
{
    DatePttItem::DatePttItem(time_t _time, qint64 _msg, qint64 _seq)
        : time_(_time)
        , msg_(_msg)
        , seq_(_seq)
    {
        auto dt = QDateTime::fromSecsSinceEpoch(_time);
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
        , datePressed_(false)
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

        playIcon_ = getPlayButtonIcon();
        textIcon_ = Utils::renderSvgScaled(qsl(":/ptt/text_icon"), QSize(BUTTON_SIZE, BUTTON_SIZE), Styling::getParameters().getColor(Styling::StyleVariable::PRIMARY));

        name_ = TextRendering::MakeTextUnit(Logic::GetFriendlyContainer()->getFriendly(_sender), Data::MentionMap(), TextRendering::LinksVisible::DONT_SHOW_LINKS);
        name_->init(Fonts::appFontScaled(16), Styling::getParameters().getColor(Styling::StyleVariable::TEXT_SOLID));
        name_->evaluateDesiredSize();

        const QString id = Ui::ComplexMessage::extractIdFromFileSharingUri(_link);
        durationSec_ = Ui::ComplexMessage::extractDurationFromFileSharingId(id);

        time_ = TextRendering::MakeTextUnit(formatDuration(durationSec_, progress_));
        time_->init(Fonts::appFontScaled(13), Styling::getParameters().getColor(Styling::StyleVariable::PRIMARY));
        time_->evaluateDesiredSize();

        setMoreButtonState(ButtonState::HIDDEN);
    }

    void PttItem::draw(QPainter& _p, const QRect& _rect)
    {
        _p.setRenderHints(QPainter::Antialiasing | QPainter::SmoothPixmapTransform);
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

        const auto showPttRecognized = config::get().is_on(config::features::ptt_recognition);
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

            QRect textIconRect(width_ - Utils::scale_value(HOR_OFFSET + BUTTON_SIZE / 2 + TEXT_BUTTON_SIZE / 2 + RIGHT_OFFSET - PREVIEW_RIGHT_OFFSET), Utils::scale_value(TEXT_BUTTON_SIZE / 2) + _rect.y(), Utils::scale_value(TEXT_BUTTON_SIZE), Utils::scale_value(TEXT_BUTTON_SIZE));
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

        {
            Utils::PainterSaver ps(_p);
            const auto color = Styling::getParameters().getColor(Styling::StyleVariable::PRIMARY);
            _p.setPen(color);
            _p.setBrush(color);
            const auto bulletR = getBulletSize(progressHovered_ || progressPressed_);
            QRect bullet(cur - bulletR / 2, Utils::scale_value(PROGRESS_OFFSET) + offset + _rect.y() - bulletR / 2, bulletR, bulletR);
            _p.drawEllipse(bullet);
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

    void PttItem::setPending(bool _flag)
    {
        isPending_ = _flag;
    }

    bool PttItem::isPending() const
    {
        return isPending_;
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
        if (time_)
        {
            time_->setText(formatDuration(durationSec_, progress_));
            time_->evaluateDesiredSize();
        }
    }

    void PttItem::setPlaying(bool _playing)
    {
        pause_ = false;
        playing_ = _playing;
        playIcon_ = getPlayButtonIcon(_playing);
    }

    void PttItem::setLocalPath(const QString& _localPath)
    {
        localPath_ = _localPath;
    }

    void PttItem::setPause(bool _pause)
    {
        playing_ = false;
        pause_ = _pause;
        playIcon_ = getPlayButtonIcon(!_pause);
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
        if (!config::get().is_on(config::features::ptt_recognition))
            return false;

        QRect r(width_ - Utils::scale_value(HOR_OFFSET + BUTTON_SIZE + RIGHT_OFFSET - PREVIEW_RIGHT_OFFSET), Utils::scale_value(VER_OFFSET), Utils::scale_value(BUTTON_SIZE), Utils::scale_value(BUTTON_SIZE));
        return r.contains(_pos);
    }

    bool PttItem::isOverProgressBar(const QPoint& _pos) const
    {
        return getPlaybackRect().contains(_pos);
    }

    bool PttItem::isOverBullet(const QPoint& _pos) const
    {
        return progressHovered_;
    }

    void PttItem::updateBulletHoverState(const QPoint& _pos)
    {
        progressHovered_ = getBulletRect().contains(_pos);
    }

    void PttItem::changeProgressPressed(bool _pressed)
    {
        progressPressed_ = _pressed;
    }

    bool PttItem::isProgressPressed() const
    {
        return progressPressed_;
    }

    int PttItem::getProgress(const QPoint& _pos)
    {
        if (!_pos.isNull())
            progress_ = std::round(evaluateProgress(_pos) * 100);
        return progress_;
    }

    double PttItem::evaluateProgress(const QPoint& _pos) const
    {
        const auto rect = getPlaybackRect();
        return std::clamp(double(_pos.x() - rect.left()) / rect.width(), 0.0, 1.0);
    }

    QString PttItem::getTimeString(const QPoint& _pos) const
    {
        const auto progress = evaluateProgress(_pos);
        return Utils::getFormattedTime(std::chrono::milliseconds(int(std::ceil(durationSec_ * progress * 1000))));
    }

    QPoint PttItem::getTooltipPos(const QPoint& _cursorPos) const
    {
        const auto playbackRect = getPlaybackRect();
        return { std::clamp(_cursorPos.x(), playbackRect.x(), playbackRect.right()), playbackRect.y() };
    }

    QRect PttItem::getPlaybackRect() const
    {
        const auto mult = config::get().is_on(config::features::ptt_recognition) ? 2 : 1;
        const auto progressWidth = width_ - Utils::scale_value(HOR_OFFSET + RIGHT_OFFSET) - Utils::scale_value(PREVIEW_RIGHT_OFFSET) * mult - Utils::scale_value(BUTTON_SIZE) * mult;

        auto offset = 0;
        if (name_)
        {
            offset += name_->cachedSize().height();
            offset += Utils::scale_value(NAME_OFFSET);
        }

        const auto bulletR = getBulletClickableRadius();
        QRect r(Utils::scale_value(HOR_OFFSET + BUTTON_SIZE + PREVIEW_RIGHT_OFFSET), Utils::scale_value(PROGRESS_OFFSET) + offset - bulletR / 2, progressWidth, bulletR);
        return r;
    }

    QRect PttItem::getBulletRect() const
    {
        const auto r = getPlaybackRect();
        const auto cur = r.width() * progress_ / 100.0 + Utils::scale_value(HOR_OFFSET + BUTTON_SIZE + PREVIEW_RIGHT_OFFSET);
        QRect bullet(0, 0, getBulletClickableRadius(), getBulletClickableRadius());
        bullet.moveCenter(QPoint(cur, r.center().y()));
        return bullet;
    }

    void PttItem::setPlayState(const ButtonState& _state)
    {
        playState_ = _state;
    }

    ButtonState PttItem::playState() const
    {
        return playState_;
    }

    void PttItem::setTextState(const ButtonState& _state)
    {
        textState_ = _state;
    }

    ButtonState PttItem::textState() const
    {
        return textState_;
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

        datePressed_ = _active;
    }

    bool PttItem::isDatePressed() const
    {
        return datePressed_;
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
        , animation_(new QVariantAnimation(this))
    {
        connect(Ui::GetDispatcher(), &core_dispatcher::fileSharingFileDownloaded, this, &PttList::onFileDownloaded);
        connect(Ui::GetDispatcher(), &core_dispatcher::speechToText, this, &PttList::onPttText);
#ifndef STRIP_AV_MEDIA
        connect(GetSoundsManager(), &SoundsManager::pttPaused, this, &PttList::onPttPaused);
        connect(GetSoundsManager(), &SoundsManager::pttFinished, this, &PttList::onPttFinished);
#endif // !STRIP_AV_MEDIA
        connect(animation_, &QVariantAnimation::valueChanged, this, [this](const QVariant& value)
        {
            const auto cur = value.toInt();
            for (auto& i : Items_)
            {
                if (!i->isDateItem() && isItemPlaying(i))
                {
                    i->setProgress(cur);
                    if (cur == 100)
                        QTimer::singleShot(200, this, [this]() { finishPtt(playingId_, PttFinish::KeepProgress); update(); });

                    update();
                    break;
                }
            }
        });

        setMouseTracking(true);
    }

    void PttList::initFor(const QString& _aimId)
    {
        MediaContentWidget::initFor(_aimId);
        animation_->stop();
        Items_.clear();
        RequestIds_.clear();
        setFixedHeight(0);
    }

    void PttList::processItems(const QVector<Data::DialogGalleryEntry>& _entries)
    {
        auto h = height();
        for (const auto& e : _entries)
        {
            auto time = QDateTime::fromSecsSinceEpoch(e.time_);
            auto item = std::make_unique<PttItem>(e.url_, formatTimeStr(time), e.msg_id_, e.seq_, width(), e.sender_, e.outgoing_, e.time_);
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
            if (e.action_ == u"del")
            {
                Items_.erase(std::remove_if(Items_.begin(), Items_.end(), [e](const auto& i) { return i->getMsg() == e.msg_id_; }), Items_.end());
                h = 0;
                for (const auto& i : Items_)
                    h += i->getHeight();

                continue;
            }

            Utils::UrlParser parser;
            parser.process(e.url_);

            const auto isFilesharing = parser.hasUrl() && parser.getUrl().is_filesharing();
            if (!isFilesharing || e.type_ != u"ptt")
                continue;

            auto time = QDateTime::fromSecsSinceEpoch(e.time_);

            auto iter = Items_.cbegin();
            for (; iter != Items_.cend(); ++iter)
            {
                if (iter->get()->isDateItem())
                {
                    auto dt = QDateTime::fromSecsSinceEpoch(iter->get()->time());
                    if (dt.date().month() == time.date().month() && dt.date().year() == time.date().year())
                        continue;
                }

                auto curMsg = iter->get()->getMsg();
                auto curSeq = iter->get()->getSeq();
                if (curMsg > e.msg_id_ || (curMsg == e.msg_id_ && curSeq > e.seq_))
                    continue;

                break;
            }

            auto item = std::make_unique<PttItem>(e.url_, formatTimeStr(time), e.msg_id_, e.seq_, width(), e.sender_, e.outgoing_, e.time_);
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
#ifndef STRIP_AV_MEDIA
            GetSoundsManager()->pausePtt(playingId_);
#endif // !STRIP_AV_MEDIA
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
        const auto leftButton = _event->button() == Qt::LeftButton;
        auto h = 0;
        for (auto& i : Items_)
        {
            auto r = QRect(0, h, width(), i->getHeight());
            if (r.contains(_event->pos()))
            {
                auto p = _event->pos();
                p.setY(p.y() - h);
                if ((i->playClicked(p) || i->pauseClicked(p)) && leftButton)
                {
                    i->setPlayState(ButtonState::PRESSED);
                }
                else if (i->textClicked(p) && leftButton)
                {
                    i->setTextState(ButtonState::PRESSED);
                }
                else if (i->isOverProgressBar(p) && leftButton)
                {
                    i->changeProgressPressed(true);
                    continuePlaying_ = i->isPlaying();
                    updateItemProgress(i, p);
                    i->updateBulletHoverState(p);
                    updateTooltipState(i, p, h);
                }
                else
                {
                    i->setPlayState(ButtonState::NORMAL);
                    i->setTextState(ButtonState::NORMAL);
                }
            }

            i->setDateState(false, i->isOverDate(_event->pos()) && leftButton);

            if (i->isOverMoreButton(_event->pos(), h) && leftButton)
                i->setMoreButtonState(ButtonState::PRESSED);

            h += i->getHeight();
        }

        update();

        pos_ = _event->pos();
        MediaContentWidget::mousePressEvent(_event);
    }

    void PttList::mouseReleaseEvent(QMouseEvent *_event)
    {
        Utils::GalleryMediaActionCont cont(MediaTypeName, aimId_);
        for (auto& i : Items_)
        {
            if (continuePlaying_ && i->isProgressPressed())
            {
                if (lastProgress_ != 100)
                    play(i);
                else
                    continuePlaying_ = false;
            }
            i->changeProgressPressed(false);
        }

        if (_event->button() == Qt::RightButton)
        {
            cont.happened();
            return MediaContentWidget::mouseReleaseEvent(_event);
        }

        auto h = 0;
        for (auto& i : Items_)
        {
            auto r = QRect(0, h, width(), i->getHeight());
            auto p = _event->pos();
            p.ry() -= h;
            if (!r.contains(_event->pos()))
            {
                i->setPlayState(ButtonState::NORMAL);
                i->setTextState(ButtonState::NORMAL);
                i->setDateState(false, false);
                i->setMoreButtonState(ButtonState::HIDDEN);
                i->updateBulletHoverState(p);
                h += i->getHeight();
                continue;
            }

            if (i->playClicked(p))
            {
                if (i->playState() == ButtonState::PRESSED)
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
                    cont.happened();
                }
                i->setPlayState(ButtonState::HOVERED);
            }
            else if (i->pauseClicked(p))
            {
                if (i->playState() == ButtonState::PRESSED)
                {
                    i->setPause(true);
#ifndef STRIP_AV_MEDIA
                    GetSoundsManager()->pausePtt(playingId_);
#endif // !STRIP_AV_MEDIA
                    lastProgress_ = animation_->state() == QAbstractAnimation::Stopped ? 0 : animation_->currentValue().toDouble();
                    animation_->pause();
                    cont.happened();
                }
                i->setPlayState(ButtonState::HOVERED);
            }
            else if (i->textClicked(p))
            {
                if (i->textState() == ButtonState::PRESSED)
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
                    cont.happened();
                }
                i->setTextState(ButtonState::HOVERED);
            }
            else
            {
                i->setPlayState(ButtonState::NORMAL);
                i->setTextState(ButtonState::NORMAL);
            }

            if (i->isOverDate(_event->pos()))
            {
                if (i->isDatePressed())
                {
                    Q_EMIT Logic::getContactListModel()->select(aimId_, i->getMsg(), Logic::UpdateChatSelection::No);
                    cont.happened();
                }
                i->setDateState(true, false);
            }
            else
            {
                i->setDateState(false, false);
            }

            if (i->isOverMoreButton(_event->pos(), h))
            {
                if (i->moreButtonState() == ButtonState::PRESSED)
                {
                    cont.happened();
                    showContextMenu(ItemData(i->getLink(), i->getMsg(), i->sender(), i->time()), _event->pos(), true);
                }
                i->setMoreButtonState(ButtonState::HOVERED);
            }
            else
            {
                i->setMoreButtonState(ButtonState::NORMAL);
            }

            i->updateBulletHoverState(p);
            const auto overProgress = i->isOverProgressBar(p);
            if (overProgress)
                updateTooltipState(i, p, h);

            h += i->getHeight();
        }


        update();
    }

    void PttList::mouseMoveEvent(QMouseEvent* _event)
    {
        auto h = 0;
        auto point = false;
        const bool leftButtonPressed = _event->buttons() & Qt::LeftButton;
        const bool rightButtonPressed = _event->buttons() & Qt::RightButton;
        bool tooltipForced = false;

        for (auto& i : Items_)
        {
            auto r = QRect(0, h, width(), i->getHeight());
            const auto progressPressed = i->isProgressPressed();
            const auto oneOfButtonsPressed = i->playState() == ButtonState::PRESSED || i->textState() == ButtonState::PRESSED || i->isDatePressed() || i->moreButtonState() == ButtonState::PRESSED;
            const auto containsCursor = r.contains(_event->pos());
            const auto hovered = containsCursor && !leftButtonPressed && !rightButtonPressed;

            auto p = _event->pos();
            p.ry() -= h;
            if (hovered && !oneOfButtonsPressed)
            {
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
                    i->setMoreButtonState(ButtonState::NORMAL);
                }
            }
            if (!hovered && !oneOfButtonsPressed && !progressPressed)
                i->setMoreButtonState(ButtonState::HIDDEN);

            const auto overProgress = i->isOverProgressBar(p);
            const auto progressHovered = overProgress && !leftButtonPressed;
            const auto dragProgress = progressPressed && leftButtonPressed;

            if (!leftButtonPressed && !rightButtonPressed)
                i->updateBulletHoverState(p);

            if ((progressHovered || dragProgress) && !oneOfButtonsPressed && !rightButtonPressed)
            {
                point = true;
                continuePlaying_ &= progressPressed;

                tooltipForced = true;
                if (dragProgress)
                    updateItemProgress(i, p);
                updateTooltipState(i, p, h);
            }

            point |= oneOfButtonsPressed;

            h += i->getHeight();
        }

        Tooltip::forceShow(tooltipForced);
        if (!tooltipForced)
            Tooltip::hide();
        point &= !rightButtonPressed;
        setCursor(point ? Qt::PointingHandCursor : Qt::ArrowCursor);
        update();
    }

    void PttList::leaveEvent(QEvent* _event)
    {
        for (auto& i : Items_)
        {
            i->setMoreButtonState(ButtonState::HIDDEN);
            i->changeProgressPressed(false);
        }

        Tooltip::forceShow(false);
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
                lastProgress_ = i->getProgress();
                if (i->isPending())
                    updateAnimation(i);
                else
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

    void PttList::finishPtt(int id, const PttFinish _state)
    {
        if (id != -1 && id == playingId_)
        {
            for (auto& i : Items_)
            {
                if (!i->isDateItem() && isItemPlaying(i))
                {
                    animation_->stop();
                    i->setPlaying(false);
                    if (_state == PttFinish::DropProgress)
                        i->setProgress(0);
                    playingId_ = -1;
                    playingIndex_ = std::make_pair(-1, -1);
                    break;
                }
            }
        }
    }

    void PttList::play(const std::unique_ptr<BasePttItem>& _item)
    {
#ifndef STRIP_AV_MEDIA
        if (!_item->isPlaying() || _item->getLocalPath().isEmpty())
            return;

        if (!isItemPlaying(_item))
            finishPtt(playingId_, PttFinish::KeepProgress);

        int duration = initAnimation(_item, AnimationState::Play);
        lastProgress_ = lastProgress_ == 100 ? 0 : lastProgress_;
        animation_->stop();


        auto updateProgress = [this, &_item, duration]()
        {
            GetSoundsManager()->setProgressOffset(playingId_, (lastProgress_ * 1.) / 100);
            animation_->setStartValue(lastProgress_);
            animation_->setDuration(duration * (100. - lastProgress_) / 100);
            _item->setProgress(lastProgress_);
            animation_->start();
        };

#ifdef USE_SYSTEM_OPENAL
        QTimer::singleShot(20, this, updateProgress);
#else
        updateProgress();
#endif

#endif // !STRIP_AV_MEDIA
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
        auto prevDt = QDateTime::fromSecsSinceEpoch(iter->get()->time()).date();
        ++iter;

        while (iter != Items_.end())
        {
            auto isDate = iter->get()->isDateItem();
            auto dt = QDateTime::fromSecsSinceEpoch(iter->get()->time()).date();
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
                if (!i->isDateItem() && isItemPlaying(i))
                {
                    if (!continuePlaying_)
                        i->setPause(true);
                    lastProgress_ = animation_->state() == QAbstractAnimation::Stopped ? lastProgress_ : animation_->currentValue().toInt();
                    if (animation_->state() != QAbstractAnimation::Stopped)
                        animation_->pause();
                    update();
                    break;
                }
            }
        }
    }

    void Ui::PttList::updateTooltipState(const std::unique_ptr<BasePttItem>& _item, const QPoint& _p, int _dH)
    {
        if (_item->isOverProgressBar(_p) || _item->isProgressPressed())
        {
            tooltipPos_ = _item->getTooltipPos(_p);
            tooltipPos_.ry() += _dH;
            tooltipText_ = _item->getTimeString(_p);
            showTooltip();
        }
        else
        {
            Tooltip::hide();
        }

    }

    bool Ui::PttList::isItemPlaying(const std::unique_ptr<BasePttItem>& _item) const
    {
        return _item->getMsg() == playingIndex_.first && _item->getSeq() == playingIndex_.second;
    }

    int Ui::PttList::initAnimation(const std::unique_ptr<BasePttItem>& _item, AnimationState _state)
    {
        int duration = 0;
#ifndef STRIP_AV_MEDIA
        const auto storedProgress = _item->getProgress();
        playingId_ = GetSoundsManager()->playPtt(_item->getLocalPath(), playingId_, duration);
        if (_state == AnimationState::Pause)
            GetSoundsManager()->pausePtt(playingId_);

        playingIndex_ = { _item->getMsg(), _item->getSeq() };
        animation_->setEndValue(100);

        if (_state == AnimationState::Play)
            Ui::GetDispatcher()->post_stats_to_core(core::stats::stats_event_names::chatscr_playptt_action, { { "from_gallery", "yes" },  { "chat_type", Ui::getStatsChatType() } });
        else
            animation_->stop();
        lastProgress_ = storedProgress;
#endif // !STRIP_AV_MEDIA
        return duration;
    }

    void Ui::PttList::updateAnimation(std::unique_ptr<BasePttItem>& _item)
    {
        auto duration = initAnimation(_item, AnimationState::Pause);
        lastProgress_ = _item->getProgress();
        animation_->setStartValue(lastProgress_);
        animation_->setDuration(duration * (100. - lastProgress_) / 100);
        _item->setProgress(lastProgress_);
        _item->setPending(false);
        update();
    }

    void Ui::PttList::updateItemProgress(std::unique_ptr<BasePttItem>& _item, const QPoint& _pos)
    {
        _item->getProgress(_pos);
        auto localPath = _item->getLocalPath();
        if (localPath.isEmpty())
        {
            auto reqId = Ui::GetDispatcher()->downloadSharedFile(aimId_, _item->getLink(), true, QString(), true);
            _item->setReqId(reqId);
            _item->setPending(true);
            RequestIds_.push_back(reqId);
        }
        else
        {
            updateAnimation(_item);
        }
    }

    void Ui::PttList::showTooltip() const
    {
        auto pos = mapToGlobal(tooltipPos_);
        const auto r = getBulletClickableRadius();
        pos.rx() -= r / 2;
        Tooltip::show(tooltipText_, QRect(pos, QSize(r, r)));
    }
}
