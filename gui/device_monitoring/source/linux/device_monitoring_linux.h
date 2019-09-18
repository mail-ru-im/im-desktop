#pragma once

#include "../device_monitoring_impl.h"
#include "libudev.h"

namespace device {

class DeviceMonitoringLinux : public DeviceMonitoringImpl
{
public:
    DeviceMonitoringLinux();
    virtual ~DeviceMonitoringLinux();

    bool Start() override;
    void Stop() override;

private:
    void Stop_internal();
    void OnEvent();
    int GetDescriptor();

    void *_libUdev;
    struct udev *(*_udev_new)(void);
    struct udev_device *(*_udev_monitor_receive_device)(struct udev_monitor *udev_monitor);
    int (*_udev_monitor_get_fd)(struct udev_monitor *udev_monitor);
    struct udev_monitor *(*_udev_monitor_new_from_netlink)(struct udev *udev, const char *name);
    int (*_udev_monitor_filter_add_match_subsystem_devtype)(struct udev_monitor *udev_monitor, const char *subsystem, const char *devtype);
    int (*_udev_monitor_enable_receiving)(struct udev_monitor *udev_monitor);
    void (*_udev_monitor_unref)(struct udev_monitor *udev_monitor);
    void (*_udev_device_unref)(struct udev_device *udev_device);
    void (*_udev_unref)(struct udev *udev);

    struct udev *_udev;
    struct udev_monitor *_monitor;
    bool _registered;
    std::unique_ptr<QSocketNotifier> _notifier;
};

}
