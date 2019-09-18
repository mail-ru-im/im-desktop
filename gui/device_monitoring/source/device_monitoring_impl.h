#ifndef __DEVICE_MONITORING_IMPL_H__
#define __DEVICE_MONITORING_IMPL_H__

#include "../interface/device_monitoring.h"

#include <boost/thread/shared_mutex.hpp>

namespace device {

class DeviceMonitoringImpl
: public DeviceMonitoring
, public DeviceMonitoringCallback {
    DeviceMonitoringCallback* _captureDeviceCallback;
    boost::shared_mutex  _callbackLock;

protected:
    DeviceMonitoringImpl();
    void DeviceMonitoringListChanged() override;
    void DeviceMonitoringBluetoothHeadsetChanged(bool connected) override;

public:
    virtual ~DeviceMonitoringImpl();

    void RegisterCaptureDeviceInfoObserver(DeviceMonitoringCallback& viECaptureDeviceInfoCallback) override;
    void DeregisterCaptureDeviceInfoObserver() override;
};

}
#endif//__DEVICE_MONITORING_IMPL_H__
