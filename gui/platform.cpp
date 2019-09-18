#include "stdafx.h"

#include "platform.h"

PLATFORM_NS_BEGIN

OsxVersion osxVersion()
{
    #if defined(__APPLE__)
        return (OsxVersion)QSysInfo().macVersion();
    #else
        assert(!"this is not osx dude");
        return OsxVersion::UNKNOWN;
    #endif
}

PLATFORM_NS_END