#pragma once

namespace Utils
{

struct ShadowLayer
{
    int xOffset_ = 0;
    int yOffset_ = 0;
    QColor color_;
};

void drawLayeredShadow(QPainter& _p, const QPainterPath& _path, const std::vector<ShadowLayer>& _layers);
void drawRoundedShadow(QPaintDevice* _paintDevice, const QRect& _rect, int _width, QStyle::State _state);

}
