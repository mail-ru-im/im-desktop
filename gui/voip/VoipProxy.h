#pragma once
#include "../../core/Voip/VoipManagerDefines.h"
#include "src/CallStateInternal.h"
#include "VoipController.h"

#define DEFAULT_DEVICE_UID "default_device"

namespace core
{
    class coll_helper;
}

namespace Ui
{
    class core_dispatcher;
    class gui_coll_helper;
    enum class MicroIssue;
}

namespace voip_manager
{
    struct Contact;
    struct ContactEx;
    struct FrameSize;
}

namespace voip_proxy
{
    extern const std::string kCallType_NONE;

    enum EvoipDevTypes
    {
        kvoipDevTypeUndefined     = 0,
        kvoipDevTypeAudioCapture  = 1,
        kvoipDevTypeAudioPlayback = 2,
        kvoipDevTypeVideoCapture  = 3
    };

    enum EvoipVideoDevTypes
    {
        kvoipDeviceCamera,
        kvoipDeviceVirtualCamera,
        kvoipDeviceDesktop
    };

    enum CheckActiveCallResult
    {
        CallEnded,
        NoActiveCall,
        Cancelled,
        SameCall
    };

    struct device_desc
    {
        std::string name;
        std::string uid;
        EvoipDevTypes dev_type = voip_proxy::kvoipDevTypeVideoCapture;
        EvoipVideoDevTypes video_dev_type = voip_proxy::kvoipDeviceCamera;
    };

    using device_desc_vector = std::vector<voip_proxy::device_desc>;

    enum class TermincationReasonGroup
    {
        PRIVACY = 0,
        INVALID_CALLER,
        UNKNOWN
    };

    class VoipEmojiManager
    {
        struct CodeMap
        {
            size_t id;
            size_t codePointSize;
            std::string path;
            unsigned sw;
            unsigned sh;
            std::vector<unsigned> codePoints;
        };

        std::vector<CodeMap>   codeMaps_;
        size_t                 activeMapId_;
        QImage                 activeMap_;

    public:
        VoipEmojiManager();
        ~VoipEmojiManager();

    public:
        bool addMap(const unsigned _sw, const unsigned _sh, const std::string& _path, const std::vector<unsigned>& _codePoints, const size_t _size);
        bool getEmoji(const unsigned _codePoint, const size_t _size, QImage& _image);
    };

    class VoipController : public QObject
    {
        Q_OBJECT

        Q_SIGNALS:
        void onVoipShowVideoWindow(bool _show);
        void onVoipVolumeChanged(const std::string& _deviceType, int _vol);
        void onVoipCallNameChanged(const voip_manager::ContactsList& _contacts);
        void onVoipCallIncoming(const std::string& call_id, const std::string& _contact, const std::string& call_type);
        void onVoipCallIncomingAccepted(const std::string& call_id);
        void onVoipDeviceListUpdated(voip_proxy::EvoipDevTypes deviceType, const voip_proxy::device_desc_vector& _devices);
        void onVoipMediaLocalAudio(bool _enabled);
        void onVoipMediaLocalVideo(bool _enabled);
        void onVoipMediaRemoteVideo(const voip_manager::VideoEnable& videoEnable);
        void onVoipCallCreated(const voip_manager::ContactEx& _contactEx);
        void onVoipCallDestroyed(const voip_manager::ContactEx& _contactEx);
        void onVoipCallConnected(const voip_manager::ContactEx& _contactEx);
        void onVoipCallTimeChanged(unsigned _secElapsed, bool _hasCall);
        void onVoipResetComplete(); //hack for correct close
        void onVoipWindowRemoveComplete(quintptr _winId);
        void onVoipWindowAddComplete(quintptr _winId);
        void onVoipMaskEngineEnable(bool _enable);
        void onVoipVoiceEnable(bool _enable);
        void onVoipVadInfo(const std::vector<im::VADInfo> &_peers);
        void onVoipVideoDeviceSelected(const voip_proxy::device_desc& device);
        void onVoipHideControlsWhenRemDesktopSharing(bool _hide);

    private Q_SLOTS:
        void _updateCallTime();
        void _checkIgnoreContact(const QString& contact);

        void avatarChanged(const QString& aimId);

    private:
        //voip_manager::CipherState cipherState_;
        Ui::core_dispatcher& dispatcher_;
        QTimer               callTimeTimer_;
        unsigned             callTimeElapsed_ = 0;
        VoipEmojiManager     voipEmojiManager_;
        voip_manager::ContactsList activePeerList_;
        // It is currenlty connected contacts, contacts, which do not accept call, are not contained in this list.
        QList<voip_proxy::device_desc> screenList_;
        voip_proxy::device_desc currentSelectedVideoDevice_;
        std::unique_ptr<Ui::gui_coll_helper> pendingCallStart_;
        std::string vcsConferenceUrl_;
        std::string callType_ = kCallType_NONE;
        std::string chatId_;
        bool vcsWebinar_ = false;

        // Map to store call termination reasons for groupcalls [reason, disconnected list]
        std::map<TermincationReasonGroup, std::vector<std::string>> disconnectedPeers_;

        std::map<std::string, voip_manager::ContactEx> inCallsList_; // for better IMDESKTOP-15780 case when answer incoming from chat through join to this chat

        // For each device type.
        std::vector<device_desc> devices_[3];
        std::unordered_map<EvoipDevTypes, device_desc> activeDevices_;

