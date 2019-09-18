#include "stdafx.h"

#include "CircleHover.h"
#include "utils/utils.h"

namespace
{
    constexpr std::chrono::milliseconds getAnimDuration() noexcept { return std::chrono::milliseconds(200); }
}

namespace Ui
{
    CircleHover::CircleHover()
        : QWidget(nullptr),
        minDiameter_(Utils::scale_value(36)),
        maxDiameter_(Utils::scale_value(92))
    {
        setWindowFlags(Qt::FramelessWindowHint | Qt::SubWindow | Qt::WindowTransparentForInput);
        setAttribute(Qt::WA_NoSystemBackground);
        setAttribute(Qt::WA_TranslucentBackground);
        setAttribute(Qt::WA_TransparentForMouseEvents);
        setAttribute(Qt::WA_ShowWithoutActivating);
        setFocusPolicy(Qt::NoFocus);

        setFixedSize(maxDiameter_ * 1.1, maxDiameter_ * 1.1);
    }

    CircleHover::~CircleHover() = default;

    void CircleHover::setColor(const QColor& _color)
    {
        if (color_ != _color)
        {
            color_ = _color;
            update();
        }
    }

    void CircleHover::setDestWidget(QPointer<QWidget> _dest)
    {
        dest_ = std::move(_dest);
    }

    void CircleHover::showAnimated()
    {
        if (dest_)
        {
            anim_.start([this]()
            {
                raise();
                update();
            }, 0., 1., getAnimDuration().count(), anim::sineInOut);
            move(dest_->mapToGlobal(dest_->rect().center()) - rect().center());
            raise();
            show();
        }
    }

    void CircleHover::hideAnimated()
    {
        if (dest_ && isVisible())
        {
            move(dest_->mapToGlobal(dest_->rect().center()) - rect().center());
            //show();
            //raise();

            anim_.start([this]()
            {
                raise();
                update();
            }, 1., 0., getAnimDuration().count(), anim::sineInOut);
        }
        else
        {
            hide();
        }
    }

    void CircleHover::forceHide()
    {
        anim_.finish();
        hide();
    }

    void CircleHover::paintEvent(QPaintEvent*)
    {
        QPainter p(this);
        p.setRenderHint(QPainter::Antialiasing, true);
        p.setRenderHint(QPainter::HighQualityAntialiasing, true);
        const auto r = rect();
        p.fillRect(r, Qt::transparent);
        p.setOpacity(anim_.current());
        p.translate(r.center());
        p.setPen(Qt::NoPen);
        p.setBrush(color_);
        const auto diameter = minDiameter_ + anim_.current() * (maxDiameter_ - minDiameter_);
        p.drawEllipse(QRect(-diameter / 2, -diameter / 2, diameter, diameter));
    }
}