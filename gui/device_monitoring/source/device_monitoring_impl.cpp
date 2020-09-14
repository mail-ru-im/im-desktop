#include "stdafx.h"
#include "device_monitoring_impl.h"
#include "../../core_dispatcher.h"

#include <boost/thread/locks.hpp>

namespace device
{

DeviceMonitoringImpl::DeviceMonitoringImpl()
    : _captureDeviceCallback(nullptr)
{
}

DeviceMonitoringImpl::~DeviceMonitoringImpl()
{
    boost::unique_lock cs(_callbackLock);
    _captureDeviceCallback = nullptr;
}

void DeviceMonitoringImpl::DeviceMonitoringVideoListChanged()
{
    Ui::GetDispatcher()->getVoipController().notifyDevicesChanged(false);
}

void DeviceMonitoringImpl::DeviceMonitoringAudioListChanged()
{
    Ui::GetDispatcher()->getVoipController().notifyDevicesChanged(true);
    boost::shared_lock cs(_callbackLock);
    if (_captureDeviceCallback)
        _captureDeviceCallback->DeviceMonitoringListChanged();
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
