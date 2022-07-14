#include "stdafx.h"
#include <memory>
#include <time.h>
#include <string>
#ifdef _WIN32
    #include <windows.h>
    #include <assert.h>
    #include <unordered_map>
#endif

#include "VoipManager.h"
#include "VoipController.h"
#include "VoipSerialization.h"
#include "../src/CallStateInternal.h"
#include "../connections/base_im.h"

#include "call_busy.h"
#include "call_connected.h"
#include "call_dial.h"
#include "call_end.h"
#include "call_hold.h"
#include "call_incoming.h"
#include "call_start.h"

#include "../utils.h"
#include "../../common.shared/version_info.h"
#include "../../common.shared/config/config.h"
#include "../../common.shared/omicron_keys.h"
#include "VoipProtocol.h"
#include "../async_task.h"

#include "rapidjson/document.h"
#include "../../libomicron/include/omicron/omicron.h"

#include "../tools/system.h"
#include "../tools/features.h"
#include "../../common.shared/json_helper.h"
#include <iostream>
#include "../../core/configuration/app_config.h"
#include "../../core/configuration/external_config.h"

using namespace rapidjson;
// In seconds: 1 week.
//constexpr std::chrono::seconds OLDER_RTP_DUMP_TIME_DIFF = std::chrono::hours(7 * 24);
//static const std::string RTP_DUMP_EXT = ".pcapng";

#define VOIP_ASSERT(exp) im_assert(exp)

#define VOIP_ASSERT_RETURN(exp) \
    if (!(exp)) \
    { \
        VOIP_ASSERT(false); \
        return; \
    }

#define VOIP_ASSERT_RETURN_VAL(exp, val) \
    if (!(exp)) \
    { \
        VOIP_ASSERT(false); \
        return (val); \
    }

#define VOIP_ASSERT_ACTION(exp, action) \
    if (!(exp)) \
    { \
        VOIP_ASSERT(false); \
        action; \
    }

#define VOIP_ASSERT_BREAK(exp) \
    if (!(exp)) \
    { \
        VOIP_ASSERT(false); \
        break; \
    }

#define SIGNAL_NOTIFICATION(type, key) \
{ \
    if (key) \
    { \
        __signal_to_ui(_dispatcher, type, *key); \
    } \
    else \
    {\
        __signal_to_ui(_dispatcher, type); \
    } \
}

#define PSTN_POSTFIX "@pstn"

namespace {
    std::string from_unicode_to_utf8(const std::wstring& str)
    {
        std::wstring_convert<std::codecvt_utf8<wchar_t>, wchar_t> cv;
        return cv.to_bytes(str);
    }

    template<typename __Param>
    void __signal_to_ui(core::core_dispatcher& dispatcher, voip_manager::eNotificationTypes ntype, const __Param& vtype)
    {
        core::coll_helper coll(dispatcher.create_collection(), true);
        ntype >> coll;
        vtype >> coll;
        dispatcher.post_message_to_gui("voip_signal", 0, coll.get());
    }

    void __signal_to_ui(core::core_dispatcher& dispatcher, voip_manager::eNotificationTypes ntype)
    {
        core::coll_helper coll(dispatcher.create_collection(), true);
        ntype >> coll;
        dispatcher.post_message_to_gui("voip_signal", 0, coll.get());
    }

    void *bitmapHandleFromRGBData(unsigned width, unsigned height, const void *data)
    {
        if (width == 0 || height == 0 || data == nullptr)
            return nullptr;
        voip::BitmapDesc *b = new voip::BitmapDesc;
        b->rgba_pixels = malloc(4*width*height);
        b->width  = width;
        b->height = height;
        memcpy((void *)b->rgba_pixels, data, 4*width*height);
        return (void*)b;
    }

    void bitmapReleaseHandle(void* hbmp)
    {
        voip::BitmapDesc *b = (voip::BitmapDesc *)hbmp;
        if (b->rgba_pixels)
            free((void *)b->rgba_pixels);
        delete b;
    }

    std::string normalizeUid(const std::string& uid)
    {
        std::string _uid = uid;
        auto position = _uid.find("@uin.icq");
        if (std::string::npos == position)
        {
            position = _uid.find(PSTN_POSTFIX);
        }
        if (std::string::npos != position)
        {
            _uid = _uid.substr(0, position);
        }
        return _uid;
    }

    struct uncopyable
    {
    protected:
        uncopyable() { }
    private:
        uncopyable(const uncopyable&);
        uncopyable& operator=(const uncopyable&);
    };

#ifdef _WIN32
    enum { kMaxPath = MAX_PATH + 1 };
#else
    enum { kMaxPath = 300 };
#endif
}

namespace voip_manager
{
    typedef voip::CallId VoipKey;

    void addVoipStatistic(std::stringstream& s, const core::coll_helper& params)
    {
        enum STAT_TYPE {ST_STRING = 0, ST_INT, ST_BOOL};

        auto readParam = [&params](const std::string& name, STAT_TYPE type) -> std::string
        {
            switch (type)
            {
                case ST_STRING: return params.get_value_as_string(name);
                case ST_INT   : return std::to_string(params.get_value_as_int(name));
                case ST_BOOL  : return params.get_value_as_bool(name) ? "true" : "false";
            }
            return "Unknown type";
        };

        // Type&Name -> to param type.
        std::map<std::pair<std::string, std::string>, STAT_TYPE> paramsExtractor = {
            { { "voip_call_start", "call_type" }, ST_STRING },
            { { "voip_call_start", "contact" }, ST_STRING },
            { { "voip_call_start", "mode" }, ST_STRING },
            { { "voip_call_start", "video" }, ST_BOOL },
            { { "voip_call_media_switch", "video" }, ST_BOOL },
            { { "voip_sounds_mute", "mute" }, ST_BOOL },
            { { "voip_call_decline", "mode" }, ST_STRING },
            { { "voip_call_decline", "contact" }, ST_STRING },
            { { "voip_call_accept", "mode" }, ST_STRING },
            { { "voip_call_accept", "contact" }, ST_STRING },
            { { "device_change", "dev_type" }, ST_STRING },
            { { "device_change", "uid" }, ST_STRING },
            { { "device_change", "force_reset" }, ST_BOOL },
            { { "audio_playback_mute", "mute" }, ST_STRING }};

        for (const auto& paramExtractor : paramsExtractor)
        {
            if (paramExtractor.first.first == params.get_value_as_string("type") && params.is_value_exist(paramExtractor.first.second))
            {
                s << "; " << paramExtractor.first.second << ":" << readParam(paramExtractor.first.second, paramExtractor.second);
            }
        }
    }

