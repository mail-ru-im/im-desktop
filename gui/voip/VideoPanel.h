#pragma once
#include "CommonUI.h"
#include "VoipProxy.h"
#include "ScreenFrame.h"
#include "../controls/ClickWidget.h"
#include "media/permissions/MediaCapturePermissions.h"


namespace voip_manager
{
    struct ContactEx;
    struct Contact;
}

namespace Previewer
{
    class CustomMenu;
}

namespace Ui
{
    struct UIEffects;
    class Toast;
    class InputBgWidget;
    class PanelToolButton;

    namespace TextRendering
    {
        class TextUnit;
    }

    class VideoPanel : public QWidget
    {
        Q_OBJECT

    Q_SIGNALS:
        void onkeyEscPressed();
        void onCameraClickOn();
        void onShareScreenClick(bool _on);
        void onGoToChatButton();
        void onMicrophoneClick();
        void onSpeakerClick();
        void needShowScreenPermissionsPopup(media::permissions::DeviceType type);
        void addUserToConference();
        void inviteVCSUrl(const QString& _url) const;
        void lockAnimations();
        void unlockAnimations();
        void requestHangup();

    public Q_SLOTS:
        void onVideoOnOffClicked();
        void onShareScreen();
        void onSpeakerOnOffClicked();
        void onCaptureAudioOnOffClicked();
        void onClickCopyVCSLink();
        void onClickInviteVCS();

    private Q_SLOTS:

        void onClickGoChat();

        void onVoipMediaLocalVideo(bool _enabled);

        void onVoipVideoDeviceSelected(const voip_proxy::device_desc& _desc);

        void onVoipMediaLocalAudio(bool _enabled);

        void onVoipVolumeChanged(const std::string& _deviceType, int _vol);

        void onChangeConferenceMode();
        void onClickSettings();

        void onVoipCallNameChanged(const voip_manager::ContactsList& _contacts);

        void onDeviceSettingsClicked();
        void inviteButtonClicked();

        void onShareScreenStateChanged(ScreenSharingManager::SharingState _state, int);

    public:
        VideoPanel(QWidget* _parent);
        ~VideoPanel();

        static bool isRunningUnderWayland();
        void setVisible(bool _visible) override;

        void setContacts(std::vector<voip_manager::Contact> _contacts, bool _activeCall);
        bool isPreventFadeOut() const;

        void callDestroyed();

        void showToast(VideoWindowToastProvider::Type _type);
        void hideToast();

        bool showPermissionPopup();

        void updatePanelButtons();
        void setConferenceMode(bool _isConferenceAll);

    private:
        void updateVideoDeviceButtonsState();

        enum class MenuAction
        {
            GoToChat,
            ConferenceAllTheSame,
            ConferenceOneIsBig,
            AddUser,
            CallSettings,
            ShareScreen,
            CopyVCSLink,
            InviteVCS,
            SelectDevice,
            SelectDeviceOn
        };

        template<typename Slot>
        void addMenuAction(MenuAction _action, const QString& _addText, Slot&& _slot);
        template<typename Slot>
        void addMenuAction(MenuAction _action, Slot&& _slot);
        template<typename Slot>
        void addMenuAction(const QPixmap& _icon, const QString& _text, Slot&& _slot);

        static QString menuActionIconPath(MenuAction _action);
        static QString menuActionText(MenuAction _action);
        void resetMenu();
        void closeMenu();

        void showShareScreenMenu(int _screenCount);

        void resetPanelButtons(bool _enable = true);

        void onActiveDeviceChange(const voip_proxy::device_desc& _device);

        void onScreensChanged();

        struct DeviceInfo
        {
            std::string name;
            std::string uid;
        };

        std::vector<DeviceInfo> aCapDeviceList;
        std::vector<DeviceInfo> aPlaDeviceList;
        std::vector<DeviceInfo> vCapDeviceList;

    protected:
        void resizeEvent(QResizeEvent* _e) override;
        void keyReleaseEvent(QKeyEvent* _e) override;
        void moveEvent(QMoveEvent* _e) override;
        void hideEvent(QHideEvent* _e) override;
        bool event(QEvent* _e) override;
        bool eventFilter(QObject* _watched, QEvent* _event) override;

    private:
        PanelToolButton* createButton(const QString& _text, bool _checkable) const;

    private:
        QWidget* container_ = nullptr;
        QWidget* parent_ = nullptr;
        QWidget* rootWidget_ = nullptr;
        QHBoxLayout* rootLayout_ = nullptr;

        std::vector<voip_manager::Contact> activeContact_;
        PanelToolButton* stopCallButton_ = nullptr;
        PanelToolButton* videoButton_ = nullptr;
        PanelToolButton* shareScreenButton_ = nullptr;
        PanelToolButton* microphoneButton_ = nullptr;
        PanelToolButton* speakerButton_ = nullptr;
        PanelToolButton* inviteButton_ = nullptr;

        QPointer<Previewer::CustomMenu> menu_;
        QPointer<Toast> toast_;

        bool isFadedVisible_ = false;
        bool doPreventFadeIn_ = false;
        bool localVideoEnabled_ = false;
        bool isScreenSharingEnabled_ = false;
        bool isCameraEnabled_ = true;
        bool isConferenceAll_ = true;
        bool isParentMinimizedFromHere_ = false;
        bool isBigConference_ = false;
        bool isInviteButtonEnabled_ = false;

        Qt::WindowStates prevParentState_ = Qt::WindowNoState;
    };
}
