#include "stdafx.h"

#include "DrawUtils.h"

namespace Utils
{

void drawLayeredShadow(QPainter& _p, const QPainterPath& _path, const std::vector<ShadowLayer>& _layers)
{
    for (const auto& layer : _layers)
        _p.fillPath(_path.translated(layer.xOffset_, layer.yOffset_), layer.color_);
}

void setShadowGradient(QGradient& _gradient, QStyle::State _state)
{
    const bool active = _state & QStyle::State_Active;
    QColor windowGradientColor(0, 0, 0);
    windowGradientColor.setAlphaF(0.2);
    _gradient.setColorAt(0, windowGradientColor);
    windowGradientColor.setAlphaF(active ? 0.08 : 0.04);
    _gradient.setColorAt(0.2, windowGradientColor);
    windowGradientColor.setAlphaF(0.02);
    _gradient.setColorAt(0.6, active ? windowGradientColor : Qt::transparent);
    _gradient.setColorAt(1, Qt::transparent);
}

void drawRoundedShadow(QPaintDevice* _paintDevice, const QRect& _rect, int _width, QStyle::State _state)
{
    QPainter p(_paintDevice);
    QRect right = QRect(QPoint(_rect.width() - _width, _rect.y() + _width + 1), QPoint(_rect.width(), _rect.height() - _width - 1));
    QRect left = QRect(QPoint(_rect.x(), _rect.y() + _width + 1), QPoint(_rect.x() + _width, _rect.height() - _width - 1));
    QRect top = QRect(QPoint(_rect.x() + _width + 1, _rect.y()), QPoint(_rect.width() - _width - 1, _rect.y() + _width));
    QRect bottom = QRect(QPoint(_rect.x() + _width + 1, _rect.height() - _width), QPoint(_rect.width() - _width - 1, _rect.height()));

    QRect topLeft = QRect(_rect.topLeft(), QPoint(_rect.x() + _width, _rect.y() + _width));
    QRect topRight = QRect(QPoint(_rect.width() - _width, _rect.y()), QPoint(_rect.width(), _rect.y() + _width));
    QRect bottomLeft = QRect(QPoint(_rect.x(), _rect.height() - _width), QPoint(_rect.x() + _width, _rect.height()));
    QRect bottomRight = QRect(QPoint(_rect.width() - _width, _rect.height() - _width), _rect.bottomRight());

    QLinearGradient lg = QLinearGradient(right.topLeft(), right.topRight());
    setShadowGradient(lg, _state);
    p.fillRect(right, QBrush(lg));

    lg = QLinearGradient(left.topRight(), left.topLeft());
    setShadowGradient(lg, _state);
    p.fillRect(left, QBrush(lg));

    lg = QLinearGradient(top.bottomLeft(), top.topLeft());
    setShadowGradient(lg, _state);
    p.fillRect(top, QBrush(lg));

    lg = QLinearGradient(bottom.topLeft(), bottom.bottomLeft());
    setShadowGradient(lg, _state);
    p.fillRect(bottom, QBrush(lg));

    QRadialGradient g = QRadialGradient(topLeft.bottomRight(), _width);
    setShadowGradient(g, _state);
    p.fillRect(topLeft, QBrush(g));

    g = QRadialGradient(topRight.bottomLeft(), _width);
    setShadowGradient(g, _state);
    p.fillRect(topRight, QBrush(g));

    g = QRadialGradient(bottomLeft.topRight(), _width);
    setShadowGradient(g, _state);
    p.fillRect(bottomLeft, QBrush(g));

    g = QRadialGradient(bottomRight.topLeft(), _width);
    setShadowGradient(g, _state);
    p.fillRect(bottomRight, QBrush(g));
}

}
