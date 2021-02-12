#include "stdafx.h"
#include "device_monitoring_windows.h"

#include <Dshow.h>
#include <Dbt.h>
#include <MMDeviceAPI.h>

namespace device
{
struct DeviceListMonitorContext : public IMMNotificationClient
{
    DeviceMonitoringWindows* deviceChangedCallback_;
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
    deviceChangedCallback_->DeviceMonitoringAudioListChanged();
    return S_OK;
}


DeviceMonitoringWindows::DeviceMonitoringWindows() = default;

DeviceMonitoringWindows::~DeviceMonitoringWindows()
{
    stopImpl();
}

bool DeviceMonitoringWindows::Start()
{
    if (qApp)
    {
        auto hr = CoInitialize(nullptr);
        if (FAILED(hr))
        {
            comInit = false;
            return false;
        }
        comInit = true;
        context_ = std::make_unique<DeviceListMonitorContext>();
        context_->deviceChangedCallback_ = this;
        enumerator_ = nullptr;
        hr = ::CoCreateInstance(__uuidof(MMDeviceEnumerator), NULL, CLSCTX_INPROC_SERVER, IID_PPV_ARGS(&enumerator_));
        if (SUCCEEDED(hr))
            hr = enumerator_->RegisterEndpointNotificationCallback(context_.get());

        qApp->installNativeEventFilter(this);
        return true;
    }
    return false;
}

void DeviceMonitoringWindows::Stop()
{
    stopImpl();
}

void DeviceMonitoringWindows::stopImpl()
{
    if (qApp)
    {
        qApp->removeNativeEventFilter(this);
        if (enumerator_)
        {
            enumerator_->UnregisterEndpointNotificationCallback(context_.get());
            enumerator_->Release();
            enumerator_ = nullptr;
        }
        if (std::exchange(comInit, false))
            CoUninitialize();
    }
}

bool DeviceMonitoringWindows::nativeEventFilter(const QByteArray& _eventType, void* _message, long* _result)
{
    if (const auto msg = static_cast<MSG*>(_message); msg->message == WM_DEVICECHANGE)
    {
        DeviceMonitoringVideoListChanged();
        DeviceMonitoringAudioListChanged();
    }
    return false;
}

}
