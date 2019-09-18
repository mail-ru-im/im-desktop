#include "stdafx.h"

#include "graphicsEffects.h"
#include "utils.h"

namespace Utils
{
    OpacityEffect::OpacityEffect(QWidget* _parent)
        : QGraphicsEffect(_parent)
        , opacity_(1.0)
    {
    }

    void OpacityEffect::setOpacity(const double _opacity)
    {
        if (auto newOpacity = std::clamp(_opacity, 0.0, 1.0); opacity_ != newOpacity)
        {
            opacity_ = newOpacity;
            update();
        }
    }

    void OpacityEffect::draw(QPainter* _painter)
    {
        if (qFuzzyCompare(opacity_, 0.0))
            return;

        QPoint offset;
        Qt::CoordinateSystem system = sourceIsPixmap() ? Qt::LogicalCoordinates : Qt::DeviceCoordinates;
        QPixmap pixmap = sourcePixmap(system, &offset, QGraphicsEffect::NoPad);
        if (pixmap.isNull())
            return;

        Utils::PainterSaver ps(*_painter);
        _painter->setOpacity(opacity_);
        if (system == Qt::DeviceCoordinates)
            _painter->setWorldTransform(QTransform());
        _painter->drawPixmap(offset, pixmap);
    }
}