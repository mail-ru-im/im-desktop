#ifndef __DEVICE_MONITORING_WINDOWS_H__
#define __DEVICE_MONITORING_WINDOWS_H__

#include "../device_monitoring_impl.h"

namespace device {

class DeviceMonitoringWindows : public DeviceMonitoringImpl {
    HANDLE _workingThread;
    DWORD _id;

    static DWORD WINAPI _worker_thread_proc(_In_ LPVOID lpParameter);

public:
    DeviceMonitoringWindows();
    virtual ~DeviceMonitoringWindows();

    bool Start() override;
    void Stop() override;
};

}

#endif//__DEVICE_MONITORING_WINDOWS_H__