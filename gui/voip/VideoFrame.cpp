#include "stdafx.h"
#include "VideoFrame.h"
#include "CommonUI.h"

#ifdef __APPLE__
    #include "macos/VideoFrameMacos.h"
#elif _WIN32
    #include "win32/VideoFrameWin32.h"
#elif __linux__
    #include "linux/VideoFrameLinux.h"
#else
#ifndef STRIP_VOIP
    #error "video frame need to create"
#endif //STRIP_VOIP
#endif

platform_specific::GraphicsPanel* platform_specific::GraphicsPanel::create(
    QWidget* _parent,
    std::vector<QPointer<Ui::BaseVideoPanel>>& _panels, bool primaryVideo, bool titlePar)
{
#ifdef __APPLE__
    return  platform_macos::GraphicsPanelMacos::create(_parent, _panels, primaryVideo, titlePar);
#elif _WIN32
    return  new platform_win32::GraphicsPanelWin32(_parent, _panels, primaryVideo);
#elif __linux__
    return  new platform_linux::GraphicsPanelLinux(_parent, _panels, primaryVideo);
#else
    return nullptr;
#endif
}

WId platform_specific::GraphicsPanel::frameId() const
{
    return QWidget::winId();
}

void platform_specific::GraphicsPanel::addPanels(std::vector<QPointer<Ui::BaseVideoPanel>>& /*panels*/)
{

}

void platform_specific::GraphicsPanel::clearPanels()
{

}

void platform_specific::GraphicsPanel::fullscreenModeChanged(bool /*fullscreen*/)
{

}

void platform_specific::GraphicsPanel::fullscreenAnimationStart() {}

void platform_specific::GraphicsPanel::fullscreenAnimationFinish() {}

void platform_specific::GraphicsPanel::windowWillDeminiaturize() {}

void platform_specific::GraphicsPanel::windowDidDeminiaturize() {}


