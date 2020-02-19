#pragma once

#include "CameraTypes.h"
#include "CameraEnumerator.h"
#include <api/mediastreaminterface.h> // GetVideoSource()


namespace camera
{

class Camera
{
public:
    //
    // Базовый интерфейс камеры для звонка
    //
    class Observer
    {
    public:
        virtual void onStatusChanged(State state, const std::string &error) = 0;
    };

    // platform_object - for Android is OpenGLES context
    static std::unique_ptr<Camera> CreateDefaultCamera(rtc::Thread *work_thread, void *platform_object);

    virtual ~Camera() = default;

    virtual void SetObserver(std::weak_ptr<Observer> observer) = 0;
    virtual void SelectDevice(CameraEnumerator::Type capturerType, CameraEnumerator::DeviceId capturerID) = 0;

public:
    //
    // Расширенный интерфейс камеры с элементами управления для снапов/масок
    // Записи фото и веидео
    //

    class ObserverExt
    {
    public:
        virtual void onPhotoFrameReady(const std::vector<unsigned char> &jpegData, bool success) = 0;
        virtual void onFlashStatusChanged(FlashState flashModes) = 0;
        virtual void onPointOfInterestChanged(float x, float y, bool adjusting) = 0;
    };
    virtual void SetObserverExt(std::weak_ptr<ObserverExt> observer) = 0;

    virtual void SetOutputFormatRequest(int width, int height, int fps) = 0;
    virtual void CapturePhoto(bool isHighres) = 0;
    virtual void SetZoom(float zoomFactor) = 0;
    virtual void SetPointOfInterest(float x, float y) = 0;
    virtual void SetMode(CaptureMode captureMode) = 0;
    virtual void SetFlashMode(FlashMode flash) = 0; //Direct Set Flash
    virtual void SwitchFlashMode() = 0; // Most usable case

public:
    virtual rtc::scoped_refptr<webrtc::VideoTrackSourceInterface> GetVideoSource() = 0;

public:

};

}
