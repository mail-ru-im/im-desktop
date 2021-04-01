#include "stdafx.h"

#include "../utils/utils.h"
#include "../utils/gui_metrics.h"

#include "ImageViewerImpl.h"
#ifndef STRIP_AV_MEDIA
#include "../main_window/mplayer/VideoPlayer.h"
#endif // !STRIP_AV_MEDIA
#include "../gui_settings.h"

#ifdef __APPLE__
#include "utils/macos/mac_support.h"
#endif

namespace
{
    const auto smoothUpdateTimeout = 100;

    auto minImageSize()
    {
        return Utils::scale_value(48);
    }
    auto minVideoSize()
    {
        return Utils::scale_value(380);
    }
}

Previewer::AbstractViewer::~AbstractViewer()
{
}

bool Previewer::AbstractViewer::canScroll() const
{
    return canScroll_;
}

QRect Previewer::AbstractViewer::rect() const
{
    return targetRect_;
}

void Previewer::AbstractViewer::scale(double _newScaleFactor, QPoint _anchor)
{
    auto actualScaleFactor = preferredScaleFactor_ * _newScaleFactor;

    const auto newSize = imageSize_ * actualScaleFactor;

    if (newSize.width() > viewportRect_.width()
        || newSize.height() > viewportRect_.height())
    {
        targetRect_ = getTargetRect(actualScaleFactor);

        const auto x = (newSize.width() > viewportRect_.width())
                    ? (newSize.width() - viewportRect_.width()) / actualScaleFactor / 2
                    : 0;

        const auto y = (newSize.height() > viewportRect_.height())
                    ? (newSize.height() - viewportRect_.height()) / actualScaleFactor / 2
                    : 0;

        fragment_ = QRect(x + offset_.x(), y + offset_.y(),
            targetRect_.width() / actualScaleFactor, targetRect_.height() / actualScaleFactor);

        fixBounds(imageSize_, fragment_);

        canScroll_ = true;
    }
    else
    {
        offset_ = QPoint();

        targetRect_ = getTargetRect(actualScaleFactor);

        fragment_ = QRect({0, 0}, imageSize_);

        canScroll_ = false;
    }

    scaleFactor_ = _newScaleFactor;

    doResize(fragment_, targetRect_);

    repaint();
}

void Previewer::AbstractViewer::repaint()
{
    static_cast<QWidget*>(parent())->update();
}

QRect Previewer::AbstractViewer::getTargetRect(double _scaleFactor) const
{
    QRect viewportRect;
    if (_scaleFactor > preferredScaleFactor_)
        viewportRect = viewportRect_;
    else
        viewportRect = initialViewport_;

    const auto imageFragment = QRect(0, 0, imageSize_.width() * _scaleFactor, imageSize_.height() * _scaleFactor);

    QRect viewport = imageFragment.intersected(viewportRect);

    const auto dx = initialViewport_.width() / 2 - imageFragment.width() / 2;
    const auto dy = initialViewport_.height() / 2 - imageFragment.height() / 2;
    const auto topLeft = QPoint(dx > 0 ? dx : 0, dy > 0 ? dy : 0);

    viewport.translate(topLeft);

    return viewport;
}

void Previewer::AbstractViewer::move(const QPoint& _offset)
{
}

void Previewer::AbstractViewer::paint(QPainter& _painter)
{
    doPaint(_painter, fragment_, targetRect_);
}

double Previewer::AbstractViewer::getPreferredScaleFactor() const
{
    return preferredScaleFactor_;
}

double Previewer::AbstractViewer::getScaleFactor() const
{
    return scaleFactor_;
}

double Previewer::AbstractViewer::actualScaleFactor() const
{
    return preferredScaleFactor_ * scaleFactor_;
}

Previewer::AbstractViewer::AbstractViewer(const QSize& _viewportSize, QWidget* _parent)
    : QObject(_parent)
    , canScroll_(false)
    , viewportRect_(QPoint(), _viewportSize)
    , preferredScaleFactor_(0.0)
    , scaleFactor_(0.0)
{
}

