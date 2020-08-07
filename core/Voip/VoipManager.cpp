#include "stdafx.h"
#include <memory>
#include <mutex>
#include <time.h>
#include <string>
#ifdef _WIN32
    #include <windows.h>
    #include <assert.h>
    #include <unordered_map>
#endif
#if __APPLE__
    #include <CoreFoundation/CoreFoundation.h>
    #include <ImageIO/ImageIO.h>
#endif

#include "VoipManager.h"
#include "VoipController.h"
#include "VoipSerialization.h"
#include "render/BuiltinVideoWindowRenderer.h"
#include "window_settings.h"
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
#include "../tools/json_helper.h"
#include <iostream>
#include "../../core/configuration/app_config.h"

using namespace rapidjson;
using StaticImageId = voip::BuiltinVideoWindowRenderer::StaticImageId;
// In seconds: 1 week.
//constexpr std::chrono::seconds OLDER_RTP_DUMP_TIME_DIFF = std::chrono::hours(7 * 24);
//static const std::string RTP_DUMP_EXT = ".pcapng";

#define VOIP_ASSERT(exp) assert(exp)

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
            { { "voip_call_volume_change", "volume" }, ST_INT },
            { { "voip_call_media_switch", "video" }, ST_BOOL },
            { { "voip_sounds_mute", "mute" }, ST_BOOL },
            { { "voip_call_decline", "mode" }, ST_STRING },
            { { "voip_call_decline", "contact" }, ST_STRING },
            { { "voip_call_accept", "mode" }, ST_STRING },
            { { "voip_call_accept", "contact" }, ST_STRING },
            { { "device_change", "dev_type" }, ST_STRING },
            { { "device_change", "uid" }, ST_STRING },
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
            voip::CallParticipantInfo::State peer_state = voip::CallParticipantInfo::State::UNKNOWN;
            bool    remote_cam_en = false;
            bool    remote_mic_en = false;
            bool    delete_mark = false;
        };
        typedef std::shared_ptr<ConnectionDesc> ConnectionDescPtr;
        typedef std::list<ConnectionDescPtr> ConnectionDescList;

        /**
         * Call statistic data.
         *
         */
        struct CallStatistic
        {
            bool incoming = false; // Incoming or outgoing.
            voip::TerminateReason endReason = voip::TR_INTERNAL_ERROR;
            bool isVideo = false; // Has video or not.
            uint32_t membersNumber = 0; // Number of members without myself.

            std::string getEndReason() const
            {
                std::string endNames[17] = {"HANGED_UP", "REJECTED", "BUSY", "HANDLED_BY_ANOTHER_INSTANCE",
                    "UNAUTHORIZED", "ALLOCATE_FAILED", "ANSWER_TIMEOUT", "CONNECT_TIMEOUT", "INVALID_DOMAIN",
                    "CALLER_IS_SYSPICIOUS", "FORBIDDEN_BY_CALLEE_PRIVACY", "CALLER_MUST_BE_AUTHORIZED_BY_CAPCHA",
                    "BAD_URI", "NOT_AVAILABLE_NOW", "PARTICIPANTS_LIMIT_EXCEEDED", "DURATION_LIMIT_EXCEEDED",
                    "INTERNAL_ERROR" };
                return endNames[endReason];
            }

            void setEndReason(voip::TerminateReason reason)
            {
                endReason = reason;
            }

            void updateVideoFlag(bool videoState)
            {
                isVideo = isVideo || videoState;
            }

            // Duration will be rounded in seconds:
            // To 10 sec if t < 3 min,
            // To 1 min  if t < 30 min
            // To 5 min  if t < 60 min
            // To 10 min if t >= 60 min
            static uint32_t getDuration(uint32_t duration)
            {
                uint32_t res = duration;
                uint32_t borderTime[] = { 0, 3 * 60, 30 * 60, 60 * 60,  std::numeric_limits<uint32_t>::max()};
                uint32_t roundValue[] = { 10, 1 * 60, 5 * 60, 10 * 60};
                for (uint32_t i = 0; i < sizeof(roundValue) / sizeof(roundValue[0]); i++)
                {
                    if (res >= borderTime[i] && res < borderTime[i + 1])
                    {
                        return uint32_t(res / (float)roundValue[i]) * roundValue[i];
                    }
                }
                return res;
            }

            // speaker or headset
            static std::string getAudioDevice(const std::string& audioDevice)
            {
                // TODO: improve this. Use voip to get audio type or add more patterns.
                std::string headsetPattern = "Headset";
                auto it = std::search(
                    audioDevice.begin(), audioDevice.end(),
                    headsetPattern.begin(), headsetPattern.end(),
                    [](char ch1, char ch2) { return std::toupper(ch1) == std::toupper(ch2); }
                );
                return (it != audioDevice.end()) ? "Headset" : "Speaker";
            }
        };

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

            CallStatistic statistic;

            // Is this call current
            bool current()
            {
                return started > 0;
            }
        };

        typedef std::shared_ptr<CallDesc> CallDescPtr;
        typedef std::list<CallDescPtr> CallDescList;

        struct WindowDesc
        {
            std::shared_ptr<voip::BuiltinVideoWindowRenderer> _renderer;
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

        std::recursive_mutex     _windowsMx;
        std::vector<WindowDesc*> _windows;

        int64_t _voip_initialization = 0;

        std::recursive_mutex _callsMx;
        CallDescList         _calls;

        std::function<void()>          callback_;
        std::shared_ptr<VoipProtocol> _voipProtocol;

        std::recursive_mutex _voipDescMx;
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

        typedef struct { void* bitmaps[AVATAR_THEME_TYPE_MAX][Img_Max]; } bitmaps_cache;
        std::map<std::string, bitmaps_cache> _bitmapCache;

        core::core_dispatcher& _dispatcher;

        std::shared_ptr<core::async_executer> _async_tasks;

        std::string _maskModelPath;

        bool sendViaIm_ = false;
        bool _defaultVideoDevStored = false, _defaultAudioPlayDevStored = false, _defaultAudioCapDevStored = false;
        bool _voipDestroyed = false;
        bool _masksEngineInited = false;
        bool _needToRunMask = false; // Does user try to use mask early?
        bool _videoTxRequested = false;
        bool _captureDeviceSet = false;
        std::string _lastAccount;

        // Save devices/UIDS for statistic
        std::recursive_mutex _audioPlayDeviceMx;
        std::map<std::string, std::string> _audioPlayDeviceMap; // Mpa uid to names

    private:
        std::shared_ptr<im::VoipController> _get_engine(bool skipCreation = false);
        //std::string _get_rtp_dump_path() const;
        std::string _get_log_path() const;
        void _setup_log_folder() const;

        bool _init_sounds_sc(std::shared_ptr<im::VoipController> engine);

        bool is_vcs_call() const { return voip::kCallType_VCS == _currentCallType; }
        bool is_webinar() const { return _currentCallVCSWebinar; }
        bool is_pinned_room_call() const { return !_chatId.empty(); }
        void _update_theme(CallDescPtr callDesc);
        void _update_window_theme(const WindowDesc& window_desc, bool active_video_conf, bool outgoing, bool incoming, int peersCount);
        void _update_video_window_state(CallDescPtr call);

        bool _call_create (const VoipKey& call_id, const UserID& user_id, CallDescPtr& desc);
        bool _call_destroy(const VoipKey& call_id, voip::TerminateReason sessionEvent);
        bool _call_get    (const VoipKey& call_id, CallDescPtr& desc);

        ConnectionDescPtr _create_connection(const UserID& user_id);
        bool _connection_destroy(CallDescPtr& desc, const UserID& user_uid);
        bool _connection_get (CallDescPtr& desc, const UserID& user_uid, ConnectionDescPtr& connectionDesc);

        void _call_request_connections(CallDescPtr call);
        bool _call_have_established_connection();
        bool _call_have_outgoing_connection();

        inline const VoipSettings _getVoipSettings() const;
        WindowDesc *_find_window(void *handle);

        void _load_avatars       (const UserID& user_uid, bool request_if_not_exists = true);
        void _set_image_to_render(AvatarThemeType theme, StaticImageId type, const std::string &contact, voip::BuiltinVideoWindowRenderer *render, void *hbmp);
        void _update_device_list (DeviceType dev_type);
        void _notify_remote_video_state_changed(const std::string &call_id, ConnectionDescPtr connection);

        void _protocolSendAlloc  (const char* data, unsigned size); //executes in VOIP SIGNALING THREAD context
        void _protocolSendMessage(const VoipProtoMsg& data); //executes in VOIP SIGNALING THREAD context

        void setupAvatarForeground(); // Setup fade for video.

        void initMaskEngine(const std::string& modelDir); // Init mask engine.
        void unloadMaskEngine();
        void loadMask(const std::string& maskPath);          // Load mask.

        void _cleanupCalls(); // Remove empty calls from list.

        void walkThroughCurrentCall(std::function<bool(ConnectionDescPtr)> func);    // Enum all connection of current call.
        void walkThroughCalls(std::function<bool(CallDescPtr)> func);                // Enum all call.
        bool walkThroughCallConnections(CallDescPtr call, std::function<bool(ConnectionDescPtr)> func);                // Enum all connections of call.

        CallDescPtr get_current_call();  // @return current active call.

        // @return list of window related with this call.
        std::vector<void*> _get_call_windows(const VoipKey &call_id);

        void _sendCallStatistic(CallDescPtr call);

        void _updateAudioPlayDevice(const std::vector<voip_manager::device_description>& devices);
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
        bool call_have_call               (const Contact& contact) override;
        void call_request_calls           () override;
        void call_stop_smart              (const std::function<void()>&) override;

        void mute_incoming_call_sounds    (bool mute) override;
        void minimal_bandwidth_switch() override;
        bool has_created_call() override;

        void call_report_user_rating(const UserRatingReport& _ratingReport) override;

        //=========================== IWindowManager API ===========================
        void window_add           (voip_manager::WindowParams& windowParams) override;
        void window_remove        (void* hwnd) override;
        void window_set_bitmap    (const WindowBitmap& bmp) override;
        void window_set_bitmap    (const UserBitmap& bmp) override;
        void window_set_conference_layout(void* hwnd, voip_manager::VideoLayout layout) override;
        void window_switch_aspect (const std::string& contact, void* hwnd) override;
        void window_set_offsets   (void* hwnd, unsigned l, unsigned t, unsigned r, unsigned b) override;
        void window_set_primary   (void* hwnd, const std::string& contact) override;
        void window_set_hover     (void* hwnd, bool hover) override;
        void window_set_large     (void* hwnd, bool large) override;

        //=========================== IMediaManager API ===========================
        void media_video_en       (bool enable) override;
        void media_audio_en       (bool enable) override;
        bool local_video_enabled  () override;
        bool local_audio_enabled  () override;
        bool remote_video_enabled (const std::string& call_id, const std::string& contact) override;
        bool remote_audio_enabled () override;
        bool remote_audio_enabled (const std::string& call_id, const std::string& contact) override;

        //=========================== IMaskManager API ===========================
        void load_mask(const std::string& path) override;
        unsigned int version() override;
        void set_model_path(const std::string& path) override;
        void init_mask_engine() override;
        void unload_mask_engine() override;

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
        void     set_device        (DeviceType device_type, const std::string& device_guid) override;
        void     set_device_mute   (DeviceType deviceType, bool mute) override;
        bool     get_device_mute   (DeviceType deviceType) override;
        void     set_device_volume (DeviceType deviceType, float volume) override;
        float    get_device_volume (DeviceType deviceType) override;
        void     notify_devices_changed() override;
        void     update() override;

        //=========================== Memory Consumption =============================
        int64_t get_voip_initialization_memory() const override;

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

        void onCameraListUpdated          (const camera::CameraEnumerator::CameraInfoList &camera_list) override;
        void onAudioDevicesListUpdated    (im::AudioDeviceType type, const im::AudioDeviceInfoList &audio_list) override;

        void onStateUpdated(const voip::CallStateInternal &call_state) override;

        //=========================== voip::BuiltinVideoWindowRenderer::Notifications ===
        void OnMouseTap(const voip::PeerId &name, voip::BuiltinVideoWindowRenderer::MouseTap mouseTap, voip::BuiltinVideoWindowRenderer::ViewArea viewArea, float x, float y);
        //void OnFrameSizeChanged(float aspect);

        //=========================== camera::Masker::Observer ==========================
        void onMaskEngineStatusChanged(bool engine_loaded, const std::string &error_message) override;
        void onMaskStatusChanged(camera::Masker::Status status, const std::string &path) override;

    public:
        VoipManagerImpl (core::core_dispatcher& dispatcher);
        ~VoipManagerImpl();
    };

    VoipManagerImpl::VoipManagerImpl(core::core_dispatcher& dispatcher)
        : _dispatcher(dispatcher)
        , _async_tasks(std::make_shared<core::async_executer>("voipMgrImpl"))
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
            //assert(openResult);
        }
    }

    VoipManagerImpl::~VoipManagerImpl()
    {
        _engine.reset();
        std::lock_guard<std::recursive_mutex> __lock(_callsMx);
        VOIP_ASSERT(_calls.empty());
        for (auto &it: _bitmapCache)
        {
            bitmaps_cache &bmps = it.second;
            for (int theme = 0; theme < AVATAR_THEME_TYPE_MAX; theme++)
                for (int type = 0; type < Img_Max; type++)
                {
                    if (!bmps.bitmaps[theme][type])
                        continue;
                    bitmapReleaseHandle(bmps.bitmaps[theme][type]);
                }
        }
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

    /**
     * Theme list:
     * MiniWindow:
     * theme4 - avatar + user name text / w/o hover
     * theme5 - avatar only / hover
     * Main Window:
     * theme1 - one is big   call/outgoing/conference
     * theme2 - all the same call/outgoing/conference
     * theme6 - all the same call/outgoing/conference for 5 peers.
     * theme7 - all the same call/outgoing/conference for large window.
     * Incomming
     * theme3 - the same for all states.
     *
     */
    void VoipManagerImpl::_update_theme(CallDescPtr callDesc)
    {
        auto engine = _get_engine(true);
        VOIP_ASSERT_RETURN(!!engine);

        std::lock_guard<std::recursive_mutex> __lock(_callsMx);
        const bool active_video_conf = is_pinned_room_call() || (callDesc->call_state == kCallState_Connected && callDesc->connections.size() > 1);
        const bool outgoing          = callDesc->outgoing;

        std::lock_guard<std::recursive_mutex> lock(_windowsMx);
        std::for_each(_windows.begin(), _windows.end(), [active_video_conf, this, callDesc, engine, outgoing](WindowDesc *window_desc)
            {
                if (window_desc->call_id == callDesc->call_id)
                {
                    _update_window_theme(*window_desc, active_video_conf, outgoing && !window_desc->incoming, window_desc->incoming, callDesc->connections.size());
                }
            });
    }

    void VoipManagerImpl::_update_window_theme(const WindowDesc& window_desc, bool active_video_conf, bool outgoing, bool incoming, int peersCount)
    {
        auto engine = _get_engine(true);
        VOIP_ASSERT_RETURN(!!engine);

        MainVideoLayout mainLayout;
        mainLayout.hwnd   = window_desc.handle;
        mainLayout.layout = window_desc.layout;

        auto allTheSameCurrenTheme = (peersCount < 4 ? (window_desc.large ? voip::WindowTheme_Seven : voip::WindowTheme_Two) : voip::WindowTheme_Six);

        if (window_desc.miniWindow)
        {
            mainLayout.layout = AllTheSame;
            window_desc._renderer->SetWindowTheme(window_desc.hover ? voip::WindowTheme_Five : voip::WindowTheme_Four, false);
            mainLayout.type = active_video_conf ? MVL_CONFERENCE : MVL_SIGNLE_CALL;
            SIGNAL_NOTIFICATION(kNotificationType_MainVideoLayoutChanged, &mainLayout);
        } else if (incoming)
        {
            window_desc._renderer->SetWindowTheme(voip::WindowTheme_Three, true);
            mainLayout.type = MVL_INCOMING;
            SIGNAL_NOTIFICATION(kNotificationType_MainVideoLayoutChanged, &mainLayout);
        } else if (active_video_conf)
        {
            window_desc._renderer->SetWindowTheme((is_pinned_room_call() && 0 == peersCount) ? voip::WindowTheme_One /* lock theme if room empty */ :
                (window_desc.layout == AllTheSame ? allTheSameCurrenTheme : voip::WindowTheme_One), true);
            mainLayout.type = MVL_CONFERENCE;
            SIGNAL_NOTIFICATION(kNotificationType_MainVideoLayoutChanged, &mainLayout);
        } else if (outgoing)
        {
            window_desc._renderer->SetWindowTheme((window_desc.layout == AllTheSame ? allTheSameCurrenTheme : voip::WindowTheme_One), true);
            mainLayout.type = MVL_OUTGOING;
            SIGNAL_NOTIFICATION(kNotificationType_MainVideoLayoutChanged, &mainLayout);
        } else
        {
            window_desc._renderer->SetWindowTheme((window_desc.layout == AllTheSame ? allTheSameCurrenTheme : voip::WindowTheme_One), true);
            mainLayout.type = MVL_SIGNLE_CALL;
            SIGNAL_NOTIFICATION(kNotificationType_MainVideoLayoutChanged, &mainLayout);
        }
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

#ifndef STRIP_VOIP
            auto beforeVoip = core::utils::get_current_process_real_mem_usage();
            _engine = std::make_shared<im::VoipController>(*this);
            VOIP_ASSERT_RETURN_VAL(_engine, nullptr);

            omicronlib::json_string config_json = omicronlib::_o(omicron::keys::voip_config, omicronlib::json_string(""));

            _engine->Initialize(this, config_json);
            _wimConverter = std::make_unique<voip::WIMSignallingConverter>(0, 1, 0, 1);

            auto afterVoip = core::utils::get_current_process_real_mem_usage();
            _voip_initialization = afterVoip - beforeVoip;
#endif

            _init_sounds_sc(_engine);
        }
        return _engine;
    }

    bool VoipManagerImpl::_call_create(const VoipKey& call_id, const UserID& user_id, CallDescPtr& desc)
    {
#ifndef STRIP_VOIP
        //VOIP_ASSERT_RETURN_VAL(!user_id.normalized_id.empty(), false);
        std::lock_guard<std::recursive_mutex> __lock(_callsMx);
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

    bool VoipManagerImpl::_call_destroy(const VoipKey& key, voip::TerminateReason endReason)
    {
        std::lock_guard<std::recursive_mutex> __lock(_callsMx);
        walkThroughCalls([key, this, endReason](CallDescPtr call)
            {
                if (call->call_id == key)
                {
                    call->connections.clear();
                    call->statistic.setEndReason(endReason);
                    _sendCallStatistic(call);
                    return true;
                }
                return false;
            });

        _cleanupCalls();

        // Reset flag if last call was removed.
        if (_calls.empty())
        {
            std::lock_guard<std::recursive_mutex> __vdlock(_voipDescMx);
            _needToRunMask = false;
            loadMask(""); // reset mask
        }
        return true;
    }

    void VoipManagerImpl::media_video_en(bool enable)
    {
        _videoTxRequested = enable;
        // We enable video device is selected correct video capture type.
        auto engine = _get_engine();
        auto currentCall = get_current_call();
        if (currentCall)
        {
            engine->Action_EnableCamera(currentCall->call_id, enable && _captureDeviceSet);
            currentCall->statistic.updateVideoFlag(enable);
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
        std::lock_guard<std::recursive_mutex> __vdlock(_voipDescMx);
        return _voip_desc.local_cam_en;
    }

    bool VoipManagerImpl::local_audio_enabled()
    {
        std::lock_guard<std::recursive_mutex> __vdlock(_voipDescMx);
        return _voip_desc.local_aud_en;
    }

    bool VoipManagerImpl::remote_video_enabled(const std::string& call_id, const std::string& contact)
    {
        ConnectionDescPtr connectionDesc;
        std::lock_guard<std::recursive_mutex> __lock(_callsMx);
        CallDescPtr call_desc;
        _call_get(call_id, call_desc);
        VOIP_ASSERT_RETURN_VAL(_connection_get(call_desc, contact, connectionDesc), false);

        return connectionDesc->remote_cam_en;
    }

    bool VoipManagerImpl::remote_audio_enabled()
    {
        bool res = false;
        walkThroughCurrentCall([&res](VoipManagerImpl::ConnectionDescPtr connection) -> bool {
                res = connection->remote_mic_en;
                return res;
            });
        return res;
    }

    bool VoipManagerImpl::remote_audio_enabled(const std::string& call_id, const std::string& contact)
    {
        ConnectionDescPtr desc;
        std::lock_guard<std::recursive_mutex> __lock(_callsMx);
        CallDescPtr call_desc;
        _call_get(call_id, call_desc);
        VOIP_ASSERT_RETURN_VAL(_connection_get(call_desc, contact, desc), false);

        return desc->remote_mic_en;
    }

    bool VoipManagerImpl::_call_get(const VoipKey& call_id, CallDescPtr& desc)
    {
        std::lock_guard<std::recursive_mutex> __lock(_callsMx);
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
            set_device(VideoCapturing, _voip_desc.vCaptureDevice); // set last selected camera (sharing can be enabled during last call)
            _videoTxRequested = params.video;
            _currentCallType = params.is_vcs ? voip::kCallType_VCS : params.is_pinned_room ? voip::kCallType_PINNED_ROOM : voip::kCallType_NORMAL;
            std::string app_data;
            if (params.is_vcs)
            {
                VOIP_ASSERT_RETURN(contacts.size() == 1);
                app_data = su::concat("{ \"call_type\" : \"VCS\", \"url\" : \"", contacts[0], "\" }");
            } else if (params.is_pinned_room)
            {
                _chatId = contacts[0].c_str();
                app_data = su::concat("{ \"call_type\" : \"pinned_room\", \"url\" : \"", _chatId, "\" }");
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
        std::lock_guard<std::recursive_mutex> __lock(_callsMx);
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

        //_load_avatars(contact.contact);

        set_device(VideoCapturing, _voip_desc.vCaptureDevice); // set last selected camera (sharing can be enabled during last call)
        _videoTxRequested = video;
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
        std::lock_guard<std::recursive_mutex> __lock(_callsMx);
        return _calls.size();
    }

    void VoipManagerImpl::_call_request_connections(CallDescPtr call)
    {
        std::lock_guard<std::recursive_mutex> __lock(_callsMx);
        ContactsList contacts;
        if (call)
        {
            contacts.contacts.reserve(call->connections.size());

            walkThroughCallConnections(call, [&call, &contacts](ConnectionDescPtr connection) -> bool
            {
                contacts.contacts.push_back(Contact(call->call_id, connection->user_id));
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

    void VoipManagerImpl::call_request_calls()
    {

        bool found = false;
        walkThroughCalls([this, &found](VoipManagerImpl::CallDescPtr call)
        {
            found = true;
            _call_request_connections(call);
            return false;
        });
        if (!found)
        {
            _call_request_connections(nullptr);
        }
    }

    bool VoipManagerImpl::call_have_call(const Contact& contact)
    {
        std::lock_guard<std::recursive_mutex> __lock(_callsMx);
        for (auto it = _calls.begin(); it != _calls.end(); ++it)
        {
            auto call = *it;
            for (auto it_conn = (*it)->connections.begin(); it_conn != (*it)->connections.end(); ++it_conn)
            {
                auto conn = *it_conn;
                VOIP_ASSERT_ACTION(!!conn, continue);
                if (call->call_id == contact.call_id && conn->user_id == contact.contact)
                    return true;
            }
        }
        return false;
    }

    bool VoipManagerImpl::_call_have_established_connection()
    {
        std::lock_guard<std::recursive_mutex> __lock(_callsMx);
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
        std::lock_guard<std::recursive_mutex> lock(_windowsMx);
        for (size_t ix = 0; ix < _windows.size(); ++ix)
        {
            WindowDesc *window_description = _windows[ix];
            if (window_description->handle == handle)
                return window_description;
        }
        return nullptr;
    }

    void VoipManagerImpl::window_add(voip_manager::WindowParams& windowParams)
    {
        VOIP_ASSERT_RETURN(!!windowParams.hwnd);
        if (_find_window(windowParams.hwnd))
            return;
        VOIP_ASSERT_RETURN(!windowParams.call_id.empty() || !_currentCall.empty());
        auto engine = _get_engine();
        VOIP_ASSERT_RETURN(!!engine);

        std::string settings_json;
        if (windowParams.isPrimary)
        {
            settings_json = priWindowSettings;
        } else if (windowParams.isIncoming)
        {
            settings_json = incomingWindowSettings;
        } else
        {
            settings_json = secWindowSettings;
        }

        Document json_document;
        json_document.Parse(settings_json);
        float scale = windowParams.scale;
        Value &ws = json_document["WindowSettings"];
        Value &ad = ws["AvatarDesc"];
        Value &lp = ws["LayoutParams"];
        if (windowParams.isPrimary)
        {
            ad["WindowTheme_One"]["width"]      = (int)(voip_manager::kAvatarDefaultSize * scale);
            ad["WindowTheme_One"]["height"]     = (int)(voip_manager::kAvatarDefaultSize * scale);
            ad["WindowTheme_One"]["textWidth"]  = (int)(voip_manager::kNickTextW * scale);
            ad["WindowTheme_One"]["textHeight"] = (int)(voip_manager::kNickTextH * scale);
            ad["WindowTheme_Two"]["textWidth"]  = (int)(voip_manager::kNickTextAllSameW * scale);
            ad["WindowTheme_Two"]["textHeight"] = (int)(voip_manager::kNickTextAllSameH * scale);
            ad["WindowTheme_Six"]["textWidth"]  = (int)(voip_manager::kNickTextAllSameW5peers * scale);
            ad["WindowTheme_Six"]["textHeight"] = (int)(voip_manager::kNickTextAllSameH * scale);
            ad["WindowTheme_Seven"]["textWidth"]  = (int)(voip_manager::kNickTextAllSameWLarge * scale);
            ad["WindowTheme_Seven"]["textHeight"] = (int)(voip_manager::kNickTextAllSameH * scale);
            ad["WindowTheme_One"]["offsetLeft"]   = (int)(12 * scale);
            ad["WindowTheme_One"]["offsetRight"]  = (int)(12 * scale);
            ad["WindowTheme_One"]["offsetTop"]    = (int)(12 * scale);
            ad["WindowTheme_One"]["offsetBottom"] = (int)(12 * scale);

            lp["WindowTheme_One"]["traySize"]   = (int)(20 * scale);
            lp["WindowTheme_Two"]["traySize"]   = (int)(20 * scale);
            lp["WindowTheme_Six"]["traySize"]   = (int)(20 * scale);
            lp["WindowTheme_Seven"]["traySize"] = (int)(20 * scale);
            lp["WindowTheme_One"]["channelsGapWidth"]   = (int)(20 * scale);
            lp["WindowTheme_Two"]["channelsGapWidth"]   = (int)(20 * scale);
            lp["WindowTheme_Six"]["channelsGapWidth"]   = (int)(20 * scale);
            lp["WindowTheme_Seven"]["channelsGapWidth"] = (int)(20 * scale);
            lp["WindowTheme_One"]["channelsGapHeight"]   = (int)(20 * scale);
            lp["WindowTheme_Two"]["channelsGapHeight"]   = (int)(20 * scale);
            lp["WindowTheme_Six"]["channelsGapHeight"]   = (int)(20 * scale);
            lp["WindowTheme_Seven"]["channelsGapHeight"] = (int)(20 * scale);

            ws["header_height_pix"] = (int)(32 * scale);
            ws["detachedStreamsRoundedRadius"] = (int)(4 * scale);
            ws["detachedStreamMaxArea"] = (int)((60 * scale) * (60 * scale) * 10000);
            ws["detachedStreamMinArea"] = (int)((55 * scale) * (55 * scale));

            ws["disable_mouse_events_handler"] = is_vcs_call();
            if (is_pinned_room_call())
                ws["previewEnabledAlways"] = true;
        } else if (!windowParams.isIncoming)
        {
            ad["WindowTheme_Four"]["textWidth"]  = (int)(voip_manager::kNickTextMiniWW * scale);
            ad["WindowTheme_Four"]["textHeight"] = (int)(voip_manager::kNickTextMiniWH * scale);
            ad["WindowTheme_Five"]["textWidth"]  = (int)(voip_manager::kNickTextMiniWW * scale);
            ad["WindowTheme_Five"]["textHeight"] = (int)(voip_manager::kNickTextMiniWH * scale);
#ifdef __linux__
            ws["disable_mouse_events_handler"] = false;
#endif
        }

        StringBuffer buffer;
        Writer<StringBuffer> writer(buffer);
        json_document.Accept(writer);
        settings_json = buffer.GetString();

        voip::ChannelStatusContexts ctx;
        auto deleter = [](void* ptr)
        {
            if (ptr)
                ::bitmapReleaseHandle(ptr);
        };
        using SafeHBMP = std::unique_ptr<void, std::function<void(void*)>>;

        //SafeHBMP cameraStatusHBMP = SafeHBMP(bitmapHandleFromRGBData(windowParams.cameraStatus.bitmap.w, windowParams.cameraStatus.bitmap.h, windowParams.cameraStatus.bitmap.data), deleter);
        SafeHBMP callingHBMP      = SafeHBMP(bitmapHandleFromRGBData(windowParams.calling.bitmap.w,      windowParams.calling.bitmap.h,      windowParams.calling.bitmap.data), deleter);
        SafeHBMP connectingHBMP   = SafeHBMP(bitmapHandleFromRGBData(windowParams.connecting.bitmap.w,   windowParams.connecting.bitmap.h,   windowParams.connecting.bitmap.data), deleter);
        SafeHBMP closedByBusyHBMP = SafeHBMP(bitmapHandleFromRGBData(windowParams.closedByBusy.bitmap.w, windowParams.closedByBusy.bitmap.h, windowParams.closedByBusy.bitmap.data), deleter);

        voip::WindowThemeType themes[] = { voip::WindowTheme_One, voip::WindowTheme_Two, voip::WindowTheme_Three,
            voip::WindowTheme_Four, voip::WindowTheme_Five, voip::WindowTheme_Six, voip::WindowTheme_Seven };
        for (auto theme: themes)
        {
            //ctx[theme].videoPaused  = cameraStatusHBMP.get();
            ctx[theme].connecting   = connectingHBMP.get();
            ctx[theme].calling      = connectingHBMP.get();
            ctx[theme].closedByBusy = closedByBusyHBMP.get();
            ctx[theme].ringing      = callingHBMP.get();
            ctx[theme].allocating   = connectingHBMP.get();
        }

        class NotificationsEx: public voip::BuiltinVideoWindowRenderer::Notifications
        {
            VoipManagerImpl *impl;
        public:
            NotificationsEx(VoipManagerImpl *impl_): impl(impl_) {}
            virtual ~NotificationsEx() {};
            void OnMouseTap(const voip::PeerId &name, voip::BuiltinVideoWindowRenderer::MouseTap mouseTap, voip::BuiltinVideoWindowRenderer::ViewArea viewArea, float x, float y) override
            {
                impl->OnMouseTap(name, mouseTap, viewArea, x, y);
            }
            void OnFrameSizeChanged(float aspect) override
            {
                //impl->OnFrameSizeChanged(aspect);
            }
        };

        std::shared_ptr<voip::BuiltinVideoWindowRenderer::Notifications> notifications(new NotificationsEx(this));
        WindowDesc *window_description = new WindowDesc;
        window_description->_renderer = voip::BuiltinVideoWindowRenderer::Create(windowParams.hwnd, settings_json, ctx, notifications);
        window_description->handle = windowParams.hwnd;
        window_description->aspectRatio = 0.0f;
        window_description->scaleCoeffitient = windowParams.scale >= 0.01 ? windowParams.scale : 1.0f;
        window_description->layout_type = "LayoutType_One";
        window_description->layout = OneIsBig;
        window_description->incoming = windowParams.isIncoming;
        window_description->miniWindow = !windowParams.isPrimary && !windowParams.isIncoming;
        window_description->call_id = !windowParams.call_id.empty() ? windowParams.call_id : _currentCall;
        window_description->_renderer->SetDisableLocalPreview(is_webinar() || (is_vcs_call() && !windowParams.isIncoming));

        {
            std::lock_guard<std::recursive_mutex> lock(_windowsMx);
            if (windowParams.isPrimary)
                _currentCallHwnd = window_description->handle;
            _windows.push_back(window_description);
        }

        for (auto &it: _bitmapCache)
        {
            bitmaps_cache &bmps = it.second;
            for (int theme = 0; theme < AVATAR_THEME_TYPE_MAX; theme++)
                for (int type = 0; type < Img_Max; type++)
                {
                    if (!bmps.bitmaps[theme][type])
                        continue;
                    _set_image_to_render((AvatarThemeType)theme, (StaticImageId)type, it.first, window_description->_renderer.get(), bmps.bitmaps[theme][type]);
                }
        }

        // Send contects for new window.
        CallDescPtr desc;
        VOIP_ASSERT_RETURN(!!_call_get(window_description->call_id, desc));
        _call_request_connections(desc);

        setupAvatarForeground();

        engine->VideoWindowAdd(desc->call_id, window_description->_renderer);

        const intptr_t winId = (intptr_t)windowParams.hwnd;
        SIGNAL_NOTIFICATION(kNotificationType_VoipWindowAddComplete, &winId);

        _update_theme(desc);
    }

    void VoipManagerImpl::window_remove(void *hwnd)
    {
        auto engine = _get_engine(true);
        if (!engine)
        {
            std::lock_guard<std::recursive_mutex> lock(_windowsMx);
            _windows.clear();
            return;
        }
        // We cannot use async WindowRemove, because it desyncs addWindow and remove.
        // As result we can hide video window on start call.
        WindowDesc *desc = _find_window(hwnd);
        if (desc)
        {
            std::lock_guard<std::recursive_mutex> lock(_windowsMx);
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
        }
        if (desc)
        {
            if (desc->_renderer)
                engine->VideoWindowRemove(desc->call_id, desc->_renderer);
            delete desc;
        }
        const intptr_t winId = (intptr_t)hwnd;
        SIGNAL_NOTIFICATION(kNotificationType_VoipWindowRemoveComplete, &winId)
    }

    void VoipManagerImpl::window_set_bitmap(const WindowBitmap& bmp)
    {
        VOIP_ASSERT_RETURN(bmp.hwnd);
        VOIP_ASSERT_RETURN(bmp.bitmap.w);
        VOIP_ASSERT_RETURN(bmp.bitmap.h);
        VOIP_ASSERT_RETURN(bmp.bitmap.size);
        VOIP_ASSERT_RETURN(bmp.bitmap.data);

        WindowDesc *desc = _find_window(bmp.hwnd);
        VOIP_ASSERT_RETURN(desc);
        void *hbmp = bitmapHandleFromRGBData(bmp.bitmap.w, bmp.bitmap.h, bmp.bitmap.data);
        VOIP_ASSERT_RETURN(hbmp);
        desc->_renderer->SetStaticImage(voip::WindowTheme_Two, StaticImageId::Img_Background, "", hbmp);
        desc->_renderer->SetStaticImage(voip::WindowTheme_One, StaticImageId::Img_Background, "", hbmp);
        desc->_renderer->SetStaticImage(voip::WindowTheme_Three, StaticImageId::Img_Background, "", hbmp);
        desc->_renderer->SetStaticImage(voip::WindowTheme_Four, StaticImageId::Img_Background, "", hbmp);
        desc->_renderer->SetStaticImage(voip::WindowTheme_Five, StaticImageId::Img_Background, "", hbmp);
        desc->_renderer->SetStaticImage(voip::WindowTheme_Six, StaticImageId::Img_Background, "", hbmp);

        bitmapReleaseHandle(hbmp);
    }

    void VoipManagerImpl::_set_image_to_render(AvatarThemeType theme, StaticImageId type, const std::string &contact, voip::BuiltinVideoWindowRenderer *render, void *hbmp)
    {
        if (PREVIEW_RENDER_NAME == contact && StaticImageId::Img_Avatar == type && !_currentCall.empty() && is_vcs_call())
        {
            auto engine = _get_engine();
            if (engine)
                engine->_voip->Call_SetDisabledVideoFrame(_currentCall, hbmp);
        }
        switch (theme)
        {
        case MAIN_ALL_THE_SAME:
            render->SetStaticImage(voip::WindowTheme_Two, type, contact, hbmp);
            break;
        case MAIN_ALL_THE_SAME_5_PEERS:
            render->SetStaticImage(voip::WindowTheme_Six, type, contact, hbmp);
            break;
        case MAIN_ALL_THE_SAME_LARGE:
            render->SetStaticImage(voip::WindowTheme_Seven, type, contact, hbmp);
            break;
        case MAIN_ONE_BIG:
            render->SetStaticImage(voip::WindowTheme_One, type, contact, hbmp);
            break;
        case MINI_WINDOW:
            render->SetStaticImage(voip::WindowTheme_Four, type, contact, hbmp);
            if (StaticImageId::Img_Username != type)
                render->SetStaticImage(voip::WindowTheme_Five, type, contact, hbmp);
            break;
        case INCOME_WINDOW:
            render->SetStaticImage(voip::WindowTheme_Three, type, contact, hbmp);
            break;
        default:
            assert(false);
        }
    }

    void VoipManagerImpl::window_set_bitmap(const UserBitmap& bmp)
    {
        VOIP_ASSERT_RETURN(bmp.bitmap.w);
        VOIP_ASSERT_RETURN(bmp.bitmap.h);
        VOIP_ASSERT_RETURN(bmp.bitmap.size);
        VOIP_ASSERT_RETURN(bmp.bitmap.data);
        VOIP_ASSERT_RETURN(!bmp.contact.empty());

        bool create = _bitmapCache.find(bmp.contact) == _bitmapCache.end();
        bitmaps_cache &bmps = _bitmapCache[bmp.contact];
        if (create)
            memset(&bmps, 0, sizeof(bmps));
        void *hbmp = bitmapHandleFromRGBData(bmp.bitmap.w, bmp.bitmap.h, bmp.bitmap.data);
        if (bmps.bitmaps[bmp.theme][bmp.type])
            bitmapReleaseHandle(bmps.bitmaps[bmp.theme][bmp.type]);
        bmps.bitmaps[bmp.theme][bmp.type] = hbmp;
        for (auto &it: _windows)
        {
            WindowDesc *desc = it;
            _set_image_to_render(bmp.theme, (StaticImageId)bmp.type, bmp.contact, desc->_renderer.get(), hbmp);
        }
    }

    void VoipManagerImpl::window_set_conference_layout(void* hwnd, voip_manager::VideoLayout layout)
    {
        auto window_description = _find_window(hwnd);
        VOIP_ASSERT_RETURN(window_description);

        window_description->layout = layout;

        CallDescPtr desc;
        _call_get(window_description->call_id, desc);
        VOIP_ASSERT_RETURN(desc);

        _update_theme(desc);
    }

    void VoipManagerImpl::window_switch_aspect(const std::string& contact, void *hwnd)
    {
        VOIP_ASSERT_RETURN(!contact.empty());
        VoipManagerImpl::WindowDesc *winDesc = _find_window(hwnd);
        if (!winDesc)
            return;
        winDesc->_renderer->SwitchAspectMode(contact.c_str(), true);
    }

    void VoipManagerImpl::window_set_offsets(void *hwnd, unsigned l, unsigned t, unsigned r, unsigned b)
    {
        VoipManagerImpl::WindowDesc *winDesc = _find_window(hwnd);
        if (!winDesc)
            return;
        bool bOverlaped = true;
        winDesc->_renderer->SetControlsStatus(winDesc->hover, l, t, r, b, true, bOverlaped);
    }

    void VoipManagerImpl::window_set_primary(void *hwnd, const std::string& contact)
    {
        VoipManagerImpl::WindowDesc *winDesc = _find_window(hwnd);
        if (!winDesc)
            return;
        winDesc->_renderer->SetPrimary(contact.c_str(), true);
    }

    void VoipManagerImpl::window_set_hover(void *hwnd, bool hover)
    {
        VoipManagerImpl::WindowDesc *winDesc = _find_window(hwnd);
        if (!winDesc)
            return;
        winDesc->hover = hover;
        // TODO: pass this for all windows.
        if (winDesc->miniWindow)
        {
            _update_window_theme(*winDesc, false, false, false, 2);
        }
    }

    void VoipManagerImpl::window_set_large(void* hwnd, bool large)
    {
        VoipManagerImpl::WindowDesc *winDesc = _find_window(hwnd);
        if (!winDesc)
            return;
        bool oldLarger = winDesc->large;
        winDesc->large = large;
        if (oldLarger != large)
        {
            CallDescPtr desc;
            _call_get(winDesc->call_id, desc);
            if (desc)
            {
                _update_window_theme(*winDesc, false, desc->outgoing, false, desc->connections.size());
            }
        }
    }

    bool VoipManagerImpl::get_device_list(DeviceType device_type, std::vector<device_description>& dev_list)
    {
        dev_list.clear();
        std::lock_guard<std::recursive_mutex> __vdlock(_voipDescMx);
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

        if (dev_type == AudioPlayback)
        {
            _updateAudioPlayDevice(dev_list.devices);
        }
        SIGNAL_NOTIFICATION(kNotificationType_DeviceListChanged, &dev_list);
    }

    void VoipManagerImpl::mute_incoming_call_sounds(bool mute)
    {
        {
            std::lock_guard<std::recursive_mutex> __vdlock(_voipDescMx);
            _voip_desc.incomingSoundsMuted = mute;
        }
        auto engine = _get_engine(true);
        if (engine)
        {
            engine->MuteSounds(mute);
        }
    }

    void VoipManagerImpl::minimal_bandwidth_switch()
    {
    }

    bool VoipManagerImpl::has_created_call()
    {
        std::lock_guard<std::recursive_mutex> __lock(_callsMx);
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

    void VoipManagerImpl::_protocolSendAlloc(const char *data, unsigned size)
    {
        core::core_dispatcher& dispatcher = _dispatcher;
        std::string dataBuf(data, size);

        _dispatcher.execute_core_context([dataBuf, &dispatcher, this] ()
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
            auto onErrorHandler = [__imPtr, packet, this] (int32_t err)
            {
                (void)this;
                auto ptrIm = __imPtr.lock();
                VOIP_ASSERT_RETURN(!!ptrIm);
                ptrIm->handle_net_error(err);
            };

            auto resultHandler = _voipProtocol->post_packet(packet, onErrorHandler);
            VOIP_ASSERT_RETURN(!!resultHandler);

            resultHandler->on_result_ = [packet, __imPtr, this] (int32_t _error)
            {
                (void)this;
                auto ptrIm = __imPtr.lock();
                VOIP_ASSERT_RETURN(!!ptrIm);
                auto response = packet->getRawData();
                if (!!response && response->available())
                {
                    uint32_t responseSize = response->available();
                    ptrIm->on_voip_proto_msg(true, (const char*)response->read(responseSize), responseSize, std::make_shared<core::auto_callback>([](int32_t){}));
                }
            };
        });
    }

    void VoipManagerImpl::_protocolSendMessage(const VoipProtoMsg& data)
    {
        core::core_dispatcher& dispatcher = _dispatcher;
        _dispatcher.execute_core_context([data, &dispatcher, this] ()
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
            auto onErrorHandler = [__imPtr, packet, this] (int32_t err)
            {
                (void)this;
                auto ptrIm = __imPtr.lock();
                VOIP_ASSERT_RETURN(!!ptrIm);
                ptrIm->handle_net_error(err);
            };

            auto resultHandler = _voipProtocol->post_packet(packet, onErrorHandler);
            VOIP_ASSERT_RETURN(!!resultHandler);

            resultHandler->on_result_ = [packet, __imPtr, data, this] (int32_t _error)
            {
                (void)this;
                auto ptrIm = __imPtr.lock();
                VOIP_ASSERT_RETURN(!!ptrIm);
                auto response = packet->getRawData();
                bool success  = _error == 0 && !!response && response->available();
                ptrIm->on_voip_proto_ack(data, success);
            };
        });
    }

    void VoipManagerImpl::OnAllocateCall(const voip::CallId &call_id)
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
    }

    void VoipManagerImpl::OnSendSignalingMsg(const voip::CallId &call_id, const voip::PeerId &to, const voip::SignalingMsg &message)
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
                engine->_voip->Call_Allocated(_currentCall, call_guid, ice_params);
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
                    engine->_voip->SignalingMsgReceived(m.first, m.second);
                }
            } else
            {
                VOIP_ASSERT(0);
            }
        }
    }

    void VoipManagerImpl::_load_avatars(const UserID& user_uid, bool request_if_not_exists/* = true*/) {
        _dispatcher.execute_core_context([user_uid, this] ()
        {
            auto im = _dispatcher.find_im_by_id(0);
            assert(!!im);

            im->get_contact_avatar(0, user_uid.normalized_id, kAvatarRequestSize, false);
        });
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

    void VoipManagerImpl::set_device(DeviceType deviceType, const std::string& deviceUid)
    {
        auto engine = _get_engine();
        VOIP_ASSERT_RETURN(!!engine);
        {
            std::lock_guard<std::recursive_mutex> __vdlock(_voipDescMx);
            if (AudioPlayback == deviceType)
                _voip_desc.aPlaybackDevice = deviceUid;
        }
        device_list *list = nullptr;
        switch (deviceType)
        {
            case AudioRecording: list = &_audioRecordDevices; break;
            case AudioPlayback:  list = &_audioPlayDevices;   break;
            case VideoCapturing: list = &_captureDevices;     break;
        }
        if (!engine || !list)
            return;
        if (VideoCapturing == deviceType)
        {
            camera::CameraEnumerator::CameraInfo cam_info;
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
            if (!desc || deviceUid.empty())
            {   // empty deviceUid means no camera
                device_description desc;
                cam_info.type = camera::CameraEnumerator::RealCamera;
                engine->Action_SelectCamera(cam_info);
                SIGNAL_NOTIFICATION(kNotificationType_MediaLocVideoDeviceChanged, &desc);
                _captureDeviceSet = false;
                media_video_en(_videoTxRequested);
                return;
            }
            switch (desc->videoCaptureType)
            {
            case VC_DeviceVirtualCamera:
                cam_info.type = camera::CameraEnumerator::Type::VirtualCamera; break;
            case VC_DeviceDesktop:
                cam_info.type = camera::CameraEnumerator::Type::ScreenCapture; break;
            case VC_DeviceCamera:
            default:
                cam_info.type = camera::CameraEnumerator::RealCamera;
            }
            cam_info.deviceId = desc->uid;
            engine->Action_SelectCamera(cam_info);
            SIGNAL_NOTIFICATION(kNotificationType_MediaLocVideoDeviceChanged, desc);
            _captureDeviceSet = true;
            media_video_en(_videoTxRequested);
        } else
        {
            assert(!deviceUid.empty());
            engine->Action_SelectAudioDevice(deviceType == AudioPlayback ? im::AudioDevicePlayback : im::AudioDeviceRecording, deviceUid);
        }
        return;
    }

    void VoipManagerImpl::set_device_mute(DeviceType deviceType, bool mute)
    {
        auto engine = _get_engine();
        VOIP_ASSERT_RETURN(!!engine);
        VOIP_ASSERT_RETURN(AudioPlayback == deviceType);
        std::lock_guard<std::recursive_mutex> __vdlock(_voipDescMx);
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
        std::lock_guard<std::recursive_mutex> __vdlock(_voipDescMx);
        return _voip_desc.mute_en;
    }

    void VoipManagerImpl::set_device_volume(DeviceType deviceType, float volume)
    {
        auto engine = _get_engine();
        VOIP_ASSERT_RETURN(!!engine);
        _voip_desc.volume = volume;
        // We do not touch system volume anymore. In recent systems user should change it manually per application.
        //engine->SetAudioDeviceVolume(deviceType == AudioRecording ? im::AudioDeviceRecording : im::AudioDevicePlayback, volume);
    }

    float VoipManagerImpl::get_device_volume(DeviceType deviceType)
    {
        auto engine = _get_engine();
        VOIP_ASSERT_RETURN_VAL(!!engine, 0.0f);
        return _voip_desc.volume;
    }

    void VoipManagerImpl::notify_devices_changed()
    {
        if (!_engine)
            return;
#ifndef STRIP_VOIP
        auto engine = _get_engine();
        engine->UpdateCameraList();
        engine->UpdateAudioDevicesList();
#endif
    }

    void VoipManagerImpl::update()
    {
        auto engine = _get_engine();
        VOIP_ASSERT_RETURN(!!engine);
    }

    int64_t VoipManagerImpl::get_voip_initialization_memory() const
    {
        return _voip_initialization;
    }

    void VoipManagerImpl::reset()
    {
        call_stop_smart([=]()
        {
            _voipDestroyed = true;
            SIGNAL_NOTIFICATION(kNotificationType_VoipResetComplete, &_voipDestroyed);
        });
    }

    void VoipManagerImpl::load_mask(const std::string& path)
    {
        loadMask(path);
    }

    unsigned int VoipManagerImpl::version()
    {
        unsigned int res = 0;
#ifndef STRIP_VOIP
        res = im::VoipController::GetMaskEngineVersion();
#endif
        return res;
    }

    void VoipManagerImpl::set_model_path(const std::string& path)
    {
        // In case, when we already have model, but we need to reload old and load new.
        if (_masksEngineInited)
            initMaskEngine(std::string());

        _masksEngineInited = false;
        _maskModelPath = path;
        if (_needToRunMask)
        {
            initMaskEngine(_maskModelPath);
        }
    }

    void VoipManagerImpl::setupAvatarForeground()
    {
        unsigned char foreground[] = { 0, 0, 0, 255 / 2 };
        voip_manager::UserBitmap bmp;
        bmp.bitmap.data = (void*)foreground;
        bmp.bitmap.size = 4;
        bmp.bitmap.w = 1;
        bmp.bitmap.h = 1;
        bmp.contact = PREVIEW_RENDER_NAME;
        bmp.type  = voip::BuiltinVideoWindowRenderer::Img_UserForeground;
        bmp.theme = voip_manager::INCOME_WINDOW;

        window_set_bitmap(bmp);
    }

    void VoipManagerImpl::initMaskEngine(const std::string& modelDir)
    {
        auto engine = _get_engine();
        VOIP_ASSERT_RETURN(!!engine);

        if (!_masksEngineInited || modelDir.empty())
        {
            _async_tasks->run_async_function([engine, modelDir] {
                engine->InitMaskEngine(modelDir.c_str());
                return 0;
            }
            )->on_result_ = [](int32_t error) {};
        }
    }

    void VoipManagerImpl::unloadMaskEngine()
    {
        if (auto engine = _get_engine())
        {
            _async_tasks->run_async_function([engine = std::move(engine)] {
                engine->UnloadMaskEngine();
                return 0;
                }
            )->on_result_ = [wr_this = weak_from_this()](int32_t error)
            {
                auto ptr_this = wr_this.lock();
                if (!ptr_this)
                    return;

                ptr_this->_masksEngineInited = false;
            };
        }
    }

    void VoipManagerImpl::loadMask(const std::string& maskPath)
    {
        auto engine = _get_engine();
        VOIP_ASSERT_RETURN(!!engine);

        NamedResult loadMaskRes;
        loadMaskRes.name = maskPath;
        loadMaskRes.result = false;

        if (_masksEngineInited)
        {
            engine->LoadMask(maskPath.c_str());
        }
        SIGNAL_NOTIFICATION(kNotificationType_LoadMask, &loadMaskRes);
    }

    void VoipManagerImpl::init_mask_engine()
    {
        _needToRunMask = true;
        if (!_maskModelPath.empty())
        {
            initMaskEngine(_maskModelPath);
        }
    }

    void VoipManagerImpl::unload_mask_engine()
    {
        unloadMaskEngine();
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
        std::lock_guard<std::recursive_mutex> __lock(_callsMx);
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
        std::lock_guard<std::recursive_mutex> __lock(_callsMx);
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

    void VoipManagerImpl::_cleanupCalls()
    {
        std::lock_guard<std::recursive_mutex> __lock(_callsMx);
        _calls.erase(
            std::remove_if(_calls.begin(), _calls.end(), [](CallDescPtr call)
                {
                    return call->connections.empty();
                }
            ), _calls.end());
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
        std::lock_guard<std::recursive_mutex> lock(_windowsMx);
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
        std::lock_guard<std::recursive_mutex> __lock(_callsMx);
        desc->connections.erase(std::remove_if(desc->connections.begin(), desc->connections.end(),
            [&user_uid](ConnectionDescPtr connections)
            {
                return connections->user_id == user_uid;
            }
            ), desc->connections.end());
        return true;
    }

    void VoipManagerImpl::_sendCallStatistic(CallDescPtr call)
    {
        core::stats::event_props_type props;
        const auto& stat = call->statistic;

        props.emplace_back("Init", stat.incoming ? "Callee" : "Caller");
        props.emplace_back("EndReason", stat.getEndReason());
        if (call->started > 0)
            props.emplace_back("Duration", std::to_string(stat.getDuration((clock() - call->started) / 1000)));
        else
            props.emplace_back("Duration", std::to_string(0));

        std::string audioDevideUID;
        {
            std::lock_guard<std::recursive_mutex> __vdlock(_voipDescMx);
            audioDevideUID = _voip_desc.aPlaybackDevice;
        }
        std::string audioDevideName;

        if (!audioDevideUID.empty())
        {
            std::lock_guard<std::recursive_mutex> __lock(_audioPlayDeviceMx);
            if (_audioPlayDeviceMap.find(audioDevideUID) != _audioPlayDeviceMap.end())
            {
                audioDevideName = _audioPlayDeviceMap[audioDevideUID];
            }
        }

        auto audioDevideStatistic = stat.getAudioDevice(audioDevideName);
        if (!audioDevideStatistic.empty())
            props.emplace_back("AudioPath", audioDevideStatistic);

        props.emplace_back("CallType", stat.isVideo ? "Video" : "Audio");
        props.emplace_back("Members", std::to_string(stat.membersNumber));

        _dispatcher.insert_event(core::stats::stats_event_names::voip_calls_users_tech, std::move(props));
    }

    void VoipManagerImpl::_updateAudioPlayDevice(const std::vector<voip_manager::device_description>& devices)
    {
        std::lock_guard<std::recursive_mutex> __lock(_audioPlayDeviceMx);
        _audioPlayDeviceMap.clear();
        for (auto device_desc: devices)
        {
            _audioPlayDeviceMap[device_desc.uid] = device_desc.name;
        }
    }

    void VoipManagerImpl::_updateAccount(const std::string &account)
    {
        if (_lastAccount != account)
        {
            auto engine = _get_engine();
            VOIP_ASSERT_RETURN(engine);

            if (config::get().is_on(config::features::statistics))
                engine->VoipStat_Enable(true, account, _get_log_path());

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
    }

    void VoipManagerImpl::onStartedOutgoingCall(const voip::CallId &call_id, const std::vector<std::string> &participants)
    {
        CallDescPtr desc;
        VOIP_ASSERT_RETURN(!!_call_get(call_id, desc));
        desc->call_type = _currentCallType;
        desc->chat_id   = _chatId;

        ContactEx contact_ex;
        contact_ex.contact.call_id = call_id;
        contact_ex.contact.contact = participants.empty() ? "PINNED_ROOM" : normalizeUid(participants[0]);
        contact_ex.call_type       = desc->call_type;
        contact_ex.chat_id         = desc->chat_id;
        contact_ex.incoming        = !desc->outgoing;
        contact_ex.windows         = _get_call_windows(call_id);
        SIGNAL_NOTIFICATION(kNotificationType_CallCreated, &contact_ex);

        _call_request_connections(desc);
        _update_theme(desc);

        desc->statistic.incoming = false; // Fill statistics.
        desc->statistic.updateVideoFlag(true);

        _update_theme(desc);
        _update_video_window_state(desc);

        media_audio_en(true);
    }

    void VoipManagerImpl::onIncomingCall(const voip::CallId &call_id, const std::string &from_user_id, bool is_video, const voip::CallStateInternal &call_state, const voip::CallAppData &app_data)
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
        contact_ex.call_type       = desc->call_type;
        contact_ex.incoming        = !desc->outgoing;
        contact_ex.windows         = _get_call_windows(call_id);
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
                    assert(desc->call_type == call_type);
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

        // Fill statistics.
        desc->statistic.incoming = true;
        desc->statistic.updateVideoFlag(is_video);

        _update_theme(desc);
        _update_video_window_state(desc);

        if (desc->connections.size() == 1)
        {
            media_audio_en(true);
        }
    }

    void VoipManagerImpl::onRinging(const voip::CallId &call_id)
    {
    }

    void VoipManagerImpl::onOutgoingCallAccepted(const voip::CallId &call_id, const voip::CallAppData &app_data_from_callee)
    {
        CallDescPtr desc;
        VOIP_ASSERT_RETURN(!!_call_get(call_id, desc));
        assert(desc->outgoing);
        desc->call_state = kCallState_Accepted;

        ConnectionDescPtr connection = desc->connections.size() ? desc->connections.front() : 0;
        VOIP_ASSERT_RETURN(connection);

        ContactEx contact_ex;
        contact_ex.contact.call_id = call_id;
        contact_ex.contact.contact = normalizeUid(connection->user_id);
        contact_ex.call_type       = desc->call_type;
        contact_ex.incoming        = !desc->outgoing;
        contact_ex.windows         = _get_call_windows(call_id);

        SIGNAL_NOTIFICATION(kNotificationType_CallOutAccepted, &contact_ex);
        _update_video_window_state(desc);

        _load_avatars(connection->user_id);
        _update_theme(desc);
    };

    void VoipManagerImpl::onConnecting(const voip::CallId &call_id)
    {
        onCallActive(call_id);
    }

    void VoipManagerImpl::onCallActive(const voip::CallId &call_id)
    {
        CallDescPtr desc;
        VOIP_ASSERT_RETURN(!!_call_get(call_id, desc));
        desc->call_state = kCallState_Connected;
        if (!desc->started)
            desc->started = (unsigned)clock();

        ConnectionDescPtr connection = desc->connections.size() ? desc->connections.front() : 0;
        VOIP_ASSERT_RETURN(connection);

        ContactEx contact_ex;
        contact_ex.contact.call_id = call_id;
        contact_ex.contact.contact = normalizeUid(connection->user_id);
        contact_ex.call_type       = desc->call_type;
        contact_ex.incoming        = !desc->outgoing;
        contact_ex.windows         = _get_call_windows(call_id);
        SIGNAL_NOTIFICATION(kNotificationType_CallConnected, &contact_ex);
    }

    void VoipManagerImpl::onCallTerminated(const voip::CallId &call_id, bool terminated_locally, voip::TerminateReason reason)
    {
        std::lock_guard<std::recursive_mutex> __lock(_callsMx);
        CallDescPtr desc;
        VOIP_ASSERT_RETURN(!!_call_get(call_id, desc));

        std::string user_id = "conference_end";
        ConnectionDescPtr connection = desc->connections.size() ? desc->connections.front() : 0;
        if (connection)
            user_id = connection->user_id.normalized_id;

        ContactEx contact_ex;
        contact_ex.contact.call_id = call_id;
        contact_ex.contact.contact = user_id;
        contact_ex.call_type       = desc->call_type;
        contact_ex.current_call    = _currentCall == call_id;
        contact_ex.incoming        = !desc->outgoing;
        contact_ex.windows         = _get_call_windows(call_id);
        contact_ex.terminate_reason = (int)reason;
        SIGNAL_NOTIFICATION(kNotificationType_CallDestroyed, &contact_ex);

        _call_destroy(call_id, reason);
        desc = nullptr;
        _update_video_window_state(desc);
        call_request_calls();

        if (_currentCall != call_id)
            return;
        // cleanup state if current active call ended
        _voip_desc.first_notify_send = false;
        _voip_desc.local_aud_en = false;
        _voip_desc.local_cam_en = false;
        _videoTxRequested = false;
        _chatId = "";
        _currentCall = "";
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
                std::vector<WindowDesc*> windows;
                {
                    std::lock_guard<std::recursive_mutex> lock(_windowsMx);
                    windows = std::exchange(_windows, {});
                    _currentCallHwnd = nullptr;
                }
                for (auto wnd: windows)
                {
                    delete wnd;
                }
            }
            sendViaIm_ = false;
            if (callback_)
                callback_();
            callback_ = {};
            return;
        }
        if (!_to_accept_call.empty())
        {
            call_accept(_to_accept_call, _lastAccount, _to_start_video);
            _to_accept_call = "";
        }
    }

    void VoipManagerImpl::onCallParticipantsUpdate(const voip::CallId &call_id, const std::vector<voip::CallParticipantInfo> &participants)
    {
        bool list_updated = false;
        CallDescPtr desc;
        if (!_call_get(call_id, desc))
            return;
        {
            std::lock_guard<std::recursive_mutex> __lock(_callsMx);
            walkThroughCallConnections(desc, [](ConnectionDescPtr connection) -> bool
            {
                connection->delete_mark = true;
                return false;
            });
        }
        desc->statistic.membersNumber = participants.size();
        for (auto &it: participants)
        {
            const voip::CallParticipantInfo &info = it;
            ConnectionDescPtr connection;
            if (!_connection_get(desc, info.user_id, connection))
            {
                std::lock_guard<std::recursive_mutex> __lock(_callsMx);
                connection = _create_connection(info.user_id);
                desc->connections.push_back(connection);
                list_updated = true;
            }
            if (voip::CallParticipantInfo::State::HANGUP != info.state)
                connection->delete_mark = false;
            connection->peer_state = info.state;
            bool remote_cam_en = info.media_sender.sending_video || info.media_sender.sending_desktop;
            bool remote_cam_en_changed = remote_cam_en != connection->remote_cam_en;
            connection->remote_cam_en = remote_cam_en;
            connection->remote_mic_en = info.media_sender.sending_audio;

            if (remote_cam_en_changed)
            {
                desc->statistic.updateVideoFlag(connection->remote_cam_en);
                _notify_remote_video_state_changed(call_id, connection);
            }
        }
        {
            std::lock_guard<std::recursive_mutex> __lock(_callsMx);
            desc->connections.erase(
            std::remove_if(desc->connections.begin(), desc->connections.end(), [&list_updated](ConnectionDescPtr connection)
                {
                    if (connection->delete_mark)
                        list_updated = true;
                    return connection->delete_mark;
                }
            ), desc->connections.end());
        }
        if (list_updated)
            _call_request_connections(desc);
    }

    void VoipManagerImpl::onCameraListUpdated(const camera::CameraEnumerator::CameraInfoList &camera_list)
    {
        _captureDevices.type = VideoCapturing;
        _captureDevices.devices.clear();
        for (auto &it: camera_list)
        {
            device_description dev;
            dev.name = it.name;
            dev.type = VideoCapturing;
            switch (it.type)
            {
            case camera::CameraEnumerator::Type::VirtualCamera:
                dev.videoCaptureType = VC_DeviceVirtualCamera; break;
            case camera::CameraEnumerator::Type::ScreenCapture:
                dev.videoCaptureType = VC_DeviceDesktop; break;
            case camera::CameraEnumerator::RealCamera:
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
            if (_captureDevices.devices.size() && VC_DeviceCamera == _captureDevices.devices[0].videoCaptureType)
                set_device(VideoCapturing, _captureDevices.devices[0].uid);
        }
    }

    void VoipManagerImpl::onAudioDevicesListUpdated(im::AudioDeviceType type, const im::AudioDeviceInfoList &audio_list)
    {
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
            if (list->devices.size())
                set_device(AudioPlayback, list->devices[0].uid);
        } else if (AudioRecording == dev_type && !_defaultAudioCapDevStored)
        {
            _defaultAudioCapDevStored = true;
            if (list->devices.size())
                set_device(AudioRecording, list->devices[0].uid);
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
        auto currentCall = get_current_call();
        if (!currentCall || currentCall->call_id != call_state.id)
            return;
        bool local_cam_en = call_state.local_media_status.sender_state.sending_video || call_state.local_media_status.sender_state.sending_desktop;
        bool changed = _voip_desc.local_aud_en != call_state.local_media_status.sender_state.sending_audio;
        changed = changed || _voip_desc.local_cam_en != local_cam_en;
        changed = changed || _voip_desc.local_cam_allowed != call_state.local_media_status.allowed_send_audio;
        changed = changed || _voip_desc.local_aud_allowed != call_state.local_media_status.allowed_send_video;
        changed = changed || _voip_desc.local_desktop_allowed != call_state.local_media_status.allowed_send_desktop;
        _voip_desc.local_aud_en = call_state.local_media_status.sender_state.sending_audio;
        _voip_desc.local_cam_en = local_cam_en;
        _voip_desc.local_aud_allowed = call_state.local_media_status.allowed_send_audio;
        _voip_desc.local_cam_allowed = call_state.local_media_status.allowed_send_video;
        _voip_desc.local_desktop_allowed = call_state.local_media_status.allowed_send_desktop;
        if (changed || !_voip_desc.first_notify_send)
            SIGNAL_NOTIFICATION(kNotificationType_MediaLocParamsChanged, &_voip_desc);
        _voip_desc.first_notify_send = true;

        assert(is_vcs_call() ? call_state.call_type == voip::kCallType_VCS : is_pinned_room_call() ? call_state.call_type == voip::kCallType_PINNED_ROOM : call_state.call_type.empty());
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
        if (call_state.peers().empty() && !_zero_participants)
        {   // room becomes empty - lock theme
            _update_theme(currentCall);
        }
        _zero_participants = call_state.peers().empty();
    }

    void VoipManagerImpl::OnMouseTap(const voip::PeerId &name, voip::BuiltinVideoWindowRenderer::MouseTap mouseTap, voip::BuiltinVideoWindowRenderer::ViewArea viewArea, float x, float y)
    {
        MouseTap mt;
        if (!_currentCallHwnd)
            return;
        mt.area    = (ViewArea)viewArea;
        //mt.call_id = call->call_id;
        mt.contact = normalizeUid(name);
        mt.hwnd    = _currentCallHwnd;
        mt.tap     = (MouseTapEnum)mouseTap;
        SIGNAL_NOTIFICATION(kNotificationType_MouseTap, &mt);
    }

    /*void VoipManagerImpl::OnFrameSizeChanged(float aspect)
    {
        VoipManagerImpl::CallDescPtr call = get_current_call();
        if (!call)
            return;
        std::lock_guard<std::recursive_mutex> lock(_windowsMx);
        WindowDesc *window_description = nullptr;
        for (size_t ix = 0; ix < _windows.size(); ++ix)
        {
            window_description = _windows[ix];
            if (window_description->call_id == call->call_id)
                break;
        }
        VOIP_ASSERT_RETURN(window_description)

        FrameSize fs;
        fs.hwnd = (intptr_t)window_description->handle;
        fs.aspect_ratio = aspect;
        SIGNAL_NOTIFICATION(kNotificationType_FrameSizeChanged, &fs);
    }*/

    void VoipManagerImpl::onMaskEngineStatusChanged(bool engine_loaded, const std::string &error_message)
    {
        _masksEngineInited = engine_loaded;
        EnableParams maskEnable;
        maskEnable.enable = _masksEngineInited;
        SIGNAL_NOTIFICATION(kNotificationType_MaskEngineEnable, &maskEnable);
    }

    void VoipManagerImpl::onMaskStatusChanged(camera::Masker::Status status, const std::string &path)
    {
    }

    VoipManager *createVoipManager(core::core_dispatcher& dispatcher)
    {
        return new(std::nothrow) VoipManagerImpl(dispatcher);
    }
}
