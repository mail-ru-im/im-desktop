#include "stdafx.h"

#include "GradientWidget.h"

namespace Ui
{
    GradientWidget::GradientWidget(QWidget* _parent, const QColor& _left, const QColor& _right, const double _lPos, const double _rPos)
        : QWidget(_parent)
        , left_(_left)
        , right_(_right)
        , leftPos_(_lPos)
        , rightPos_(_rPos)
    {
        setAttribute(Qt::WA_TransparentForMouseEvents);
    }

    void GradientWidget::updateColors(const QColor& _left, const QColor& _right)
    {
        left_ = _left.isValid() ? _left : Qt::transparent;
        right_ = _right.isValid() ? _right : Qt::transparent;
        update();
    }

    void GradientWidget::paintEvent(QPaintEvent* _event)
    {
        if (left_.alpha() == 0 && right_.alpha() == 0)
            return;

        QLinearGradient gradient(rect().topLeft(), rect().topRight());
        gradient.setColorAt(leftPos_, left_);
        gradient.setColorAt(rightPos_, right_);

        QPainter p(this);
        p.fillRect(rect(), gradient);
    }

}