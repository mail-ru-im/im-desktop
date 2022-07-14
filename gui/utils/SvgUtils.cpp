#include "stdafx.h"

#include "SvgUtils.h"
#include "utils.h"

namespace
{
    Utils::ColorLayers colorsFromContainers(Utils::ColorContainerLayers& _containers)
    {
        Utils::ColorLayers result;
        result.reserve(_containers.size());
        std::transform(
            _containers.begin(),
            _containers.end(),
            std::back_inserter(result),
            [](std::pair<QString, Styling::ColorContainer>& _container) { return std::make_pair(_container.first, _container.second.actualColor()); });
        return result;
    }

    Utils::ColorLayers colorsFromParameters(const Utils::ColorParameterLayers& _pararmeters)
    {
        Utils::ColorLayers result;
        result.reserve(_pararmeters.size());
        std::transform(
            _pararmeters.begin(),
            _pararmeters.end(),
            std::back_inserter(result),
            [](const auto& _parameter) { return std::make_pair(_parameter.first, _parameter.second.color()); });
        return result;
    }

    Utils::ColorContainerLayers containersFromParameters(const Utils::ColorParameterLayers& _pararmeters)
    {
        Utils::ColorContainerLayers result;
        result.reserve(_pararmeters.size());
        std::transform(
            _pararmeters.begin(),
            _pararmeters.end(),
            std::back_inserter(result),
            [](const auto& _parameter) { return std::make_pair(_parameter.first, Styling::ColorContainer { _parameter.second }); });
        return result;
    }
} // namespace

QPixmap Utils::renderSvg(const QString& _filePath, const QSize& _scaledSize, const QColor& _tintColor, const KeepRatio _keepRatio)
{
    if (Q_UNLIKELY(!QFileInfo::exists(_filePath)))
        return QPixmap();

    QSvgRenderer renderer(_filePath);
    const auto defSize = renderer.defaultSize();

    if (Q_UNLIKELY(_scaledSize.isEmpty() && defSize.isEmpty()))
        return QPixmap();

    QSize resultSize;
    QRect bounds;

    if (!_scaledSize.isEmpty())
    {
        resultSize = scale_bitmap(_scaledSize);
        bounds = QRect(QPoint(), resultSize);

        if (_keepRatio == KeepRatio::yes)
        {
            const double wRatio = defSize.width() / (double)resultSize.width();
            const double hRatio = defSize.height() / (double)resultSize.height();
            constexpr double epsilon = std::numeric_limits<double>::epsilon();

            if (Q_UNLIKELY(std::fabs(wRatio - hRatio) > epsilon))
            {
                const auto resultCenter = bounds.center();
                bounds.setSize(defSize.scaled(resultSize, Qt::KeepAspectRatio));
                bounds.moveCenter(resultCenter);
            }
        }
    }
    else
    {
        resultSize = scale_bitmap_with_value(defSize);
        bounds = QRect(QPoint(), resultSize);
    }

    QPixmap pixmap(resultSize);
    pixmap.fill(Qt::transparent);

    {
        QPainter painter(&pixmap);
        renderer.render(&painter, bounds);

        if (_tintColor.isValid())
        {
            painter.setCompositionMode(QPainter::CompositionMode_SourceIn);
            painter.fillRect(bounds, _tintColor);
        }
    }

    check_pixel_ratio(pixmap);

    return pixmap;
}

QPixmap Utils::renderSvgScaled(const QString& _resourcePath, const QSize& _unscaledSize, const QColor& _tintColor, const KeepRatio _keepRatio)
{
    return renderSvg(_resourcePath, Utils::scale_value(_unscaledSize), _tintColor, _keepRatio);
}

