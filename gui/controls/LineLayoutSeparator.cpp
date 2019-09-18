#include "stdafx.h"

#include "LineLayoutSeparator.h"
#include "../styles/ThemeParameters.h"

namespace Ui
{
    QColor LineLayoutSeparator::defaultColor()
    {
        return Styling::getParameters().getColor(Styling::StyleVariable::BASE_BRIGHT);
    }

    LineLayoutSeparator::LineLayoutSeparator(int _width, Direction _direction, QWidget* _parent)
        : LineLayoutSeparator(_width, _direction, defaultColor(), _parent)
    {
    }

    LineLayoutSeparator::LineLayoutSeparator(int _width, Direction _direction, const QColor& _color, QWidget* _parent)
        : QWidget(_parent)
        , color_(_color)
    {
        if (_direction == Direction::Ver)
        {
            setFixedWidth(_width);
            setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Expanding);
        }
        else
        {
            setFixedHeight(_width);
            setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
        }
    }

    LineLayoutSeparator::~LineLayoutSeparator()
    {
    }

    void LineLayoutSeparator::paintEvent(QPaintEvent * _event)
    {
        QPainter p(this);

        p.setRenderHint(QPainter::Antialiasing);
        p.setRenderHint(QPainter::SmoothPixmapTransform);

        p.fillRect(rect(), color_);
    }
}