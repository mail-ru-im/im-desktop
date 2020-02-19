#include "stdafx.h"

#include "graphicsEffects.h"
#include "utils.h"
#include "InterConnector.h"
#include "main_window/MainWindow.h"

namespace
{
    int getScreenRatio()
    {
        if (auto mw = Utils::InterConnector::instance().getMainWindow())
        {
            if (auto wh = mw->windowHandle())
                return wh->devicePixelRatio();
        }
        return Utils::scale_bitmap_ratio();
    }
}

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

    GradientEdgesEffect::GradientEdgesEffect(QWidget* _parent, const int _width, const Qt::Alignment _sides, const int _offset)
        : QGraphicsEffect(_parent)
        , offset_(_offset)
        , width_(_width)
        , sides_(_sides)
    {
        assert(width_ > 0);
    }

    void GradientEdgesEffect::setSides(const Qt::Alignment _sides)
    {
        if (sides_ != _sides)
        {
            sides_ = _sides;
            update();
        }
    }

    void GradientEdgesEffect::setWidth(const int _width)
    {
        if (width_ != _width)
        {
            width_ = _width;
            update();
        }
    }

    void GradientEdgesEffect::setOffset(const int _offset)
    {
        if (offset_ != _offset)
        {
            offset_ = _offset;
            update();
        }
    }

    void GradientEdgesEffect::draw(QPainter* _painter)
    {
        QPoint offset;
        Qt::CoordinateSystem system = sourceIsPixmap() ? Qt::LogicalCoordinates : Qt::DeviceCoordinates;
        QPixmap pixmap = sourcePixmap(system, &offset, QGraphicsEffect::NoPad);
        if (pixmap.isNull())
            return;

        if (sides_)
        {
            QPainter p(&pixmap);
            p.setRenderHint(QPainter::Antialiasing);
            p.setCompositionMode(QPainter::CompositionMode_DestinationIn);

            if (sides_ & Qt::AlignLeft)
            {
                QLinearGradient gradient(offset_, 0, offset_ + width_, 0);
                gradient.setColorAt(0., Qt::transparent);
                gradient.setColorAt(1., Qt::black);

                p.fillRect(QRect(0, 0, offset_ + width_, pixmap.height()), gradient);
            }

            if (sides_ & Qt::AlignRight)
            {
                const auto w = pixmap.width() / getScreenRatio();
                QLinearGradient gradient(w - offset_- width_, 0, w - offset_, 0);
                gradient.setColorAt(0., Qt::black);
                gradient.setColorAt(1., Qt::transparent);

                p.fillRect(QRect(w - offset_ - width_, 0, width_ + offset_, pixmap.height()), gradient);
            }
        }

        Utils::PainterSaver ps(*_painter);
        if (system == Qt::DeviceCoordinates)
            _painter->setWorldTransform(QTransform());
        _painter->drawPixmap(offset, pixmap);
    }
}