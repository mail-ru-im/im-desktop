#include "stdafx.h"

#include "../utils/utils.h"
#include "../utils/gui_metrics.h"
#include "main_window/mplayer/VideoPlayer.h"

#include "ImageViewerImpl.h"

#include "ImageViewerWidget.h"

namespace
{
    const int maxZoomSteps = 5;
    const int maxWheelStep = 500;
    const int viewerLoadDelay = 100;

    QPixmap scaledPreview(const QPixmap& _preview, const QSize& _originSize, const QSize& _maxSize)
    {
        auto imageSize = _originSize;

        if (imageSize.width() > _maxSize.width() || imageSize.height() > _maxSize.height())
            imageSize = imageSize.scaled(_maxSize, Qt::KeepAspectRatio);

        return _preview.scaled(imageSize, Qt::KeepAspectRatio);
    }
}


Previewer::ImageViewerWidget::ImageViewerWidget(QWidget* _parent)
    : QLabel(_parent)
    , zoomStep_(0)
    , videoWidthOffset_(0)
    , parent_(_parent)
    , firstOpen_(true)
{
    setAlignment(Qt::AlignCenter);
    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    setMouseTracking(true);


    viewerLoadTimer_.setSingleShot(true);
    viewerLoadTimer_.setInterval(viewerLoadDelay);
    connect(&viewerLoadTimer_, &QTimer::timeout, this, &ImageViewerWidget::onViewerLoadTimeout);
}

Previewer::ImageViewerWidget::~ImageViewerWidget()
{
}

void Previewer::ImageViewerWidget::showMedia(const MediaData& _mediaData)
{
    clear();
    zoomStep_ = 0;
    tmpViewer_.reset();

    setCursor(Qt::ArrowCursor);

    static QMimeDatabase db;
    QMimeType mime = db.mimeTypeForFile(_mediaData.fileName, QMimeDatabase::MatchContent);
    QString type = mime.preferredSuffix();

    if (Utils::is_image_extension_not_gif(type))
    {
        QPixmap pixmap;
        QSize originalSize;
        Utils::loadPixmapScaled(_mediaData.fileName, Utils::getMaxImageSize(), pixmap, originalSize);
        viewer_ = JpegPngViewer::create(pixmap, size(), this, viewSize_);

        if (firstOpen_)
            statistic::getGuiMetrics().eventGalleryPhotoLoaded();
    }
    else if (_mediaData.attachedPlayer)
    {
        viewer_ = FFMpegViewer::create(_mediaData, maxVideoSize(viewSize_), this, firstOpen_);
        connect(viewer_.get(), &FFMpegViewer::doubleClicked, this, &ImageViewerWidget::closeRequested);
        connect(viewer_.get(), &FFMpegViewer::fullscreenToggled, this, &ImageViewerWidget::fullscreenToggled);
        connect(viewer_.get(), &FFMpegViewer::playCLicked, this, &ImageViewerWidget::playClicked);
        connect(viewer_.get(), &FFMpegViewer::rightClicked, this, &ImageViewerWidget::rightClicked);
    }
    else
    {
        // video viewer is created after delay of 'viewerLoadDelay' ms, to avoid freezing in case of fast scrolling,
        // it happens because creation of separate window and gpu renderer takes some time
        currentData_ = _mediaData;
        viewerLoadTimer_.start();
    }

    firstOpen_ = false;
    repaint();
}

QWidget* Previewer::ImageViewerWidget::getParentForContextMenu() const
{
    return viewer_ ? viewer_->getParentForContextMenu() : nullptr;
}

void Previewer::ImageViewerWidget::showPixmap(const QPixmap &_pixmap, const QSize &_originalImageSize, bool _isVideoPreview)
{
    clear();
    zoomStep_ = 0;
    tmpViewer_.reset();

    QSize maxSize;
    if (_isVideoPreview)
        maxSize = maxVideoSize(viewSize_);
    else
        maxSize = Utils::getMaxImageSize();

    viewer_ = JpegPngViewer::create(scaledPreview(_pixmap, _originalImageSize, maxSize), size(), this, viewSize_, _isVideoPreview);
    repaint();
}

bool Previewer::ImageViewerWidget::tryScale(QWheelEvent* _event)
{
    if (viewer_ && _event->modifiers().testFlag(Qt::ControlModifier))
    {
        const auto delta = std::copysign(std::min(maxWheelStep, std::abs(_event->delta())), _event->delta());
        scaleBy(1. + delta / maxWheelStep, _event->pos());
        return true;
    }
    return false;
}

