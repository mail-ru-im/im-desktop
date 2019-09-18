#ifndef __VOIP_PROXY_H__
#define __VOIP_PROXY_H__
#include "../../core/Voip/VoipManagerDefines.h"

namespace core
{
    class coll_helper;
}

namespace Ui
{
    class core_dispatcher;
    class gui_coll_helper;
}

namespace voip_manager
{
    struct Contact;
    struct ContactEx;
    struct FrameSize;
}

namespace voip_masks
{
    class MaskManager;
}

namespace voip_proxy
{

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

    struct device_desc
    {
        std::string name;
        std::string uid;
        EvoipDevTypes dev_type;
        EvoipVideoDevTypes video_dev_type;
        bool isActive;
    };

    using device_desc_vector = std::vector<voip_proxy::device_desc>;

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
        void onVoipMuteChanged(const std::string& _deviceType, bool _muted);
        void onVoipCallNameChanged(const voip_manager::ContactsList& _contacts);
        void onVoipCallIncoming(const std::string& _account, const std::string& _contact);
        void onVoipCallIncomingAccepted(const voip_manager::ContactEx& _contactEx);
        void onVoipDeviceListUpdated(voip_proxy::EvoipDevTypes deviceType, const voip_proxy::device_desc_vector& _devices);
        void onVoipMediaLocalAudio(bool _enabled);
        void onVoipMediaLocalVideo(bool _enabled);
        void onVoipMediaRemoteVideo(const voip_manager::VideoEnable& videoEnable);
        void onVoipMouseTapped(quintptr _winId, const std::string& _tapType, const std::string& _contact);
        void onVoipButtonTapped(const voip_manager::ButtonTap& button);
        void onVoipCallOutAccepted(const voip_manager::ContactEx& _contactEx);
        void onVoipCallCreated(const voip_manager::ContactEx& _contactEx);
        void onVoipCallDestroyed(const voip_manager::ContactEx& _contactEx);
        void onVoipCallConnected(const voip_manager::ContactEx& _contactEx);
        void onVoipCallTimeChanged(unsigned _secElapsed, bool _hasCall);
        void onVoipFrameSizeChanged(const voip_manager::FrameSize& _fs);
        void onVoipResetComplete(); //hack for correct close
        void onVoipWindowRemoveComplete(quintptr _winId);
        void onVoipWindowAddComplete(quintptr _winId);
        void onVoipUpdateCipherState(const voip_manager::CipherState& _state);
        void onVoipMinimalBandwidthChanged(bool _enable);
        void onVoipMaskEngineEnable(bool _enable);
        void onVoipLoadMaskResult(const std::string& path, bool result);
        void onVoipChangeWindowLayout(intptr_t hwnd, bool bTray, const std::string& layout);
        void onVoipMainVideoLayoutChanged(const voip_manager::MainVideoLayout&);
        void onVoipVideoDeviceSelected(const voip_proxy::device_desc& device);

    private Q_SLOTS:
        void _updateCallTime();
        void _checkIgnoreContact(const QString& contact);

        void updateUserAvatar(const voip_manager::ContactEx& _contactEx);
        void updateUserAvatar(const std::string& _account, const std::string& _contact);
        void avatarChanged(const QString& aimId);
        void updateVoipMaskEngineEnable(bool _enable);

        void onCallCreate(const voip_manager::ContactEx& _contactEx);

    private:
        voip_manager::CipherState cipherState_;
        Ui::core_dispatcher& dispatcher_;
        QTimer               callTimeTimer_;
        unsigned             callTimeElapsed_;
        VoipEmojiManager     voipEmojiManager_;
        voip_manager::ContactsList activePeerList_;
        // It is currenlty connected contacts, contacts, which do not accept call, are not contained in this list.
        std::list<voip_manager::Contact> connectedPeerList_;
        bool haveEstablishedConnection_;
        bool iTunesWasPaused_;
        std::unique_ptr<voip_masks::MaskManager> maskManager_;
        bool maskEngineEnable_;
        bool voipIsKilled_; // Do we kill voip on exiting from programm.
        QList<voip_proxy::device_desc> screenList_;
        voip_proxy::device_desc lastSelectedCamera_;
        voip_proxy::device_desc currentSelectedVideoDevice_;
        bool localCameraEnabled_;
        bool localVideoEnabled_;

