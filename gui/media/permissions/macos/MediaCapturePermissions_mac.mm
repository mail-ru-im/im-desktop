#include "stdafx.h"

#include "../MediaCapturePermissions.h"

#import <AVFoundation/AVFoundation.h>
#import <AppKit/AppKit.h>

namespace
{
    media::permissions::Permission MediaAuthorizationStatus(NSString* mediaType)
    {
        if (@available(macOS 10.14, *))
        {
            Class aClass = [AVCaptureDevice class];
            SEL selector = @selector(authorizationStatusForMediaType:);
            if ([aClass respondsToSelector:selector])
            {
                NSInteger status = (NSInteger)[aClass performSelector:selector withObject:mediaType];
                switch (status)
                {
                case AVAuthorizationStatusAuthorized:
                    return media::permissions::Permission::Allowed;
                case AVAuthorizationStatusNotDetermined:
                    return media::permissions::Permission::NotDetermined;
                case AVAuthorizationStatusDenied:
                    return media::permissions::Permission::Denied;
                case AVAuthorizationStatusRestricted:
                    return media::permissions::Permission::Restricted;
                default:
                    assert(!"authorizationStatusForMediaType");
                }
            }
            else
            {
                qWarning() << "authorizationStatusForMediaType could not be executed";
            }
            return media::permissions::Permission::Allowed;
        }

        return media::permissions::Permission::Allowed;
    }

    media::permissions::Permission isScreenCaptureAllowed()
    {
        if (@available(macOS 10.15, *))
        {
            CFArrayRef windowList = CGWindowListCopyWindowInfo(kCGWindowListOptionOnScreenOnly, kCGNullWindowID);
            NSUInteger numberOfWindows = CFArrayGetCount(windowList);
            NSUInteger numberOfWindowsWithName = 0;
            for (int idx = 0; idx < numberOfWindows; ++idx) {
                NSDictionary *windowInfo = (NSDictionary *)CFArrayGetValueAtIndex(windowList, idx);
                NSString *windowName = windowInfo[(id)kCGWindowName];
                if (windowName) {
                    numberOfWindowsWithName++;
                } else {
                    //no kCGWindowName detected -> not enabled
                    break; //breaking early, numberOfWindowsWithName not increased
                }

            }
            CFRelease(windowList);

            if (numberOfWindows != numberOfWindowsWithName)
                return media::permissions::Permission::Denied;
        }

        return media::permissions::Permission::Allowed;
    }
}

constexpr auto getMediaType(media::permissions::DeviceType type) noexcept
{
    if (type == media::permissions::DeviceType::Microphone)
        return AVMediaTypeAudio;
    else if (type == media::permissions::DeviceType::Camera)
        return AVMediaTypeVideo;
    return AVMediaTypeAudio; // default
}

namespace media::permissions
{
    Permission checkPermission(DeviceType type)
    {
        if (type == DeviceType::Screen)
            return isScreenCaptureAllowed();

        return MediaAuthorizationStatus(getMediaType(type));
    }

    void requestPermission(DeviceType type, PermissionCallback _callback)
    {
        if (type == DeviceType::Screen)
        {
            if (@available(macOS 10.15, *))
            {
                CGDisplayStreamRef stream = CGDisplayStreamCreate(CGMainDisplayID(), 1, 1, kCVPixelFormatType_32BGRA, nil, ^(CGDisplayStreamFrameStatus status, uint64_t displayTime, IOSurfaceRef frameSurface, CGDisplayStreamUpdateRef updateRef) {});
                if (stream)
                    CFRelease(stream);
            }

            if (_callback)
                _callback(true);
        }
        else if (@available(macOS 10.14, *))
        {
            Class aClass = [AVCaptureDevice class];
            SEL selector = @selector(requestAccessForMediaType:completionHandler:);
            if ([aClass respondsToSelector:selector])
            {
                dispatch_async(dispatch_get_main_queue(), ^{
                    [aClass performSelector:selector withObject:getMediaType(type) withObject:^(BOOL granted)
                    {
                        if (_callback)
                            _callback(bool(granted));
                    }];
                });
            }
            else
            {
                qWarning() << "requestAccessForMediaType could not be executed";
            }
        }
        else
        {
            if (_callback)
                _callback(true);
        }
    }

    void openPermissionSettings(DeviceType type)
    {
        switch (type)
        {
        case DeviceType::Microphone:
            [[NSWorkspace sharedWorkspace] openURL:[NSURL URLWithString:@"x-apple.systempreferences:com.apple.preference.security?Privacy_Microphone"]];
            break;
        case DeviceType::Camera:
            [[NSWorkspace sharedWorkspace] openURL:[NSURL URLWithString:@"x-apple.systempreferences:com.apple.preference.security?Privacy_Camera"]];
            break;
        case DeviceType::Screen:
            [[NSWorkspace sharedWorkspace] openURL:[NSURL URLWithString:@"x-apple.systempreferences:com.apple.preference.security?Privacy_ScreenCapture"]];
            break;
        }
    }
}
