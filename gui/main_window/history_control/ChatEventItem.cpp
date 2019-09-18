#include "stdafx.h"


#include "../../controls/TextUnit.h"
#include "../../fonts.h"
#include "../../app_config.h"

#include "../../utils/log/log.h"
#include "../../utils/utils.h"
#include "../../styles/ThemeParameters.h"
#include "../mediatype.h"

#include "ChatEventInfo.h"
#include "MessageStyle.h"

#include "ChatEventItem.h"

namespace Ui
{
    namespace
    {
        qint32 getBubbleHorMargin(int _width)
        {
            if (_width >= Ui::MessageStyle::fiveHeadsWidth())
                return Utils::scale_value(136);

            if (_width >= Ui::MessageStyle::fourHeadsWidth())
                return Utils::scale_value(116);

            if (_width >= Ui::MessageStyle::threeHeadsWidth())
                return Utils::scale_value(96);

            return Utils::scale_value(32);
        }

        qint32 getTextHorPadding()
        {
            return Utils::scale_value(8);
        }

        qint32 getTextTopPadding()
        {
            return Utils::scale_value(4);
        }

        qint32 getTextBottomPadding()
        {
            return Utils::scale_value(5);//ask Andrew to explain
        }

        qint32 getWidgetMinHeight()
        {
            return Utils::scale_value(22);
        }

        constexpr auto fontSize() noexcept { return 12; }
    }

    ChatEventItem::ChatEventItem(const HistoryControl::ChatEventInfoSptr& _eventInfo, const qint64 _id, const qint64 _prevId)
        : MessageItemBase(nullptr)
        , TextWidget_(nullptr)
        , EventInfo_(_eventInfo)
        , height_(0)
        , id_(_id)
        , prevId_(_prevId)
    {
        setMouseTracking(true);
    }

    ChatEventItem::ChatEventItem(QWidget* _parent, const HistoryControl::ChatEventInfoSptr& _eventInfo, const qint64 _id, const qint64 _prevId)
        : MessageItemBase(_parent)
        , TextWidget_(nullptr)
        , EventInfo_(_eventInfo)
        , height_(0)
        , id_(_id)
        , prevId_(_prevId)
    {
        assert(EventInfo_);

        TextWidget_ = TextRendering::MakeTextUnit(
            EventInfo_->formatEventText(),
            {},
            EventInfo_->isCaptchaPresent() ? TextRendering::LinksVisible::SHOW_LINKS : TextRendering::LinksVisible::DONT_SHOW_LINKS
        );

        TextWidget_->applyLinks(EventInfo_->getMembersLinks());

        setMouseTracking(true);
    }

    ChatEventItem::~ChatEventItem()
    {
    }

    QString ChatEventItem::formatRecentsText() const
    {
        return EventInfo_->formatEventText();
    }

    MediaType ChatEventItem::getMediaType() const
    {
        return MediaType::noMedia;
    }

    void ChatEventItem::setLastStatus(LastStatus _lastStatus)
    {
        if (getLastStatus() != _lastStatus)
        {
            assert(_lastStatus == LastStatus::None);
            HistoryControlPageItem::setLastStatus(_lastStatus);
            updateGeometry();
            update();
        }
    }

    int32_t ChatEventItem::evaluateTextWidth(const int32_t _widgetWidth)
    {
        assert(_widgetWidth > 0);

        const auto maxBubbleWidth = _widgetWidth - 2 * getBubbleHorMargin(_widgetWidth);
        const auto maxBubbleContentWidth = maxBubbleWidth - 2 * getTextHorPadding();

        return maxBubbleContentWidth;
    }

    void ChatEventItem::updateStyle()
    {
        const auto textColor = Styling::getParameters(getContact()).getColor(Styling::StyleVariable::CHATEVENT_TEXT);

        TextWidget_->init(Fonts::adjustedAppFont(fontSize()), textColor, textColor, MessageStyle::getSelectionColor(), MessageStyle::getHighlightColor(), TextRendering::HorAligment::CENTER);
        TextWidget_->applyFontToLinks(Fonts::adjustedAppFont(fontSize(), Fonts::FontWeight::SemiBold));
        TextWidget_->getHeight(evaluateTextWidth(width()));

        updateSize();
    }

    void ChatEventItem::updateFonts()
    {
        updateStyle();
    }

