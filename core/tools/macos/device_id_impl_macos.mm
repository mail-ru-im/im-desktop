#include "stdafx.h"
#include "tools/device_id.h"

#import <IOKit/IOKitLib.h>
#import <Foundation/Foundation.h>
#import <CoreServices/CoreServices.h>

static std::string serialNumber()
{
    NSString *serial = nil;
    io_service_t platform = IOServiceGetMatchingService(kIOMasterPortDefault, IOServiceMatching("IOPlatformExpertDevice"));
    if (platform)
    {
        CFTypeRef serialNumberAsCFString = IORegistryEntryCreateCFProperty(platform, CFSTR(kIOPlatformSerialNumberKey), kCFAllocatorDefault, 0);
        if (serialNumberAsCFString)
            serial = CFBridgingRelease(serialNumberAsCFString);

        IOObjectRelease(platform);
    }

    if (serial == nil)
        return std::string();

    return std::string([serial UTF8String], [serial lengthOfBytesUsingEncoding:NSUTF8StringEncoding]);
}

static unsigned short getCpuHash()
{
#if defined(__x86_64__)
    return 1;
#elif defined(__arm64)
    return 2;
#else
    return 0;
#endif
}

std::string core::tools::impl::get_device_id_impl()
{
    std::string device_id = serialNumber() + std::to_string(getCpuHash());
    return device_id;
}

