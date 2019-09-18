#pragma once

#include <QMacCocoaViewContainer>

class MetalRenderer_p;

class MetalRenderer : public QMacCocoaViewContainer, public Ui::FrameRenderer
{
public:
    MetalRenderer(QWidget* _parent = nullptr);
    ~MetalRenderer();

    QPaintEngine* paintEngine() const override { return nullptr; }

private:
    virtual QWidget* getWidget() override;

    virtual void redraw() override;

    void paint();

    void updateFrame(QPixmap _image) override;

    virtual void filterEvents(QObject* _parent) override;

    virtual void setWidgetVisible(bool _visible) override;

    virtual void resizeEvent(QResizeEvent *_event) override;

    std::unique_ptr<MetalRenderer_p> d;
};
