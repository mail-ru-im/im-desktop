#pragma once

namespace media::permissions
{
    enum class DeviceType
    {
        Microphone,
        Camera
    };

    enum class Permission
    {
        NotDetermined = 0,
        Restricted = 1,
        Denied = 2,
        Allowed = 3,
        MaxValue = Allowed
    };

    using PermissionCallback = std::function<void(bool)>;

    Permission checkPermission(DeviceType);
    void requestPermission(DeviceType, PermissionCallback);

    void openPermissionSettings(DeviceType);
}
