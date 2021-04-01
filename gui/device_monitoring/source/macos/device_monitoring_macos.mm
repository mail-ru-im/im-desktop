#include "device_monitoring_macos.h"

#import <Foundation/Foundation.h>
#import <AVFoundation/AVFoundation.h>
#import <IOKit/graphics/IOGraphicsLib.h>

#define TRACEFXN

@interface CaptureDeviceMonitor : NSObject {
    device::DeviceMonitoringMacos *_captureObserver;
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
        self.audioDevices = [AVCaptureDevice devicesWithMediaType:AVMediaTypeAudio];
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

- (void)setCaptureObserver:(device::DeviceMonitoringMacos *)observer
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
            if (_captureObserver)
                _captureObserver->DeviceMonitoringVideoListChanged();
            return;
        }
    }

    {
        NSArray *adevices = [AVCaptureDevice devicesWithMediaType:AVMediaTypeAudio];
        for (AVCaptureDevice *adevice in adevices) {
            if(NSNotFound == [self.audioDevices indexOfObject:adevice])
            {
                self.audioDevices = adevices;
                if (_captureObserver)
                    _captureObserver->DeviceMonitoringAudioListChanged();
                return;
            }
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
            if (_captureObserver)
                _captureObserver->DeviceMonitoringVideoListChanged();
            return;
        }
    }

    {
        NSArray *adevices = [AVCaptureDevice devicesWithMediaType:AVMediaTypeAudio];
        for (AVCaptureDevice *adevice in self.audioDevices) {
            if(NSNotFound == [adevices indexOfObject:adevice])
            {
                self.audioDevices = adevices;
                if (_captureObserver)
                    _captureObserver->DeviceMonitoringAudioListChanged();
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
    im_assert(nullptr == _objcInstance);
}

bool DeviceMonitoringMacos::Start()
{
    if (_isStarted)
        return false;
    SetHandlers(true);
    _objcInstance = [[CaptureDeviceMonitor alloc] init];
    [(CaptureDeviceMonitor*)_objcInstance setCaptureObserver:this];

    _isStarted = true;

    if (qApp)
    {
        QObject::connect(qApp, &QGuiApplication::screenAdded, this, &DeviceMonitoringMacos::DeviceMonitoringVideoListChanged, Qt::UniqueConnection);
        QObject::connect(qApp, &QGuiApplication::screenRemoved, this, &DeviceMonitoringMacos::DeviceMonitoringVideoListChanged, Qt::UniqueConnection);
        QObject::connect(qApp, &QGuiApplication::primaryScreenChanged, this, &DeviceMonitoringMacos::DeviceMonitoringVideoListChanged, Qt::UniqueConnection);
    }
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
    if (qApp)
        QObject::disconnect(qApp, nullptr, this, nullptr);
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
    ptrThis->DeviceMonitoringAudioListChanged();
    return 0;
}

}
