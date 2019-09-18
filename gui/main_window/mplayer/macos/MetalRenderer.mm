#include "stdafx.h"

#include "../../mplayer/FFMpegPlayer.h"
#include "MetalRenderer.h"
#include "MetalView.h"


class MetalRenderer_p
{
public:
    MetalView* view_ = nullptr;
};


MetalRenderer::MetalRenderer(QWidget* _parent)
    : QMacCocoaViewContainer(nullptr, _parent)
    , d(std::make_unique<MetalRenderer_p>())
{

    d->view_ = [[MetalView alloc] init];
    setCocoaView(d->view_);
    [d->view_ release];

    setAttribute(Qt::WA_PaintOnScreen);
}

MetalRenderer::~MetalRenderer()
{
}

QWidget* MetalRenderer::getWidget()
{
    return this;
}

void MetalRenderer::redraw()
{
}

void MetalRenderer::paint()
{

}

void MetalRenderer::updateFrame(QPixmap _image)
{
    FrameRenderer::updateFrame(_image);

    auto image = std::move(_image).toImage();
    auto cgimage = image.toCGImage();

    auto imageSize = image.size();

    [d->view_ setFrameWithSize : cgimage
                          size : NSMakeSize(imageSize.width(), imageSize.height())];

    [cgimage release];
}

void MetalRenderer::filterEvents(QObject* _parent)
{
    installEventFilter(_parent);
}

void MetalRenderer::setWidgetVisible(bool _visible)
{
    setVisible(_visible);
}

void MetalRenderer::resizeEvent(QResizeEvent* _event)
{
    onSize(_event->size());

    QMacCocoaViewContainer::resizeEvent(_event);
}
