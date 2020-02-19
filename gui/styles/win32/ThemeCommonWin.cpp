#include "stdafx.h"

#include "../ThemeCommon.h"

namespace Styling
{
    bool isAppInDarkMode()
    {
        HKEY hKey;
        LSTATUS res = RegOpenKeyEx(HKEY_CURRENT_USER, L"Software\\Microsoft\\Windows\\CurrentVersion\\Themes\\Personalize", 0, KEY_QUERY_VALUE, &hKey);
        if (res == ERROR_SUCCESS)
        {
            DWORD value = 0;
            DWORD length = sizeof(value);
            res = RegQueryValueEx(hKey, L"AppsUseLightTheme", NULL, NULL, reinterpret_cast<LPBYTE>(&value), &length);
            RegCloseKey(hKey);
            if (res == ERROR_SUCCESS)
                return !value;
        }
        return false;
    }
}
