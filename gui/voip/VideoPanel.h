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
    class PanelButton;

    namespace TextRendering
    {
        class TextUnit;
    }

    class VideoPanel : public BaseBottomVideoPanel
    {
        Q_OBJECT

        Q_SIGNALS:
        void onMouseEnter();
        void onMouseLeave();
        void onkeyEscPressed();
        void updateConferenceMode(voip_manager::VideoLayout _layout);
        void onShowMaskPanel();
        void onCameraClickOn();
        void onShareScreenClick(bool _on);
        void onGoToChatButton();
        void onMicrophoneClick();
        void onSpeakerClick();
        void needShowScreenPermissionsPopup(media::permissions::DeviceType type);
        void addUserToConference();
        void parentWindowWillDeminiaturize();
        void parentWindowDidDeminiaturize();
        void closeActiveMask();
        void inviteVCSUrl(const QString& _url) const;

    private Q_SLOTS:
        void onHangUpButtonClicked();
        void onVideoOnOffClicked();

        void onClickGoChat();

        void onVoipMediaLocalVideo(bool _enabled);

        void onShareScreen();

        void onVoipVideoDeviceSelected(const voip_proxy::device_desc& _desc);

        void onCaptureAudioOnOffClicked();
        void onVoipMediaLocalAudio(bool _enabled);

        void onSpeakerOnOffClicked();
        void onVoipVolumeChanged(const std::string& _deviceType, int _vol);

        void onMoreButtonClicked();
        void onChangeConferenceMode();
        void onClickSettings();

        void onClickOpenMasks();
        void onVoipCallNameChanged(const voip_manager::ContactsList& _contacts);

        void onClickCopyVCSLink();
        void onClickInviteVCS();

        void onDeviceSettingsClick(const PanelButton* _button, voip_proxy::EvoipDevTypes _type);
        void addButtonClicked();

        void onShareScreenStateChanged(ScreenSharingManager::SharingState _state, int);

    public:
        VideoPanel(QWidget* _parent, QWidget* _container);
        ~VideoPanel();

        static bool isRunningUnderWayland();
        void setVisible(bool _visible) override;

        void updatePosition(const QWidget& _parent) override;

        void fadeIn(unsigned int _kAnimationDefDuration) override;
        void fadeOut(unsigned int _kAnimationDefDuration) override;
        bool isFadedIn() override;

        bool isActiveWindow();
        void setContacts(std::vector<voip_manager::Contact> _contacts, bool _activeCall);

        bool isPreventFadeOut() const;
        void setPreventFadeIn(bool _doPrevent);

        void changeConferenceMode(voip_manager::VideoLayout _layout);

        void callDestroyed();

        enum class ToastType
        {
            CamNotAllowed,
            DesktopNotAllowed,
            MasksNotAllowed,
            MicNotAllowed,
            LinkCopied,
            EmptyLink,
            DeviceUnavailable
        };
        void showToast(ToastType _type);
        void showToast(const QString& _text, int _maxLineCount = 1);
        void hideToast();

        bool showPermissionPopup();

        void updatePanelButtons();

    private:
        void updateVideoDeviceButtonsState();

        enum class MenuAction
        {
            GoToChat,
            ConferenceAllTheSame,
            ConferenceOneIsBig,
            OpenMasks,
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
        void showMenuAt(const QPoint& _pos);
        void showMenuAtButton(const PanelButton* _button);
        void closeMenu();

        void resetPanelButtons(bool _enable = true);

        void onActiveDeviceChange(const voip_proxy::device_desc& _device);

        void updateToastPosition();

        struct DeviceInfo
        {
            std::string name;
            std::string uid;
        };

        std::vector<DeviceInfo> aCapDeviceList;
        std::vector<DeviceInfo> aPlaDeviceList;
        std::vector<DeviceInfo> vCapDeviceList;

    protected:
        void changeEvent(QEvent* _e) override;
        void enterEvent(QEvent* _e) override;
        void leaveEvent(QEvent* _e) override;
        void resizeEvent(QResizeEvent* _e) override;
        void keyReleaseEvent(QKeyEvent* _e) override;
        void paintEvent(QPaintEvent* _e) override;
        void moveEvent(QMoveEvent* _e) override;
        bool event(QEvent* _e) override;

    private:
        QWidget* container_ = nullptr;
        QWidget* parent_ = nullptr;
        QWidget* rootWidget_ = nullptr;
        QHBoxLayout* rootLayout_ = nullptr;

        std::vector<voip_manager::Contact> activeContact_;
        PanelButton* stopCallButton_ = nullptr;
        PanelButton* videoButton_ = nullptr;
        PanelButton* shareScreenButton_ = nullptr;
        PanelButton* microphoneButton_ = nullptr;
        PanelButton* speakerButton_ = nullptr;
        PanelButton* moreButton_ = nullptr;
        PanelButton* addButton_ = nullptr;

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
        bool isMasksAllowed_ = true;
        bool isAddButtonEnabled_ = false;

        Qt::WindowStates prevParentState_ = Qt::WindowNoState;
    };
}
