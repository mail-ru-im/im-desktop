#include "stdafx.h"

#include "ScrollButton.h"
#include "utils/PainterPath.h"
#include "utils/utils.h"
#include "styles/ThemeParameters.h"

namespace
{
    int buttonSize() { return Utils::scale_value(32); }
    QSize iconSize() { return Utils::scale_value(QSize(20, 20)); }
    QPixmap icon(const Styling::StyleVariable _var)
    {
        return Utils::renderSvg(qsl(":/controls/arrow_right"), iconSize(), Styling::getParameters().getColor(_var));
    }
}

namespace Ui
{
    ScrollButton::ScrollButton(QWidget* _parent, const ScrollButton::ButtonDirection _dir)
        : ClickableWidget(_parent)
        , dir_(_dir)
        , withBackground_(false)
    {
        setFixedSize(buttonSize(), buttonSize() + Utils::getShadowMargin() * 2);

        auto bubbleRect = QRect(0, 0, buttonSize(), buttonSize());
        bubbleRect.moveCenter(rect().center());

        bubble_.addEllipse(bubbleRect);

        connect(this, &ClickableWidget::hoverChanged, this, Utils::QOverload<>::of(&ScrollButton::update));
        connect(this, &ClickableWidget::pressChanged, this, Utils::QOverload<>::of(&ScrollButton::update));
    }

    void ScrollButton::setBackgroundVisible(const bool _visible)
    {
        if (withBackground_ != _visible)
        {
            withBackground_ = _visible;
            update();
        }
    }

    void ScrollButton::paintEvent(QPaintEvent* _e)
    {
        QPainter p(this);
        p.setRenderHints(QPainter::Antialiasing | QPainter::SmoothPixmapTransform);
        p.setPen(Qt::NoPen);

        if (withBackground_)
        {
            Utils::drawBubbleShadow(p, bubble_);
            p.fillPath(bubble_, getBubbleColor());
        }

        const auto icon = dir_ == ScrollButton::ButtonDirection::right ? getIcon() : Utils::mirrorPixmapHor(getIcon());
        const auto x = (width() - icon.width() / Utils::scale_bitmap_ratio()) / 2;
        const auto y = (height() - icon.height() / Utils::scale_bitmap_ratio()) / 2;
        p.drawPixmap(x, y, icon);
    }

    QPixmap ScrollButton::getIcon() const
    {
        QPixmap ret;
        if (isPressed())
        {
            static const auto pm = icon(Styling::StyleVariable::BASE_PRIMARY_ACTIVE);
            ret = pm;
        }
        else if (isHovered())
        {
            static const auto pm = icon(Styling::StyleVariable::BASE_PRIMARY_HOVER);
            ret = pm;
        }
        else
        {
            static const auto pm = icon(Styling::StyleVariable::BASE_PRIMARY);
            ret = pm;
        }
        return ret;
    }

    QColor ScrollButton::getBubbleColor() const
    {
        if (isPressed())
            return Styling::getParameters().getColor(Styling::StyleVariable::CHAT_PRIMARY_ACTIVE);
        else if (isHovered())
            return Styling::getParameters().getColor(Styling::StyleVariable::CHAT_PRIMARY_HOVER);

        return Styling::getParameters().getColor(Styling::StyleVariable::CHAT_PRIMARY);
    }
}