    class VoipManagerImpl
        : private uncopyable
        , public VoipManager
        , public im::VoipControllerObserver
        , public voip::VoipEngine::ISignalingObserver
        , public std::enable_shared_from_this<VoipManagerImpl>
    {

        enum eCallState
        {
            kCallState_Unknown = 0,
            kCallState_Created,

            kCallState_Initiated,
            kCallState_Accepted,

            kCallState_Connected,
            kCallState_Disconnected,
        };

        struct VoipSettings
        {
            bool enableRtpDump;
            bool enableVoipLogs;
            VoipSettings() : enableRtpDump(false), enableVoipLogs(false) { }
        };

        /**
         * This struct hold normalized user id and original id.
         *
         */
        struct UserID
        {
            UserID(const std::string& original_id) : original_id(original_id)
            {
                normalized_id = normalizeUid(original_id);
            }
            UserID(const char *original_id) : original_id(original_id)
            {
                normalized_id = normalizeUid(original_id);
            }

            operator std::string() const
            {
                return normalized_id;
            }
            operator const std::string&() const
            {
                return normalized_id;
            }
            bool operator== (const UserID& anotherId)
            {
                return anotherId.normalized_id == normalized_id;
            }

            std::string original_id;
            std::string normalized_id;
        };

        /**
         * ConnectionDesc is description for P2P connection.
         * Video conferension contains list of ConnectionDesc.
         *
         */
        struct ConnectionDesc
        {
            UserID  user_id = UserID("");
            bool    remote_cam_en = false;
            bool    remote_mic_en = false;
            bool    remote_sending_desktop = false;
            bool    connected_state = false;
            bool    delete_mark = false;
        };
        typedef std::shared_ptr<ConnectionDesc> ConnectionDescPtr;
        typedef std::list<ConnectionDescPtr> ConnectionDescList;

        /**
         * CallDesc is description for one talk.
         * Video conferension is one CallDesc with list of ConnectionDesc.
         */
        struct CallDesc
        {
            VoipKey call_id;
            ConnectionDescList connections;

            std::string call_type;
            std::string chat_id;
            eCallState call_state = kCallState_Created;
            bool       outgoing = false;
            uint32_t   started = 0; // GetTickCount()
            unsigned   quality = 0; // 0 - 100%

            // Is this call current
            bool current()
            {
                return started > 0;
            }

            bool is_remote_sending_desktop() const
            {
                return std::any_of(connections.begin(), connections.end(), [](auto x) { return x->remote_sending_desktop; });
            }
        };

        typedef std::shared_ptr<CallDesc> CallDescPtr;
        typedef std::list<CallDescPtr> CallDescList;

        struct WindowDesc
        {
            im::FramesCallback* _renderer;
            void *handle;
            std::string layout_type;
            float aspectRatio;
            float scaleCoeffitient;
            VoipKey call_id;
            VideoLayout layout;
            bool miniWindow;
            bool hover;
            bool incoming;
            bool large;

            WindowDesc() : handle(nullptr), layout_type("LayoutType_One"), aspectRatio(0.0f),
                scaleCoeffitient(0.0f), layout(OneIsBig), miniWindow(false),
                hover(false), incoming(false), large(false) {}
        };

        struct RequestInfo
        {
            std::string requestId;
            std::chrono::milliseconds ts;
        };

        std::vector<WindowDesc*> _windows;

        int64_t _voip_initialization = 0;

        CallDescList         _calls;

        std::function<void()>          callback_;
        std::shared_ptr<VoipProtocol> _voipProtocol;

        VoipDesc   _voip_desc;

        std::shared_ptr<im::VoipController>  _engine;
        std::unique_ptr<voip::WIMSignallingConverter> _wimConverter;
        device_list _captureDevices, _audioRecordDevices, _audioPlayDevices;
        voip::CallId _currentCall;
        void *_currentCallHwnd = nullptr;
        std::string _currentCallType;
        std::string _currentCallVCSName;
        std::string _currentCallVCSUrl;
        std::string _chatId;
        bool _currentCallVCSWebinar = false;
        std::string _to_accept_call;
        bool _to_start_video = false;
        bool _zero_participants = false;

        void* _avatarCache = nullptr;

        core::core_dispatcher& _dispatcher;

        bool sendViaIm_ = false;
        bool _defaultVideoDevStored = false, _defaultAudioPlayDevStored = false, _defaultAudioCapDevStored = false;
        bool _voipDestroyed = false;
        bool _videoTxRequested = false;
        bool _captureDeviceSet = false;
        std::string _lastAccount;
        im::CameraInfo _captureDeviceInfo;

    private:
        std::shared_ptr<im::VoipController> _get_engine(bool skipCreation = false);
        //std::string _get_rtp_dump_path() const;
        std::string _get_log_path() const;
        void _setup_log_folder() const;

        bool _init_sounds_sc(std::shared_ptr<im::VoipController> engine);

        bool is_vcs_call() const { return voip::kCallType_VCS == _currentCallType; }
        bool is_vcs_v2_call() const { return voip::kCallType_LINK == _currentCallType; }
        bool is_webinar() const { return _currentCallVCSWebinar; }
        bool is_pinned_room_call() const { return !_chatId.empty(); }
        void _update_video_window_state(CallDescPtr call);

        bool _call_create (const VoipKey& call_id, const UserID& user_id, CallDescPtr& desc);
        bool _call_destroy(const CallDescPtr& call, voip::TerminateReason sessionEvent);
        bool _call_get    (const VoipKey& call_id, CallDescPtr& desc);

        ConnectionDescPtr _create_connection(const UserID& user_id);
        bool _connection_destroy(CallDescPtr& desc, const UserID& user_uid);
        bool _connection_get (CallDescPtr& desc, const UserID& user_uid, ConnectionDescPtr& connectionDesc);

        void _call_request_connections(CallDescPtr call);
        bool _call_have_established_connection();
        bool _call_have_outgoing_connection();

        inline const VoipSettings _getVoipSettings() const;
        WindowDesc *_find_window(void *handle);
        WindowDesc *_find_primary_window(const VoipKey &call_id);

        void _update_device_list (DeviceType dev_type);
        void _notify_remote_video_state_changed(const std::string &call_id, ConnectionDescPtr connection);
        void _notify_hide_controls(bool hide);

        void _protocolSendAlloc  (const char* data, unsigned size); //executes in VOIP SIGNALING THREAD context
        void _protocolSendMessage(const VoipProtoMsg& data); //executes in VOIP SIGNALING THREAD context

        void walkThroughCurrentCall(std::function<bool(ConnectionDescPtr)> func);    // Enum all connection of current call.
        void walkThroughCalls(std::function<bool(CallDescPtr)> func);                // Enum all call.
        bool walkThroughCallConnections(CallDescPtr call, std::function<bool(ConnectionDescPtr)> func);                // Enum all connections of call.

        CallDescPtr get_current_call();  // @return current active call.

        // @return list of window related with this call.
        std::vector<void*> _get_call_windows(const VoipKey &call_id);

        //void _cleanOldRTPDumps();

        bool _is_invite_packet(const char *data, unsigned len);

        void _updateAccount(const std::string &account);
        void parseAuxCallName(CallDescPtr desc, const voip::CallStateInternal &call_state);

    public:
        //=========================== ICallManager API ===========================
        void call_create                  (const std::vector<std::string> &contacts, std::string &account, const CallStartParams &params) override;
        void call_stop                    () override;
        void call_accept                  (const std::string &call_id, const std::string &account, bool video) override;
        void call_decline                 (const Contact& contact, bool busy, bool conference) override;
        unsigned call_get_count           () override;
        void call_stop_smart              (const std::function<void()>&) override;

        void mute_incoming_call_sounds    (bool mute) override;
        bool has_created_call() override;

        void call_report_user_rating(const UserRatingReport& _ratingReport) override;
        void peer_resolution_changed(const std::string &contact, int width, int height) override;

        //=========================== IWindowManager API ===========================
        void window_add           (voip_manager::WindowParams& windowParams) override;
        void window_remove        (void* hwnd) override;
        void window_set_bitmap    (const UserBitmap& bmp) override;

        //=========================== IMediaManager API ===========================
        void media_video_en       (bool enable) override;
        void media_audio_en       (bool enable) override;
        bool local_video_enabled  () override;
        bool local_audio_enabled  () override;

        //=========================== IVoipManager API ===========================
        void reset() override;

        //=========================== voip::VoipEngine::ISignalingObserver =============
        void OnAllocateCall(const voip::CallId &call_id) override;
        void OnSendSignalingMsg(const voip::CallId &call_id, const voip::PeerId &to, const voip::SignalingMsg &message) override;

        //=========================== IConnectionManager API ===========================
        void ProcessVoipMsg(const std::string& account_uid, int voipIncomingMsg, const char *data, unsigned len) override;
        void ProcessVoipAck(const voip_manager::VoipProtoMsg& msg, bool success) override;

        //=========================== IDeviceManager API ===========================
        unsigned get_devices_number(DeviceType device_type) override;
        bool     get_device_list   (DeviceType device_type, std::vector<device_description>& dev_list) override;
        bool     get_device        (DeviceType device_type, unsigned index, std::string& device_name, std::string& device_guid) override;
        void     set_device        (DeviceType device_type, const std::string& device_guid, bool force_reset) override;
        void     set_device_mute   (DeviceType deviceType, bool mute) override;
        bool     get_device_mute   (DeviceType deviceType) override;
        void     notify_devices_changed(DeviceClass deviceClass) override;
        void     update() override;

        //=========================== Memory Consumption =============================
        int64_t get_voip_initialization_memory() const override;

        //=========================== Statistics Send ================================
        void voip_statistics_enable() override;

        //=========================== VoipControllerObserver =========================
        void onPreOutgoingCall            (const voip::CallId &call_id, const std::vector<std::string> &participants)  override;
        void onStartedOutgoingCall        (const voip::CallId &call_id, const std::vector<std::string> &participants)  override;
        void onIncomingCall               (const voip::CallId &call_id, const std::string &from_user_id, bool is_video, const voip::CallStateInternal &call_state, const voip::CallAppData &app_data) override;
        void onRinging                    (const voip::CallId &call_id) override;
        void onOutgoingCallAccepted       (const voip::CallId &call_id, const voip::CallAppData &app_data_from_callee)override;
        void onConnecting                 (const voip::CallId &call_id) override;
        void onCallActive                 (const voip::CallId &call_id) override;
        void onCallTerminated             (const voip::CallId &call_id, bool terminated_locally, voip::TerminateReason reason) override;
        void onCallParticipantsUpdate     (const voip::CallId &call_id, const std::vector<voip::CallParticipantInfo> &participants) override;

        void onCameraListUpdated          (const im::CameraInfoList &camera_list) override;
        void onAudioDevicesListUpdated    (im::AudioDeviceType type, const im::AudioDeviceInfoList &audio_list) override;

        void onStateUpdated(const voip::CallStateInternal &call_state) override;

        //=========================== camera::Masker::Observer ==========================
        void onMaskEngineStatusChanged(bool engine_loaded, const std::string &error_message) override {};
        void onMaskStatusChanged(im::MaskerStatus status, const std::string &path) override {};
        void onVoiceDetectChanged(bool voice) override;
        void onRemoteVoices(const std::vector<im::VADInfo> &_participants) override;
    public:
        VoipManagerImpl (core::core_dispatcher& dispatcher);
        virtual ~VoipManagerImpl();
    };

    VoipManagerImpl::VoipManagerImpl(core::core_dispatcher& dispatcher)
        : _dispatcher(dispatcher)
    {
        srand(time(nullptr));
        const VoipSettings voipSettings = _getVoipSettings();
        if (voipSettings.enableVoipLogs)
        {
            std::wstring filePath = core::utils::get_product_data_path();
            VOIP_ASSERT_RETURN(!filePath.empty());
#ifdef _WIN32
            filePath += L"\\voip_log.txt";
#else
            filePath += L"/voip_log.txt";
#endif
            //const bool openResult = _voipLogger.logOpen(from_unicode_to_utf8(filePath));
            //im_assert(openResult);
        }
    }

    VoipManagerImpl::~VoipManagerImpl()
    {
        _engine.reset();
        VOIP_ASSERT(_calls.empty());
        if (_avatarCache)
            bitmapReleaseHandle(_avatarCache);
    }

