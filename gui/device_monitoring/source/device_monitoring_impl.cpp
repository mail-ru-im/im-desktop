#include "stdafx.h"
#include "device_monitoring_impl.h"

#include <boost/thread/locks.hpp>

namespace device
{

DeviceMonitoringImpl::DeviceMonitoringImpl()
: _captureDeviceCallback(nullptr)
{

}

DeviceMonitoringImpl::~DeviceMonitoringImpl()
{
    {
        boost::unique_lock cs(_callbackLock);
        _captureDeviceCallback = nullptr;
    }

}

void DeviceMonitoringImpl::DeviceMonitoringListChanged()
{
    boost::shared_lock cs(_callbackLock);
    //const uint32_t deviceNumber = GetNumberOfDevices();
    if (/*deviceNumber != _videoCaptureDeviceNumber &&*/ _captureDeviceCallback)
    {
        _captureDeviceCallback->DeviceMonitoringListChanged();
    //    _videoCaptureDeviceNumber = deviceNumber;
    }
}

void DeviceMonitoringImpl::DeviceMonitoringBluetoothHeadsetChanged(bool connected)
{
    boost::shared_lock cs(_callbackLock);
    if (_captureDeviceCallback)
        _captureDeviceCallback->DeviceMonitoringBluetoothHeadsetChanged(connected);
}

void DeviceMonitoringImpl::RegisterCaptureDeviceInfoObserver(DeviceMonitoringCallback& viECaptureDeviceInfoCallback)
{
    boost::unique_lock cs(_callbackLock);
    _captureDeviceCallback = &viECaptureDeviceInfoCallback;
}

void DeviceMonitoringImpl::DeregisterCaptureDeviceInfoObserver()
{
    boost::unique_lock cs(_callbackLock);
    _captureDeviceCallback = NULL;
}

}
