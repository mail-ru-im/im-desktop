#include "stdafx.h"

#include "MessagesScrollbar.h"
#include "../../utils/utils.h"

namespace
{
    constexpr std::chrono::milliseconds autoScrollTimeout = std::chrono::minutes(5);
    const auto minOpacity = 0.f;
    const auto maxOpacity = 1.f;
}

namespace Ui
{
    MessagesScrollbar::MessagesScrollbar(QWidget *page)
        : QScrollBar(page)
        , AutoscrollEnablerTimer_(new QTimer(this))
        , OpacityAnimation_(minOpacity, maxOpacity, this, false)
        , CanScrollDown_(false)
        , IsHovered_(false)
    {
        AutoscrollEnablerTimer_->setInterval(autoScrollTimeout.count());
        AutoscrollEnablerTimer_->setSingleShot(true);

        connect(AutoscrollEnablerTimer_, &QTimer::timeout, this, &MessagesScrollbar::onAutoScrollTimer);

        setContextMenuPolicy(Qt::NoContextMenu);

        setMinimum(0);
        setMaximum(0);
        setPageStep(100);
    }

    bool MessagesScrollbar::canScrollDown() const
    {
        return CanScrollDown_;
    }

    bool MessagesScrollbar::isInFetchRangeToBottom(const int32_t scrollPos) const
    {
        const auto min = minimum();
        const auto max = maximum();
        const auto range = max - min;
        return scrollPos == -1 || (scrollPos <= max && ((max - scrollPos) < int32_t(range * 0.1)));
    }

    bool MessagesScrollbar::isInFetchRangeToTop(const int32_t scrollPos) const
    {
        const auto min = minimum();
        const auto max = maximum();
        const auto range = max - min;
        return scrollPos == -1 || (scrollPos >= min && ((scrollPos - min) < int32_t(range * 0.1)));
    }

    void MessagesScrollbar::scrollToBottom()
    {
        setValue(maximum());
    }

    bool MessagesScrollbar::isAtBottom() const
    {
        return (value() >= maximum());
    }

    bool MessagesScrollbar::isVisible() const
    {
        return OpacityAnimation_.currentOpacity() != minOpacity && QScrollBar::isVisible();
    }

    void MessagesScrollbar::fadeOut()
    {
        OpacityAnimation_.fadeOut();
    }

    void MessagesScrollbar::fadeIn()
    {
        OpacityAnimation_.fadeIn();
    }

    bool MessagesScrollbar::isHovered() const
    {
        return IsHovered_;
    }

    bool MessagesScrollbar::event(QEvent *_event)
    {
        switch (_event->type())
        {
        case QEvent::HoverEnter:
        {
            setHovered(true);
            setVisible(true);
        }
        break;
        case QEvent::HoverLeave:
        {
            setHovered(false);
        }
        break;
        default:
            break;
        }

        return QScrollBar::event(_event);
    }

    void MessagesScrollbar::setHovered(bool _hovered)
    {
        if (IsHovered_ == _hovered)
            return;

        IsHovered_ = _hovered;
        emit hoverChanged(_hovered);
    }

    void MessagesScrollbar::onAutoScrollTimer()
    {
        CanScrollDown_ = true;
    }

    void MessagesScrollbar::setVisible(bool visible)
    {
        if (visible != isVisible())
            OpacityAnimation_.setOpacity(visible ? maxOpacity : minOpacity);

        QScrollBar::setVisible(visible);
    }

    int32_t MessagesScrollbar::getPreloadingDistance()
    {
        return Utils::scale_value(2000);
    }
}