    const VoipManagerImpl::VoipSettings VoipManagerImpl::_getVoipSettings() const
    {
        VoipSettings settings;
        std::wstring filePath = core::utils::get_product_data_path();
        VOIP_ASSERT_RETURN_VAL(!filePath.empty(), settings);
#ifdef _WIN32
        filePath += L"\\settings\\voip_config.txt";
#else
        filePath += L"/settings/voip_config.txt";
#endif

        std::ifstream fileStream;
        fileStream.open(from_unicode_to_utf8(filePath));
        if (!fileStream.is_open())
        {
            return settings;
        }

        auto __removeSpaces = [] (std::string& str)
        {
            str.erase (std::remove(str.begin(), str.end(), ' '), str.end());
        };

        while (fileStream.good())
        {
            std::string line;
            std::getline(fileStream, line);

            const std::string::size_type position = line.find('=');
            VOIP_ASSERT_ACTION(position != std::string::npos, continue);

            std::string param = line.substr(0, position);
            VOIP_ASSERT_ACTION(!param.empty(), continue);

            std::string value = line.substr(position + 1);
            VOIP_ASSERT_ACTION(!value.empty(), continue);

            __removeSpaces(param);
            __removeSpaces(value);
            std::transform(param.begin(), param.end(), param.begin(), ::tolower);

            if (std::string::npos != param.find("enablertpdump"))
            {
                settings.enableRtpDump = value[0] != '0';
            }
            else if (std::string::npos != param.find("enablevoiplogs"))
            {
                settings.enableVoipLogs = value[0] != '0';
            }
        }
        return settings;
    }

    bool VoipManagerImpl::_init_sounds_sc(std::shared_ptr<im::VoipController> engine)
    {
        VOIP_ASSERT_RETURN_VAL(!!engine, false);
        std::vector<unsigned> vibroPatternMs;
        std::vector<uint8_t> v_call_busy_data(call_busy_data, call_busy_data + sizeof(call_busy_data));
        std::vector<uint8_t> v_call_connected_data(call_connected_data, call_connected_data + sizeof(call_connected_data));
        std::vector<uint8_t> v_call_dial_data(call_dial_data, call_dial_data + sizeof(call_dial_data));
        std::vector<uint8_t> v_call_end_data(call_end_data, call_end_data + sizeof(call_end_data));
        std::vector<uint8_t> v_call_hold_data(call_hold_data, call_hold_data + sizeof(call_hold_data));
        std::vector<uint8_t> v_call_incoming_data(call_incoming_data, call_incoming_data + sizeof(call_incoming_data));
        std::vector<uint8_t> v_call_start_data(call_start_data, call_start_data + sizeof(call_start_data));
        engine->RegisterSoundEvent(voip::HangupRemoteBusy, false, v_call_busy_data, vibroPatternMs, 0);
        engine->RegisterSoundEvent(voip::Connected, false, v_call_connected_data, vibroPatternMs, 0);
        engine->RegisterSoundEvent(voip::WaitingForAccept_Confirmed, true, v_call_dial_data, vibroPatternMs, 0);
        engine->RegisterSoundEvent(voip::HangupLocal, false, v_call_end_data, vibroPatternMs, 0);
        engine->RegisterSoundEvent(voip::HangupRemote, false, v_call_end_data, vibroPatternMs, 0);
        engine->RegisterSoundEvent(voip::HangupHandledByAnotherInstance, false, v_call_end_data, vibroPatternMs, 0);
        engine->RegisterSoundEvent(voip::HangupByError, false, v_call_end_data, vibroPatternMs, 0);
        engine->RegisterSoundEvent(voip::Hold, true, v_call_hold_data, vibroPatternMs, 0);
        engine->RegisterSoundEvent(voip::IncomingInvite, true, v_call_incoming_data, vibroPatternMs, 0);
        engine->RegisterSoundEvent(voip::WaitingForAccept, true, v_call_start_data, vibroPatternMs, 0);
        engine->RegisterSoundEvent(voip::Reconnecting, true, v_call_start_data, vibroPatternMs, 0);
        engine->RegisterSoundEvent(voip::Connecting, true, v_call_start_data, vibroPatternMs, 0);
        engine->RegisterSoundEvent(voip::HangupRejectRemote, false, v_call_end_data, vibroPatternMs, 0);
        return true;
    }

    /*std::string VoipManagerImpl::_get_rtp_dump_path() const
    {
        return _get_log_path() + "/RtpDump";
    }*/

    std::string VoipManagerImpl::_get_log_path() const
    {
        return from_unicode_to_utf8(core::utils::get_logs_path().wstring());
    }

    void VoipManagerImpl::_setup_log_folder() const
    {
        std::string log_path = _get_log_path();
        constexpr size_t max_log_folder = 0;
        im::VoipController::EnableLogToDir(log_path, "voip", max_log_folder, im::LogLevel::Info);
    }

    std::shared_ptr<im::VoipController> VoipManagerImpl::_get_engine(bool skipCreation/* = false*/)
    {
#ifndef STRIP_VOIP
        if (!_engine)
        {
            if (_voipDestroyed || skipCreation)
                return nullptr;

            _setup_log_folder();

            std::string app_name;
            app_name += config::get().string(config::values::product_name_short);
            app_name += ".desktop ";
            app_name += core::tools::version_info().get_version();

            im::VoipController::SetApplicationName(app_name);

            auto beforeVoip = core::utils::get_current_process_real_mem_usage();
            omicronlib::json_string config_json = omicronlib::_o(omicron::keys::voip_config, omicronlib::json_string(""));
            auto engine = im::VoipController::CreateVoipControllerImpl(*this, this, config_json);
            VOIP_ASSERT_RETURN_VAL(engine, nullptr);
            _engine.reset(engine);

            _wimConverter = std::make_unique<voip::WIMSignallingConverter>(0, 1, 0, 1);

            auto afterVoip = core::utils::get_current_process_real_mem_usage();
            _voip_initialization = afterVoip - beforeVoip;

            _init_sounds_sc(_engine);

            // First device scan. Note, calling Update*List() will in turn call onAudioDevicesListUpdated(), onCameraListUpdated()
            engine->UpdateAudioDevicesList();
            engine->UpdateCameraList();
        }
        return _engine;
#else
        return nullptr
#endif
    }

    bool VoipManagerImpl::_call_create(const VoipKey& call_id, const UserID& user_id, CallDescPtr& desc)
    {
#ifndef STRIP_VOIP
        //VOIP_ASSERT_RETURN_VAL(!user_id.normalized_id.empty(), false);
        auto engine = _get_engine(true);

        if (engine) {
            bool dump_enable = core::configuration::get_app_config().is_save_rtp_dumps_enabled();
            engine->EnableAecDump(dump_enable);
        }

        ConnectionDescPtr connection = nullptr;
        if (!user_id.normalized_id.empty())
        {
            connection = _create_connection(user_id);
            VOIP_ASSERT_RETURN_VAL(!!connection, false);
        }

        desc.reset(new(std::nothrow) CallDesc());
        VOIP_ASSERT_RETURN_VAL(!!desc, false);
        desc->call_id  = call_id;
        if (!user_id.normalized_id.empty())
            desc->connections.push_back(connection);

        _calls.push_back(desc);
#endif
        return true;
    }

    void VoipManagerImpl::_notify_remote_video_state_changed(const std::string &call_id, ConnectionDescPtr connection)
    {
        VideoEnable videoEnable = { connection->remote_cam_en, Contact(call_id, connection->user_id)};
        SIGNAL_NOTIFICATION(kNotificationType_MediaRemVideoChanged, &videoEnable);
    }

    void VoipManagerImpl::_notify_hide_controls(bool hide)
    {
        EnableParams hideControls;
        hideControls.enable = hide;
        SIGNAL_NOTIFICATION(kNotificationType_HideControlsWhenRemDesktopSharing, &hideControls);
    }

    bool VoipManagerImpl::_call_destroy(const CallDescPtr& call, voip::TerminateReason endReason)
    {
        call->connections.clear();
        _calls.erase(std::remove_if(_calls.begin(), _calls.end(), [call](const CallDescPtr& c)
                {
                    return c == call;
                }
            ), _calls.end());

        return true;
    }

    void VoipManagerImpl::media_video_en(bool enable)
    {
        _videoTxRequested = enable;
        // We enable video device is selected correct video capture type.
        auto engine = _get_engine();
        VOIP_ASSERT_RETURN(!!engine);
        auto currentCall = get_current_call();
        if (currentCall)
        {
            if (enable && _captureDeviceSet)
                engine->Action_SelectCamera(_captureDeviceInfo);
            engine->Action_EnableCamera(currentCall->call_id, enable && _captureDeviceSet);
        }
    }

    void VoipManagerImpl::media_audio_en(bool enable)
    {
        auto engine = _get_engine();
        VOIP_ASSERT_RETURN(!!engine);
        auto currentCall = get_current_call();
        if (currentCall)
            engine->Action_EnableMicrophone(currentCall->call_id, enable);
    }

    bool VoipManagerImpl::local_video_enabled()
    {
        return _voip_desc.local_cam_en;
    }

    bool VoipManagerImpl::local_audio_enabled()
    {
        return _voip_desc.local_aud_en;
    }

    bool VoipManagerImpl::_call_get(const VoipKey& call_id, CallDescPtr& desc)
    {
        bool res = false;
        walkThroughCalls([&call_id, &res, &desc](CallDescPtr call) -> bool
            {
                if (call->call_id == call_id)
                {
                    desc = call;
                    res = true;
                }
                return res;
            });
        return res;
    }

