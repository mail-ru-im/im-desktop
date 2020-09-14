#pragma once
#include <string>
#include <vector>

#define PREVIEW_RENDER_NAME "@camera_stream_id"
#define DEFAULT_DEVICE_UID "default_device"

enum DeviceType
{
    AudioRecording = 0,
    AudioPlayback,
    VideoCapturing,
};

enum DeviceClass
{
    Audio = 0,
    Video
};

enum VideoCaptureType
{
    VC_DeviceCamera,
    VC_DeviceVirtualCamera,
    VC_DeviceDesktop
};

enum StaticImageIdEnum {
    Img_Logo = 0,
    Img_Avatar,
    Img_Username,
    Img_UserBackground,
    Img_UserForeground,
    Img_Max
};

enum ViewArea {
    ViewArea_Primary,
    ViewArea_Detached,
    ViewArea_Tray,
    ViewArea_Default,
    ViewArea_Background,
};

enum MouseTapEnum {
    MouseTap_Single,
    MouseTap_Double,
    MouseTap_Long,
    MouseTap_Over,
};

enum TerminateReason {
    TR_HANGUP = 0,          // Use Call_Terminate(TR_HANGUP) to hangup active call (after it was accepted).
                            // OnCallTerminated(TR_HANGUP) signaled when remote/local side hang up the call.

    TR_REJECT,              // Use Call_Terminate(TR_REJECT) to reject incoming call.
                            // OnCallTerminated(TR_REJECT) signaled when remote/local side rejects the call.

    TR_BUSY,                // Use Call_Terminate(TR_BUSY) to terminate call by BUSY reason (e.g. GSM call is active)
                            // OnCallTerminated(TR_BUSY) signaled when local/remote side terminated the call using BUSY
    TR_HANDLED_BY_ANOTHER_INSTANCE, // Call was handled by another device of logged-in user.
                                    // OnCallTerminated(TR_HANDLED_BY_ANOTHER_INSTANCE) is signaled when another device accepts the call.
                                    // Note: this status requires signaling server's support.

    TR_UNAUTHORIZED,        // Call was rejected due to security errors

    TR_ALLOCATE_FAILED,     // OnCallTerminated(TR_ALLOCATE_FAILED) signaled if app didn't provide ALLOCATE data or it is invalid.

    TR_ANSWER_TIMEOUT,      // OnCallTerminated(TR_ANSWER_TIMEOUT) signaled when:
                            //      - remote side hasn't answered (accepted or rejected) for outgoing call
                            //      - local user hasn't answered (so neither AcceptIncomingCall nor TerminateCall were called)

    TR_CONNECT_TIMEOUT,     // OnCallTerminated(TR_CONNECT_TIMEOUT) signaled when connection cannot be established or lost

    TR_NOT_FOUND,
    TR_BLOCKED_BY_CALLER_IS_STRANGER,
    TR_BLOCKED_BY_CALLEE_PRIVACY,
    TR_CALLER_MUST_BE_AUTHORIZED_BY_CAPCHA,

    // VCS reasons
    TR_BAD_URI,
    TR_NOT_AVAILABLE_NOW,
    TR_PARTICIPANTS_LIMIT_EXCEEDED,
    TR_DURATION_LIMIT_EXCEEDED,

    TR_INTERNAL_ERROR       // OnCallTerminated(TR_INTERNAL_ERROR) signaled on internal error (todo: more info about that)
};

struct VoipDesc
{
    bool first_notify_send = false;
    bool local_cam_en = false;
    bool local_aud_en = true;
    bool local_cam_allowed = true;
    bool local_aud_allowed = true;
    bool local_desktop_allowed = true;
    bool mute_en = false;
    bool incomingSoundsMuted = false;
    float volume = 1.0f;

    std::string aPlaybackDevice;
    std::string vCaptureDevice;
};

namespace voip_manager
{
    enum eNotificationTypes
    {
        kNotificationType_Undefined = 0,