void Previewer::AbstractViewer::init(const QSize &_imageSize, const QSize& _viewport, const int _minSize)
{
    imageSize_ = _imageSize;
    initialViewport_ = QRect(QPoint(0, 0), _viewport);

    preferredScaleFactor_ = getPreferredScaleFactor(imageSize_, _minSize);
    offset_ = QPoint();

    scale(1);
}

void Previewer::AbstractViewer::doResize(const QRect& _source, const QRect& _target)
{
}

bool Previewer::AbstractViewer::closeFullscreen()
{
    return false;
}

bool Previewer::AbstractViewer::isZoomSupport() const
{
    return true;
}

double Previewer::AbstractViewer::getPreferredScaleFactor(const QSize& _imageSize, const int _minSize) const
{
    const auto size = Utils::scale_value(_imageSize);

    bool needToDownscale = initialViewport_.width() < size.width() || initialViewport_.height() < size.height();
    bool needToUpscale = _imageSize.width() < _minSize || _imageSize.height() < _minSize;

    if (needToDownscale)
    {
        const auto kx = size.width() / static_cast<double>(initialViewport_.width());
        const auto ky = size.height() / static_cast<double>(initialViewport_.height());

        return 1.0 / std::max(kx, ky) * Utils::getScaleCoefficient();
    }
    else if (needToUpscale)
    {
        const auto kx = size.width() / static_cast<double>(_minSize);
        const auto ky = size.height() / static_cast<double>(_minSize);

        auto sf = 1.0 / std::min(kx, ky) * Utils::getScaleCoefficient();

        if (size.width() * sf > initialViewport_.width())  // check if upscaled image exceeds initial viewport size
            return initialViewport_.width() / static_cast<double>(size.width()) * Utils::getScaleCoefficient();
        else if (size.height() * sf > initialViewport_.height())
            return initialViewport_.height() / static_cast<double>(size.height()) * Utils::getScaleCoefficient();
        else
            return sf;
    }

    return 1.0 * Utils::getScaleCoefficient();
}

void Previewer::AbstractViewer::fixBounds(const QSize& _bounds, QRect& _child)
{
    if (_child.x() < 0)
        _child.moveLeft(0);
    else if (_child.x() + _child.width() >= _bounds.width())
        _child.moveRight(_bounds.width());

    if (_child.y() < 0)
        _child.moveTop(0);
    else if (_child.y() + _child.height() >= _bounds.height())
        _child.moveBottom(_bounds.height());
}

std::unique_ptr<Previewer::AbstractViewer> Previewer::GifViewer::create(const QString& _fileName, const QSize& _viewportSize, QWidget* _parent)
{
    im_assert(_parent);
    auto viewer = std::unique_ptr<AbstractViewer>(new GifViewer(_fileName, _viewportSize, _parent));
//    auto gifViewer = static_cast<GifViewer*>(viewer.get());
//    gifViewer->init(gifViewer->gif_.frameRect(), _viewportSize);
    return viewer;
}

QWidget* Previewer::GifViewer::getParentForContextMenu() const
{
    return qobject_cast<QWidget*>(parent());
}

Previewer::GifViewer::GifViewer(const QString& _fileName, const QSize& _viewportSize, QWidget* _parent)
    : AbstractViewer(_viewportSize, _parent)
{
    gif_.setFileName(_fileName);
    connect(&gif_, &QMovie::frameChanged, this, &GifViewer::repaint);
    gif_.start();
}

void Previewer::GifViewer::doPaint(QPainter& _painter, const QRect& _source, const QRect& _target)
{
    auto pixmap = gif_.currentPixmap();
    Utils::check_pixel_ratio(pixmap);
    _painter.drawPixmap(_target, pixmap, _source);
}

