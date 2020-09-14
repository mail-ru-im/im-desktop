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
        : QWidget(nullptr)
        , minDiameter_(Utils::scale_value(36))
        , maxDiameter_(Utils::scale_value(92))
        , anim_(new QVariantAnimation(this))
    {
        setWindowFlags(Qt::FramelessWindowHint | Qt::SubWindow | Qt::WindowTransparentForInput);
        setAttribute(Qt::WA_NoSystemBackground);
        setAttribute(Qt::WA_TranslucentBackground);
        setAttribute(Qt::WA_TransparentForMouseEvents);
        setAttribute(Qt::WA_ShowWithoutActivating);
        setFocusPolicy(Qt::NoFocus);

        setFixedSize(maxDiameter_ * 1.1, maxDiameter_ * 1.1);

        anim_->setStartValue(0.0);
        anim_->setEndValue(1.0);
        anim_->setDuration(getAnimDuration().count());
        anim_->setEasingCurve(QEasingCurve::InOutSine);
        connect(anim_, &QVariantAnimation::valueChanged, this, [this]()
        {
            raise();
            update();
        });
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
            anim_->stop();
            anim_->setDirection(QAbstractAnimation::Forward);
            anim_->start();

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

            anim_->stop();
            anim_->setDirection(QAbstractAnimation::Backward);
            anim_->start();
        }
        else
        {
            hide();
        }
    }

    void CircleHover::forceHide()
    {
        anim_->stop();
        hide();
    }

    void CircleHover::paintEvent(QPaintEvent*)
    {
        QPainter p(this);
        p.setRenderHint(QPainter::Antialiasing, true);
        const auto r = rect();
        const auto curAnim = anim_->currentValue().toDouble();
        p.fillRect(r, Qt::transparent);
        p.setOpacity(curAnim);
        p.translate(r.center());
        p.setPen(Qt::NoPen);
        p.setBrush(color_);
        const auto diameter = minDiameter_ + curAnim * (maxDiameter_ - minDiameter_);
        p.drawEllipse(QRect(-diameter / 2, -diameter / 2, diameter, diameter));
    }
}