#include "stdafx.h"
#include "VideoFrameWin32.h"
#include <windows.h>
#include <objbase.h>
#include <ObjectArray.h>
#include "../CommonUI.h"
#include "../../utils/win32/VirtualDesktopManager.h"

platform_win32::GraphicsPanelWin32::GraphicsPanelWin32(
    QWidget* _parent,
    std::vector<QPointer<Ui::BaseVideoPanel>>&/* panels*/, bool /*primaryVideo*/)
    : platform_specific::GraphicsPanel(_parent)
{

    //for (unsigned ix = 0; ix < panels.size(); ix++) {
    //    assert(parent);
    //    if (!parent) { continue; }

    //    assert(panels[ix]);
    //    if (!panels[ix]) { continue; }
    //
    //    HWND panelView = (HWND)panels[ix]->winId();
    //    assert(panelView);
    //    if (!panelView) { continue; }

    //    HWND parentView = (HWND)parent->winId();
    //    assert(parentView);
    //    if (!parentView) { continue; }
    //
    //    ::SetParent(panelView, parentView);
    //    DWORD style = ::GetWindowLong(panelView, GWL_STYLE);
    //    style &= ~(WS_POPUP | WS_CAPTION);
    //    style |= WS_CHILD;
    //    SetWindowLong(panelView, GWL_STYLE, style);
    //}
}

platform_win32::GraphicsPanelWin32::~GraphicsPanelWin32()
{
}

void platform_win32::GraphicsPanelWin32::setOpacity(double _opacity)
{
    HWND hwnd = (HWND)parentWidget()->winId();
    long wAttr = GetWindowLong(hwnd, GWL_EXSTYLE);
    SetWindowLong(hwnd, GWL_EXSTYLE, wAttr | WS_EX_LAYERED);
    SetLayeredWindowAttributes(hwnd, 0, _opacity * 0xFF, 0x02);
}

void platform_win32::showOnCurrentDesktop(QWidget* _w, QWidget* _initiator, platform_specific::ShowCallback _callback)
{
    if (QOperatingSystemVersion::current() >= QOperatingSystemVersion::Windows10)
    {
        // As Subwindow DetachedVideoWnd tries to keep it's previous desktop, so we move it first
        VirtualDesktops::getVirtualDesktopManager()->moveWidgetToCurrentDesktop(_initiator);
        VirtualDesktops::getVirtualDesktopManager()->moveWidgetToCurrentDesktop(_w);
    }

    if (_callback)
        _callback();
}