std::unique_ptr<Previewer::AbstractViewer> Previewer::JpegPngViewer::create(const QPixmap &_image, const QSize &_viewportSize, QWidget *_parent, const QSize &_initialViewport, const bool _isVideoPreview)
{
    im_assert(_parent);
    auto viewer = std::unique_ptr<AbstractViewer>(new JpegPngViewer(_image, _viewportSize, _parent, _initialViewport));
    auto jpegViewer = static_cast<JpegPngViewer*>(viewer.get());
    jpegViewer->init(jpegViewer->originalImage_.size(), _initialViewport, _isVideoPreview ? minVideoSize() : minImageSize());
    return viewer;
}

Previewer::JpegPngViewer::JpegPngViewer(const QPixmap &_image, const QSize &_viewportSize, QWidget *_parent, const QSize &_initialViewport)
    : AbstractViewer(_viewportSize, _parent)
    , originalImage_(_image)
    , smoothUpdate_(true)
{
    targetRect_ = QRect(0,0, _initialViewport.width(), _initialViewport.height());
    canScroll_ = true;
    originalImage_.setDevicePixelRatio(1);

    smoothUpdateTimer_.setSingleShot(true);
    smoothUpdateTimer_.setInterval(smoothUpdateTimeout);

    connect(&smoothUpdateTimer_, &QTimer::timeout, this, [this]()
    {
        smoothUpdate_ = true;
        repaint();
    });
}

void Previewer::JpegPngViewer::scale(double _newScaleFactor, QPoint _anchor)
{
    if (_anchor.isNull())
        _anchor = viewportRect_.center();

    const auto newActualScaleFactor = _newScaleFactor * preferredScaleFactor_;

    const auto oldpoint = _anchor - offset_;
    const auto currentStepScale = newActualScaleFactor / actualScaleFactor();
    const auto newpoint = oldpoint * currentStepScale;
    const auto diff = newpoint - oldpoint;
    offset_ -= diff;

    scaleFactor_ = _newScaleFactor;
    canScroll_ = originalImage_.width() * newActualScaleFactor > viewportRect_.width() || originalImage_.height() * newActualScaleFactor > viewportRect_.height();

    smoothUpdate_ = false;
    smoothUpdateTimer_.start();

    repaint();
}

bool Previewer::JpegPngViewer::scaleBy(double _scaleFactorDiff, QPoint _anchor)
{
    scale(scaleFactor_ * _scaleFactorDiff, _anchor);
    return true;
}

void Previewer::JpegPngViewer::move(const QPoint &_offset)
{
    offset_ -= _offset;

    smoothUpdate_ = false;
    smoothUpdateTimer_.start();

    repaint();
}

QWidget* Previewer::JpegPngViewer::getParentForContextMenu() const
{
    return qobject_cast<QWidget*>(parent());
}

void Previewer::JpegPngViewer::constrainOffset()
{
    const auto imageSize = originalImage_.size() / originalImage_.devicePixelRatio() * actualScaleFactor();

    // x
    if (imageSize.width() > viewportRect_.width())
    {
        const auto maxAbsXOffset = imageSize.width() - viewportRect_.width();
        offset_.setX(std::min(0, offset_.x()));
        offset_.setX(std::max<int>(-maxAbsXOffset, offset_.x()));
    }
    else
    {
        const auto xOffset = (viewportRect_.width() - imageSize.width()) / 2;
        offset_.setX(xOffset);
    }

    // y
    if (imageSize.height() > viewportRect_.height())
    {
        const auto maxAbsYOffset = imageSize.height() - initialViewport_.height();
        offset_.setY(std::min(0, offset_.y()));
        offset_.setY(std::max<int>(-maxAbsYOffset, offset_.y()));
    }
    else
    {
        const auto yOffset = (initialViewport_.height() - imageSize.height()) / 2;
        offset_.setY(yOffset);
    }

    targetRect_ = QRect(offset_, imageSize).intersected(viewportRect_);
}

void Previewer::JpegPngViewer::doPaint(QPainter &_painter, const QRect &_source, const QRect &_target)
{
    constrainOffset();

    if (smoothUpdate_)
        _painter.setRenderHint(QPainter::SmoothPixmapTransform);

    _painter.translate(offset_);
    _painter.scale(actualScaleFactor(), actualScaleFactor());
    _painter.drawPixmap(0, 0, originalImage_);
}
