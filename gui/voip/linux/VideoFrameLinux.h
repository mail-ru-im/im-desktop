#ifndef __VIDEO_FRAME_LINUX_H__
#define __VIDEO_FRAME_LINUX_H__

#include <QWidget>
#include "../VideoFrame.h"
#include <GLFW/glfw3.h>
#include <mutex>

namespace Ui
{
    class BaseVideoPanel;
}

namespace platform_linux
{

class GraphicsPanelLinux : public platform_specific::GraphicsPanel
{
    Q_OBJECT
public:
    GraphicsPanelLinux(QWidget* _parent, std::vector<QPointer<Ui::BaseVideoPanel>>& _panels, bool primaryVideo);
    virtual ~GraphicsPanelLinux();

    WId frameId() const;
    void resizeEvent(QResizeEvent * event) override;

private:
    static QWidget *parent_;

    static void keyCallback(GLFWwindow* _window, int _key, int _scancode, int _actions, int _mods);

private:
    GLFWwindow *videoWindow_;
};

}

#endif//__VIDEO_FRAME_WIN32_H__
