#ifndef __DEVICE_MONITORING_H__
#define __DEVICE_MONITORING_H__

namespace device
{

class DeviceMonitoringCallback
{
public:
    virtual ~DeviceMonitoringCallback() = default;
    virtual void DeviceMonitoringListChanged() = 0; // audio devices only if possible
};

class DeviceMonitoring : public QObject
{
    Q_OBJECT
public:
    virtual void RegisterCaptureDeviceInfoObserver(DeviceMonitoringCallback& viECaptureDeviceInfoCallback) = 0;
    virtual void DeregisterCaptureDeviceInfoObserver() = 0;

    virtual bool Start() = 0;
    virtual void Stop() = 0;

    virtual ~DeviceMonitoring() { }
};

class DeviceMonitoringModule
{
public:
    static std::unique_ptr<DeviceMonitoring> CreateDeviceMonitoring();
};

}
#endif//__DEVICE_MONITORING_H__
