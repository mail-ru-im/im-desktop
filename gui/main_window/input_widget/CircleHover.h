#pragma once

#include "animation/animation.h"

namespace Ui
{
    class CircleHover;

    class CircleHoverAnimated
    {
    public:
        virtual void setCircleHover(std::unique_ptr<CircleHover>&&) = 0;
        virtual void enableCircleHover(bool _val) = 0;
        virtual ~CircleHoverAnimated() = default;
    };

    class CircleHover : public QWidget
    {
        Q_OBJECT
    public:
        explicit CircleHover();
        ~CircleHover();

        void setColor(const QColor& _color);
        void setDestWidget(QPointer<QWidget> _dest);

        void showAnimated();
        void hideAnimated();
        void forceHide();

    protected:
        void paintEvent(QPaintEvent*) override;

    private:
        const int minDiameter_;
        const int maxDiameter_;
        QColor color_;

        QPointer<QWidget> dest_;

        anim::Animation anim_;
    };
}
