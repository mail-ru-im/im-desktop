#include "stdafx.h"
#include "device_monitoring_linux.h"
#include <dlfcn.h>

namespace device
{


DeviceMonitoringLinux::DeviceMonitoringLinux()
    : _registered(false)
{

}

DeviceMonitoringLinux::~DeviceMonitoringLinux()
{
    Stop_internal();
}

template <class T>
static inline void get_lib_sym(void *lib, const char *name, T& proc)
{
    proc = (T)dlsym(lib, name);
}

bool DeviceMonitoringLinux::Start()
{
    _libUdev = dlopen("libudev.so", RTLD_NOW | RTLD_GLOBAL);
    if (!_libUdev)
        _libUdev = dlopen("libudev.so.1", RTLD_NOW | RTLD_GLOBAL);
    if (!_libUdev)
        _libUdev = dlopen("libudev.so.2", RTLD_NOW | RTLD_GLOBAL);
    if (!_libUdev)
        return false;

    get_lib_sym(_libUdev, "udev_new", _udev_new);
    get_lib_sym(_libUdev, "udev_monitor_receive_device", _udev_monitor_receive_device);
    get_lib_sym(_libUdev, "udev_monitor_get_fd", _udev_monitor_get_fd);
    get_lib_sym(_libUdev, "udev_monitor_new_from_netlink", _udev_monitor_new_from_netlink);
    get_lib_sym(_libUdev, "udev_monitor_filter_add_match_subsystem_devtype", _udev_monitor_filter_add_match_subsystem_devtype);
    get_lib_sym(_libUdev, "udev_monitor_enable_receiving", _udev_monitor_enable_receiving);
    get_lib_sym(_libUdev, "udev_monitor_unref", _udev_monitor_unref);
    get_lib_sym(_libUdev, "udev_device_unref", _udev_device_unref);
    get_lib_sym(_libUdev, "udev_unref", _udev_unref);
    if (!_udev_new || !_udev_monitor_receive_device || !_udev_monitor_get_fd || !_udev_monitor_new_from_netlink || !_udev_monitor_filter_add_match_subsystem_devtype ||
        !_udev_monitor_enable_receiving || !_udev_monitor_unref || !_udev_device_unref || !_udev_unref)
        return false;

    _udev = _udev_new();
    if (!_udev)
        return false;
    _monitor = _udev_monitor_new_from_netlink(_udev, "udev");
    if (!_monitor)
    {
        Stop();
        return false;
    }
    _udev_monitor_filter_add_match_subsystem_devtype(_monitor, "sound", nullptr);
    _udev_monitor_enable_receiving(_monitor);
    auto sd = GetDescriptor();

    _notifier = std::make_unique<QSocketNotifier>(sd, QSocketNotifier::Read);
    _notifier->setEnabled(true);

    QObject::connect(_notifier.get(), &QSocketNotifier::activated, [this]()
    {
        OnEvent();
    });
    _registered = true;
    return true;
}

void DeviceMonitoringLinux::Stop_internal()
{
    if (_registered)
        _registered = false;

    _notifier.reset();

    if (_monitor)
        _udev_monitor_unref(_monitor);
    if (_udev)
        _udev_unref(_udev);
    if (_libUdev)
        dlclose(_libUdev);
    _monitor = nullptr;
    _udev = nullptr;
    _libUdev = nullptr;
}

void DeviceMonitoringLinux::Stop()
{
    Stop_internal();
}

void DeviceMonitoringLinux::OnEvent()
{
    struct udev_device *device = _udev_monitor_receive_device(_monitor);
    if (device == nullptr)
        return;
#ifdef _DEBUG
    const char *cap = udev_device_get_property_value(device, "ID_V4L_CAPABILITIES");
    printf("CAPABILITIES: %s\n", cap);
    printf("%s (%s) (%s)\n",
           udev_device_get_property_value(device, "ID_V4L_PRODUCT"),
           udev_device_get_devnode(device),
           udev_device_get_action(device));
    fflush(stdout);
#endif
    _udev_device_unref(device);

    DeviceMonitoringListChanged();
}

int DeviceMonitoringLinux::GetDescriptor()
{
    return _udev_monitor_get_fd(_monitor);
}

}
