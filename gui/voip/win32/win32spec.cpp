#ifdef _WIN32
#include <dwmapi.h>
#include "win32spec.h"
#include "../../utils/win32/VirtualDesktopManager.h"

namespace platform_win32
{
    QRect fromWinRect(const RECT& _rc)
    {
        return QRect(_rc.left, _rc.top, _rc.right - _rc.left, _rc.bottom - _rc.top);
    }

    void showInWorkspace(QWidget* _w, QWidget* _initiator)
    {
        if (QOperatingSystemVersion::current() >= QOperatingSystemVersion::Windows10)
        {
            // As Subwindow DetachedVideoWnd tries to keep it's previous desktop, so we move it first
            VirtualDesktops::getVirtualDesktopManager()->moveWidgetToCurrentDesktop(_initiator);
            VirtualDesktops::getVirtualDesktopManager()->moveWidgetToCurrentDesktop(_w);
        }
    }

    bool windowIsOverlapped(QWidget* _window)
    {
        im_assert(_window);

        HWND hwnd = (HWND)_window->effectiveWinId();
        if (!::IsWindowVisible(hwnd))
            return false;

        RECT rc;
        ::GetWindowRect(hwnd, &rc);

        bool hasOverlapping = false;
        const QRect rect = fromWinRect(rc);

        QRegion region = rect;

        //Find the top-most window.
        HWND window = ::GetTopWindow(::GetDesktopWindow());

        // Traverse from the highest z-order to the lowest by using GW_HWNDNEXT.
        // We're using a do-while here because we don't want to skip the top-most
        // window in this loop.
        do
        {
            // Stop traversing if we found self or
            // if region is already empty
            if (window == hwnd || region.isEmpty())
                break;

            if (!::IsWindowVisible(window))
                continue; // Invisible window - continue

            BOOL bIsCloaked = FALSE;
            ::DwmGetWindowAttribute(window, DWMWA_CLOAKED, &bIsCloaked, sizeof(BOOL));
            if (bIsCloaked == TRUE)
                continue; // Window is not a "real" visible window - continue

            ::GetWindowRect(window, &rc);
            const QRect wRect = fromWinRect(rc);
            if (!region.intersects(wRect))
                continue; // We hasn't got intersection - continue

            region -= wRect;
            hasOverlapping = true;

        } while (window = ::GetWindow(window, GW_HWNDNEXT));

        if (!hasOverlapping) // We are a top-most window or no intersections found
            return false;
        else if (region.isEmpty()) // We are fully overlapped by other windows
            return true;

        // We are partially overlapped by other windows
        const int originalArea = rect.width() * rect.height();

        // QRegion consists of non-intersected rects, so
        // we can simply accumulate areas of overlapped rects
        // to get the final area
        int overlapArea = 0;
        for (const auto& r : region)
            overlapArea += r.width() * r.height();

        return overlapArea < originalArea * 0.6;
    }

    bool isSpacebarDown()
    {
        return GetKeyState(VK_SPACE) < 0;
    }
}

#endif
