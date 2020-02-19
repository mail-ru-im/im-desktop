#pragma once

#include <string>
#include <map>
#include <vector>

namespace camera {

class CameraEnumerator
{
public:
    enum Location {
        Front   = 0,
        Back    = 1,
        Unknown = 2
    };

    enum Type {
        RealCamera,
        VirtualCamera,
        ScreenCapture
    };

    using DeviceId = std::string;

    struct CameraInfo {
        std::string  name;
        Type         type     = RealCamera;
        Location     location = Unknown;
        DeviceId     deviceId;

        // Fuck C++11
        // https://stackoverflow.com/questions/39344444/brace-aggregate-initialization-for-structs-with-default-values
        CameraInfo(std::string name_, Type type_, Location location_, DeviceId deviceId_)
            : name(name_), type(type_), location(location_), deviceId(deviceId_) {}

        CameraInfo() {};
    };

    using CameraInfoList = std::vector<CameraInfo>;

    static bool GetDevicesList(Type deviceType, CameraInfoList& list, const std::string &virtual_camera_dir = "");

    struct CaptureFormat {
        struct FramerateRange  {
            int min = 0;
            int max = 0;
        } framerate;
        int width = 0;
        int height = 0;
    };

    using CaptureFormatList = std::vector<CaptureFormat>;

    static bool GetCaptureFormatList(Type deviceType, DeviceId device, CaptureFormatList& list);


    static Location StringToCameraLocation(const std::string& location);
    static Type StringToCameraType(const std::string& type);
    static std::string CameraLocationToString(Location location);
    static std::string CameraTypeToString(Type type);
};

}
