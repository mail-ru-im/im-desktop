#ifndef __DEVICE_MONITORING_IMPL_H__
#define __DEVICE_MONITORING_IMPL_H__

#include "../interface/device_monitoring.h"

#include <boost/thread/shared_mutex.hpp>

namespace device {

class DeviceMonitoringImpl
: public DeviceMonitoring
{
    Q_OBJECT
    DeviceMonitoringCallback* _captureDeviceCallback;
    boost::shared_mutex  _callbackLock;

protected:
    DeviceMonitoringImpl();

public:
    virtual ~DeviceMonitoringImpl();

    void DeviceMonitoringVideoListChanged();
    void DeviceMonitoringAudioListChanged();
    void RegisterCaptureDeviceInfoObserver(DeviceMonitoringCallback& viECaptureDeviceInfoCallback) override;
    void DeregisterCaptureDeviceInfoObserver() override;
};

}
#endif//__DEVICE_MONITORING_IMPL_H__
