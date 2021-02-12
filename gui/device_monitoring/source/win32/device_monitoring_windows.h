#ifndef __DEVICE_MONITORING_WINDOWS_H__
#define __DEVICE_MONITORING_WINDOWS_H__

#include "../device_monitoring_impl.h"

struct IMMDeviceEnumerator;

namespace device
{

struct DeviceListMonitorContext;

class DeviceMonitoringWindows : public DeviceMonitoringImpl, QAbstractNativeEventFilter
{
    Q_OBJECT

    void stopImpl();
    std::unique_ptr<DeviceListMonitorContext> context_;
    IMMDeviceEnumerator* enumerator_ = nullptr;
    bool comInit = false;
public:
    DeviceMonitoringWindows();
    virtual ~DeviceMonitoringWindows();

    bool Start() override;
    void Stop() override;

    bool nativeEventFilter(const QByteArray& _eventType, void* _message, long* _result);
};

}

#endif//__DEVICE_MONITORING_WINDOWS_H__