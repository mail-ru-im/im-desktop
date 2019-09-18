#include "device_monitoring_macos.h"

#import <Foundation/Foundation.h>
#import <AVFoundation/AVFoundation.h>
#import <IOKit/graphics/IOGraphicsLib.h>

#define TRACEFXN

@interface CaptureDeviceMonitor : NSObject {
    device::DeviceMonitoringCallback *_captureObserver;
}

@property (retain) NSArray *videoCaptureDevices;
@property (retain) NSArray *audioDevices;

@end

@implementation CaptureDeviceMonitor

@synthesize videoCaptureDevices = _videoCaptureDevices;
@synthesize audioDevices = _audioDevices;

- (id)init
{
    TRACEFXN
    self = [super init];
    if (self) {
        [[NSNotificationCenter defaultCenter] addObserver:self
                                                 selector:@selector(addDevices:)
                                                     name:AVCaptureDeviceWasConnectedNotification
                                                   object:nil];
        [[NSNotificationCenter defaultCenter] addObserver:self
                                                 selector:@selector(removeDevices:)
                                                     name:AVCaptureDeviceWasDisconnectedNotification
                                                   object:nil];
        self.videoCaptureDevices = [AVCaptureDevice devicesWithMediaType:AVMediaTypeVideo];
        self.audioDevices        = [AVCaptureDevice devicesWithMediaType:AVMediaTypeAudio];
    }
    return self;
}

- (void)dealloc
{
    TRACEFXN
    // Remove Observers
    [[NSNotificationCenter defaultCenter] removeObserver:self];
    self.videoCaptureDevices = nil;
    self.audioDevices = nil;
    [super dealloc];   
}

- (void)setCaptureObserver:(device::DeviceMonitoringCallback *)observer
{
    _captureObserver = observer;
}

- (void)addDevices:(NSNotification *)notification
{
    {
        NSArray *vdevices = [AVCaptureDevice devicesWithMediaType:AVMediaTypeVideo];
        for (AVCaptureDevice *device in vdevices) {
            if(NSNotFound == [self.videoCaptureDevices indexOfObject:device])
            {
                self.videoCaptureDevices = vdevices;
                if(_captureObserver) {
                    _captureObserver->DeviceMonitoringListChanged();
                }
                return;
            }
        }
    }
    {
        NSArray *adevices = [AVCaptureDevice devicesWithMediaType:AVMediaTypeAudio];
        for (AVCaptureDevice *adevice in adevices) {
            if(NSNotFound == [self.audioDevices indexOfObject:adevice])
            {
                self.audioDevices = adevices;
                if(_captureObserver) {
                    _captureObserver->DeviceMonitoringListChanged();
                }
                return;
            }
        }
    }

}
- (void)removeDevices:(NSNotification *)notification
{
    {
        NSArray *vdevices = [AVCaptureDevice devicesWithMediaType:AVMediaTypeVideo];
        for (AVCaptureDevice *device in self.videoCaptureDevices) {
            if(NSNotFound == [vdevices indexOfObject:device])
            {
                self.videoCaptureDevices = vdevices;
                if(_captureObserver) {
                    _captureObserver->DeviceMonitoringListChanged();
                }
                return;
            }
        }
    }
    {
        NSArray *adevices = [AVCaptureDevice devicesWithMediaType:AVMediaTypeAudio];
        for (AVCaptureDevice *adevice in self.audioDevices) {
            if(NSNotFound == [adevices indexOfObject:adevice])
            {
                self.audioDevices = adevices;
                if(_captureObserver) {
                    _captureObserver->DeviceMonitoringListChanged();
                }
                return;
            }
        }
    }
}

@end

namespace device {

DeviceMonitoringMacos::DeviceMonitoringMacos()
: _objcInstance(nullptr)
{}

DeviceMonitoringMacos::~DeviceMonitoringMacos()
{
    assert(nullptr == _objcInstance);
}

bool DeviceMonitoringMacos::Start()
{
    if (_objcInstance) {
        return false;
    }

    _objcInstance = [[CaptureDeviceMonitor alloc] init];
    [(CaptureDeviceMonitor*)_objcInstance setCaptureObserver:this];

    return true;
}

void DeviceMonitoringMacos::Stop()
{
    if (_objcInstance) {
        [(CaptureDeviceMonitor*)_objcInstance setCaptureObserver:nil];
        [(CaptureDeviceMonitor*)_objcInstance release];
        _objcInstance = nullptr;
    }
}

}
