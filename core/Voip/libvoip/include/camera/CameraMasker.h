#pragma once
#include <string>
#include <memory>
#include "CameraTypes.h"

namespace rtc {
    class Thread;
}

namespace camera
{

class Camera;

class Masker
{
public:
    enum Status {
        Ok,
        Cancel,
        Fail
    };

    class Observer
    {
    public:
        virtual ~Observer() = default;
        virtual void onMaskEngineStatusChanged(bool engine_loaded,
                const std::string &error_message) = 0;
        virtual void onMaskStatusChanged(Status status, const std::string &path) = 0;
        virtual void onMaskTapEnabled(bool enabled) = 0;
    };

    static unsigned GetMaskEngineVersion();
    virtual void InitMaskEngine(std::string path_to_model,
                                bool allowOutputAsPlatformBuffer) = 0;
    virtual void UnloadMaskEngine() = 0;
    virtual void LoadMask(std::string path_to_mask) = 0;
    virtual ~Masker() = default;

    virtual Camera& getCamera() = 0;
    virtual void SetMaskerObserver(std::weak_ptr<Observer> observer) = 0;
    virtual void EnableMaskProcessing(bool enable) = 0;

    virtual void TapOnMask(PoiTap tap, float x, float y) = 0;

    static std::unique_ptr<Masker> CreateDefaultCameraWithMask(rtc::Thread *work_thread, void *platform_object);
};

}
