#pragma once

namespace Ui
{

class FrameRenderer
{
    QColor fillColor_;

    std::function<void(const QSize _sz)> sizeCallback_;

protected:
    QImage activeImage_;

    QPainterPath clippingPath_;
    bool fullScreen_;
    bool fillClient_;

protected:

    virtual void renderFrame(QPainter& _painter, const QRect& _clientRect);

    virtual void drawActiveImage(QPainter& _p, const QRect& _target, const QRect& _source);

    void onSize(const QSize _sz);

public:

    virtual ~FrameRenderer();

    virtual void updateFrame(const QImage& _image);
    const QImage& getActiveImage() const noexcept { return activeImage_; }

    bool isActiveImageNull() const;

    virtual QWidget* getWidget() = 0;

    virtual void redraw() = 0;

    virtual void filterEvents(QObject* _parent) = 0;

    void setClippingPath(QPainterPath _clippingPath);

    void setFullScreen(bool _fullScreen);

    void setFillColor(const QColor& _color);

    void setFillClient(bool _fill);

    virtual void setWidgetVisible(bool _visible) = 0;

    void setSizeCallback(std::function<void(const QSize)> _callback);

    FrameRenderer()
        : fullScreen_(false)
        , fillClient_(false)
    {
    }
};

class GDIRenderer : public QWidget, public FrameRenderer
{
    virtual QWidget* getWidget() override;

    virtual void redraw() override;

    virtual void paintEvent(QPaintEvent* _e) override;

    virtual void filterEvents(QObject* _parent) override;

    virtual void setWidgetVisible(bool _visible) override;

    virtual void resizeEvent(QResizeEvent *_event) override;

public:

    GDIRenderer(QWidget* _parent);
};

#ifndef __linux__
class OpenGLRenderer : public QOpenGLWidget, public FrameRenderer
{
    virtual QWidget* getWidget() override;

    virtual void redraw() override;

    void paint();

    virtual void paintEvent(QPaintEvent* _e) override;

    virtual void paintGL() override;

    virtual void filterEvents(QObject* _parent) override;

    virtual void setWidgetVisible(bool _visible) override;

    virtual void resizeEvent(QResizeEvent *_event) override;

public:
    OpenGLRenderer(QWidget* _parent);

};

class MacOpenGLRenderer : public OpenGLRenderer
{
public:
    MacOpenGLRenderer(QWidget* _parent);
protected:
    void drawActiveImage(QPainter& _p, const QRect& _target, const QRect& _source) override;
};

#endif //__linux__

class DialogRenderer : public GDIRenderer
{
public:
    DialogRenderer(QWidget* _parent);

protected:
    void renderFrame(QPainter& _painter, const QRect& _clientRect) override;
};



}