    void ChatEventItem::updateSize()
    {
        updateSize(size());
    }

    void ChatEventItem::updateSize(const QSize& _newSize)
    {
        // setup the text control and get it dimensions
        const auto maxTextWidth = evaluateTextWidth(_newSize.width());
        const auto textHeight = TextWidget_->getHeight(maxTextWidth);
        const auto textWidth = TextWidget_->getMaxLineWidth();

        // evaluate bubble width
        const auto bubbleWidth = textWidth + 2 * getTextHorPadding();

        // evaluate bubble height
        const auto bubbleHeight = textHeight + getTextTopPadding() + getTextBottomPadding();

        BubbleRect_ = QRect(0, 0, bubbleWidth, bubbleHeight);
        BubbleRect_.moveCenter(QRect(0, 0, _newSize.width(), bubbleHeight).center());

        const auto topPadding = Utils::scale_value(8);
        BubbleRect_.moveTop(topPadding);

        // setup geometry
        height_ = bubbleHeight + topPadding + bottomOffset();

        TextWidget_->setOffsets((width() - TextWidget_->cachedSize().width()) / 2, BubbleRect_.top() + getTextTopPadding());
    }

    bool ChatEventItem::isOutgoing() const
    {
        return false;
    }

    int32_t ChatEventItem::getTime() const
    {
        return 0;
    }

    int ChatEventItem::bottomOffset() const
    {
        auto margin = Utils::scale_value(2);

        if (isChat() && hasHeads() && headsAtBottom())
            margin += MessageStyle::getLastReadAvatarSize() + MessageStyle::getLastReadAvatarOffset();

        return margin;
    }

    void ChatEventItem::paintEvent(QPaintEvent*)
    {
        QPainter p(this);

        p.setRenderHint(QPainter::Antialiasing);
        p.setRenderHint(QPainter::SmoothPixmapTransform);
        p.setRenderHint(QPainter::TextAntialiasing);
        p.setPen(Qt::NoPen);

        p.setBrush(Styling::getParameters(getContact()).getColor(Styling::StyleVariable::CHATEVENT_BACKGROUND));
        p.drawRoundedRect(BubbleRect_, MessageStyle::getBorderRadius(), MessageStyle::getBorderRadius());

        TextWidget_->drawSmart(p, BubbleRect_.center().y());

        drawHeads(p);

        if (Q_UNLIKELY(Ui::GetAppConfig().IsShowMsgIdsEnabled()))
        {
            p.setPen(TextWidget_->getColor());
            p.setFont(Fonts::appFontScaled(10, Fonts::FontWeight::SemiBold));

            const auto x = BubbleRect_.right() + 1 - MessageStyle::getBorderRadius();
            const auto y = BubbleRect_.bottom() + 1;
            Utils::drawText(p, QPointF(x, y), Qt::AlignRight | Qt::AlignBottom, QString::number(getId()));
        }
    }

    QSize ChatEventItem::sizeHint() const
    {
        return QSize(0, height_);
    }

    void ChatEventItem::resizeEvent(QResizeEvent* _event)
    {
        HistoryControlPageItem::resizeEvent(_event);
        updateSize(_event->size());
    }

    void ChatEventItem::mousePressEvent(QMouseEvent* _event)
    {
        pressPos_ = _event->pos();
        return HistoryControlPageItem::mousePressEvent(_event);
    }

    void ChatEventItem::mouseMoveEvent(QMouseEvent* _event)
    {
        if (TextWidget_->isOverLink(_event->pos()))
            setCursor(Qt::PointingHandCursor);
        else
            setCursor(Qt::ArrowCursor);

        return HistoryControlPageItem::mouseMoveEvent(_event);
    }

    void ChatEventItem::mouseReleaseEvent(QMouseEvent* _event)
    {
        if (Utils::clicked(pressPos_, _event->pos()))
            TextWidget_->clicked(_event->pos());

        return HistoryControlPageItem::mouseReleaseEvent(_event);
    }

    void ChatEventItem::clearSelection()
    {
        TextWidget_->clearSelection();
    }

    qint64 ChatEventItem::getId() const
    {
        return id_;
    }

    qint64 ChatEventItem::getPrevId() const
    {
        return prevId_;
    }

    void ChatEventItem::setQuoteSelection()
    {
        /// TODO-quote
        assert(0);
    }
};
