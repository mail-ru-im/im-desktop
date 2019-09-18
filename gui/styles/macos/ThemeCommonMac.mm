#include "stdafx.h"

#include "../ThemeCommon.h"

#include <AppKit/AppKit.h>

namespace Styling
{
    bool isAppInDarkMode()
    {
#if QT_MACOS_PLATFORM_SDK_EQUAL_OR_ABOVE(__MAC_10_14)
        if (@available(macOS 10.14, *)) {
            auto appearance = [NSApp.effectiveAppearance bestMatchFromAppearancesWithNames:
                    @[ NSAppearanceNameAqua, NSAppearanceNameDarkAqua ]];
            return [appearance isEqualToString:NSAppearanceNameDarkAqua];
        }
#endif
        return false;
    }
}
