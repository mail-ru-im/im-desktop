#include "stdafx.h"

#include "DrawUtils.h"

namespace Utils
{

void drawLayeredShadow(QPainter& _p, const QPainterPath& _path, const std::vector<ShadowLayer>& _layers)
{
    for (const auto& layer : _layers)
        _p.fillPath(_path.translated(layer.xOffset_, layer.yOffset_), layer.color_);
}

}
