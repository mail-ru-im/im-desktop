#include "stdafx.h"

#include "../AudioUtils.h"

#import <AudioToolbox/AudioServices.h>
#import <Foundation/NSObject.h>

namespace
{
    AudioDeviceID defaultInputDeviceID()
    {
        AudioDeviceID   inputDeviceID = kAudioObjectUnknown;
        // get output device device
        UInt32 propertySize = 0;
        OSStatus status = noErr;
        AudioObjectPropertyAddress propertyAOPA;
        propertyAOPA.mScope = kAudioObjectPropertyScopeGlobal;
        propertyAOPA.mElement = kAudioObjectPropertyElementMaster;
        propertyAOPA.mSelector = kAudioHardwarePropertyDefaultInputDevice;
        if (!AudioHardwareServiceHasProperty(kAudioObjectSystemObject, &propertyAOPA))
        {
            return inputDeviceID;
        }
        propertySize = sizeof(AudioDeviceID);
        status = AudioHardwareServiceGetPropertyData(kAudioObjectSystemObject, &propertyAOPA, 0, NULL, &propertySize, &inputDeviceID);
        return inputDeviceID;
    }


    double getVolume(AudioDeviceID deviceId)
    {
        Float32         outputVolume;
        UInt32 propertySize = 0;
        OSStatus status = noErr;
        AudioObjectPropertyAddress propertyAOPA;
        propertyAOPA.mElement = kAudioObjectPropertyElementMaster;
        propertyAOPA.mSelector = kAudioHardwareServiceDeviceProperty_VirtualMasterVolume;
        propertyAOPA.mScope = kAudioDevicePropertyScopeInput;
        if (deviceId == kAudioObjectUnknown)
        {
            return 0.0;
        }
        if (!AudioHardwareServiceHasProperty(deviceId, &propertyAOPA))
        {
            NSLog(@"No volume returned for device 0x%0x", deviceId);
            return 0.0;
        }
        propertySize = sizeof(Float32);
        status = AudioHardwareServiceGetPropertyData(deviceId, &propertyAOPA, 0, NULL, &propertySize, &outputVolume);
        if (status)
        {
            NSLog(@"No volume returned for device 0x%0x", deviceId);
            return 0.0;
        }
        return std::clamp(double(outputVolume), 0.0, 1.0);
    }

}

namespace ptt
{
    double getVolumeDefaultDevice()
    {
        return getVolume(defaultInputDeviceID());
    }
}
