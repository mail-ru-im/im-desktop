#pragma once
#include "media/permissions/MediaCapturePermissions.h"
#include "renders/IRender.h"

namespace voip_proxy
{
    struct device_desc;
}

namespace Ui
{
    struct VideoPeer
    {
        std::unique_ptr<const im::IVideoFrame> lastFrame_;
        bool lastFrameDrawed_ = false;
    };

    class VideoWindow : public QWidget
    {
        Q_OBJECT
    public:
        VideoWindow();
        ~VideoWindow();

        QRect toastRect() const;

    public Q_SLOTS:
        void raiseWindow();
        void hideFrame();
        void showFrame();
        void toggleFullscreen();
        void setGridView();
        void setSpeakerView();
        void showMicroPermissionPopup();
        void switchVideoEnableByShortcut();
        void switchScreenSharingEnableByShortcut();
        void switchSoundEnableByShortcut();
        void switchMicrophoneEnableByShortcut();

    Q_SIGNALS:
        void drawFrames();
        void windowStatesChanged(Qt::WindowStates);

    private Q_SLOTS:
        void onDrawFrames();
        void removeUnneededRemoteVideo();
        void onVoipCallNameChanged(const voip_manager::ContactsList&);
        void onVoipCallTimeChanged(unsigned _secElapsed, bool _hasCall);
        void onVoipCallDestroyed(const voip_manager::ContactEx& _contactEx);
        void onVoipCallConnected(const voip_manager::ContactEx& _contactEx);
        void onVoipMediaRemoteVideo(const voip_manager::VideoEnable& videoEnable);
        void onVoipWindowRemoveComplete(quintptr _winId);
        void onVoipWindowAddComplete(quintptr _winId);
        void onVoipCallCreated(const voip_manager::ContactEx& _contactEx);
        void onVoipMediaLocalAudio(bool _enabled);
        void onVoipMediaLocalVideo(bool _enabled);
        void onVoipVoiceEnable(bool _enable);
        void onVoipVideoDeviceSelected(const voip_proxy::device_desc& _desc);
        void onAudioDeviceListChanged();
        void onHangupRequested();

        void checkOverlap();
        void onSpacebarTimeout();

        void checkMicroPermission();
        void updateMicroAlertState();

        void onSetPreviewPrimary();
        void onSetContactPrimary();
        void onSetContactPrimary(const std::string& _contact);
        void onScreenSharing(bool _on);
        void onLinkCopyRequest();
        void onAddUserRequest();
        void onInviteVCSUrlRequest(const QString& _url);

        void showScreenPermissionsPopup(media::permissions::DeviceType type);

        void fullscreenAnimationFinish(); // for MacOS only
        void minimizeAnimationFinish(); // MacOS only

    protected:
        bool event(QEvent* _event) override;
        bool eventFilter(QObject* _watched, QEvent* _event) override;
        void hoverEnter(QHoverEvent* _event);
        void hoverMove(QHoverEvent* _event);
        void hoverLeave(QHoverEvent* _event);
        void windowStateChanged(QWindowStateChangeEvent* _event);

        void showEvent(QShowEvent* _event) override;
        void hideEvent(QHideEvent* _event) override;
        void resizeEvent(QResizeEvent* _event) override;
        void paintEvent(QPaintEvent* _event) override;
        void closeEvent(QCloseEvent* _event) override;

        void keyPressEvent(QKeyEvent* _event) override;
        void keyReleaseEvent(QKeyEvent* _event) override;
        void mouseDoubleClickEvent(QMouseEvent* _event) override;
        void contextMenuEvent(QContextMenuEvent* _event) override;

        bool nativeEvent(const QByteArray& _eventType, void* _message, long* _result) override;

    private:
        friend class VideoWindowPrivate;
        std::unique_ptr<class VideoWindowPrivate> d;
    };
}
