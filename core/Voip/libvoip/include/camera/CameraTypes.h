#pragma once

namespace camera {

    enum State {
        Started,         // StartCapture  success
        Stopped,         // StartCapture  stopped
        StartFailed,     // StartCapture  failed
        PhotoFailed      // CapturePhoto failed
    };

    //
    // Flash Control:
    //
    // ANDROID used with Exposure param                IOS
    //      CONTROL_AE_MODE_OFF                        AVCaptureFlashModeOff  & AVCaptureTorchModeOff
    //      CONTROL_AE_MODE_ON                         AVCaptureFlashModeOff  & AVCaptureTorchModeOff
    //      CONTROL_AE_MODE_ON_AUTO_FLASH              AVCaptureFlashModeAuto & AVCaptureTorchModeAuto
    //      CONTROL_AE_MODE_ON_ALWAYS_FLASH            AVCaptureFlashModeOn   & AVCaptureTorchModeOn
    //      CONTROL_AE_MODE_ON_AUTO_FLASH_REDEYE        + autoRedEyeReductionEnabled

    enum FlashMode {
        FlashModeOff = 1,
        FlashModeAlways = 2,
        FlashModeAuto = 3,
        FlashModeAutoRedEye = 4
    };

    enum FlashState {
        FlashStateUnavailable = -1,
        FlashStateOff = FlashModeOff,
        FlashStateAlways = FlashModeAlways,
        FlashStateAuto = FlashModeAuto,
        FlashStateAutoRedEye = FlashModeAutoRedEye
    };

    //
    // Camera Presets
    //
    //  IOS                     AF                AE          POI
    //      Preview:        Continuous        Continuous  ResetByMove
    //      VideoCall:      Continuous        Continuous  ResetByMove
    //      VideoRecording: Smooth(or Locked) Continuous  LockedByUser

    enum CaptureMode {
        Preview,       // (DEFAULT) Camera preview, like original device cam
        VideoCall,     // Regular call
        VideoRecording // Video recoding (smooth autofocus, not disable manual POI)
    };

    enum PoiTap {
        SingleTap,
        DoubleTap,
        LongTap
    };

}