    void VoipManagerImpl::call_create(const std::vector<std::string> &contacts, std::string &account, const CallStartParams &params)
    {
        VOIP_ASSERT_RETURN(!contacts.empty());
        auto engine = _get_engine();
        VOIP_ASSERT_RETURN(!!engine);

        CallDescPtr currentCall = get_current_call();
        if (currentCall && !params.attach)
            return;

        _updateAccount(account);

        if (params.attach)
        {   // Append connection to current call.
            VOIP_ASSERT_RETURN(!!currentCall);
            engine->Action_InviteParticipants(currentCall->call_id, contacts);
        } else
        {
            _videoTxRequested = params.video;
            set_device(VideoCapturing, _voip_desc.vCaptureDevice, false); // set last selected camera (sharing can be enabled during last call)
            _currentCallType = (params.is_pinned_room || params.is_vcs) ? voip::kCallType_PINNED_ROOM : voip::kCallType_NORMAL;
            std::string app_data;
            if (params.is_pinned_room || params.is_vcs)
            {
                _chatId = contacts[0];
                bool oldVCS = !features::is_vcs_call_by_link_v2_enabled();
                if (params.is_vcs && oldVCS)
                {
                    _currentCallType = voip::kCallType_VCS;
                    app_data = su::concat("{ \"call_type\" : \"VCS\", \"url\" : \"", _chatId, "\" }");
                }
                else if (params.is_vcs)
                {
                    _currentCallType = voip::kCallType_LINK;
                    _currentCallVCSUrl = _chatId;
                    app_data = su::concat("{ \"call_type\" : \"link\", \"url\" : \"", _chatId.substr(_chatId.find_last_of("/") + 1), "\" }");
                }
                else
                {
                    _currentCallType = voip::kCallType_PINNED_ROOM;
                    app_data = su::concat("{ \"call_type\" : \"pinned_room\", \"url\" : \"", _chatId, "\" }");
                }
                std::vector<std::string> local_contacts = contacts;
                local_contacts.erase(local_contacts.begin());
                engine->Action_StartOutgoingCall(local_contacts, _videoTxRequested && _captureDeviceSet, app_data);
                return;
            }
            engine->Action_StartOutgoingCall(contacts, _videoTxRequested && _captureDeviceSet, app_data);
        }
    }

    void VoipManagerImpl::call_stop()
    {
        auto engine = _get_engine(true);
        VOIP_ASSERT_RETURN(!!engine);
        if (!_currentCall.empty())
            engine->Action_DeclineCall(_currentCall, false);
    }

    void VoipManagerImpl::call_stop_smart(const std::function<void()>& callback)
    {
        auto engine = _get_engine(true);
        if (!engine)
        {
            callback();
            return;
        }
        const bool haveCalls = !_calls.empty();
        if (!haveCalls)
        {
            callback();
            return;
        }
        // Stop all calls.
        callback_  = callback;
        sendViaIm_ = true;
        for (const auto& c: _calls)
        {
            engine->Action_DeclineCall(c->call_id, false);
        }
    }

    void VoipManagerImpl::call_accept(const std::string &call_id, const std::string &account, bool video)
    {
        VOIP_ASSERT_RETURN(!call_id.empty());
        auto engine = _get_engine();
        VOIP_ASSERT_RETURN(!!engine);

        _updateAccount(account);

        auto currentCall = get_current_call();
        if (currentCall)
        {
            _to_accept_call = call_id;
            _to_start_video = video;
            engine->Action_DeclineCall(_currentCall, false);
            return;
        }

        CallDescPtr desc;
        VOIP_ASSERT_RETURN(!!_call_get(call_id, desc));
        VOIP_ASSERT_RETURN(!desc->outgoing);
        _currentCall = call_id;
        _currentCallType = desc->call_type;
        _chatId = desc->chat_id;
        desc->call_state = kCallState_Accepted;
        if (!desc->started)
            desc->started = (unsigned)clock();

        ContactEx contact_ex;
        contact_ex.contact.call_id = call_id;
        contact_ex.call_type       = desc->call_type;
        contact_ex.chat_id         = desc->chat_id;
        contact_ex.incoming        = !desc->outgoing;

        SIGNAL_NOTIFICATION(kNotificationType_CallInAccepted, &contact_ex);
        _update_video_window_state(desc);

        _videoTxRequested = video;
        set_device(VideoCapturing, _voip_desc.vCaptureDevice, false); // set last selected camera (sharing can be enabled during last call)
        media_audio_en(true);
        engine->Action_AcceptIncomingCall(call_id, _videoTxRequested && _captureDeviceSet);
    }

    void VoipManagerImpl::call_decline(const Contact& contact, bool busy, bool conference)
    {
        VOIP_ASSERT_RETURN(!contact.contact.empty());
        auto engine = _get_engine();
        VOIP_ASSERT_RETURN(!!engine);
        CallDescPtr desc;
        std::string call_id;
        if (contact.call_id.empty())
        {
            desc = get_current_call();
            if (!desc)
                return;
            call_id = desc->call_id;
        } else
        {
            call_id = contact.call_id;
            VOIP_ASSERT_RETURN(!!_call_get(call_id, desc));
        }
        if (conference)
        {
            std::vector<voip::PeerId> participants;
            participants.push_back(contact.contact.c_str());
            engine->Action_DropParticipants(call_id, participants);
        } else
        {
            engine->Action_DeclineCall(call_id, busy);
        }
    }

    unsigned VoipManagerImpl::call_get_count()
    {
        return _calls.size();
    }

    void VoipManagerImpl::_call_request_connections(CallDescPtr call)
    {
        ContactsList contacts;
        if (call)
        {
            contacts.contacts.reserve(call->connections.size());

            walkThroughCallConnections(call, [&call, &contacts](ConnectionDescPtr connection) -> bool
            {
                Contact contact(call->call_id, connection->user_id);
                contact.connected_state    = connection->connected_state;
                contact.remote_cam_enabled = connection->remote_cam_en;
                contact.remote_mic_enabled = connection->remote_mic_en;
                contact.remote_sending_desktop = connection->remote_sending_desktop;
                contacts.contacts.push_back(contact);
                return false;
            });

            contacts.windows = _get_call_windows(call->call_id);
            contacts.conference_name = _currentCallVCSName;
            contacts.conference_url = _currentCallVCSUrl;
            contacts.is_webinar = _currentCallVCSWebinar;
            contacts.isActive = call->current() || call->outgoing;
        }
        SIGNAL_NOTIFICATION(kNotificationType_CallPeerListChanged, &contacts);
    }

    bool VoipManagerImpl::_call_have_established_connection()
    {
        for (auto it = _calls.begin(); it != _calls.end(); ++it)
        {
            auto call = *it;
            VOIP_ASSERT_ACTION(!!call, continue);
            if (call->current())
                return true;
        }
        return false;
    }

    bool VoipManagerImpl::_call_have_outgoing_connection()
    {
        bool res = false;
        walkThroughCalls([&res](CallDescPtr call)
        {
            res = call->outgoing;
            return res;
        });
        return res;
    }

    VoipManagerImpl::WindowDesc *VoipManagerImpl::_find_window(void *handle)
    {
        for (size_t ix = 0; ix < _windows.size(); ++ix)
        {
            WindowDesc *window_description = _windows[ix];
            if (window_description->handle == handle)
                return window_description;
        }
        return nullptr;
    }

    VoipManagerImpl::WindowDesc *VoipManagerImpl::_find_primary_window(const VoipKey &call_id)
    {
        auto it = std::find_if(_windows.begin(), _windows.end(), [call_id](WindowDesc* window) { return window->call_id == call_id && !window->incoming && !window->miniWindow; });
        return it != _windows.end() ? *it : nullptr;
    }

    void VoipManagerImpl::window_add(voip_manager::WindowParams& windowParams)
    {
        VOIP_ASSERT_RETURN(!!windowParams.hwnd);
        if (_find_window(windowParams.hwnd))
            return;
        VOIP_ASSERT_RETURN(!windowParams.call_id.empty() || !_currentCall.empty());
        auto engine = _get_engine();
        VOIP_ASSERT_RETURN(!!engine);

        im::FramesCallback* render = (im::FramesCallback*)windowParams.hwnd;
        WindowDesc *window_description = new WindowDesc;
        window_description->_renderer = render;
        window_description->handle = windowParams.hwnd;
        window_description->aspectRatio = 0.0f;
        window_description->scaleCoeffitient = windowParams.scale >= 0.01 ? windowParams.scale : 1.0f;
        window_description->layout_type = "LayoutType_One";
        window_description->layout = OneIsBig;
        window_description->incoming = windowParams.isIncoming;
        window_description->miniWindow = !windowParams.isPrimary && !windowParams.isIncoming;
        window_description->call_id = !windowParams.call_id.empty() ? windowParams.call_id : _currentCall;

        if (windowParams.isPrimary)
            _currentCallHwnd = window_description->handle;
        _windows.push_back(window_description);

        if (_avatarCache)
            engine->Call_SetDisabledVideoFrame(_currentCall, _avatarCache);

        // Send contects for new window.
        CallDescPtr desc;
        VOIP_ASSERT_RETURN(!!_call_get(window_description->call_id, desc));
        _call_request_connections(desc);

        engine->Action_AttachRender(desc->call_id, *render);

        const intptr_t winId = (intptr_t)windowParams.hwnd;
        SIGNAL_NOTIFICATION(kNotificationType_VoipWindowAddComplete, &winId);
    }

    void VoipManagerImpl::window_remove(void *hwnd)
    {
        auto engine = _get_engine(true);
        if (!engine)
        {
            _windows.clear();
            return;
        }
        // We cannot use async WindowRemove, because it desyncs addWindow and remove.
        // As result we can hide video window on start call.
        WindowDesc *desc = _find_window(hwnd);
        if (desc)
        {
            if (_currentCallHwnd == desc->handle)
                _currentCallHwnd = nullptr;
            for (auto it = _windows.begin(); it != _windows.end(); ++it)
            {
                if (*it == desc)
                {
                    _windows.erase(it);
                    break;
                }
            }
            if (desc->_renderer)
                engine->Action_DetachRender(desc->call_id);
            delete desc;
        }
        const intptr_t winId = (intptr_t)hwnd;
        SIGNAL_NOTIFICATION(kNotificationType_VoipWindowRemoveComplete, &winId)
    }

    void VoipManagerImpl::window_set_bitmap(const UserBitmap& bmp)
    {
        if (PREVIEW_RENDER_NAME == bmp.contact && Img_Avatar == (StaticImageIdEnum)bmp.type && !_currentCall.empty() && is_vcs_call())
        {
            auto engine = _get_engine();
            if (_avatarCache)
                bitmapReleaseHandle(_avatarCache);
            _avatarCache = bitmapHandleFromRGBData(bmp.bitmap.w, bmp.bitmap.h, bmp.bitmap.data);
            if (engine)
                engine->Call_SetDisabledVideoFrame(_currentCall, _avatarCache);
        }
    }

