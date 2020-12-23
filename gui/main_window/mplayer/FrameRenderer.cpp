#include "stdafx.h"

#include "FrameRenderer.h"
#include "utils/utils.h"
#include "../history_control/complex_message/MediaUtils.h"

namespace Ui
{

FrameRenderer::~FrameRenderer() = default;

void FrameRenderer::renderFrame(QPainter& _painter, const QRect& _clientRect)
{
    if (fillColor_.isValid())
        _painter.fillRect(_clientRect, fillColor_);

    if (activeImage_.isNull())
        return;

    const auto imageSize = activeImage_.size();

    QSize scaledSize;
    QRect sourceRect;

    if (fillClient_)
    {
        scaledSize = _clientRect.size();
        const auto size = _clientRect.size().scaled(imageSize, Qt::KeepAspectRatio);
        sourceRect = QRect({}, size);
        if (imageSize.width() > size.width())
            sourceRect.moveLeft((imageSize.width() - size.width()) / 2);
    }
    else
    {
        int32_t h = (int32_t) (((double) imageSize.height() / (double) imageSize.width()) * (double) _clientRect.width());

        if (h > _clientRect.height())
        {
            h = _clientRect.height();

            scaledSize.setWidth(((double) imageSize.width() / (double) imageSize.height()) * (double) _clientRect.height());
        }
        else
        {
            scaledSize.setWidth(_clientRect.width());
        }

        scaledSize.setHeight(h);
    }

    const int cx = (_clientRect.width() - scaledSize.width()) / 2;
    const int cy = (_clientRect.height() - scaledSize.height()) / 2;

    QRect drawRect({ cx, cy }, scaledSize);
    drawActiveImage(_painter, drawRect, sourceRect);
}

void FrameRenderer::drawActiveImage(QPainter& _p, const QRect& _target, const QRect& _source)
{
    if (fillClient_)
        _p.drawImage(_target, activeImage_, _source);
    else
        _p.drawImage(_target, activeImage_);
}

void FrameRenderer::updateFrame(const QImage& _image)
{
    activeImage_ = _image;
}

bool FrameRenderer::isActiveImageNull() const
{
    return activeImage_.isNull();
}

void FrameRenderer::setClippingPath(QPainterPath _clippingPath)
{
    clippingPath_ = _clippingPath;
}

void FrameRenderer::setFullScreen(bool _fullScreen)
{
    fullScreen_ = _fullScreen;
}

void FrameRenderer::setFillColor(const QColor& _color)
{
    fillColor_ = _color;
}

void FrameRenderer::setFillClient(bool _fill)
{
    fillClient_ = _fill;
}

void FrameRenderer::setSizeCallback(std::function<void(const QSize)> _callback)
{
    sizeCallback_ = _callback;
}

void FrameRenderer::onSize(const QSize _sz)
{
    if (sizeCallback_)
        sizeCallback_(_sz);
}




GDIRenderer::GDIRenderer(QWidget* _parent)
    : QWidget(_parent)
{
    setMouseTracking(true);
}

QWidget* GDIRenderer::getWidget()
{
    return this;
}

void GDIRenderer::redraw()
{
    update();
}

void GDIRenderer::paintEvent(QPaintEvent* _e)
{
    QPainter p(this);

    QRect clientRect = geometry();

    p.setRenderHints(QPainter::SmoothPixmapTransform | QPainter::Antialiasing);

    if (!fullScreen_ && !clippingPath_.isEmpty())
        p.setClipPath(clippingPath_);

    renderFrame(p, clientRect);
}

void GDIRenderer::resizeEvent(QResizeEvent *_event)
{
    onSize(_event->size());
}

void GDIRenderer::filterEvents(QObject* _parent)
{
    installEventFilter(_parent);
}

void GDIRenderer::setWidgetVisible(bool _visible)
{
    setVisible(_visible);
}

#ifndef __linux__
OpenGLRenderer::OpenGLRenderer(QWidget* _parent)
    : QOpenGLWidget(_parent)
{
    //if (platform::is_apple())
        setFillColor(Qt::GlobalColor::black);

    setAutoFillBackground(false);

    setMouseTracking(true);
}

QWidget* OpenGLRenderer::getWidget()
{
    return this;
}

void OpenGLRenderer::redraw()
{
    update();
}

void OpenGLRenderer::paint()
{
    QPainter p(this);

    QRect clientRect = geometry();

    if (!fullScreen_)
    {
        if (!clippingPath_.isEmpty())
        {
            p.setClipPath(clippingPath_);
        }
    }

    p.setRenderHint(QPainter::SmoothPixmapTransform, true);

    renderFrame(p, clientRect);
}

void OpenGLRenderer::paintEvent(QPaintEvent* _e)
{
    paint();

    QOpenGLWidget::paintEvent(_e);
}

void OpenGLRenderer::paintGL()
{
    paint();

    QOpenGLWidget::paintGL();
}

void OpenGLRenderer::resizeEvent(QResizeEvent *_event)
{
    onSize(_event->size());

    QOpenGLWidget::resizeEvent(_event);
}

void OpenGLRenderer::filterEvents(QObject* _parent)
{
    installEventFilter(_parent);
}

void OpenGLRenderer::setWidgetVisible(bool _visible)
{
    setVisible(_visible);
}

MacOpenGLRenderer::MacOpenGLRenderer(QWidget* _parent)
    : OpenGLRenderer(_parent)
{

}

void MacOpenGLRenderer::drawActiveImage(QPainter& _p, const QRect& _target, const QRect& _source)
{
    // draw pixmap instead of image to fix alignment problems
    if (fillClient_)
        _p.drawPixmap(_target, QPixmap::fromImage(activeImage_), _source);
    else
        _p.drawPixmap(_target, QPixmap::fromImage(activeImage_));
}

#endif //__linux__

DialogRenderer::DialogRenderer(QWidget* _parent) : GDIRenderer(_parent)
{

}

void DialogRenderer::renderFrame(QPainter& _painter, const QRect& _clientRect)
{
    ComplexMessage::MediaUtils::drawMediaInRect(_painter, _clientRect, QPixmap::fromImage(activeImage_), Utils::scale_value(activeImage_.size()));
}

}
