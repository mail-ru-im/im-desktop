#ifndef __DEVICE_MONITORING_AVF_H__
#define __DEVICE_MONITORING_AVF_H__

#import <CoreAudio/CoreAudio.h>
#import <MacTypes.h>
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
private:
    bool _isStarted = false;
    void SetHandlers(bool isSet);

    static OSStatus objectListenerProc(
            AudioObjectID objectId,
            UInt32 numberAddresses,
            const AudioObjectPropertyAddress addresses[],
            void* clientData);

};

}

#endif//__DEVICE_MONITORING_AVF_H__