    bool VoipManagerImpl::get_device_list(DeviceType device_type, std::vector<device_description>& dev_list)
    {
        dev_list.clear();
        device_list *list = nullptr;
        switch (device_type)
        {
            case AudioRecording: list = &_audioRecordDevices; break;
            case AudioPlayback:  list = &_audioPlayDevices;   break;
            case VideoCapturing: list = &_captureDevices;     break;
        }
        if (!list || !list->devices.size())
            return true;

        dev_list.reserve(list->devices.size());
        for (unsigned i = 0; i < list->devices.size(); i++)
        {
            device_description desc;
            desc.name = list->devices[i].name;
            desc.uid  = list->devices[i].uid;
            desc.type = device_type;
            desc.videoCaptureType = list->devices[i].videoCaptureType;
            dev_list.push_back(desc);
        }
        return true;
    }

    void VoipManagerImpl::_update_device_list(DeviceType dev_type)
    {
        device_list dev_list;
        dev_list.type = dev_type;
        VOIP_ASSERT_RETURN(get_device_list(dev_type, dev_list.devices));

        SIGNAL_NOTIFICATION(kNotificationType_DeviceListChanged, &dev_list);
    }

    void VoipManagerImpl::mute_incoming_call_sounds(bool mute)
    {
        _voip_desc.incomingSoundsMuted = mute;
        auto engine = _get_engine(true);
        if (engine)
        {
            engine->MuteSounds(mute);
        }
    }

    bool VoipManagerImpl::has_created_call()
    {
        return !_calls.empty();
    }

    void VoipManagerImpl::call_report_user_rating(const UserRatingReport &_ratingReport)
    {
        auto engine = _get_engine(true);
        if (!!engine)
        {
            std::ostringstream survey_data;
            if (!_ratingReport.survey_id_.empty())
                survey_data << "q=" << _ratingReport.survey_id_;
            for (const auto& r: _ratingReport.reasons_)
                survey_data << " " << r;
            engine->UserRateLastCall(_ratingReport.aimid_.c_str(), _ratingReport.score_, survey_data.str().c_str());
        }
    }

    void VoipManagerImpl::peer_resolution_changed(const std::string &contact, int width, int height)
    {
        auto engine = _get_engine(true);
        if (!engine)
            return;
        auto currentCall = get_current_call();
        if (!currentCall)
            return;
        engine->Action_UpdateRenderDimension(currentCall->call_id, contact, width, height);
    }

    void VoipManagerImpl::_protocolSendAlloc(const char *data, unsigned size)
    {
        core::core_dispatcher& dispatcher = _dispatcher;
        std::string dataBuf(data, size);

        _dispatcher.execute_core_context({ [dataBuf, &dispatcher, this]()
        {
            if (!_voipProtocol)
            {
                _voipProtocol.reset(new(std::nothrow) VoipProtocol());
            }
            VOIP_ASSERT_RETURN(!!_voipProtocol);
            auto im = dispatcher.find_im_by_id(0);
            VOIP_ASSERT_RETURN(!!im);
            auto packet = im->prepare_voip_msg(dataBuf);
            VOIP_ASSERT_RETURN(!!packet);

            std::weak_ptr<core::base_im> __imPtr = im;
            auto onErrorHandler = [__imPtr, packet, this](int32_t err)
            {
                (void)this;
                auto ptrIm = __imPtr.lock();
                VOIP_ASSERT_RETURN(!!ptrIm);
                ptrIm->handle_net_error(packet->get_url(), err);
            };

            auto resultHandler = _voipProtocol->post_packet(packet, onErrorHandler);
            VOIP_ASSERT_RETURN(!!resultHandler);

            resultHandler->on_result_ = [packet, __imPtr, this](int32_t _error)
            {
                (void)this;
                auto ptrIm = __imPtr.lock();
                VOIP_ASSERT_RETURN(!!ptrIm);
                auto response = packet->getRawData();
                if (!!response && response->available())
                {
                    uint32_t responseSize = response->available();
                    ptrIm->on_voip_proto_msg(true, (const char*)response->read(responseSize), responseSize, std::make_shared<core::auto_callback>([](int32_t) {}));
                }
            };
        } });
    }

    void VoipManagerImpl::_protocolSendMessage(const VoipProtoMsg& data)
    {
        core::core_dispatcher& dispatcher = _dispatcher;
        _dispatcher.execute_core_context({ [data, &dispatcher, this]()
        {
            if (!_voipProtocol)
            {
                _voipProtocol.reset(new(std::nothrow) VoipProtocol());
            }
            VOIP_ASSERT_RETURN(!!_voipProtocol);
            auto im = dispatcher.find_im_by_id(0);
            VOIP_ASSERT_RETURN(!!im);
            auto packet = im->prepare_voip_pac(data);
            VOIP_ASSERT_RETURN(!!packet);

            std::weak_ptr<core::base_im> __imPtr = im;
            auto onErrorHandler = [__imPtr, packet, this](int32_t err)
            {
                (void)this;
                auto ptrIm = __imPtr.lock();
                VOIP_ASSERT_RETURN(!!ptrIm);
                ptrIm->handle_net_error(packet->get_url(), err);
            };

            auto resultHandler = _voipProtocol->post_packet(packet, onErrorHandler);
            VOIP_ASSERT_RETURN(!!resultHandler);

            resultHandler->on_result_ = [packet, __imPtr, data, this](int32_t _error)
            {
                (void)this;
                auto ptrIm = __imPtr.lock();
                VOIP_ASSERT_RETURN(!!ptrIm);
                auto response = packet->getRawData();
                bool success = _error == 0 && !!response && response->available();
                ptrIm->on_voip_proto_ack(data, success);
            };
        } });
    }

    void VoipManagerImpl::OnAllocateCall(const voip::CallId &call_id)
    {
        _dispatcher.execute_core_context({ [this, call_id]
        {
            int wim_msg_id = 0;
            std::string wim_msg;

            CallDescPtr desc;
            VOIP_ASSERT_RETURN(_call_get(call_id, desc));
            ConnectionDescPtr connection = desc->connections.size() ? desc->connections.front() : 0;
            //VOIP_ASSERT_RETURN(connection);
            if (!_wimConverter->PackAllocRequest(connection ? connection->user_id.original_id : _chatId, wim_msg_id, wim_msg))
            {
                VOIP_ASSERT(false);
            }
            if (sendViaIm_)
                _dispatcher.post_voip_alloc(0, wim_msg.c_str(), wim_msg.length());
            else
                _protocolSendAlloc(wim_msg.c_str(), wim_msg.length());
        } });
    }

    void VoipManagerImpl::OnSendSignalingMsg(const voip::CallId &call_id, const voip::PeerId &to, const voip::SignalingMsg &message)
    {
        _dispatcher.execute_core_context({ [this, call_id, to, message]
        {
            int wim_msg_id = 0;
            std::string wim_msg;
            if (!_wimConverter->PackSignalingMsg(message, to, wim_msg_id, wim_msg))
            {
                VOIP_ASSERT(false);
            }
            VoipProtoMsg msg;
            msg.msg = wim_msg_id;
            msg.request.assign(wim_msg.c_str(), wim_msg.length());

            if (sendViaIm_)
                _dispatcher.post_voip_message(0, msg);
            else
                _protocolSendMessage(msg);
        } });
    }

    void VoipManagerImpl::ProcessVoipAck(const voip_manager::VoipProtoMsg& msg, bool success)
    {
    }

    void VoipManagerImpl::ProcessVoipMsg(const std::string& account_id, int voipIncomingMsg, const char *data, unsigned len)
    {
        auto engine = _get_engine();
        VOIP_ASSERT_RETURN(!!engine);

        _updateAccount(account_id);

        if (_wimConverter->IsAllocMessageId(voipIncomingMsg))
        {
            if (_currentCall.empty())
            {
                VOIP_ASSERT_RETURN(0);
            }
            std::string call_guid, ice_params, peer_id;
            if (_wimConverter->UnpackAllocResponse(voipIncomingMsg, data, call_guid, ice_params, peer_id))
            {
                engine->Call_Allocated(_currentCall, call_guid, ice_params);
            } else
            {
                VOIP_ASSERT(0);
            }
        } else
        {
            // this is not an allocate but signaling message
            std::list< std::pair<std::string /*from*/, voip::SignalingMsg /*msg*/> > message_list;
            if (_wimConverter->UnpackSignalingMsg(voipIncomingMsg, data, message_list))
            {
                for(const auto &m : message_list)
                {
                    if constexpr (platform::is_apple())
                    {
                        static bool last_dnd_mode = false;
                        if (voip::SignalingMsg::INVITE == m.second.type)
                        {
                            auto mode = core::tools::system::is_do_not_dirturb_on();
                            if (last_dnd_mode != mode)
                            {
                                //engine->MuteAllIncomingSoundNotifications(mode);
                                last_dnd_mode = mode;
                            }
                        }
                    }
                    engine->SignalingMsgReceived(m.first, m.second);
                }
            } else
            {
                VOIP_ASSERT(0);
            }
        }
    }

    unsigned VoipManagerImpl::get_devices_number(DeviceType device_type)
    {
        unsigned dev_num = 0;
        switch (device_type)
        {
            case AudioRecording: dev_num = _audioRecordDevices.devices.size(); break;
            case AudioPlayback:  dev_num = _audioPlayDevices.devices.size(); break;
            case VideoCapturing: dev_num = _captureDevices.devices.size(); break;
        }
        return dev_num;
    }