        kNotificationType_CallCreated,
        kNotificationType_CallOutAccepted,
        kNotificationType_CallInAccepted,
        kNotificationType_CallConnected,
        kNotificationType_CallDisconnected,
        kNotificationType_CallDestroyed,
        kNotificationType_CallPeerListChanged,

        kNotificationType_QualityChanged,

        kNotificationType_MediaLocParamsChanged,
        kNotificationType_MediaRemVideoChanged,
        kNotificationType_MediaLocVideoDeviceChanged,

        kNotificationType_DeviceListChanged,
        kNotificationType_DeviceStarted,
        kNotificationType_DeviceVolChanged,

        kNotificationType_MouseTap,
        kNotificationType_LayoutChanged,

        kNotificationType_ShowVideoWindow,
        //kNotificationType_FrameSizeChanged,
        kNotificationType_VoipResetComplete,
        kNotificationType_VoipWindowRemoveComplete,
        kNotificationType_VoipWindowAddComplete,
        //kNotificationType_CipherStateChanged,

        //kNotificationType_MinimalBandwidthChanged,
        kNotificationType_MaskEngineEnable,
        kNotificationType_LoadMask,

        //kNotificationType_ConnectionDestroyed,
        kNotificationType_MainVideoLayoutChanged,
        kNotificationType_ConfPeerDisconnected,
    };

    struct VoipProxySettings
    {
        enum eProxyType
        {
            kProxyType_None = 0,
            kProxyType_Http,
            kProxyType_Socks4,
            kProxyType_Socks4a,
            kProxyType_Socks5
        };

        eProxyType   type;
        std::wstring serverUrl;
        std::wstring userName;
        std::wstring userPassword;

        VoipProxySettings() : type(kProxyType_None) { }
    };

    struct DeviceState
    {
        DeviceType type;
        std::string uid;
        bool        success;
    };

    struct DeviceMute
    {
        DeviceType type;
        bool       mute;
    };

    /*struct CipherState
    {
        enum State
        {
            kCipherStateUnknown,
            kCipherStateEnabled,
            kCipherStateFailed,
            kCipherStateNotSupportedByPeer
        } state;
        std::string secureCode;
    };*/

    struct DeviceVol
    {
        DeviceType type;
        float      volume;
    };

    struct DeviceInterrupt
    {
        DeviceType type;
        bool       interrupt;
    };

    struct MouseTap
    {
        std::string call_id;
        std::string contact;

        void* hwnd;

        MouseTapEnum tap;
        ViewArea area;
    };

    struct MissedCall
    {
        std::string account;
        std::string contact;

        unsigned   ts;
    };

    struct LayoutChanged
    {
        bool  tray;
        std::string layout_type;
        void* hwnd;
    };

    struct FrameSize
    {
        intptr_t hwnd;
        float    aspect_ratio;
    };

    struct Contact
    {
        std::string call_id;
        std::string contact;

        Contact()
        {
        }

        Contact(const std::string& call_id_, const std::string& contact_) : call_id(call_id_), contact(contact_)
        {
        }

        bool operator==(const Contact& otherContact) const
        {
            return otherContact.call_id == call_id && otherContact.contact == contact;
        }
    };

    struct ContactEx
    {
        Contact  contact;
        std::string call_type;
        std::string chat_id;
        bool     current_call = false; // for kNotificationType_CallDestroyed, true for current active call
        bool     incoming = false;
        int      terminate_reason = 0; // for kNotificationType_CallDestroyed

        // Affected window for call with this contact.
        std::vector<void*> windows;
    };

    struct ConfPeerInfo
    {
        std::string peerId;
        int terminate_reason = 0;
    };

    using ConfPeerInfoV = std::vector<ConfPeerInfo>;

    struct device_description
    {
        std::string uid;
        std::string name;
        DeviceType type = VideoCapturing;
        VideoCaptureType videoCaptureType = VC_DeviceCamera;
        bool is_default = false;
    };

    struct device_list
    {
        DeviceType type;
        std::vector<voip_manager::device_description> devices;
    };

    struct VideoEnable
    {
        bool enable;
        Contact  contact;
    };

