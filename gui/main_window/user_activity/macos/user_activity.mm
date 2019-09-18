#include "stdafx.h"

#include "../user_activity.h"

#include <Cocoa/Cocoa.h>
#include <CoreFoundation/CFURL.h>
#include <IOKit/IOKitLib.h>
#include <IOKit/hidsystem/ev_keymap.h>

namespace UserActivity
{
    typedef std::chrono::time_point<std::chrono::system_clock> timepoint;

    timepoint _lastActivityTime;

    timepoint& lastActivityTime()
    {
        static timepoint tp;

        return tp;
    }

    bool objc_idleTime(uint64_t &idleTime)
    {
        CFMutableDictionaryRef properties = 0;
        CFTypeRef obj;
        mach_port_t masterPort;
        io_iterator_t iter;
        io_registry_entry_t curObj;

        IOMasterPort(MACH_PORT_NULL, &masterPort);

        /* Get IOHIDSystem */
        IOServiceGetMatchingServices(masterPort, IOServiceMatching("IOHIDSystem"), &iter);

        if (iter == 0)
        {
            return false;
        }
        else
        {
            curObj = IOIteratorNext(iter);
        }

        if (IORegistryEntryCreateCFProperties(curObj, &properties, kCFAllocatorDefault, 0) == KERN_SUCCESS && properties != NULL)
        {
            obj = CFDictionaryGetValue(properties, CFSTR("HIDIdleTime"));
            CFRetain(obj);
        }
        else
        {
            return false;
        }

        uint64 err = ~0L, result = err;
        if (obj)
        {
            CFTypeID type = CFGetTypeID(obj);

            if (type == CFDataGetTypeID())
            {
                CFDataGetBytes((CFDataRef)obj, CFRangeMake(0, sizeof(result)), (UInt8*)&result);
            }
            else if (type == CFNumberGetTypeID())
            {
                CFNumberGetValue((CFNumberRef)obj, kCFNumberSInt64Type, &result);
            }
            else
            {
                // error
            }

            CFRelease(obj);

            if (result != err)
            {
                result /= 1000000; // return as ms
            }
        }
        else
        {
            // error
        }

        CFRelease((CFTypeRef)properties);
        IOObjectRelease(curObj);
        IOObjectRelease(iter);

        if (result == err)
            return false;

        idleTime = result;
        return true;
    }

    std::chrono::milliseconds getLastActivityMs()
    {
        uint64_t idleTime = 0LL;

        if (objc_idleTime(idleTime))
        {
            return std::chrono::milliseconds(idleTime);
        }
        else
        {
            return std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now() - _lastActivityTime);
        }
    }

    std::chrono::milliseconds getLastAppActivityMs()
    {
        return std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now() - _lastActivityTime);
    }

    void raiseUserActivity()
    {
        _lastActivityTime = std::chrono::system_clock::now();
    }
}
