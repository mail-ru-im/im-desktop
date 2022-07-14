#include "stdafx.h"

#include "GradientWidget.h"

namespace
{
    constexpr std::chrono::milliseconds defaultAnimationDuration() { return std::chrono::milliseconds(150); }
} // namespace

namespace Ui
{
    GradientWidget::GradientWidget(QWidget* _parent, const Styling::ColorParameter& _left, const Styling::ColorParameter& _right, Qt::Orientation _orientation, const double _lPos, const double _rPos)
        : QWidget(_parent)
        , left_(_left)
        , right_(_right)
        , orientation_(_orientation)
        , leftPos_(_lPos)
        , rightPos_(_rPos)
    {
        setAttribute(Qt::WA_TransparentForMouseEvents);
    }

    void GradientWidget::updateColors(const Styling::ColorParameter& _left, const Styling::ColorParameter& _right)
    {
        left_ = Styling::ColorContainer{ _left.color().isValid() ? _left : Styling::ColorParameter{ Qt::transparent } };
        right_ = Styling::ColorContainer{ _right.color().isValid() ? _right : Styling::ColorParameter{ Qt::transparent } };
        gradient_.setColorAt(leftPos_, left_.cachedColor());
        gradient_.setColorAt(rightPos_, right_.cachedColor());
        update();
    }

    void GradientWidget::paintEvent(QPaintEvent* _event)
    {
        if (!isValid())
            return;

        QPainter p(this);
        if (left_.canUpdateColor() || right_.canUpdateColor())
        {
            gradient_.setColorAt(leftPos_, left_.actualColor());
            gradient_.setColorAt(rightPos_, right_.actualColor());
        }
        p.fillRect(rect(), gradient_);
    }

    void GradientWidget::resizeEvent(QResizeEvent* _event)
    {
        QWidget::resizeEvent(_event);

        if (!isValid())
            return;

        gradient_.setStart(rect().topLeft());
        gradient_.setFinalStop(orientation_ == Qt::Horizontal ? rect().topRight() : rect().bottomLeft());
        gradient_.setColorAt(leftPos_, left_.cachedColor());
        gradient_.setColorAt(rightPos_, right_.cachedColor());
    }

    bool GradientWidget::isValid() const { return left_.cachedColor().alpha() != 0 || right_.cachedColor().alpha() != 0; }

    AnimatedGradientWidget::AnimatedGradientWidget(QWidget* _parent,
        const Styling::ColorParameter& _left,
        const Styling::ColorParameter& _right,
        Qt::Orientation _orientation,
        const double _lPos,
        const double _rPos)
        : GradientWidget(_parent, _left, _right, _orientation, _lPos, _rPos)
        , effect_ { new QGraphicsOpacityEffect(this) }
        , animation_ { new QTimeLine(defaultAnimationDuration().count(), this) }
    {
        effect_->setOpacity(0.0);
        setGraphicsEffect(effect_);

        connect(animation_, &QTimeLine::valueChanged, effect_, &QGraphicsOpacityEffect::setOpacity);
    }

    void AnimatedGradientWidget::fadeIn()
    {
        const bool animationRunning = isAnimationRunning();
        if ((effect_->opacity() == 1.0) || animationRunning && isFadingIn())
            return;
        animation_->setDirection(QTimeLine::Forward);
        if (!animationRunning)
            animation_->start();
    }

    void AnimatedGradientWidget::fadeOut()
    {
        const bool animationRunning = isAnimationRunning();
        if ((effect_->opacity() == 0.0) || animationRunning && isFadingOut())
            return;
        animation_->setDirection(QTimeLine::Backward);
        if (!animationRunning)
            animation_->start();
    }

    bool AnimatedGradientWidget::isAnimationRunning() const { return animation_->state() == QTimeLine::Running; }

    bool AnimatedGradientWidget::isFadingIn() const { return animation_->direction() == QTimeLine::Forward; }

    bool AnimatedGradientWidget::isFadingOut() const { return animation_->direction() == QTimeLine::Backward; }
} // namespace Ui
