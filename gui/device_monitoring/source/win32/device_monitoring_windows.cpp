#include "stdafx.h"
#include "device_monitoring_windows.h"

#include <Dshow.h>
#include <vector>
#include <Dbt.h>
#include <memory>
#include <MMDeviceAPI.h>

namespace device
{

struct DeviceListMonitorContext: public IMMNotificationClient
{
    DeviceMonitoringCallback* deviceChangedCallback_;
private:
    // IMMNotificationClient implementation.
    STDMETHOD_(ULONG, AddRef)() override;
    STDMETHOD_(ULONG, Release)() override;
    STDMETHOD(QueryInterface)(REFIID iid, void** object) override;
    STDMETHOD(OnPropertyValueChanged)(LPCWSTR device_id, const PROPERTYKEY key) override;
    STDMETHOD(OnDeviceAdded)(LPCWSTR device_id) override;
    STDMETHOD(OnDeviceRemoved)(LPCWSTR device_id) override;
    STDMETHOD(OnDeviceStateChanged)(LPCWSTR device_id, DWORD new_state) override;
    STDMETHOD(OnDefaultDeviceChanged)(EDataFlow flow, ERole role, LPCWSTR new_default_device_id) override;
};

STDMETHODIMP_(ULONG) DeviceListMonitorContext::AddRef()
{
    return 1;
}

STDMETHODIMP_(ULONG) DeviceListMonitorContext::Release()
{
    return 1;
}

STDMETHODIMP DeviceListMonitorContext::QueryInterface(REFIID iid, void** object)
{
    if (iid == IID_IUnknown || iid == __uuidof(IMMNotificationClient))
    {
        *object = static_cast<IMMNotificationClient*>(this);
        return S_OK;
    }
    *object = nullptr;
    return E_NOINTERFACE;
}

STDMETHODIMP DeviceListMonitorContext::OnPropertyValueChanged(LPCWSTR device_id, const PROPERTYKEY key)
{
    return S_OK;
}

STDMETHODIMP DeviceListMonitorContext::OnDeviceAdded(LPCWSTR device_id)
{
    return S_OK;
}

STDMETHODIMP DeviceListMonitorContext::OnDeviceRemoved(LPCWSTR device_id)
{
    return S_OK;
}

STDMETHODIMP DeviceListMonitorContext::OnDeviceStateChanged(LPCWSTR device_id, DWORD new_state)
{
    return S_OK;
}

STDMETHODIMP DeviceListMonitorContext::OnDefaultDeviceChanged(EDataFlow flow, ERole role, LPCWSTR new_default_device_id)
{
    deviceChangedCallback_->DeviceMonitoringListChanged();
    return S_OK;
}

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
        if (msg == WM_DEVICECHANGE/* || msg == WM_TIMER*/)
        {
            DeviceListMonitorWnd *pThis = (DeviceListMonitorWnd *)GetWindowLongPtr(hwnd, GWLP_USERDATA);
            if (pThis)
                pThis->_deviceChangedCallback.DeviceMonitoringListChanged();
        }
        else if (msg == WM_CREATE)
        {
            DeviceListMonitorWnd* pThis = (DeviceListMonitorWnd *)(((CREATESTRUCT *)lParam)->lpCreateParams);
            SetWindowLongPtr(hwnd, GWLP_USERDATA, (LONG_PTR)pThis);
            //SetTimer(hwnd, 1, 2000, NULL);
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
            return false;

        hwnd = CreateWindowEx(
            WS_EX_CLIENTEDGE,
            className(),
            nullptr,
            WS_OVERLAPPEDWINDOW,
            CW_USEDEFAULT, CW_USEDEFAULT, 240, 120,
            nullptr, nullptr, nullptr, this);

        return hwnd != nullptr;
    }

    void Destroy()
    {
        if (hwnd)
            DestroyWindow(hwnd);
    }
};

DWORD DeviceMonitoringWindows::_worker_thread_proc(_In_ LPVOID lpParameter)
{
    if (!lpParameter)
    {
        assert(false);
        return 1;
    }
    std::unique_ptr<DeviceListMonitorContext> dlmc((DeviceListMonitorContext*)lpParameter);
    if (!dlmc->deviceChangedCallback_)
    {
        assert(false);
        return 1;
    }

    CoInitializeEx(NULL, COINIT_MULTITHREADED);
    IMMDeviceEnumerator* device_enumerator = nullptr;
    HRESULT hr = ::CoCreateInstance(__uuidof(MMDeviceEnumerator), NULL, CLSCTX_INPROC_SERVER, IID_PPV_ARGS(&device_enumerator));
    if (SUCCEEDED(hr))
    {
        hr = device_enumerator->RegisterEndpointNotificationCallback(dlmc.get());
    }

    DeviceListMonitorWnd deviceListMonitorWnd(*dlmc->deviceChangedCallback_);
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
    if (device_enumerator)
    {
        device_enumerator->UnregisterEndpointNotificationCallback(dlmc.get());
        device_enumerator->Release();
    }
    CoUninitialize();
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

    dlmc->deviceChangedCallback_ = this;
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
