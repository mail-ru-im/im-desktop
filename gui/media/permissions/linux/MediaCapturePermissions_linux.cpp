#include "stdafx.h"

#include "../MediaCapturePermissions.h"

namespace media::permissions
{

    Permission checkPermission(DeviceType)
    {
        return Permission::Allowed;
    }

    void requestPermission(DeviceType, PermissionCallback _callback)
    {
        if (_callback)
            _callback(true);
    }

    void openPermissionSettings(DeviceType)
    {
    }
}
