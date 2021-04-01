#include "stdafx.h"
#include "VideoFrameLinux.h"
#include "../CommonUI.h"
#include "../../core_dispatcher.h"
#include <QtGui/private/qscreen_p.h>

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
            im_assert(0);
        }
    }
    ~GLFWScope() { glfwTerminate(); }
} g_GLFWScope;

QWidget* platform_linux::GraphicsPanelLinux::parent_ = nullptr;

platform_linux::GraphicsPanelLinux::GraphicsPanelLinux(QWidget* _parent,
    std::vector<QPointer<Ui::BaseVideoPanel>>&/* panels*/, bool /*primaryVideo*/)
    : platform_specific::GraphicsPanel(_parent)
{
    parent_ = _parent;
}

platform_linux::GraphicsPanelLinux::~GraphicsPanelLinux()
{
    parent_ = nullptr;
    if (videoWindow_)
        freeNative();
}

void platform_linux::GraphicsPanelLinux::initNative(platform_specific::ViewResize _mode, QSize _size)
{
    im_assert(!videoWindow_);
    if (videoWindow_)
        return;
    glfwWindowHint(GLFW_RESIZABLE, GLFW_TRUE);
    glfwWindowHint(GLFW_TRANSPARENT_FRAMEBUFFER, GLFW_TRUE);

    const auto passMouseEvents = _mode == platform_specific::ViewResize::Adjust || Ui::GetDispatcher()->getVoipController().isCallVCS();
    glfwWindowHint(GLFW_MOUSE_PASSTHRU, passMouseEvents ? GLFW_TRUE : GLFW_FALSE);
    std::lock_guard<std::mutex> lock(g_wnd_mutex);
    glfwSetRoot((void*)QWidget::winId());
    auto s = QHighDpi::toNativePixels(_size.isEmpty() ? size() : _size, window()->windowHandle());
    videoWindow_ = glfwCreateWindow(s.width(), s.height(), "Video", NULL, NULL);
    if (!videoWindow_)
        return;
    glfwMakeContextCurrent(videoWindow_);
    glfwSetKeyCallback(videoWindow_, keyCallback);
    glfwSetInputMode(videoWindow_, GLFW_STICKY_MOUSE_BUTTONS, 1);
    glfwSetInputMode(videoWindow_, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
    {
        fprintf(stderr, "error: glad init failed\n");
        im_assert(0);
    }
    glfwMakeContextCurrent(0);
}

void platform_linux::GraphicsPanelLinux::freeNative()
{
    im_assert(videoWindow_);
    if (!videoWindow_)
        return;
    glfwDestroyWindow(videoWindow_);
    videoWindow_ = nullptr;
}

void platform_linux::GraphicsPanelLinux::setOpacity(double _opacity)
{
    if (!videoWindow_ || !isOpacityAvailable())
        return;
    glfwMakeContextCurrent(videoWindow_);
    glfwSetWindowOpacity(videoWindow_, (float)_opacity);
    glfwMakeContextCurrent(0);
}

WId platform_linux::GraphicsPanelLinux::frameId() const
{
    return (WId)videoWindow_;
}

void platform_linux::GraphicsPanelLinux::resizeEvent(QResizeEvent* _e)
{
    if (!videoWindow_)
        return;
    QWidget::resizeEvent(_e);
    if (!videoWindow_)
        return;
    QRect r = QHighDpi::toNativePixels(rect(), window()->windowHandle());
    if (r.height() != 0)
    {
        glfwSetWindowPos(videoWindow_, r.x(), r.y());
        glfwSetWindowSize(videoWindow_, r.width(), r.height());
    }
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

bool platform_linux::GraphicsPanelLinux::isOpacityAvailable() const
{
    bool res = false;
    if (videoWindow_)
    {
        glfwMakeContextCurrent(videoWindow_);
        static const QVersionNumber minReq(3, 3);
        const auto version = QString::fromUtf8((char*)glGetString(GL_VERSION));
        if (!version.isEmpty())
        {
            const auto curVersion = QVersionNumber::fromString(version);
            res = curVersion >= minReq;
        }
        glfwMakeContextCurrent(0);
    }
    return res;
}

void platform_linux::showOnCurrentDesktop(platform_specific::ShowCallback _callback)
{
    if (_callback)
        _callback();
}