    bool VoipManagerImpl::get_device(DeviceType device_type, unsigned index, std::string& device_name, std::string& device_guid)
    {
        device_list *list = nullptr;
        switch (device_type)
        {
            case AudioRecording: list = &_audioRecordDevices; break;
            case AudioPlayback:  list = &_audioPlayDevices; break;
            case VideoCapturing: list = &_captureDevices; break;
        }
        if (list)
        {
            if (index < 0 || index >= list->devices.size())
                return false;
            device_name = list->devices[index].name;
            device_guid = list->devices[index].uid;
            return true;
        }
        return false;
    }

    void VoipManagerImpl::_update_video_window_state(CallDescPtr call)
    {
        const bool enable_video = !!get_current_call();
        SIGNAL_NOTIFICATION(kNotificationType_ShowVideoWindow, &enable_video);
    }

    void VoipManagerImpl::set_device(DeviceType deviceType, const std::string& deviceUid, bool force_reset)
    {
        device_list *list = nullptr;
        switch (deviceType)
        {
            case AudioRecording: list = &_audioRecordDevices; _voip_desc.aRecordingDevice = deviceUid; break;
            case AudioPlayback:  list = &_audioPlayDevices;   _voip_desc.aPlaybackDevice  = deviceUid; break;
            case VideoCapturing: list = &_captureDevices;     break;
        }
        if (!list)
            return;
        if (VideoCapturing == deviceType)
        {
            device_description *desc = nullptr;
            for (unsigned i = 0; i < list->devices.size(); i++)
            {
                if (list->devices[i].uid == deviceUid)
                {
                    desc = &list->devices[i];
                    break;
                }
                if (!desc && VC_DeviceCamera == list->devices[i].videoCaptureType)
                    desc = &list->devices[i]; // set first available camera if not found in list
            }
            if ((!desc || VC_DeviceDesktop != desc->videoCaptureType) && !deviceUid.empty())
                _voip_desc.vCaptureDevice = deviceUid;
            if (!_engine)
                return;
            if (!desc || deviceUid.empty())
            {   // empty deviceUid means no camera
                device_description desc;
                SIGNAL_NOTIFICATION(kNotificationType_MediaLocVideoDeviceChanged, &desc);
                _captureDeviceSet = false;
                media_video_en(_videoTxRequested);
                return;
            }
            switch (desc->videoCaptureType)
            {
            case VC_DeviceVirtualCamera:
                _captureDeviceInfo.type = im::CameraInfo::Type::VirtualCamera; break;
            case VC_DeviceDesktop:
                _captureDeviceInfo.type = im::CameraInfo::Type::ScreenCapture; break;
                break;
            case VC_DeviceCamera:
            default:
                _captureDeviceInfo.type = im::CameraInfo::Type::RealCamera;
            }
            _captureDeviceInfo.deviceId = desc->uid;
            if (_videoTxRequested && _captureDeviceSet)
                _engine->Action_SelectCamera(_captureDeviceInfo);
            bool old_captureDeviceSet = _captureDeviceSet;
            _captureDeviceSet = true;
            if (!old_captureDeviceSet)
                media_video_en(_videoTxRequested);
            SIGNAL_NOTIFICATION(kNotificationType_MediaLocVideoDeviceChanged, desc);
        } else if (_engine)
        {
            im_assert(!deviceUid.empty());
            _engine->Action_SelectAudioDevice(deviceType == AudioPlayback ? im::AudioDevicePlayback : im::AudioDeviceRecording, deviceUid, force_reset);
        }
    }

    void VoipManagerImpl::set_device_mute(DeviceType deviceType, bool mute)
    {
        auto engine = _get_engine();
        VOIP_ASSERT_RETURN(!!engine);
        VOIP_ASSERT_RETURN(AudioPlayback == deviceType);
        _voip_desc.mute_en = mute;
        engine->SetAudioDeviceMute(deviceType == AudioRecording ? im::AudioDeviceRecording : im::AudioDevicePlayback, mute);
        DeviceVol vol;
        vol.type   = deviceType;
        vol.volume = mute ? 0 : _voip_desc.volume;
        SIGNAL_NOTIFICATION(kNotificationType_DeviceVolChanged, &vol);
    }

    bool VoipManagerImpl::get_device_mute(DeviceType deviceType)
    {
        auto engine = _get_engine();
        VOIP_ASSERT_RETURN_VAL(!!engine, false);
        VOIP_ASSERT_RETURN_VAL(AudioPlayback == deviceType, false);
        return _voip_desc.mute_en;
    }

    void VoipManagerImpl::notify_devices_changed(DeviceClass deviceClass)
    {
        if (!_engine)
            return;
#ifndef STRIP_VOIP
        auto engine = _get_engine();
        if (Audio == deviceClass)
            engine->UpdateAudioDevicesList();
        else
            engine->UpdateCameraList();
#endif
    }

    void VoipManagerImpl::update()
    {
        auto engine = _get_engine();
        VOIP_ASSERT_RETURN(!!engine);
        if (!_to_accept_call.empty())
        {
            call_accept(_to_accept_call, _lastAccount, _to_start_video);
            _to_accept_call = {};
        }
    }

    int64_t VoipManagerImpl::get_voip_initialization_memory() const
    {
        return _voip_initialization;
    }

    void VoipManagerImpl::voip_statistics_enable()
    {
        if (auto engine = _get_engine(true))
            engine->VoipStat_Enable(config::get().is_on(config::features::statistics), _lastAccount, _get_log_path());
    }

    void VoipManagerImpl::reset()
    {
        call_stop_smart([=]()
        {
            _voipDestroyed = true;
            SIGNAL_NOTIFICATION(kNotificationType_VoipResetComplete, &_voipDestroyed);
        });
    }

    void VoipManagerImpl::walkThroughCurrentCall(std::function<bool(VoipManagerImpl::ConnectionDescPtr)> func)
    {
        auto currentCall = get_current_call();
        VOIP_ASSERT_ACTION(!!currentCall, return);
        if (currentCall)
        {
            walkThroughCallConnections(currentCall, func);
        }
    }

    void VoipManagerImpl::walkThroughCalls(std::function<bool(VoipManagerImpl::CallDescPtr)> func)
    {
        for (auto it = _calls.begin(); it != _calls.end(); ++it)
        {
            CallDescPtr callDesc = *it;
            VOIP_ASSERT_ACTION(!!callDesc, continue);
            if (func(callDesc))
                break;
        }
    }

    bool VoipManagerImpl::walkThroughCallConnections(VoipManagerImpl::CallDescPtr call, std::function<bool(VoipManagerImpl::ConnectionDescPtr)> func)
    {
        for (auto it = call->connections.begin(); it != call->connections.end(); ++it)
        {
            ConnectionDescPtr connectionDesc = *it;
            VOIP_ASSERT_ACTION(!!connectionDesc, continue);
            if (func(connectionDesc))
                return true;
        }
        return false;
    }

    bool VoipManagerImpl::_connection_get(CallDescPtr& desc, const UserID& user_uid, ConnectionDescPtr& connectionDesc)
    {
        bool res = false;
        walkThroughCallConnections(desc, [&res, &user_uid, &connectionDesc](VoipManagerImpl::ConnectionDescPtr connection) -> bool {
            res = connection->user_id == user_uid;
            if (res)
                connectionDesc = connection;
            return res;
        });
        return res;
    }

    VoipManagerImpl::CallDescPtr VoipManagerImpl::get_current_call()
    {
        CallDescPtr res;
        walkThroughCalls([&res](CallDescPtr call)
        {
            bool found = call->outgoing || (call->call_state >= kCallState_Accepted);
            if (found)
                res = call;
            return found;
        });
        return res;
    }

    std::vector<void*> VoipManagerImpl::_get_call_windows(const VoipKey &call_id)
    {
        std::vector<void*> res;
        std::for_each(_windows.begin(), _windows.end(), [&res, call_id](WindowDesc *window)
        {
            if (call_id == window->call_id)
                res.push_back(window->handle);
        });
        return res;
    }

    VoipManagerImpl::ConnectionDescPtr VoipManagerImpl::_create_connection(const UserID& user_id)
    {
#ifndef STRIP_VOIP
        ConnectionDescPtr res = ConnectionDescPtr(new(std::nothrow) ConnectionDesc());
        res->user_id = user_id;
        return res;
#else
        return ConnectionDescPtr(0);
#endif
    }

    bool VoipManagerImpl::_connection_destroy(CallDescPtr &desc, const UserID& user_uid)
    {
        desc->connections.erase(std::remove_if(desc->connections.begin(), desc->connections.end(),
            [&user_uid](ConnectionDescPtr connections)
            {
                return connections->user_id == user_uid;
            }
            ), desc->connections.end());
        return true;
    }

    void VoipManagerImpl::_updateAccount(const std::string &account)
    {
        if (_lastAccount != account)
        {
            auto engine = _get_engine();
            VOIP_ASSERT_RETURN(engine);

            if (config::hosts::external_url_config::instance().is_external_config_loaded())
                engine->VoipStat_Enable(config::get().is_on(config::features::statistics), account, _get_log_path());

            _lastAccount = account;
        }
    }

    /*void VoipManagerImpl::_cleanOldRTPDumps()
    {
        std::string dumpsDir = _get_rtp_dump_path();
        if (!dumpsDir.empty())
        {
            try
            {
                namespace fs = boost::filesystem;
                std::vector<fs::path> file_to_remove;

                boost::system::error_code error;
                fs::directory_iterator i(fs::path(dumpsDir), error);
                fs::directory_iterator end;

                for (; i != end && !error; i.increment(error))
                {
                    const fs::path file = (*i);
                    if (file.extension() == RTP_DUMP_EXT &&
                        std::difftime(std::time(nullptr), fs::last_write_time(file)) > OLDER_RTP_DUMP_TIME_DIFF.count())
                    {
                        file_to_remove.push_back(file);
                    }
                }
                for (auto file : file_to_remove)
                    fs::remove(file, error);
            }
            catch (const std::exception&)
            {
            }
        }
    }*/

