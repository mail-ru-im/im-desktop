#include "stdafx.h"
#include "ShowHideButton.h"

#include "utils/utils.h"
#include "styles/ThemeParameters.h"

namespace
{
    QSize buttonSize()
    {
        return Utils::scale_value(QSize(64, 16));
    }

    int arrowWidth()
    {
        return Utils::scale_value(4);
    }

    int underlayWidth()
    {
        return Utils::scale_value(6);
    }

    constexpr double getBlurAlpha() noexcept
    {
        return 0.45;
    }
}

namespace Ui
{
    ShowHideButton::ShowHideButton(QWidget* _parent)
        : ClickableWidget(_parent)
    {
        setFixedSize(buttonSize());
        updateTooltipText();

        connect(this, &ClickableWidget::hoverChanged, this, Utils::QOverload<>::of(&ShowHideButton::update));
        connect(this, &ClickableWidget::pressChanged, this, Utils::QOverload<>::of(&ShowHideButton::update));
    }

    void ShowHideButton::updateBackgroundColor(const QColor& _color)
    {
        if (!_color.isValid() || _color.alpha() == 0)
        {
            auto color = Styling::getParameters().getColor(Styling::StyleVariable::BASE_GLOBALWHITE);
            color.setAlphaF(getBlurAlpha());
            bgColor_ = color;
        }
        else
        {
            bgColor_ = QColor();
        }
        update();
    }

    void ShowHideButton::setArrowState(const ArrowState _state)
    {
        if (state_ != _state)
        {
            state_ = _state;
            updateTooltipText();
            update();
        }
    }

    void ShowHideButton::paintEvent(QPaintEvent* _e)
    {
        QPainter p(this);
        p.setRenderHints(QPainter::Antialiasing);
        p.setBrush(Qt::NoBrush);

        const auto& line = getArrowLine();
        const auto drawArrow = [&line, &p](const auto& _clr, const int _width)
        {
            p.setPen(QPen(_clr, _width, Qt::SolidLine, Qt::RoundCap, Qt::MiterJoin));
            p.drawPolyline(line.data(), line.size());
        };

        if (bgColor_.isValid() && bgColor_.alpha() != 0)
            drawArrow(bgColor_, underlayWidth());

        drawArrow(getMouseStateColor(), arrowWidth());
    }

    QColor ShowHideButton::getMouseStateColor() const
    {
        if (bgColor_.isValid())
        {
            if (isPressed())
                return Styling::getParameters().getColor(Styling::StyleVariable::GHOST_PRIMARY_INVERSE_ACTIVE);
            else if (isHovered())
                return Styling::getParameters().getColor(Styling::StyleVariable::GHOST_PRIMARY_INVERSE_HOVER);
            return Styling::getParameters().getColor(Styling::StyleVariable::GHOST_PRIMARY_INVERSE);
        }

        if (isPressed())
            return Styling::getParameters().getColor(Styling::StyleVariable::BASE_SECONDARY_ACTIVE);
        else if (isHovered())
            return Styling::getParameters().getColor(Styling::StyleVariable::BASE_SECONDARY_HOVER);
        return Styling::getParameters().getColor(Styling::StyleVariable::BASE_SECONDARY);
    }

    void ShowHideButton::updateTooltipText()
    {
        hideToolTip();

        switch (state_)
        {
        case ArrowState::up:
            setTooltipText(QT_TRANSLATE_NOOP("smartreply", "Show smart replies"));
            break;

        case ArrowState::down:
            setTooltipText(QT_TRANSLATE_NOOP("smartreply", "Hide smart replies"));
            break;

        default:
            setTooltipText(QString());
            break;
        }
    }

    const std::vector<QPoint>& ShowHideButton::getArrowLine() const
    {
        if (state_ == ArrowState::up)
        {
            static const std::vector<QPoint> v =
            {
                Utils::scale_value(QPoint(22, 12)),
                Utils::scale_value(QPoint(32, 8)),
                Utils::scale_value(QPoint(42, 12)),
            };
            return v;
        }
        else if (state_ == ArrowState::down)
        {
            static const std::vector<QPoint> v =
            {
                Utils::scale_value(QPoint(22, 8)),
                Utils::scale_value(QPoint(32, 12)),
                Utils::scale_value(QPoint(42, 8)),
            };
            return v;
        }

        static const std::vector<QPoint> v =
        {
            Utils::scale_value(QPoint(22, 10)),
            Utils::scale_value(QPoint(42, 10)),
        };
        return v;
    }
}