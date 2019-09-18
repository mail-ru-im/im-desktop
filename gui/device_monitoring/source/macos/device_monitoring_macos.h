#ifndef __DEVICE_MONITORING_AVF_H__
#define __DEVICE_MONITORING_AVF_H__

#include "../device_monitoring_impl.h"

namespace device {

class DeviceMonitoringMacos : public DeviceMonitoringImpl
{
    void *_objcInstance;

public:
    DeviceMonitoringMacos();
    ~DeviceMonitoringMacos() override;

    bool Start() override;
    void Stop() override;
};

}

#endif//__DEVICE_MONITORING_AVF_H__