QPixmap Utils::renderSvgLayered(const QString& _filePath, const ColorLayers& _layers, const QSize& _scaledSize)
{
    if (Q_UNLIKELY(!QFileInfo::exists(_filePath)))
        return QPixmap();

    QSvgRenderer renderer(_filePath);
    const auto defSize = scale_bitmap_with_value(renderer.defaultSize());

    if (Q_UNLIKELY(_scaledSize.isEmpty() && defSize.isEmpty()))
        return QPixmap();

    QSize resultSize = _scaledSize.isEmpty() ? defSize : scale_bitmap(_scaledSize);
    QPixmap pixmap(resultSize);
    pixmap.fill(Qt::transparent);

    QMatrix scaleMatrix;
    if (!_scaledSize.isEmpty() && defSize != resultSize)
    {
        const auto s = double(resultSize.width()) / defSize.width();
        scaleMatrix.scale(s, s);
    }

    {
        QPainter painter(&pixmap);

        if (!_layers.empty())
        {
            for (const auto& [layerName, layerColor] : _layers)
            {
                if (!layerColor.isValid() || layerColor.alpha() == 0)
                    continue;

                im_assert(renderer.elementExists(layerName));
                if (!renderer.elementExists(layerName))
                    continue;

                QPixmap layer(resultSize);
                layer.fill(Qt::transparent);

                const auto elMatrix = renderer.matrixForElement(layerName);
                const auto elBounds = renderer.boundsOnElement(layerName);
                const auto mappedRect = scale_bitmap_with_value(elMatrix.mapRect(elBounds));
                auto layerBounds = scaleMatrix.mapRect(mappedRect);
                layerBounds.moveTopLeft(QPointF(fscale_bitmap_with_value(layerBounds.topLeft().x()), fscale_bitmap_with_value(layerBounds.topLeft().y())));

                {
                    QPainter layerPainter(&layer);
                    renderer.render(&layerPainter, layerName, layerBounds);

                    layerPainter.setCompositionMode(QPainter::CompositionMode_SourceIn);
                    layerPainter.fillRect(layer.rect(), layerColor);
                }
                painter.drawPixmap(QPoint(), layer);
            }
        }
        else
        {
            renderer.render(&painter, QRect(QPoint(), resultSize));
        }
    }

    check_pixel_ratio(pixmap);

    return pixmap;
}

bool Utils::operator==(const StyledPixmap& _lhs, const StyledPixmap& _rhs)
{
    return _lhs.path() == _rhs.path() && _lhs.size() == _rhs.size() && _lhs.color_ == _rhs.color_ && _lhs.keepRatio_ == _rhs.keepRatio_;
}

Utils::BasePixmap::BasePixmap(const QPixmap& _pixmap)
    : pixmap_ { _pixmap }
{}

void Utils::BasePixmap::setPixmap(const QPixmap& _pixmap)
{
    pixmap_ = _pixmap;
}

QPixmap Utils::BaseStyledPixmap::actualPixmap()
{
    updatePixmap();
    return cachedPixmap();
}

void Utils::BaseStyledPixmap::updatePixmap()
{
    if (canUpdate())
        setPixmap(generatePixmap());
}

Utils::BaseStyledPixmap::BaseStyledPixmap(const QPixmap& _pixmap, const QString& _path, const QSize& _size)
    : BasePixmap(_pixmap)
    , path_ { _path }
    , size_ { _size }
{}

Utils::StyledPixmap::StyledPixmap()
    : StyledPixmap(QString {}, QSize {}, Styling::ColorParameter {})
{}

Utils::StyledPixmap::StyledPixmap(const QString& _resourcePath, const QSize& _scaledSize, const Styling::ColorParameter& _tintColor, KeepRatio _keepRatio)
    : BaseStyledPixmap(Utils::renderSvg(_resourcePath, _scaledSize, _tintColor.color(), _keepRatio), _resourcePath, _scaledSize)
    , color_ { _tintColor }
    , keepRatio_ { _keepRatio }
{}

Utils::StyledPixmap Utils::StyledPixmap::scaled(const QString& _resourcePath, const QSize& _unscaledSize, const Styling::ColorParameter& _tintColor, KeepRatio _keepRatio)
{
    return StyledPixmap(_resourcePath, Utils::scale_value(_unscaledSize), _tintColor, _keepRatio);
}

bool Utils::StyledPixmap::canUpdate() const
{
    return color_.canUpdateColor();
}

QPixmap Utils::StyledPixmap::generatePixmap()
{
    return Utils::renderSvg(path(), size(), color_.actualColor(), keepRatio_);
}

Utils::LayeredPixmap::LayeredPixmap()
    : LayeredPixmap(QString {}, ColorParameterLayers {}, QSize {})
{}

Utils::LayeredPixmap::LayeredPixmap(const QString& _filePath, const ColorParameterLayers& _layers, const QSize& _scaledSize)
    : BaseStyledPixmap(Utils::renderSvgLayered(_filePath, colorsFromParameters(_layers), _scaledSize), _filePath, _scaledSize)
    , layers_ { containersFromParameters(_layers) }
{}

bool Utils::LayeredPixmap::canUpdate() const
{
    return std::any_of(layers_.begin(), layers_.end(), [](const auto& _layer) { return _layer.second.canUpdateColor(); });
}

QPixmap Utils::LayeredPixmap::generatePixmap()
{
    return Utils::renderSvgLayered(path(), colorsFromContainers(layers_), size());
}