bool Previewer::ImageViewerWidget::isZoomSupport() const
{
    if (!viewer_)
        return false;

    return viewer_->isZoomSupport();
}

void Previewer::ImageViewerWidget::reset()
{
    viewer_.reset();
    tmpViewer_.reset();
    firstOpen_ = true;
    clear();
}

void Previewer::ImageViewerWidget::connectExternalWheelEvent(std::function<void(const QPoint&)> _func)
{
    assert(viewer_);
    if (!viewer_)
        return;

    connect(viewer_.get(), &AbstractViewer::mouseWheelEvent, this, [_func](const QPoint& _delta)
    {
        _func(_delta);
    });
}

bool Previewer::ImageViewerWidget::closeFullscreen()
{
    if (!viewer_)
        return false;

    return viewer_->closeFullscreen();
}

void Previewer::ImageViewerWidget::setVideoWidthOffset(int _offset)
{
    videoWidthOffset_ = _offset;
}

void Previewer::ImageViewerWidget::setViewSize(const QSize &_viewSize)
{
    viewSize_ = _viewSize;
}

void Previewer::ImageViewerWidget::hide()
{
    if (tmpViewer_)
        tmpViewer_->hide();

    if (viewer_)
        viewer_->hide();

    QWidget::hide();
}

void Previewer::ImageViewerWidget::show()
{
    QWidget::show();

    if (viewer_)
        viewer_->show();
}

void Previewer::ImageViewerWidget::bringToBack()
{
    if (viewer_)
        viewer_->bringToBack();
}

void Previewer::ImageViewerWidget::showOverAll()
{
    if (viewer_)
        viewer_->showOverAll();
}

void Previewer::ImageViewerWidget::mousePressEvent(QMouseEvent* _event)
{
    if (!viewer_ || _event->button() != Qt::LeftButton)
    {
        _event->ignore();
        return;
    }

    mousePos_ = _event->pos();
    const auto rect = viewer_->rect();
    if (rect.contains(mousePos_))
    {
        if (viewer_->canScroll())
            setCursor(Qt::ClosedHandCursor);
        _event->accept();
    }
    else
    {
        _event->ignore();
    }
}

void Previewer::ImageViewerWidget::mouseReleaseEvent(QMouseEvent* _event)
{
    if (_event->button() == Qt::LeftButton)
        Q_EMIT clicked();

    if (!viewer_ || _event->button() != Qt::LeftButton || !viewer_->canScroll())
    {
        _event->ignore();
        return;
    }

    const auto pos = _event->pos();
    const auto rect = viewer_->rect();
    if (rect.contains(pos))
    {
        setCursor(Qt::OpenHandCursor);
        _event->accept();
    }
    else
    {
        _event->ignore();
    }
}

void Previewer::ImageViewerWidget::mouseDoubleClickEvent(QMouseEvent *_event)
{
    QLabel::mouseDoubleClickEvent(_event);
    if (!viewer_ || !viewer_->isZoomSupport())
        return;

    if (_event->buttons() == Qt::LeftButton)
        zoomIn(_event->pos());
    else if (_event->buttons() == Qt::RightButton)
        zoomOut(_event->pos());
}

void Previewer::ImageViewerWidget::mouseMoveEvent(QMouseEvent* _event)
{
    if (!viewer_)
    {
        _event->ignore();
        return;
    }

    const auto pos = _event->pos();
    const auto rect = viewer_->rect();

    if (rect.contains(pos))
    {
        if (viewer_->canScroll())
        {
            setCursor(_event->buttons() & Qt::LeftButton ? Qt::ClosedHandCursor : Qt::OpenHandCursor);
            if (_event->buttons() & Qt::LeftButton)
            {
                viewer_->move(mousePos_ - pos);
            }
        }
        else
        {
            setCursor(Qt::ArrowCursor);
        }
        _event->accept();
    }
    else
    {
        setCursor(Qt::ArrowCursor);
        _event->ignore();
    }

    mousePos_ = pos;
}

void Previewer::ImageViewerWidget::wheelEvent(QWheelEvent* _event)
{
    if (tryScale(_event))
    {
        _event->accept();
    }
    else if (viewer_ && viewer_->getScaleFactor() > viewer_->getPreferredScaleFactor())
    {
        if constexpr (platform::is_apple())
            viewer_->move(-_event->pixelDelta());
        else
            viewer_->move(-_event->angleDelta() / 4);

        _event->accept();
    }
    else
    {
        _event->ignore();
    }
}

