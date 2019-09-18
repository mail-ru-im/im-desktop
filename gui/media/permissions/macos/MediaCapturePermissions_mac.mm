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
            AVCaptureDevice* target = [AVCaptureDevice class];
            SEL selector = @selector(authorizationStatusForMediaType:);
            if ([target respondsToSelector:selector])
            {
                NSInteger status = (NSInteger)[target performSelector:selector withObject:mediaType];
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
        return MediaAuthorizationStatus(getMediaType(type));
    }

    void requestPermission(DeviceType type, PermissionCallback _callback)
    {
        if (@available(macOS 10.14, *))
        {
            AVCaptureDevice* target = [AVCaptureDevice class];
            SEL selector = @selector(requestAccessForMediaType:completionHandler:);
            if ([target respondsToSelector:selector])
            {
                [target performSelector:selector withObject:getMediaType(type) withObject:^(BOOL granted)
                {
                    if (_callback)
                        _callback(bool(granted));
                }];
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
        }
    }
}
