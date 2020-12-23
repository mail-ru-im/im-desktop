#ifndef __VIDEO_FRAME_LINUX_H__
#define __VIDEO_FRAME_LINUX_H__

#include <QWidget>
#include "../VideoFrame.h"
#include "glad.h"
#include "glfw3.h"
#include <mutex>

namespace Ui
{
    class BaseVideoPanel;
}

namespace platform_linux
{
    void showOnCurrentDesktop(platform_specific::ShowCallback _callback);

class GraphicsPanelLinux : public platform_specific::GraphicsPanel
{
    Q_OBJECT
public:
    GraphicsPanelLinux(QWidget* _parent, std::vector<QPointer<Ui::BaseVideoPanel>>& _panels, bool primaryVideo);
    virtual ~GraphicsPanelLinux();

    WId frameId() const override;
    void resizeEvent(QResizeEvent * event) override;
    void initNative(platform_specific::ViewResize _mode) override;
    void freeNative() override;
    void setOpacity(double _opacity) override;

private:
    static QWidget *parent_;

    static void keyCallback(GLFWwindow* _window, int _key, int _scancode, int _actions, int _mods);

    bool isOpacityAvailable() const;

private:
    GLFWwindow *videoWindow_ = nullptr;
};

}

#endif//__VIDEO_FRAME_WIN32_H__