        // For each device type.
        std::vector<device_desc> devices_[3];
        std::unordered_map<EvoipDevTypes, device_desc> activeDevices_;

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

        // Load and apply volume from settings.
        void loadPlaybackVolumeFromSettings();

        // This method is called, when you start or recive call.
        void onStartCall(bool _bOutgoing);
        // This method is called on end of call.
        void onEndCall();

        // Set window background.
        void setWindowBackground(quintptr _hwnd, unsigned char r, unsigned char g, unsigned char b, unsigned char a);

        void updateScreenList(const std::vector<voip_proxy::device_desc>& _devices);
        void _setSwitchVCaptureMute();
        void _setNoneVideoCaptureDevice();
        // Fix premultiply problem. Fill opacity pixels to the same color.
        void fillColorPlanes(QImage& image, QColor color);

    public:
        VoipController(Ui::core_dispatcher& _dispatcher);
        virtual ~VoipController();

    public:
        void voipReset();
        void updateActivePeerList();
        void getSecureCode(voip_manager::CipherState& _state) const;
        VoipEmojiManager& getEmojiManager()
        {
            return voipEmojiManager_;
        }

        void setWindowAdd(quintptr hwnd, bool _primaryWnd, bool _systemWd, int _panelHeight);
        void setWindowRemove(quintptr _hwnd);
        void setWindowOffsets(quintptr _hwnd, int _lpx, int _tpx, int _rpx, int _bpx);
        void setAvatars(int _size, const char* _contact, bool forPreview = false);

        void setStartA(const char* _contact, bool _attach, const char* _where = nullptr);
        void setStartV(const char* _contact, bool _attach, const char* _where = nullptr);
        void setHangup();
        void setAcceptA(const char* _contact);
        void setAcceptV(const char* _contact);
        void setDecline(const char* _contact, bool _busy);

        void setSwitchAPlaybackMute();
        void setSwitchACaptureMute();
        void setSwitchVCaptureMute();
        void setVolumeAPlayback(int _volume);
        void setRequestSettings();
        void setActiveDevice(const device_desc& _description);
        void setMuteSounds(bool _mute);

        void setAPlaybackMute(bool _mute);

        void switchMinimalBandwithMode();

        void handlePacket(core::coll_helper& _collParams);
        static std::string formatCallName(const std::vector<std::string>& _names, const char* _clip);

        void loadMask(const std::string& maskPath);

        void setWindowSetPrimary(quintptr _hwnd, const char* _contact);

        voip_masks::MaskManager* getMaskManager() const;

        bool isMaskEngineEnabled() const;
        void initMaskEngine() const;
        void resetMaskManager();
        bool isVoipKilled() const;
        bool isConnectionEstablished() const;

        void setWindowVideoLayout(quintptr _hwnd, voip_manager::VideoLayout& layout);

        const std::vector<device_desc>& deviceList(EvoipDevTypes type) const;
        std::optional<device_desc> activeDevice(EvoipDevTypes type) const;
        const std::vector<voip_manager::Contact>& currentCallContacts() const;

        // @return true is any of users are accepted call.
        bool hasEstablishCall() const;

        // For now max is 4 members.
        int maxVideoConferenceMembers() const;

        // Open chat with user
        void openChat(const QString& contact);

        // Switch share screen mode.
        void switchShareScreen(voip_proxy::device_desc const * _description);

        const QList<voip_proxy::device_desc>& screenList() const;

        // Pass hover flag to voip.
        void passWindowHover(quintptr _hwnd, bool hover);

        // Update large state for window.
        void updateLargeState(quintptr _hwnd, bool large);

    Q_SIGNALS:
        // NOTE: Only for stats-specific tasks
        void onVoipCallEndedStat(const voip_manager::CallEndStat& _stat);
    };
}

Q_DECLARE_METATYPE(voip_proxy::device_desc);
Q_DECLARE_METATYPE(voip_proxy::device_desc_vector);

#endif//__VOIP_PROXY_H__