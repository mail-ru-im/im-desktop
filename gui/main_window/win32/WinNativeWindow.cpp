#include "stdafx.h"
#include "WinNativeWindow.h"

#include <windowsx.h>
#include <Uxtheme.h>

#include "../../styles/ThemeParameters.h"

namespace
{
    auto getBgColor()
    {
        return Styling::getParameters().getColor(Styling::StyleVariable::BASE_GLOBALWHITE);
    }

    void updateSystemMenu(HWND _hWnd, HMENU& _menu)
    {
        if (!_hWnd || !_menu)
            return;

        auto maximized = Ui::isNativeWindowShowCmd(_hWnd, SW_MAXIMIZE);
        for (int i = 0; i < GetMenuItemCount(_menu); ++i)
        {
            MENUITEMINFO itemInfo = { 0 };
            itemInfo.cbSize = sizeof(itemInfo);
            itemInfo.fMask = MIIM_TYPE | MIIM_STATE | MIIM_ID;
            if (GetMenuItemInfo(_menu, i, TRUE, &itemInfo))
            {
                if (itemInfo.fType & MF_SEPARATOR)
                    continue;

                if (itemInfo.wID && !(itemInfo.fState & MF_DEFAULT))
                {
                    UINT fState = itemInfo.fState & ~(MF_DISABLED | MF_GRAYED);
                    if (itemInfo.wID == SC_CLOSE)
                        fState |= MF_DEFAULT;
                    else if ((maximized && (itemInfo.wID == SC_MAXIMIZE || itemInfo.wID == SC_SIZE || itemInfo.wID == SC_MOVE))
                             || (!maximized && itemInfo.wID == SC_RESTORE))
                        fState |= MF_DISABLED | MF_GRAYED;

                    itemInfo.fMask = MIIM_STATE;
                    itemInfo.fState = fState;
                    if (!SetMenuItemInfo(_menu, i, TRUE, &itemInfo))
                    {
                        DestroyMenu(_menu);
                        _menu = 0;
                        break;
                    }
                }
            }
            else
            {
                DestroyMenu(_menu);
                _menu = 0;
                break;
            }
        }
    }

    // unique number of the application icon in the resources
    constexpr int APP_ICON = 1;

    // DWM functions
    typedef HRESULT(__stdcall *DwmIsCompositionEnabled)(BOOL* pfEnabled);
    typedef HRESULT(__stdcall *DwmExtendFrameIntoClientArea)(HWND hWnd, _In_ const MARGINS* pMarInset);
}

namespace Ui
{
    bool WinNativeWindow::isAeroEnabled_ = false;
    HWND WinNativeWindow::childWindow_ = nullptr;
    QWidget* WinNativeWindow::childWidget_ = nullptr;

    bool isNativeWindowShowCmd(HWND _hwnd, UINT _cmd)
    {
        WINDOWPLACEMENT placement;
        if (!GetWindowPlacement(_hwnd, &placement))
            return false;

        return placement.showCmd == _cmd;
    }

    WinNativeWindow::WinNativeWindow()
        : hWnd_(nullptr)
    {
    }

    bool WinNativeWindow::init(const QString& _title)
    {
        const auto bgColor = getBgColor();
        HINSTANCE hInstance = GetModuleHandle(nullptr);
        WNDCLASSEX wcx = { 0 };

        wcx.cbSize = sizeof(WNDCLASSEX);
        wcx.style = CS_HREDRAW | CS_VREDRAW;
        wcx.hInstance = hInstance;
        wcx.lpfnWndProc = wndProc;
        wcx.cbClsExtra = 0;
        wcx.cbWndExtra = 0;
        wcx.lpszClassName = L"WindowClass";
        wcx.hbrBackground = CreateSolidBrush(RGB(bgColor.red(), bgColor.green(), bgColor.blue()));
        wcx.hCursor = LoadCursor(hInstance, IDC_ARROW);
        wcx.hIcon = LoadIcon(GetModuleHandle(nullptr), MAKEINTRESOURCE(APP_ICON));
        wcx.hIconSm = LoadIcon(GetModuleHandle(nullptr), MAKEINTRESOURCE(APP_ICON));

        if (!RegisterClassEx(&wcx))
            return false;

        auto dll = LoadLibrary(_T("dwmapi.dll"));
        if (dll)
        {
            // aero available on some versions of Windows
            if (auto compositionFunc = reinterpret_cast<DwmIsCompositionEnabled>(GetProcAddress(dll, "DwmIsCompositionEnabled")))
            {
                BOOL enabled = FALSE;
                bool success = compositionFunc(&enabled) == S_OK;
                isAeroEnabled_ = enabled && success;
            }
        }

        auto style = isAeroEnabled_ ? Style::AeroBorderless : Style::BasicBorderless;
        hWnd_ = CreateWindow(L"WindowClass", (const wchar_t*)_title.utf16(), static_cast<DWORD>(style), 0, 0, 0, 0, 0, 0, hInstance, nullptr);
        if (!hWnd_)
        {
            if (dll)
                FreeLibrary(dll);

            return false;
        }

        if (dll)
        {
            if (isAeroEnabled_)
            {
                if (auto shadowFunc = reinterpret_cast<DwmExtendFrameIntoClientArea>(GetProcAddress(dll, "DwmExtendFrameIntoClientArea")))
                {
                    const MARGINS aero_shadow_on = { 1, 1, 1, 1 };
                    shadowFunc(hWnd_, &aero_shadow_on);
                }
            }

            FreeLibrary(dll);
        }

        SetWindowLongPtr(hWnd_, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(this));
        SetWindowPos(hWnd_, nullptr, 0, 0, 0, 0, SWP_FRAMECHANGED | SWP_NOMOVE | SWP_NOSIZE);

        return true;
    }

