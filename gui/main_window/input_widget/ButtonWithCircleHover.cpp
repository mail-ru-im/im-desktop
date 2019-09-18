#include "stdafx.h"

#include "ButtonWithCircleHover.h"
#include "controls/TooltipWidget.h"

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
        tooltipTimer_.setInterval(tooltipShowDelay().count());

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
            updateHoverCircle(containsCursorUnder());
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
            updateHoverCircle(underMouse_);
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
            Tooltip::show(t, QRect(mapToGlobal(r.topLeft()), r.size()), { -1, -1 }, Tooltip::ArrowDirection::Down);
        }
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