void Previewer::ImageViewerWidget::paintEvent(QPaintEvent* _event)
{
    if (!viewer_)
        return;

    QPainter painter(this);
    viewer_->paint(painter);
}

void Previewer::ImageViewerWidget::onViewerLoadTimeout()
{
    // delay viewer resetting to avoid flickering
    tmpViewer_ = FFMpegViewer::create(currentData_, maxVideoSize(viewSize_), this, firstOpen_);
    connect(tmpViewer_.get(), &FFMpegViewer::doubleClicked, this, &ImageViewerWidget::closeRequested);
    connect(tmpViewer_.get(), &FFMpegViewer::fullscreenToggled, this, &ImageViewerWidget::fullscreenToggled);
    connect(tmpViewer_.get(), &FFMpegViewer::playCLicked, this, &ImageViewerWidget::playClicked);
    connect(tmpViewer_.get(), &FFMpegViewer::rightClicked, this, &ImageViewerWidget::rightClicked);
    connect(tmpViewer_.get(), &AbstractViewer::loaded, this, [this]()
    {
        viewer_ = std::move(tmpViewer_);
    });
}

void Previewer::ImageViewerWidget::zoomIn(const QPoint& _anchor)
{
    if (!canZoomIn())
        return;

    scale(getZoomInValue(), _anchor);
}

void Previewer::ImageViewerWidget::zoomOut(const QPoint& _anchor)
{
    if (!canZoomOut())
        return;

    scale(getZoomOutValue(), _anchor);
}

void Previewer::ImageViewerWidget::resetZoom()
{
    if (!viewer_)
        return;

    scale(1);
}

void Previewer::ImageViewerWidget::scaleBy(double _scaleFactorDiff, const QPoint &_anchor)
{
    if (!viewer_)
        return;

    if (_scaleFactorDiff > 1 && canZoomIn())
    {
        auto maxPossibleScale = getZoomValue(maxZoomSteps) / viewer_->getScaleFactor();
        if (viewer_->scaleBy(std::min(maxPossibleScale, _scaleFactorDiff), _anchor))
            Q_EMIT zoomChanged();
    }
    else if (_scaleFactorDiff < 1 && canZoomOut())
    {
        auto maxPossibleScale = getZoomValue(-maxZoomSteps) / viewer_->getScaleFactor();
        if (viewer_->scaleBy(std::max(maxPossibleScale, _scaleFactorDiff), _anchor))
            Q_EMIT zoomChanged();
    }
}

void Previewer::ImageViewerWidget::scale(double _scaleFactor, const QPoint &_anchor)
{
    if (!viewer_)
        return;

    viewer_->scale(_scaleFactor, _anchor);
    Q_EMIT zoomChanged();
}

bool Previewer::ImageViewerWidget::canZoomIn() const
{
    return currentStep() < maxZoomSteps;
}

bool Previewer::ImageViewerWidget::canZoomOut() const
{
    return currentStep() > 0 || -currentStep() < maxZoomSteps;
}

QRect Previewer::ImageViewerWidget::viewerRect() const
{
    return viewer_ ? viewer_->rect() : QRect{};
}

double Previewer::ImageViewerWidget::getScaleStep() const
{
    return 1.5;
}

double Previewer::ImageViewerWidget::getZoomInValue() const
{
    return getZoomValue(currentStep() + 1);
}

double Previewer::ImageViewerWidget::getZoomOutValue() const
{
    return getZoomValue(currentStep() - 1);
}

double Previewer::ImageViewerWidget::getZoomValue(const int _step) const
{
    return std::pow(getScaleStep(), _step);
}

int Previewer::ImageViewerWidget::currentStep() const
{
    if (!viewer_)
        return 0;

    return std::round(std::log(viewer_->getScaleFactor()) / std::log(getScaleStep())); // log_a(b) = log_c(b) / log_c(a)
}

QSize Previewer::ImageViewerWidget::maxVideoSize(const QSize& _initialViewport) const
{
    auto maxSize = size();
    maxSize.setWidth(maxSize.width() - videoWidthOffset_);
    maxSize.setHeight(_initialViewport.height());

    return maxSize;
}