        bool haveEstablishedConnection_ = false;
        bool iTunesWasPaused_ = false;
        bool localAudioEnabled_ = false;
        bool localCameraEnabled_ = false;
        bool localVideoEnabled_ = false;
        bool localAudAllowed_ = true;
        bool localCamAllowed_ = true;
        bool localDesktopAllowed_ = true;
        bool localAudPermission_ = false;
        bool localCamPermission_ = false;

        bool settingsLoaded_ = false;
        bool voipIsKilled_ = false; // Do we kill voip on exiting from programm.

        bool hideControlsWhenRemDesktopSharing_ = false;
        bool isResetDialogActive_ = false;

        enum UserBitmapParams
        {
            AVATAR_ROUND             = 0x000001,
            AVATAR_SQUARE_BACKGROUND = 0x000002,
            AVATAR_SQUARE_WITH_LETTER= 0x000004,
            USER_NAME_ONE_IS_BIG     = 0x000008,
            USER_NAME_ALL_THE_SAME   = 0x000010,
            USER_NAME_MINI           = 0x000020,
            USER_NAME_ALL_THE_SAME_5_PEERS = 0x000040,
            USER_NAME_ALL_THE_SAME_LARGE   = 0x000080,
        };
        void _prepareUserBitmaps(Ui::gui_coll_helper& _collection, const std::string& _contact,
            int _size, int userBitmapParamBitSet, voip_manager::AvatarThemeType theme);

        void onStartCall(bool _bOutgoing); // This method is called, when you start or recive call.
        void onEndCall(); // This method is called on end of call.

        void updateScreenList(const std::vector<voip_proxy::device_desc>& _devices);
        void _setSwitchVCaptureMute();

        void updateDisconnectedPeers();

    public:
        VoipController(Ui::core_dispatcher& _dispatcher);
        virtual ~VoipController();

        void loadSettings(std::function<void(voip_proxy::device_desc &description)> callback);
        void voipReset();
        void updateActivePeerList();
        //void getSecureCode(voip_manager::CipherState& _state) const;
        VoipEmojiManager& getEmojiManager()
        {
            return voipEmojiManager_;
        }

        void setWindowAdd(quintptr hwnd, const char *call_id, bool _primaryWnd, bool _systemWd, int _panelHeight);
        void setWindowRemove(quintptr _hwnd);
        void setAvatars(int _size, const char* _contact, bool forPreview = false);

        bool checkPermissions(bool _audio, bool _video, bool _doRequest = true);
        bool setStartCall(const std::vector<QString>& _contacts, bool _video, bool _attach);
        bool setStartChatRoomCall(const QStringView _chatId, const std::vector<QString>& _contacts, bool _video);
        bool setStartChatRoomCall(const QStringView _chatId, bool _video = false);
        void setStartVCS(const QString &_urlConference);
        void setHangup();
        void setAcceptCall(const char* call_id, bool video);
        void setDecline(const char* call_id, const char* _contact, bool _busy, bool conference = false);

        void setSwitchAPlaybackMute();
        void setSwitchACaptureMute();
        void setSwitchVCaptureMute();
        void setRequestSettings();
        void setActiveDevice(const device_desc& _description, bool _force_reset = false);
        void setMuteSounds(bool _mute);
        void setAPlaybackMute(bool _mute);
        void setRenderResolution(std::string_view _contact, int _width, int _height);

        void handlePacket(core::coll_helper& _collParams);
        static std::string formatCallName(const std::vector<std::string>& _names, const char* _clip);

        bool isVoipKilled() const;
        bool isConnectionEstablished() const;
        bool isCallVCS() const { return voip::kCallType_VCS == callType_; }
        bool isCallVCSLink() const { return voip::kCallType_LINK == callType_; }
        bool isCallVCSOrLink() const { return isCallVCS() || isCallVCSLink(); }
        bool isCallPinnedRoom() const { return voip::kCallType_PINNED_ROOM == callType_; }
        bool isLocalAudAllowed() const { return localAudAllowed_; }
        bool isLocalCamAllowed() const { return localCamAllowed_; }
        bool isLocalDesktopAllowed() const { return localDesktopAllowed_; }
        bool isAudPermissionGranted() const { return localAudPermission_; }
        bool isCamPermissionGranted() const { return localCamPermission_; }

        bool isHideControlsWhenRemDesktopSharing() const { return hideControlsWhenRemDesktopSharing_; }
        bool isResetCallDialogActive() const;

        bool isWebinar() const { return vcsWebinar_; }
        std::string getChatId() const { return chatId_; }

        QString getConferenceUrl() const { return QString::fromStdString(vcsConferenceUrl_); }

        const std::vector<device_desc>& deviceList(EvoipDevTypes type) const;
        std::optional<device_desc> activeDevice(EvoipDevTypes type) const;
        const std::vector<voip_manager::Contact>& currentCallContacts() const;
        bool hasActiveAudioCaptureDevice() const;

        bool hasEstablishCall() const; // @return true is any of users are accepted call.
        int maxVideoConferenceMembers() const;
        int maxUsersWithVideo() const;
        int lowerBoundForBigConference() const;

        void openChat(const QString& contact); // Open chat with user
        void switchShareScreen(voip_proxy::device_desc const * _description); // Switch share screen mode.

        const QList<voip_proxy::device_desc>& screenList() const;

        void notifyDevicesChanged(bool audio);
        void trackDevices(bool track_video = true);

        CheckActiveCallResult checkActiveCall(const std::string& _contact);

    Q_SIGNALS:
        // NOTE: Only for stats-specific tasks
        void onVoipCallEndedStat(const voip_manager::CallEndStat& _stat);
    };
}

Q_DECLARE_METATYPE(voip_proxy::device_desc);
Q_DECLARE_METATYPE(voip_proxy::device_desc_vector);
