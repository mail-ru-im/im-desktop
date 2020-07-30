#include "stdafx.h"

#include "../ThemeCommon.h"

namespace Styling
{
    bool isAppInDarkMode()
    {
        QSettings s(qsl("HKEY_CURRENT_USER\\Software\\Microsoft\\Windows\\CurrentVersion\\Themes\\Personalize"), QSettings::NativeFormat);
        return s.value(qsl("AppsUseLightTheme"), 1).toInt() == 0;
    }
}
