#ifndef __VIDEO_PANEL_H__
#define __VIDEO_PANEL_H__

#include "CommonUI.h"
#include "VoipProxy.h"

extern const QString vertSoundBg;
extern const QString horSoundBg;

namespace voip_manager
{
    struct ContactEx;
    struct Contact;
}

namespace Ui
{

    struct UIEffects;
    class MaskWidget;

    class VideoPanel : public BaseBottomVideoPanel
    {
        Q_OBJECT

        Q_SIGNALS:
        void onMouseEnter();
        void onMouseLeave();
        void onFullscreenClicked();
        void onkeyEscPressed();
        void autoHideToolTip(bool& _autoHide);
        void companionName(QString& _autoHide);
        void showToolTip(bool& _show);
        // Need to show vertical volume control in right time.
        void isVideoWindowActive(bool&);
        // Update conferention layout.
        void updateConferenceMode(voip_manager::VideoLayout layout);
        void onShowMaskPanel();
        void onCameraClickOn();
        void onShareScreenClickOn();
        void onGoToChatButton();
        void onMicrophoneClick();

    private Q_SLOTS:
        void onHangUpButtonClicked();        
        void onVideoOnOffClicked();
        void _onFullscreenClicked();
        void controlActivated(bool);

        void onClickGoChat();

        void onVoipMediaLocalVideo(bool _enabled);
        void activateVideoWindow();

        void onShareScreen();

        void onVoipVideoDeviceSelected(const voip_proxy::device_desc&);

        void onCaptureAudioOnOffClicked();
        void onVoipMediaLocalAudio(bool _enabled);

    public:
        VideoPanel(QWidget* _parent, QWidget* _container);
        ~VideoPanel();

        void setFullscreenMode(bool _en);
        void fadeIn(unsigned int kAnimationDefDuration) override;
        void fadeOut(unsigned int  kAnimationDefDuration) override;
        void forceFinishFade() override;
        bool isFadedIn() override;

        bool isActiveWindow();
        void setContacts(const std::vector<voip_manager::Contact>&);

        int heightOfCommonPanel();

        void setSelectedMask(MaskWidget* maskWidget);

    private:
        void resetHangupText();
        bool isNormalPanelMode();
        bool isFitSpacersPanelMode();
        void updateVideoDeviceButtonsState();

    protected:
        void changeEvent(QEvent* _e) override;
        void enterEvent(QEvent* _e) override;
        void leaveEvent(QEvent* _e) override;
        void resizeEvent(QResizeEvent* _e) override;        
        void keyReleaseEvent(QKeyEvent* _e) override;

    private:
        bool mouseUnderPanel_;

        QWidget* container_;
        QWidget* parent_;
        QWidget* rootWidget_;

        std::vector<voip_manager::Contact> activeContact_;
        QPushButton* addChatButton_;
        QPushButton* fullScreenButton_;
        QPushButton* stopCallButton_;
        QPushButton* videoButton_;
        QPushButton* goToChat_;
        QPushButton* shareScreenButton_;
        QPushButton* microfone_;
        MaskWidget* openMasks_;

        //std::unique_ptr<UIEffects> rootEffect_;

        QSpacerItem* leftSpacer_;
        QSpacerItem* rightSpacer_;
        bool isTakling;
        bool isFadedVisible;
        bool localVideoEnabled_;
        bool isScreenSharingEnabled_;
        bool isCameraEnabled_;
    };
}

#endif//__VIDEO_PANEL_H__
