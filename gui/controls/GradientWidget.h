#pragma once

#include "styles/ThemeColor.h"

namespace Ui
{
    class GradientWidget : public QWidget
    {
        Q_OBJECT

    public:
        GradientWidget(
            QWidget* _parent,
            const Styling::ColorParameter& _left,
            const Styling::ColorParameter& _right,
            Qt::Orientation _orientation = Qt::Horizontal,
            const double _lPos = 0.0,
            const double _rPos = 1.0);
        void updateColors(const Styling::ColorParameter& _left, const Styling::ColorParameter& _right);

    protected:
        void paintEvent(QPaintEvent* _event) override;
        void resizeEvent(QResizeEvent* _event) override;

    private:
        bool isValid() const;

    private:
        QLinearGradient gradient_;
        Styling::ColorContainer left_;
        Styling::ColorContainer right_;

        Qt::Orientation orientation_;

        double leftPos_;
        double rightPos_;
    };

    class AnimatedGradientWidget : public GradientWidget
    {
        Q_OBJECT

    public:
        AnimatedGradientWidget(QWidget* _parent,
            const Styling::ColorParameter& _left,
            const Styling::ColorParameter& _right,
            Qt::Orientation _orientation = Qt::Horizontal,
            const double _lPos = 0.0,
            const double _rPos = 1.0);
        void fadeIn();
        void fadeOut();

    private:
        bool isAnimationRunning() const;
        bool isFadingIn() const;
        bool isFadingOut() const;

    private:
        QGraphicsOpacityEffect* effect_;
        QTimeLine* animation_;
    };
} // namespace Ui
