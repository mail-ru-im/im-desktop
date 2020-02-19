#include "device_monitoring_macos.h"

#import <Foundation/Foundation.h>
#import <AVFoundation/AVFoundation.h>
#import <IOKit/graphics/IOGraphicsLib.h>

#define TRACEFXN

@interface CaptureDeviceMonitor : NSObject {
    device::DeviceMonitoringCallback *_captureObserver;
}

@property (retain) NSArray *videoCaptureDevices;

@end

@implementation CaptureDeviceMonitor

@synthesize videoCaptureDevices = _videoCaptureDevices;

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
    }
    return self;
}

- (void)dealloc
{
    TRACEFXN
    // Remove Observers
    [[NSNotificationCenter defaultCenter] removeObserver:self];
    self.videoCaptureDevices = nil;
    [super dealloc];
}

- (void)setCaptureObserver:(device::DeviceMonitoringCallback *)observer
{
    _captureObserver = observer;
}

- (void)addDevices:(NSNotification *)notification
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
- (void)removeDevices:(NSNotification *)notification
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
    if(_isStarted) return false;
    SetHandlers(true);
    _objcInstance = [[CaptureDeviceMonitor alloc] init];
    [(CaptureDeviceMonitor*)_objcInstance setCaptureObserver:this];

    _isStarted = true;

    return true;
}

void DeviceMonitoringMacos::Stop()
{
    if(!_isStarted) return;
    SetHandlers(false);
    if (_objcInstance) {
        [(CaptureDeviceMonitor*)_objcInstance setCaptureObserver:nil];
        [(CaptureDeviceMonitor*)_objcInstance release];
        _objcInstance = nullptr;
    }
    _isStarted = false;
}

typedef OSStatus AudioObjectChanePropertyListenerFuction(AudioObjectID,
                                                         const AudioObjectPropertyAddress *,
                                                         AudioObjectPropertyListenerProc,
                                                         void *);

void DeviceMonitoringMacos::SetHandlers(bool isSet)
{
    AudioObjectChanePropertyListenerFuction &AudioObjectChangePropertyListener =
            isSet ? AudioObjectAddPropertyListener : AudioObjectRemovePropertyListener;

    AudioObjectPropertyAddress propertyAddress = {kAudioHardwarePropertyDevices,
                                                  kAudioObjectPropertyScopeGlobal,
                                                  kAudioObjectPropertyElementMaster};

    AudioObjectChangePropertyListener(kAudioObjectSystemObject, &propertyAddress, objectListenerProc, this);

    propertyAddress.mSelector = kAudioHardwarePropertyDefaultInputDevice;
    AudioObjectChangePropertyListener(kAudioObjectSystemObject, &propertyAddress, objectListenerProc, this);

    propertyAddress.mSelector = kAudioHardwarePropertyDefaultOutputDevice;
    AudioObjectChangePropertyListener(kAudioObjectSystemObject, &propertyAddress, objectListenerProc, this);
}

OSStatus DeviceMonitoringMacos::objectListenerProc(
        AudioObjectID objectId,
        UInt32 numberAddresses,
        const AudioObjectPropertyAddress addresses[],
        void* clientData)
{
    auto* ptrThis = (DeviceMonitoringMacos*)clientData;
    ptrThis->DeviceMonitoringListChanged();
    return 0;
}

}