    enum { kAvatarRequestId = 0xb00b1e };
    enum { kAvatarDefaultSize = 116 };
    enum { kAvatarRequestSize = 160 };
    enum {
         kNickTextW = kAvatarDefaultSize * 2,
         kNickTextH = 36,
         kNickTextMiniWW = kAvatarDefaultSize / 2  + 38,
         kNickTextMiniWH = 36,
         kNickTextAllSameW5peers = 140,
         kNickTextAllSameW       = 210,
         kNickTextAllSameWLarge  = 420,
         kNickTextAllSameH = 36
    };
    enum { kDetachedWndAvatarWidth = 240, kDetachedWndAvatarHeight = 180};
    enum { kLogoFromBoundOffset = 15 };
    enum { kUseVoipProtocolAsDefault = 1 };
    enum { kIncomingWndAvatarSize = 120 };
    enum { kWindowLargeWidth = 840 };

    struct BitmapDescription
    {
        void*    data;
        unsigned size;

        unsigned w;
        unsigned h;
    };

    struct VoipProtoMsg
    {
        int msg;
        std::string request;
        std::string requestId;
    };

    enum AvatarThemeType : unsigned short
    {
        MAIN_ALL_THE_SAME = 0,
        MAIN_ALL_THE_SAME_5_PEERS,
        MAIN_ALL_THE_SAME_LARGE,
        MAIN_ONE_BIG,
        MINI_WINDOW,
        INCOME_WINDOW,
        AVATAR_THEME_TYPE_MAX
    };

    struct UserBitmap
    {
        int type;
        std::string       contact;
        BitmapDescription bitmap;
        AvatarThemeType   theme;
    };

    struct WindowBitmap
    {
        void* hwnd;
        BitmapDescription bitmap;
    };

    struct ButtonSet
    {
        BitmapDescription normalButton;
        BitmapDescription highlightedButton;
        BitmapDescription pressedButton;
        BitmapDescription disabledButton;
    };

    struct WindowParams
    {
        void* hwnd;
        std::string call_id;
        WindowBitmap watermark;
        WindowBitmap cameraStatus;
        WindowBitmap calling;
        WindowBitmap connecting;
        WindowBitmap closedByBusy;
        bool  isPrimary;
        bool  isIncoming;
        float scale;
    };

    struct EnableParams
    {
        bool  enable;
    };

    struct NamedResult
    {
        std::string name;
        bool result;
    };

    struct WindowState
    {
        void* hwnd;
        bool value;
    };

    struct ContactsList
    {
        std::vector<Contact> contacts;
        std::vector<void*>   windows;
        //@{ valid for VCS calls
        std::string          conference_name;
        std::string          conference_url;
        bool                 is_webinar = false;
        //@}
        bool                 isActive = false;
    };

    enum VideoLayout
    {
        AllTheSame = 0,
        OneIsBig
    };

    enum MainVideoLayoutType
    {
        MVL_OUTGOING = 0,
        MVL_CONFERENCE,
        MVL_SIGNLE_CALL,
        MVL_INCOMING,
    };

    struct MainVideoLayout
    {
        void* hwnd;
        MainVideoLayoutType type;
        VideoLayout layout;
        MainVideoLayout() : hwnd(nullptr), type(MVL_OUTGOING), layout(OneIsBig) {};
        MainVideoLayout(void* hwnd_, MainVideoLayoutType type_, VideoLayout layout_) : hwnd(hwnd_), type(type_), layout(layout_) {};
    };

    /* Stats-specific */
    class CallEndStat
    {
    public:
        explicit CallEndStat(unsigned _callTimeElapsed, const std::string& _contact)
            : callTimeElapsed_(_callTimeElapsed),
              contact_(_contact)
        {}

        CallEndStat() = default;

        unsigned callTimeInSecs() const
        {
            return callTimeElapsed_;
        }

        const std::string& contact() const
        {
            return contact_;
        }

    private:
        unsigned callTimeElapsed_ = 0;
        std::string contact_;
    };