    WinNativeWindow::~WinNativeWindow()
    {
        ShowWindow(hWnd_, SW_HIDE);
        DestroyWindow(hWnd_);
    }

    LRESULT CALLBACK WinNativeWindow::wndProc(HWND _hWnd, UINT _message, WPARAM _wParam, LPARAM _lParam)
    {
        static bool resizeByMouse = false;
        static bool childHide = false;

        WinNativeWindow *window = reinterpret_cast<WinNativeWindow*>(GetWindowLongPtr(_hWnd, GWLP_USERDATA));
        if (!window)
            return DefWindowProc(_hWnd, _message, _wParam, _lParam);

        switch (_message)
        {
        case WM_ACTIVATE:
        {
            if (childWindow_ && (_wParam == WA_ACTIVE || _wParam == WA_CLICKACTIVE))
            {
                if (reinterpret_cast<HWND>(_lParam) != childWindow_)
                {
                    SetForegroundWindow(childWindow_);
                    SetWindowPos(_hWnd, childWindow_, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE);
                    SendMessage(childWindow_, WM_ACTIVATE, _wParam, _lParam);
                    return 0;
                }
            }
            break;
        }
        case WM_NCCALCSIZE:
        {
            if (isNativeWindowShowCmd(_hWnd, SW_MAXIMIZE))
            {
                // adjust client rect to not spill over monitor edges when maximized
                auto params = reinterpret_cast<LPNCCALCSIZE_PARAMS>(_lParam);
                LPRECT rect = (_wParam == TRUE) ? &params->rgrc[0] : reinterpret_cast<LPRECT>(_lParam);
                if (auto monitor = MonitorFromRect(rect, MONITOR_DEFAULTTONEAREST))
                {
                    MONITORINFO info = { 0 };
                    info.cbSize = sizeof(info);
                    if (GetMonitorInfoW(monitor, &info))
                        *rect = info.rcWork;
                }
            }

            if (_wParam == TRUE && childWindow_ && !resizeByMouse)
            {
                ShowWindow(childWindow_, SW_HIDE);
                childHide = true;
            }

            return 0;
        }
        case WM_NCACTIVATE:
        {
            if (!isAeroEnabled_)
            {
                // Prevents window frame reappearing on window activation
                // in "basic" theme, where no aero shadow is present.
                return 1;
            }
            break;
        }
        case WM_NCPAINT:
        {
            if (QSysInfo::WindowsVersion >= QSysInfo::WV_VISTA)
                break;

            return 0;
        }
        case WM_NCHITTEST:
        {
            RECT winRect;
            GetWindowRect(_hWnd, &winRect);

            const int pixRatio = childWidget_ ? childWidget_->window()->devicePixelRatio() : 1;
            const int borderWidth = NativeWindowBorderWidth * pixRatio;
            const int x = GET_X_LPARAM(_lParam);
            const int y = GET_Y_LPARAM(_lParam);

            if (x >= winRect.left && x < winRect.left + borderWidth)
            {
                if (y >= winRect.top && y < winRect.top + borderWidth)
                    return HTTOPLEFT;
                else if (y < winRect.bottom && y >= winRect.bottom - borderWidth)
                    return HTBOTTOMLEFT;
                else
                    return HTLEFT;
            }
            else if (x < winRect.right && x >= winRect.right - borderWidth)
            {
                if (y >= winRect.top && y < winRect.top + borderWidth)
                    return HTTOPRIGHT;
                else if (y < winRect.bottom && y >= winRect.bottom - borderWidth)
                    return HTBOTTOMRIGHT;
                else
                    return HTRIGHT;
            }
            else
            {
                if (y >= winRect.top && y < winRect.top + borderWidth)
                    return HTTOP;
                else if (y < winRect.bottom && y >= winRect.bottom - borderWidth)
                    return HTBOTTOM;
                else
                    return HTCAPTION;
            }
            break;
        }
        case WM_CLOSE:
        {
            if (childWindow_)
            {
                SendMessage(childWindow_, WM_SYSCOMMAND, SC_CLOSE, 0);
                return 0;
            }
            break;
        }
        case WM_DESTROY:
        {
            PostQuitMessage(0);
            return 0;
        }
        // When this native window changes size, it needs to manually resize the QWidget child
        case WM_SIZE:
        {
            if (childWidget_ && !isNativeWindowShowCmd(_hWnd, SW_SHOWMINIMIZED))
            {
                RECT winrect;
                GetClientRect(_hWnd, &winrect);
                childWidget_->setGeometry(0, 0, winrect.right, winrect.bottom);
            }
            break;
        }
        case WM_GETMINMAXINFO:
        {
            auto minMaxInfo = reinterpret_cast<MINMAXINFO*>(_lParam);
            if (window->minimumSize_.required_)
            {
                minMaxInfo->ptMinTrackSize.x = window->getMinimumWidth();;
                minMaxInfo->ptMinTrackSize.y = window->getMinimumHeight();
            }

            if (window->maximumSize_.required_)
            {
                minMaxInfo->ptMaxTrackSize.x = window->getMaximumWidth();
                minMaxInfo->ptMaxTrackSize.y = window->getMaximumHeight();
            }

            return 0;
        }
        case WM_MOVE:
        {
            if (childWidget_ && !isNativeWindowShowCmd(_hWnd, SW_SHOWMINIMIZED))
            {
                auto wndPos = MAKEPOINTS(_lParam);
                QApplication::postEvent(childWidget_, new NativeWindowMovedEvent(wndPos.x, wndPos.y));
            }

            break;
        }
        case WM_NCRBUTTONUP:
        {
            auto mousePos = MAKEPOINTS(_lParam);

            HMENU sysMenu = GetSystemMenu(_hWnd, FALSE);
            updateSystemMenu(_hWnd, sysMenu);
            if (sysMenu)
            {
                SetForegroundWindow(_hWnd);
                if (int cmd = TrackPopupMenu(sysMenu, TPM_LEFTALIGN | TPM_TOPALIGN | TPM_LEFTBUTTON | TPM_RETURNCMD, mousePos.x, mousePos.y, 0, _hWnd, nullptr))
                {
                    SendMessage(_hWnd, WM_SYSCOMMAND, cmd, 0);
                    return 0;
                }
            }
            break;
        }
        case WM_SYSCOMMAND:
        {
            if (_wParam == SC_RESTORE && childWindow_)
                SendMessage(childWindow_, WM_SYSCOMMAND, _wParam, _lParam);

            break;
        }
        case WM_SIZING:
        {
            resizeByMouse = true;
            break;
        }
        case WM_EXITSIZEMOVE:
        {
            resizeByMouse = false;
            break;
        }
        case WM_PAINT:
        {
            if (childWindow_ && childHide)
            {
                ShowWindow(childWindow_, SW_SHOW);
                childHide = false;
            }
            break;
        }
        case WM_SETFOCUS:
        {
            if (childWindow_)
                SetFocus(childWindow_);
            break;
        }
        }

        return DefWindowProc(_hWnd, _message, _wParam, _lParam);
    }

