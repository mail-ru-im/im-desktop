#include "stdafx.h"

#include "FrameRenderer.h"
#include "utils/utils.h"
#include "../history_control/complex_message/MediaUtils.h"

namespace Ui
{

FrameRenderer::~FrameRenderer()
{

}

void FrameRenderer::renderFrame(QPainter& _painter, const QRect& _clientRect)
{
    if (activeImage_.isNull())
    {
        _painter.fillRect(_clientRect, Qt::black);
        return;
    }

    QSize imageSize = activeImage_.size();

    QSize scaledSize;
    QRect sourceRect;

    if (fillClient_)
    {
        scaledSize = QSize(_clientRect.width(), _clientRect.height());
        QSize size = _clientRect.size();
        size.scale(imageSize, Qt::KeepAspectRatio);
        sourceRect = QRect(0, 0, size.width(), size.height());
        if (imageSize.width() > size.width())
            sourceRect.moveLeft(imageSize.width() / 2 - size.width() / 2);
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

    int cx = (_clientRect.width() - scaledSize.width()) / 2;
    int cy = (_clientRect.height() - scaledSize.height()) / 2;

    QRect drawRect(cx, cy, scaledSize.width(), scaledSize.height());

    if (fillColor_.isValid())
        _painter.fillRect(_clientRect, fillColor_);

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

QImage FrameRenderer::getActiveImage() const
{
    return activeImage_;
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
    QPainter p;
    p.begin(this);

    QRect clientRect = geometry();

    p.setRenderHint(QPainter::SmoothPixmapTransform);
    p.setRenderHint(QPainter::Antialiasing);

    if (!fullScreen_ && !clippingPath_.isEmpty())
        p.setClipPath(clippingPath_);

    renderFrame(p, clientRect);

    p.end();

    QWidget::paintEvent(_e);
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
    QPainter p;
    p.begin(this);

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

    p.end();
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
