#include "stdafx.h"

#include "../MediaCapturePermissions.h"

namespace media::permissions
{
    Permission checkPermission(DeviceType type)
    {
        auto settings = [](DeviceType type) -> std::optional<QString>
        {
            if (type == DeviceType::Microphone)
                return qsl("Software\\Microsoft\\Windows\\CurrentVersion\\CapabilityAccessManager\\ConsentStore\\microphone");
            if (type == DeviceType::Camera)
                return qsl("Software\\Microsoft\\Windows\\CurrentVersion\\CapabilityAccessManager\\ConsentStore\\webcam");
            return std::nullopt;
        };

        Permission result = Permission::Allowed;
        if (const auto regPath = settings(type); regPath)
        {
            QSettings s(*regPath, QSettings::NativeFormat);
            if (s.value(qsl("Value")).toString() == u"Deny")
                result = Permission::Denied;
        }
        return result;
    }

    void requestPermission(DeviceType, PermissionCallback _callback)
    {
        if (_callback)
            _callback(true);
    }

    void openPermissionSettings(DeviceType type)
    {
        auto settings = [](DeviceType type) -> std::optional<std::wstring_view>
        {
            if (type == DeviceType::Microphone)
                return std::wstring_view(L"ms-settings:privacy-microphone");
            if (type == DeviceType::Camera)
                return std::wstring_view(L"ms-settings:privacy-webcam");
            return std::nullopt;
        };
        if (const auto s = settings(type); s)
        {
            ShellExecute(
                nullptr,
                L"open",
                s->data(),
                nullptr,
                nullptr,
                SW_SHOWDEFAULT);
        }
    }
}