    void WinNativeWindow::setGeometry(const QRect& _rect)
    {
        MoveWindow(hWnd_, _rect.x(), _rect.y(), _rect.width(), _rect.height(), 1);
    }

    HWND WinNativeWindow::hwnd() const
    {
        return hWnd_;
    }

    void WinNativeWindow::setChildWindow(HWND _hWnd)
    {
        childWindow_ = _hWnd;
    }

    void WinNativeWindow::setChildWidget(QWidget* _widget)
    {
        childWidget_ = _widget;
    }

    void WinNativeWindow::setMinimumSize(int _width, int _height)
    {
        this->minimumSize_.required_ = true;
        this->minimumSize_.width_ = _width;
        this->minimumSize_.height_ = _height;
    }

    int WinNativeWindow::getMinimumWidth() const
    {
        return minimumSize_.width_;
    }

    int WinNativeWindow::getMinimumHeight() const
    {
        return minimumSize_.height_;
    }

    void WinNativeWindow::setMaximumSize(int _width, int _height)
    {
        this->maximumSize_.required_ = true;
        this->maximumSize_.width_ = _width;
        this->maximumSize_.height_ = _height;
    }

    int WinNativeWindow::getMaximumWidth() const
    {
        return maximumSize_.width_;
    }

    int WinNativeWindow::getMaximumHeight() const
    {
        return maximumSize_.height_;
    }
}
