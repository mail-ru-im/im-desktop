#include "stdafx.h"
#include "tools/device_id.h"
#include <mach-o/arch.h>

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

unsigned short getCpuHash()
{
    const NXArchInfo* info = NXGetLocalArchInfo();
    unsigned short val = 0;
    val += (unsigned short)info->cputype;
    val += (unsigned short)info->cpusubtype;
    return val;
}

std::string core::tools::impl::get_device_id_impl()
{
    std::string device_id = serialNumber() + std::to_string(getCpuHash());
    return device_id;
}