    void VoipManagerImpl::onPreOutgoingCall(const voip::CallId &call_id, const std::vector<std::string> &participants)
    {
        _dispatcher.execute_core_context({ [this, call_id, participants]
        {
            _currentCall = call_id;
            CallDescPtr desc;
            VOIP_ASSERT_RETURN(!!_call_create(call_id, "", desc));
            desc->outgoing = true;
            desc->call_state = kCallState_Initiated;
            for (size_t i = 0; i < participants.size(); i++)
            {
                ConnectionDescPtr connection = _create_connection(participants[i]);
                desc->connections.push_back(connection);
            }
        } });
    }

    void VoipManagerImpl::onStartedOutgoingCall(const voip::CallId &call_id, const std::vector<std::string> &participants)
    {
        _dispatcher.execute_core_context({ [this, call_id, participants]
        {
            CallDescPtr desc;
            VOIP_ASSERT_RETURN(!!_call_get(call_id, desc));
            desc->call_type = _currentCallType;
            desc->chat_id = _chatId;

            ContactEx contact_ex;
            contact_ex.contact.call_id = call_id;
            contact_ex.contact.contact = participants.empty() ? "PINNED_ROOM" : normalizeUid(participants[0]);
            contact_ex.call_type = desc->call_type;
            contact_ex.chat_id = desc->chat_id;
            contact_ex.incoming = !desc->outgoing;
            contact_ex.windows = _get_call_windows(call_id);
            SIGNAL_NOTIFICATION(kNotificationType_CallCreated, &contact_ex);

            _call_request_connections(desc);

            _update_video_window_state(desc);

            media_audio_en(true);
        } });
    }

    void VoipManagerImpl::onIncomingCall(const voip::CallId &call_id, const std::string &from_user_id, bool is_video, const voip::CallStateInternal &call_state, const voip::CallAppData &app_data)
    {
        _dispatcher.execute_core_context({ [this, call_id, from_user_id, is_video, call_state]
        {
            CallDescPtr desc;
            VOIP_ASSERT_RETURN(!!_call_create(call_id, from_user_id, desc));
            desc->outgoing = false;
            desc->call_state = kCallState_Initiated;
            desc->call_type = call_state.call_type;

            ConnectionDescPtr connection;
            VOIP_ASSERT_RETURN(_connection_get(desc, from_user_id, connection));
            connection->remote_cam_en = is_video;

            ContactEx contact_ex;
            contact_ex.contact.call_id = call_id;
            contact_ex.contact.contact = normalizeUid(from_user_id);
            contact_ex.call_type = desc->call_type;
            contact_ex.incoming = !desc->outgoing;
            contact_ex.windows = _get_call_windows(call_id);
            if (!call_state.aux_call_info_json.empty())
            {
                Document json_doc;
                json_doc.Parse(call_state.aux_call_info_json);
                auto info = json_doc.FindMember("info");
                if (info != json_doc.MemberEnd() && info->value.IsObject())
                {
                    if (info->value.HasMember("call_type") && info->value["call_type"].IsString())
                    {
                        std::string call_type = info->value["call_type"].GetString();
                        im_assert(desc->call_type == call_type);
                    }
                    if (info->value.HasMember("url") && info->value["url"].IsString() && voip::kCallType_PINNED_ROOM == desc->call_type)
                    {
                        desc->chat_id = info->value["url"].GetString();
                        contact_ex.chat_id = desc->chat_id;
                    }
                }
            }

            SIGNAL_NOTIFICATION(kNotificationType_CallCreated, &contact_ex);
            _notify_remote_video_state_changed(call_id, connection);
            if (voip::kCallType_VCS == desc->call_type)
                parseAuxCallName(desc, call_state);
            else
                _call_request_connections(desc);
        } });
    }

    void VoipManagerImpl::onRinging(const voip::CallId &call_id)
    {
    }

    void VoipManagerImpl::onOutgoingCallAccepted(const voip::CallId &call_id, const voip::CallAppData &app_data_from_callee)
    {
        _dispatcher.execute_core_context({ [this, call_id]
        {
            CallDescPtr desc;
            VOIP_ASSERT_RETURN(!!_call_get(call_id, desc));
            im_assert(desc->outgoing);
            desc->call_state = kCallState_Accepted;
        } });
    };

    void VoipManagerImpl::onConnecting(const voip::CallId &call_id)
    {
        onCallActive(call_id);
    }

    void VoipManagerImpl::onCallActive(const voip::CallId &call_id)
    {
        _dispatcher.execute_core_context({ [this, call_id]
        {
            CallDescPtr desc;
            VOIP_ASSERT_RETURN(!!_call_get(call_id, desc));
            desc->call_state = kCallState_Connected;
            if (!desc->started)
                desc->started = (unsigned)clock();

            ConnectionDescPtr connection = desc->connections.size() ? desc->connections.front() : nullptr;

            ContactEx contact_ex;
            contact_ex.contact.call_id = call_id;
            contact_ex.contact.contact = connection ? normalizeUid(connection->user_id) : "";
            contact_ex.call_type = desc->call_type;
            contact_ex.incoming = !desc->outgoing;
            contact_ex.windows = _get_call_windows(call_id);
            SIGNAL_NOTIFICATION(kNotificationType_CallConnected, &contact_ex);
        } });
    }

    void VoipManagerImpl::onCallTerminated(const voip::CallId &call_id, bool terminated_locally, voip::TerminateReason reason)
    {
        _dispatcher.execute_core_context({ [this, call_id, reason]
        {
            CallDescPtr desc;
            VOIP_ASSERT_RETURN(!!_call_get(call_id, desc));

            std::string user_id = "conference_end";
            ConnectionDescPtr connection = desc->connections.size() ? desc->connections.front() : 0;
            if (connection)
                user_id = connection->user_id.normalized_id;

            ContactEx contact_ex;
            contact_ex.contact.call_id = call_id;
            contact_ex.contact.contact = user_id;
            contact_ex.call_type = desc->call_type;
            contact_ex.current_call = _currentCall == call_id;
            contact_ex.incoming = !desc->outgoing;
            contact_ex.windows = _get_call_windows(call_id);
            contact_ex.terminate_reason = (int)reason;
            SIGNAL_NOTIFICATION(kNotificationType_CallDestroyed, &contact_ex);

            _call_destroy(desc, reason);
            desc = nullptr;

            im::VoipController::WriteToVoipLog(su::concat("onCallTerminated ", call_id, " currentCall ", _currentCall));
            if (_currentCall != call_id)
                return;
            _update_video_window_state(desc);

            // cleanup state if current active call ended
            _voip_desc.first_notify_send = false;
            _voip_desc.local_aud_en = false;
            _voip_desc.local_cam_en = false;
            _videoTxRequested = false;
            _chatId = {};
            _currentCall = {};
            _currentCallType = voip::kCallType_NORMAL;
            _currentCallVCSName.clear();
            _currentCallVCSUrl.clear();
            _currentCallVCSWebinar = false;
            _zero_participants = false;

            if (_calls.empty() && sendViaIm_)
            {
                // close all windows
                {
                    auto engine = _get_engine();
                    _currentCallHwnd = nullptr;
                    for (auto wnd : _windows)
                    {
                        delete wnd;
                    }
                    _windows.clear();
                }
                sendViaIm_ = false;
                if (callback_)
                    callback_();
                callback_ = {};
                return;
            }
        } });
    }

    void VoipManagerImpl::onCallParticipantsUpdate(const voip::CallId &call_id, const std::vector<voip::CallParticipantInfo> &participants)
    {
        _dispatcher.execute_core_context({ [this, call_id, participants]
        {
            bool list_updated = false;
            int num_participants = 0;
            CallDescPtr desc;
            if (!_call_get(call_id, desc))
                return;

            walkThroughCallConnections(desc, [](ConnectionDescPtr connection) -> bool
            {
                connection->delete_mark = true;
                return false;
            });

            auto prev_remote_sending_desktop = desc->is_remote_sending_desktop();

            for (auto& it : participants)
            {
                const voip::CallParticipantInfo& info = it;
                ConnectionDescPtr connection;
                if (!_connection_get(desc, info.user_id, connection))
                {
                    connection = _create_connection(info.user_id);
                    connection->delete_mark = true;
                    desc->connections.push_back(connection);
                    list_updated = true;
                }
                if (voip::CallParticipantInfo::State::HANGUP == info.state)
                    continue;
                num_participants++;
                connection->delete_mark = false;
                bool connected_state = voip::CallParticipantInfo::State::ACTIVE == info.state;
                bool remote_desktop_enabled = info.media_sender.sending_desktop || info.media_sender.sending_desktop_as_video;
                bool remote_cam_enabled = info.media_sender.sending_video || remote_desktop_enabled;
                bool remote_cam_changed = remote_cam_enabled != connection->remote_cam_en;
                bool connected_state_changed = connected_state != connection->connected_state;
                if (remote_cam_changed || connected_state_changed || info.media_sender.sending_audio != connection->remote_mic_en ||
                    remote_desktop_enabled != connection->remote_sending_desktop)
                {
                    list_updated = true;
                }
                connection->connected_state = connected_state;
                connection->remote_cam_en = remote_cam_enabled;
                connection->remote_mic_en = info.media_sender.sending_audio;
                connection->remote_sending_desktop = remote_desktop_enabled;

                if (remote_cam_changed)
                {
                    _notify_remote_video_state_changed(call_id, connection);
                }
            }

            if (const auto remote_sending_desktop = desc->is_remote_sending_desktop(); remote_sending_desktop != prev_remote_sending_desktop)
            {
                if (auto pw = _find_primary_window(desc->call_id))
                {
                    const auto hideControls = remote_sending_desktop && pw->layout == OneIsBig;
                    _notify_hide_controls(hideControls);
                }
            }

            /* remove leaved participants, but do not remove last participant on terminate p2p call, it's needed for popup */
            if (desc->call_type == voip::kCallType_NORMAL && !num_participants && !desc->connections.empty())
            {
                (*desc->connections.begin())->delete_mark = false;
            }
            desc->connections.erase(std::remove_if(desc->connections.begin(), desc->connections.end(), [&list_updated](ConnectionDescPtr connection)
                {
                    if (connection->delete_mark)
                        list_updated = true;
                    return connection->delete_mark;
                }), desc->connections.end());
            if (list_updated)
                _call_request_connections(desc);
        } });
    }

