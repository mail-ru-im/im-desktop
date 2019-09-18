#include "stdafx.h"
#include "device_monitoring_windows.h"

#include <Dshow.h>
#include <vector>
#include <Dbt.h>
#include <memory>

namespace device
{
namespace
{

static constexpr GUID GUID_DEVINTERFACE_LIST[] = {
    { 0xA5DCBF10, 0x6530, 0x11D2, { 0x90, 0x1F, 0x00, 0xC0, 0x4F, 0xB9, 0x51, 0xED } }
};

struct DeviceListMonitorContext
{
    std::vector<GUID> guids;
    DeviceMonitoringCallback* deviceChangedCallback;
};

class DeviceListMonitorWnd
{
    DeviceMonitoringCallback& _deviceChangedCallback;
    uint32_t                _devicesNumber;
    constexpr wchar_t* className() noexcept { return L"deviceListMonitorWnd"; };

    HWND hwnd;

public:
    DeviceListMonitorWnd(DeviceMonitoringCallback& deviceChangedCallback)
    : _deviceChangedCallback(deviceChangedCallback)
    , _devicesNumber(0)
    , hwnd(nullptr)
    {

    }

    static LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
    {
        if (msg == WM_DEVICECHANGE)
        {
            DeviceListMonitorWnd *pThis = (DeviceListMonitorWnd *)GetWindowLongPtr(hwnd, GWLP_USERDATA);
            if (pThis)
                pThis->_deviceChangedCallback.DeviceMonitoringListChanged();
        }
        else if (msg == WM_CREATE)
        {
            DeviceListMonitorWnd* pThis = (DeviceListMonitorWnd *)(((CREATESTRUCT *)lParam)->lpCreateParams);
            SetWindowLongPtr(hwnd, GWLP_USERDATA, (LONG_PTR)pThis);
        }
        return DefWindowProc(hwnd, msg, wParam, lParam);
    }

    bool Create()
    {
        WNDCLASS wc = { };

        wc.lpfnWndProc = WndProc;
        wc.hInstance = GetModuleHandle(nullptr);
        wc.lpszClassName = className();

        if (!RegisterClass(&wc))
        {
            return false;
        }

        hwnd = CreateWindowEx(
            WS_EX_CLIENTEDGE,
            className(),
            nullptr,
            WS_OVERLAPPEDWINDOW,
            CW_USEDEFAULT, CW_USEDEFAULT, 240, 120,
            nullptr, nullptr, nullptr, this);

        if (hwnd == nullptr)
        {
            return false;
        }

        _deviceChangedCallback.DeviceMonitoringListChanged();
        return true;
    }

    void Destroy()
    {
        if (hwnd)
            DestroyWindow(hwnd);
    }
};

}

DWORD DeviceMonitoringWindows::_worker_thread_proc(_In_ LPVOID lpParameter)
{
    if (!lpParameter)
    {
        assert(false);
        return 1;
    }

    std::unique_ptr<DeviceListMonitorContext> dlmc((DeviceListMonitorContext*)lpParameter);
    if (dlmc->guids.empty())
    {
        assert(false);
        return 1;
    }

    if (!dlmc->deviceChangedCallback)
    {
        assert(false);
        return 1;
    }

    DeviceListMonitorWnd deviceListMonitorWnd(*dlmc->deviceChangedCallback);
    if (!deviceListMonitorWnd.Create())
    {
        CoUninitialize();
        assert(false);
        return 1;
    }

    MSG msg;
    BOOL fGotMessage;
    while ((fGotMessage = GetMessage(&msg, (HWND) nullptr, 0, 0)) != 0 && fGotMessage != -1)
    {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    deviceListMonitorWnd.Destroy();
    return 0;
}

DeviceMonitoringWindows::DeviceMonitoringWindows()
: _workingThread(nullptr)
, _id(0)
{

}

DeviceMonitoringWindows::~DeviceMonitoringWindows()
{
    assert(_workingThread == nullptr);
}

bool DeviceMonitoringWindows::Start()
{
    if (_workingThread)
        return false;

    DeviceListMonitorContext* dlmc = new(std::nothrow) DeviceListMonitorContext();
    if (!dlmc)
    {
        assert(false);
        return false;
    }

    dlmc->deviceChangedCallback = this;
    dlmc->guids.assign(GUID_DEVINTERFACE_LIST, &GUID_DEVINTERFACE_LIST[sizeof(GUID_DEVINTERFACE_LIST) / sizeof(GUID)]);
    _workingThread = ::CreateThread(nullptr, 0, (LPTHREAD_START_ROUTINE)&DeviceMonitoringWindows::_worker_thread_proc, dlmc, 0, &_id);

    return _workingThread != nullptr;
}

void DeviceMonitoringWindows::Stop()
{
    if (_workingThread)
    {
        ::PostThreadMessage(_id, WM_QUIT, 0, 0);
        constexpr auto timeout = std::chrono::seconds(3);
        ::WaitForSingleObject(_workingThread, std::chrono::milliseconds(timeout).count());
    }

    _workingThread = nullptr;
    _id = 0;
}

}
