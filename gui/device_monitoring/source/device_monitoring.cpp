#include "stdafx.h"

#include "../interface/device_monitoring.h"

#ifdef _WIN32
    #include "win32/device_monitoring_windows.h"
#elif __APPLE__
    #include "macos/device_monitoring_macos.h"
#elif __linux__
    #include "linux/device_monitoring_linux.h"
#endif

namespace device
{

std::unique_ptr<DeviceMonitoring> DeviceMonitoringModule::CreateDeviceMonitoring(bool virtual_device)
{
#if _WIN32
    return std::unique_ptr<DeviceMonitoring>(new(std::nothrow) DeviceMonitoringWindows());
#elif __APPLE__
    return std::unique_ptr<DeviceMonitoring>(new(std::nothrow) DeviceMonitoringMacos());
#elif __linux__
    return std::make_unique<DeviceMonitoringLinux>();
#else
    return nullptr;
#endif
}

}