    void VoipManagerImpl::onCameraListUpdated(const im::CameraInfoList &camera_list)
    {
        im_assert(std::this_thread::get_id() == _dispatcher.get_core_thread_id());

        // NOTE: Don't thoughtlessly put this code as core_thread task because of IMDESKTOP-15478

        _captureDevices.type = VideoCapturing;
        _captureDevices.devices.clear();
        for (auto &it: camera_list)
        {
            device_description dev;
            dev.name = it.name;
            dev.type = VideoCapturing;
            switch (it.type)
            {
            case im::CameraInfo::Type::VirtualCamera:
                dev.videoCaptureType = VC_DeviceVirtualCamera; break;
            case im::CameraInfo::Type::ScreenCapture:
                dev.videoCaptureType = VC_DeviceDesktop; break;
            case im::CameraInfo::Type::RealCamera:
            default:
                dev.videoCaptureType = VC_DeviceCamera;
            }
            dev.uid = it.deviceId;
            _captureDevices.devices.push_back(dev);
        }
        _update_device_list(VideoCapturing);
        if (!_defaultVideoDevStored)
        {
            _defaultVideoDevStored = true;
            if (!_voip_desc.vCaptureDevice.empty())
                set_device(VideoCapturing, _voip_desc.vCaptureDevice, false);
            else if (_captureDevices.devices.size() && VC_DeviceCamera == _captureDevices.devices[0].videoCaptureType)
                set_device(VideoCapturing, _captureDevices.devices[0].uid, false);
        }
    }

    void VoipManagerImpl::onAudioDevicesListUpdated(im::AudioDeviceType type, const im::AudioDeviceInfoList &audio_list)
    {
        im_assert(std::this_thread::get_id() == _dispatcher.get_core_thread_id());

        // NOTE: Don't thoughtlessly put this code as core_thread task because of IMDESKTOP-15478

        DeviceType dev_type = (type == im::AudioDeviceRecording) ? AudioRecording : AudioPlayback;
        device_list *list = (type == im::AudioDeviceRecording) ? &_audioRecordDevices : &_audioPlayDevices;
        list->type = dev_type;
        list->devices.clear();
        for (auto &it: audio_list)
        {
            device_description dev;
            dev.name = it.name;
            dev.uid =  it.uid;
            dev.type = dev_type;
            dev.videoCaptureType = VC_DeviceCamera;
            dev.is_default = it.is_default;
            list->devices.push_back(dev);
        }
        int index = 0;
        for (auto &dev: list->devices)
        {
            if (dev.is_default)
            {
                if (index)
                    std::swap(list->devices[0], list->devices[index]);
                break;
            }
            index++;
        }
        _update_device_list(dev_type);
        if (AudioPlayback == dev_type && !_defaultAudioPlayDevStored)
        {
            _defaultAudioPlayDevStored = true;
            if (!_voip_desc.aPlaybackDevice.empty())
                set_device(AudioPlayback, _voip_desc.aPlaybackDevice, false);
            else if (!list->devices.empty())
                set_device(AudioPlayback, list->devices[0].uid, false);
        } else if (AudioRecording == dev_type && !_defaultAudioCapDevStored)
        {
            _defaultAudioCapDevStored = true;
            if (!_voip_desc.aRecordingDevice.empty())
                set_device(AudioRecording, _voip_desc.aRecordingDevice, false);
            else if (!list->devices.empty())
                set_device(AudioRecording, list->devices[0].uid, false);
        }
    }

    void VoipManagerImpl::parseAuxCallName(CallDescPtr desc, const voip::CallStateInternal &call_state)
    {
        Document json_doc;
        json_doc.Parse(call_state.aux_call_info_json);

        auto iter_info = json_doc.FindMember("info");
        if (iter_info != json_doc.MemberEnd() && iter_info->value.IsObject())
        {
            auto send_update = false;
            if (std::string name; iter_info->value.HasMember("name") && core::tools::unserialize_value(iter_info->value, "name", name))
            {
                send_update |= _currentCallVCSName != name;
                _currentCallVCSName = std::move(name);
            }
            if (std::string url; iter_info->value.HasMember("url") && core::tools::unserialize_value(iter_info->value, "url", url))
            {
                send_update |= _currentCallVCSUrl != url;
                _currentCallVCSUrl = std::move(url);
            }
            if (std::string type; iter_info->value.HasMember("type") && core::tools::unserialize_value(iter_info->value, "type", type))
            {
                const auto is_webinar = type == "webinar";
                send_update |= _currentCallVCSWebinar != is_webinar;
                _currentCallVCSWebinar = is_webinar;
            }

            if (send_update)
                _call_request_connections(desc);
        }
    }

    void VoipManagerImpl::onStateUpdated(const voip::CallStateInternal &call_state)
    {
        _dispatcher.execute_core_context({ [this, call_state]
        {
            auto currentCall = get_current_call();
            if (currentCall)
                im::VoipController::WriteToVoipLog(su::concat("onStateUpdated ", call_state.id, " currentCall ", currentCall->call_id));
            if (!currentCall || currentCall->call_id != call_state.id)
                return;
            bool local_cam_en = call_state.local_media_status.sender_state.sending_video || call_state.local_media_status.sender_state.sending_desktop;
            bool changed = _voip_desc.local_aud_en != call_state.local_media_status.sender_state.sending_audio;
            changed = changed || _voip_desc.local_cam_en != local_cam_en;
            changed = changed || _voip_desc.local_cam_allowed != call_state.local_media_status.allowed_send_video;
            changed = changed || _voip_desc.local_aud_allowed != call_state.local_media_status.allowed_send_audio;
            changed = changed || _voip_desc.local_desktop_allowed != call_state.local_media_status.allowed_send_desktop;
            _voip_desc.local_aud_en = call_state.local_media_status.sender_state.sending_audio;
            _voip_desc.local_cam_en = local_cam_en;
            _voip_desc.local_aud_allowed = call_state.local_media_status.allowed_send_audio;
            _voip_desc.local_cam_allowed = call_state.local_media_status.allowed_send_video;
            _voip_desc.local_desktop_allowed = call_state.local_media_status.allowed_send_desktop;
            if (changed || !_voip_desc.first_notify_send)
                SIGNAL_NOTIFICATION(kNotificationType_MediaLocParamsChanged, &_voip_desc);
            _voip_desc.first_notify_send = true;

	        {
	            std::string string_to_log = "onStateUpdated ";
	            if (!changed)
	            {
	                string_to_log += "No changed state";
	            } else
	            {
	                string_to_log += su::concat(" local_aud_en ", _voip_desc.local_aud_en ? " 1 " : " 0 ");
	                string_to_log += su::concat(" local_cam_en ", _voip_desc.local_cam_en ? " 1 " : " 0 ");
	                string_to_log += su::concat(" local_aud_allowed ", _voip_desc.local_aud_allowed ? " 1 " : " 0 ");
	                string_to_log += su::concat(" local_cam_allowed ", _voip_desc.local_cam_allowed ? " 1 " : " 0 ");
	                string_to_log += su::concat(" local_desktop_allowed ", _voip_desc.local_desktop_allowed ? " 1 " : " 0 ");
	            }
	            im::VoipController::WriteToVoipLog(string_to_log);
	        }

            im_assert(is_vcs_call() ? call_state.call_type == voip::kCallType_VCS : is_vcs_v2_call() ? call_state.call_type == voip::kCallType_LINK : is_pinned_room_call() ? call_state.call_type == voip::kCallType_PINNED_ROOM : call_state.call_type.empty());
            if (is_vcs_call() && !call_state.aux_call_info_json.empty())
                parseAuxCallName(currentCall, call_state);
            // Signal hangup terminate reasons
            ConfPeerInfoV peers;
            for (auto &peer: call_state.peers())
            {
                if (voip::CallStateInternal::PeerStatus::RoomState::HANGUP != peer.room_state)
                    continue;
                if (peer.hangup_reason == voip::TR_NOT_FOUND || peer.hangup_reason == voip::TR_BLOCKED_BY_CALLER_IS_STRANGER ||
                    peer.hangup_reason == voip::TR_BLOCKED_BY_CALLEE_PRIVACY || peer.hangup_reason == voip::TR_CALLER_MUST_BE_AUTHORIZED_BY_CAPCHA ||
                    peer.hangup_reason == voip::TR_INTERNAL_ERROR)
                {
                    ConfPeerInfo info = { peer.peerId, (int)peer.hangup_reason };
                    peers.push_back(info);
                }
            }
            SIGNAL_NOTIFICATION(kNotificationType_ConfPeerDisconnected, &peers);
            _zero_participants = call_state.peers().empty();
        } });
    }

    void VoipManagerImpl::onVoiceDetectChanged(bool voice)
    {
        SIGNAL_NOTIFICATION(kNotificationType_VoiceDetect, &voice);
    }

    void VoipManagerImpl::onRemoteVoices(const std::vector<im::VADInfo> &_participants)
    {
        SIGNAL_NOTIFICATION(kNotificationType_VoiceVadInfo, &_participants);
    }

    VoipManager *createVoipManager(core::core_dispatcher& dispatcher)
    {
        return new(std::nothrow) VoipManagerImpl(dispatcher);
    }
}
