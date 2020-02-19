#pragma once
#include <voip/voip3.h>
#include <string>
#include <vector>
#include "sigconv/SignallingConverter.h"
#include "camera/CameraEnumerator.h"
#include "camera/CameraMasker.h"

namespace base {
    class Invoker;
}

namespace rtc {
    class Thread;
}

namespace voip {
    class MediaManager;
}

namespace im {

class CallWindow;

enum AudioDeviceType {
    AudioDeviceRecording = 0,
    AudioDevicePlayback,
};

enum class LogLevel {
    Info,
    Warning,
    Error,
    None
};

struct AudioDeviceInfo {
    std::string  name;
    std::string  uid;
    bool         is_default;
};

using AudioDeviceInfoList = std::vector<AudioDeviceInfo>;

class VoipControllerObserver {
public:
    virtual ~VoipControllerObserver() = default;
    virtual void onPreOutgoingCall            (const voip::CallId &call_id, const std::string &user_id) = 0;
    virtual void onStartedOutgoingCall        (const voip::CallId &call_id, const std::string &user_id) = 0;
    virtual void onIncomingCall               (const voip::CallId &call_id, const std::string &from_user_id, bool is_video) = 0;
    virtual void onRinging                    (const voip::CallId &call_id) = 0;
    virtual void onOutgoingCallAccepted       (const voip::CallId &call_id, const voip::CallAppData &app_data_from_callee) = 0;
    virtual void onConnecting                 (const voip::CallId &call_id) = 0;
    virtual void onCallActive                 (const voip::CallId &call_id) = 0;
    virtual void onCallTerminated             (const voip::CallId &call_id, bool terminated_locally, voip::TerminateReason reason) = 0;
    virtual void onCallParticipantsUpdate     (const voip::CallId &call_id, const std::vector<voip::CallParticipantInfo> &participants) = 0;

    virtual void onCameraListUpdated          (const camera::CameraEnumerator::CameraInfoList &camera_list) {};
    virtual void onAudioDevicesListUpdated    (AudioDeviceType type, const AudioDeviceInfoList &audio_list) {};
    virtual void onStateUpdated(const voip::CallStateInternal &call_state) {};

    virtual void onMaskEngineStatusChanged(bool engine_loaded, const std::string &error_message) = 0;
    virtual void onMaskStatusChanged(camera::Masker::Status status, const std::string &path) = 0;
};

class MaskObserver;

class VoipController
    : public voip::VoipEngine::IVoipEngineObserver
{
public:
    VoipController(VoipControllerObserver& controllerObserver);
    virtual ~VoipController();

    static void EnableLogToDir(const std::string& path, const std::string& log_prefix, size_t max_total_logs_size, LogLevel logLevel);
    static void SetApplicationName(const std::string& name);
    static void VoipStat_Enable(bool enable, const voip::PeerId& account_id, const std::string& cache_folder);

    bool Initialize(voip::VoipEngine::ISignalingObserver *signalingObserver, const std::string &config_json);

    void Action_EnableMicrophone(const voip::CallId &call_id, bool enabled);
    void Action_EnableCamera(const voip::CallId &call_id, bool enabled);

    void Action_StartOutgoingCall(const std::string& to_user_id, bool is_video_call);
    void Action_AcceptIncomingCall(const voip::CallId &call_id, bool is_video_call);
    void Action_DeclineCall(const voip::CallId &call_id, bool busy);
    void Action_InviteParticipants(const voip::CallId &call_id, const std::vector<voip::PeerId> &participants);
    void Action_DropParticipants(const voip::CallId &call_id, const std::vector<voip::PeerId> &participants);

    void Action_SelectCamera(camera::CameraEnumerator::CameraInfo cam_info);  // pass empty |cam_info| to deselect camera
    void Action_SelectAudioDevice(AudioDeviceType type, const std::string &uid);

    static unsigned GetMaskEngineVersion();
    void InitMaskEngine(std::string path_to_model);
    void UnloadMaskEngine();
    void LoadMask(std::string path_to_mask);

    void VideoWindowAdd(const voip::CallId& call_id, std::shared_ptr<voip::IVoipVideoWindowRenderer> callWindow);
    void VideoWindowRemove(const voip::CallId& call_id, std::shared_ptr<voip::IVoipVideoWindowRenderer> callWindow);
    
    void RegisterSoundEvent(voip::SoundEvent soundEvent, bool repeatInLoop, const std::vector<uint8_t>& data, const std::vector<unsigned>& vibroPatternMs, unsigned samplingRateHz);
    void MuteSounds(bool mute);
    void SetAudioDeviceMute(AudioDeviceType type, bool mute);
    void SetAudioDeviceVolume(AudioDeviceType type, float volume);
    void UpdateCameraList();
    void UpdateAudioDevicesList();

    void UserRateLastCall(const voip::PeerId &user_id, int score, const std::string &survey_data);

private:
    // voip::VoipEngine::IVoipEngineObserver handlers
    void OnIncomingCall(const voip::CallStateInternal &call_state,
                        const voip::PeerId &from,
                        bool is_video,
                        const std::vector<voip::CallParticipantInfo> &participants,
                        const voip::CallAppData &app_data_from_caller) override;
    void OnRinging(const voip::CallStateInternal &call_state) override;
    void OnOutgoingCallAccepted(const voip::CallStateInternal &call_state, const voip::CallAppData &app_data_from_callee) override;
    void OnConnecting(const voip::CallStateInternal &call_state) override;
    void OnCallActive(const voip::CallStateInternal &call_state) override;
    void OnCallTerminated(const voip::CallStateInternal &call_state, bool terminated_locally, voip::TerminateReason reason) override;
    void OnCallParticipantsUpdate(const voip::CallStateInternal &call_state, const std::vector<voip::CallParticipantInfo> &participants) override;
    void OnStateUpdated(const voip::CallStateInternal &call_state) override;

public:
    std::unique_ptr<voip::VoipEngine>   _voip;

private:
    std::unique_ptr<base::Invoker>      _invoker;
    std::unique_ptr<rtc::Thread>        _work_thread;

    VoipControllerObserver&             _observer;

    std::shared_ptr<voip::MediaManager> _mediaManager;
    std::shared_ptr<MaskObserver>       _maskObserver;
};

}