    struct UserRatingReport
    {
        explicit UserRatingReport(int _score,
                                  const std::string &_survey_id,
                                  const std::vector<std::string>& _reasons,
                                  const std::string &_guid)
            : score_(_score),
              survey_id_(_survey_id),
              reasons_(_reasons),
              aimid_(_guid)
        {
        }

        int score_;
        std::string survey_id_;
        std::vector<std::string> reasons_;
        std::string aimid_;
    };
    /* End Stats-specific*/

    struct CallStartParams
    {
        bool video = false;    /* start call with video enabled */
        bool attach = false;   /* add users to current call instead of creating new call */
        bool is_vcs = false;   /* Call to IVA VCS, contacts[0] contains URL */
        bool is_pinned_room = false; /* Call to pinned room, contacts[0] contains Chat ID */
    };

    class VoipManager
    {
    public:
        virtual ~VoipManager() = default;
        // CallManager
        virtual void call_create (const std::vector<std::string> &contacts, std::string &account, const CallStartParams &params) = 0;
        virtual void call_stop () = 0;
        virtual void call_stop_smart (const std::function<void()>&) = 0;
        virtual void call_accept (const std::string &call_id, const std::string &account, bool video) = 0;
        virtual void call_decline (const Contact& contact, bool busy, bool conference) = 0;
        virtual unsigned call_get_count () = 0;
        virtual bool call_have_call (const Contact& contact) = 0;

        virtual void mute_incoming_call_sounds(bool mute) = 0;

        virtual void minimal_bandwidth_switch() = 0;
        virtual bool has_created_call() = 0;

        virtual void call_report_user_rating(const UserRatingReport &_ratingReport) = 0;

        // DeviceManager
        virtual unsigned get_devices_number (DeviceType device_type) = 0;
        virtual bool get_device_list (DeviceType device_type, std::vector<device_description>& dev_list) = 0;

        virtual bool get_device (DeviceType device_type, unsigned index, std::string& device_name, std::string& device_guid) = 0;
        virtual void set_device (DeviceType device_type, const std::string& device_guid) = 0;

        virtual void set_device_mute (DeviceType deviceType, bool mute) = 0;
        virtual bool get_device_mute (DeviceType deviceType) = 0;

        virtual void notify_devices_changed(DeviceClass deviceClass) = 0;
        virtual void update() = 0;

        // WindowManager
        virtual void window_add (voip_manager::WindowParams& windowParams) = 0;
        virtual void window_remove (void* hwnd) = 0;

        virtual void window_set_bitmap (const WindowBitmap& bmp) = 0;
        virtual void window_set_bitmap (const UserBitmap& bmp) = 0;

        virtual void window_set_conference_layout(void* hwnd, voip_manager::VideoLayout layout) = 0;
        virtual void window_switch_aspect(const std::string& contact, void* hwnd) = 0;

        virtual void window_set_offsets (void* hwnd, unsigned l, unsigned t, unsigned r, unsigned b) = 0;
        virtual void window_set_primary(void* hwnd, const std::string& contact) = 0;
        virtual void window_set_hover(void* hwnd, bool hover) = 0;
        virtual void window_set_large(void* hwnd, bool large) = 0;

        // ConnectionManager
        virtual void ProcessVoipMsg(const std::string& account_uid, int voipIncomingMsg, const char *data, unsigned len) = 0;
        virtual void ProcessVoipAck(const voip_manager::VoipProtoMsg& msg, bool success) = 0;

        // MediaManager
        virtual void media_video_en (bool enable) = 0;
        virtual void media_audio_en (bool enable) = 0;

        virtual bool local_video_enabled () = 0;
        virtual bool local_audio_enabled () = 0;

        // MaskManager
        virtual void load_mask(const std::string& path) = 0;
        virtual unsigned version() = 0;
        virtual void set_model_path(const std::string& path) = 0;
        virtual void init_mask_engine() = 0;
        virtual void unload_mask_engine() = 0;

        // VoipManager
        virtual void reset() = 0;
        virtual int64_t get_voip_initialization_memory() const = 0;
    };
}
