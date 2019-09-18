#pragma once

#include "animation/animation.h"

namespace Ui
{
    class PttLock : public QWidget
    {
    Q_OBJECT
    public:
        explicit PttLock(QWidget* _parent);
        ~PttLock();

        void setBottom(QPoint _p);

        void showAnimated();
        void hideAnimated();
        void hideForce();

    protected:
        void paintEvent(QPaintEvent*) override;

    private:
        void showAnimatedImpl();

        void showToolTip();
        void hideToolTip();

    private:
        anim::Animation anim_;
        QPoint bottom_;
        QPixmap pixmap_;
        QTimer showTimer_;
    };
}