#include "stdafx.h"
#include "VideoFrameLinux.h"
#include "../CommonUI.h"

extern "C" {
void glfwSetRoot(void *root);
typedef void* (* GLADloadproc)(const char *name);
int gladLoadGLLoader(GLADloadproc);
}

static std::mutex g_wnd_mutex;

class GLFWScope
{
public:
    GLFWScope()
    {
        if (!glfwInit())
        {
            fprintf(stderr, "error: glfw init failed\n");
            assert(0);
        }
    }
    ~GLFWScope() { glfwTerminate(); }
} g_GLFWScope;

QWidget* platform_linux::GraphicsPanelLinux::parent_ = nullptr;

platform_linux::GraphicsPanelLinux::GraphicsPanelLinux(QWidget* _parent,
    std::vector<QPointer<Ui::BaseVideoPanel>>&/* panels*/, bool /*primaryVideo*/)
    : platform_specific::GraphicsPanel(_parent)
{
    //glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 2);
    //glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 0);
    glfwWindowHint(GLFW_RESIZABLE, 1);
    std::lock_guard<std::mutex> lock(g_wnd_mutex);
    glfwSetRoot((void*)QWidget::winId());
    videoWindow_ = glfwCreateWindow(600, 400, "Video", NULL, NULL);
    glfwMakeContextCurrent(videoWindow_);
    glfwSetKeyCallback(videoWindow_, keyCallback);
    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
    {
        fprintf(stderr, "error: glad init failed\n");
        assert(0);
    }
    glfwMakeContextCurrent(0);

    parent_ = _parent;
}

platform_linux::GraphicsPanelLinux::~GraphicsPanelLinux()
{
    parent_ = nullptr;
    glfwDestroyWindow(videoWindow_);
}

WId platform_linux::GraphicsPanelLinux::frameId() const
{
    return (WId)videoWindow_;
}

void platform_linux::GraphicsPanelLinux::resizeEvent(QResizeEvent* _e)
{
    QWidget::resizeEvent(_e);
    QRect r = rect();
    glfwSetWindowPos(videoWindow_, r.x(), r.y());
    glfwSetWindowSize(videoWindow_, r.width(), r.height());
}

void platform_linux::GraphicsPanelLinux::keyCallback(GLFWwindow* _window, int _key, int _scancode, int _actions, int _mods)
{
    if (parent_)
    {
        if (_actions == GLFW_PRESS && _mods == GLFW_MOD_CONTROL)
        {
            if (_key == GLFW_KEY_W)
                qApp->postEvent(parent_, new QKeyEvent(QEvent::KeyPress, Qt::Key_W, Qt::ControlModifier));
            else if (_key == GLFW_KEY_Q)
                qApp->postEvent(parent_, new QKeyEvent(QEvent::KeyPress, Qt::Key_Q, Qt::ControlModifier));
        }
    }
}
