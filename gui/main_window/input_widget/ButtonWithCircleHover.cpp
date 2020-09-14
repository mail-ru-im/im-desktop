#include "stdafx.h"

#include "ButtonWithCircleHover.h"
#include "controls/TooltipWidget.h"
#include "InputWidgetUtils.h"

namespace
{
    constexpr std::chrono::milliseconds tooltipShowDelay() noexcept
    {
        using namespace std::chrono_literals;
        return 400ms;
    }
}

namespace Ui
{
    ButtonWithCircleHover::ButtonWithCircleHover(QWidget* _parent, const QString& _svgName, const QSize& _iconSize, const QColor& _defaultColor)
        : CustomButton(_parent, _svgName, _iconSize, _defaultColor)
    {
        tooltipTimer_.setSingleShot(true);
        tooltipTimer_.setInterval(tooltipShowDelay());

        connect(&tooltipTimer_, &QTimer::timeout, this, [this]() {
            if (enableCircleHover_ && underMouse_)
                showToolTip();
        });
    }

    ButtonWithCircleHover::~ButtonWithCircleHover() = default;

    void ButtonWithCircleHover::setCircleHover(std::unique_ptr<CircleHover>&& _circleHover)
    {
        _circleHover->setDestWidget(this);
        circleHover_ = std::move(_circleHover);
    }

    void ButtonWithCircleHover::enableCircleHover(bool _val)
    {
        if (enableCircleHover_ != _val)
        {
            enableCircleHover_ = _val;
            updateHoverCircle(containsCursorUnder() && !underLongPress_);
            setFocusColor(enableCircleHover_ ? Qt::transparent : focusColorPrimary());
        }
    }

    void ButtonWithCircleHover::setUnderLongPress(bool _val)
    {
        if (underLongPress_ != _val)
        {
            underLongPress_ = _val;
            update();
        }
    }

    void ButtonWithCircleHover::updateHover()
    {
        updateHover(containsCursorUnder());
    }

    void ButtonWithCircleHover::updateHover(bool _hover)
    {
        if (_hover != underMouse_)
        {
            underMouse_ = _hover;
            updateHoverCircle(underMouse_ && !underLongPress_);
        }
    }

    void ButtonWithCircleHover::setRectExtention(const QMargins& _m)
    {
        extention_ = _m;
    }

    void ButtonWithCircleHover::updateHoverCircle(bool _hover)
    {
        if (!circleHover_)
            return;
        if (enableCircleHover_)
        {
            if (_hover)
            {
                enableTooltip_ = true;
                tooltipTimer_.start();
                circleHover_->showAnimated();
            }
            else
            {
                enableTooltip_ = false;
                tooltipTimer_.stop();
                circleHover_->hideAnimated();
            }
        }
        else
        {
            enableTooltip_ = false;
            hideToolTip();
            circleHover_->forceHide();
        }
    }

    bool ButtonWithCircleHover::containsCursorUnder() const
    {
        if (enableCircleHover_)
            return rect().marginsAdded(extention_).contains(mapFromGlobal(QCursor::pos()));
        return rect().contains(mapFromGlobal(QCursor::pos()));
    }

    void ButtonWithCircleHover::showToolTipForce()
    {
        if (const auto& t = getCustomToolTip(); !t.isEmpty())
        {
            const auto r = rect();
            Tooltip::forceShow(true);
            Tooltip::show(t, QRect(mapToGlobal(r.topLeft()), r.size()), {0, 0}, Tooltip::ArrowDirection::Down);
        }
    }

    void ButtonWithCircleHover::keyPressEvent(QKeyEvent* _event)
    {
        if (!enableCircleHover_)
            CustomButton::keyPressEvent(_event);
    }

    void ButtonWithCircleHover::keyReleaseEvent(QKeyEvent* _event)
    {
        if (enableCircleHover_)
        {
            _event->ignore();
            if ((_event->key() == Qt::Key_Enter || _event->key() == Qt::Key_Return) && hasFocus())
                Q_EMIT clicked();
        }
        else
        {
            CustomButton::keyReleaseEvent(_event);
        }
    }

    void ButtonWithCircleHover::focusInEvent(QFocusEvent* _event)
    {
        CustomButton::focusInEvent(_event);
        if (enableCircleHover_ && qApp->mouseButtons() == Qt::MouseButton::NoButton)
            updateHoverCircle(true);
    }

    void ButtonWithCircleHover::focusOutEvent(QFocusEvent* _event)
    {
        CustomButton::focusOutEvent(_event);
        if (enableCircleHover_ && qApp->mouseButtons() == Qt::MouseButton::NoButton)
            updateHoverCircle(false);
    }

    void ButtonWithCircleHover::showToolTip()
    {
        if (enableTooltip_)
            showToolTipForce();
    }

    void ButtonWithCircleHover::hideToolTip()
    {
        Tooltip::forceShow(false);
        Tooltip::hide();
    }
}
