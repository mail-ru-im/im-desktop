#pragma once

#include "../utils.h"

namespace Utils
{

template<typename T>
class SlideToNewStateAnimated : public QWidget
{
public:
    enum class AnimationDirection
    {
        Right,
        Left,
    };

public:
    SlideToNewStateAnimated(QWidget* _parent)
        : QWidget(_parent)
        , contentWidget_(new T(this))
        , anim_(new QVariantAnimation(this))
        , background_(Utils::scale_bitmap(contentWidget_->size()))
    {
        static_assert(std::is_base_of_v<QWidget, T>, "T must inherit QWidget");

        Utils::check_pixel_ratio(background_);
        setFixedSize(contentWidget_->size());

        anim_->setStartValue(0.0);
        anim_->setEasingCurve(QEasingCurve::InOutQuad);
        connect(anim_, &QVariantAnimation::valueChanged, this, qOverload<>(&SlideToNewStateAnimated<T>::repaint));
        connect(anim_, &QVariantAnimation::finished, this, [this]() { needPaintBackground_ = false; });
    }

    T* content() const { return contentWidget_; }

    //! It's exposed to be used in a group animation
    QVariantAnimation* animation() const { return anim_; }

    void setDuration(int _durationMs) { anim_->setDuration(_durationMs); }

    void prepareNextState(std::function<void(T*)> _contentChanger, AnimationDirection _direction)
    {
        renderContentToBackground();
        setDirection(_direction);
        content()->setVisible(false);
        needPaintBackground_ = true;
        repaint();

        _contentChanger(content());

        anim_->setEndValue(direction_ == AnimationDirection::Right ? 1.0 : -1.0);
    }

protected:
    void setDirection(AnimationDirection _direction) { direction_ = _direction; }

    void renderContentToBackground()
    {
        background_.fill(Qt::transparent);
        QPainter painter(&background_);
        contentWidget_->render(&painter);
    }

    void paintEvent(QPaintEvent*) override
    {
        const auto isAnimationRunning = anim_->state() == QAbstractAnimation::State::Running;
        if (isAnimationRunning || needPaintBackground_)
        {
            QPainter p(this);

            const auto startX = anim_->endValue().toFloat() * content()->width();
            const auto offsetX = qRound(width() * anim_->currentValue().toFloat());

            if (needPaintBackground_)
            {
                const auto bgOffset = QPoint(-offsetX, 0);
                p.drawPixmap(bgOffset, background_);
            }

            if (isAnimationRunning)
            {
                const auto widgetOffset = QPoint(startX - offsetX, 0);
                contentWidget_->setGeometry({widgetOffset, contentWidget_->geometry().size()});
                contentWidget_->setVisible(true);
            }
        }
    }

private:
    T* contentWidget_ = nullptr;
    QVariantAnimation* anim_ = nullptr;
    QPixmap background_;
    AnimationDirection direction_;
    bool needPaintBackground_ = false;
};


} // end namespace Utils



