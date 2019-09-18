#include "stdafx.h"

#include "../MediaCapturePermissions.h"

namespace media::permissions
{
    Permission checkPermission(DeviceType type)
    {
        auto settings = [](DeviceType type) -> std::optional<std::wstring_view>
        {
            if (type == DeviceType::Microphone)
                return std::wstring_view(L"Software\\Microsoft\\Windows\\CurrentVersion\\CapabilityAccessManager\\ConsentStore\\microphone");
            if (type == DeviceType::Camera)
                return std::wstring_view(L"Software\\Microsoft\\Windows\\CurrentVersion\\CapabilityAccessManager\\ConsentStore\\webcam");
            return std::nullopt;
        };
        if (const auto s = settings(type); s)
        {
            Permission result = Permission::Allowed;
            HKEY hKey;
            LSTATUS res = RegOpenKeyEx(HKEY_CURRENT_USER, s->data(), 0, KEY_QUERY_VALUE, &hKey);
            if (res == ERROR_SUCCESS)
            {
                wchar_t buf[20];
                DWORD length = sizeof(buf);
                res = RegQueryValueEx(hKey, L"Value", NULL, NULL, (LPBYTE)buf, &length);
                if (res == ERROR_SUCCESS)
                {
                    if (wcscmp(buf, L"Deny") == 0)
                        result = Permission::Denied;
                }
                RegCloseKey(hKey);
            }
            return result;
        }
        return Permission::Allowed;
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
