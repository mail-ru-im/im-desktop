#pragma once
#include "VideoFrame.h"
#include "VideoPanel.h"
#include "CommonUI.h"
#include "VoipProxy.h"
#include "media/permissions/MediaCapturePermissions.h"

namespace voip_manager
{
    struct ContactEx;
    struct FrameSize;
}

namespace Utils
{
    class OpacityEffect;
}

class MiniWindowVideoPanel;

namespace Ui
{
    class ResizeEventFilter;
    class ShadowWindowParent;
    class PanelButton;
    class TransparentPanelButton;

    class MiniWindowVideoPanel : public Ui::BaseBottomVideoPanel
    {
        Q_OBJECT

     Q_SIGNALS:
        void onMouseEnter();
        void onMouseLeave();
        void needShowScreenPermissionsPopup(media::permissions::DeviceType _type);
        void onMicrophoneClick();
        void mousePressed(const QMouseEvent& _e);
        void mouseMoved(const QMouseEvent& _e);
        void mouseDoubleClicked();
        void onOpenCallClicked();
        void onResizeClicked();
        void onHangClicked();

    private Q_SLOTS:
        void onResizeButtonClicked();
        void onOpenCallButtonClicked();
        void onHangUpButtonClicked();
        void onAudioOnOffClicked();
        void onVideoOnOffClicked();

        void onVoipMediaLocalAudio(bool _enabled);
        void onVoipMediaLocalVideo(bool _enabled);
        void onShareScreen();
        void onVoipVideoDeviceSelected(const voip_proxy::device_desc& _desc);

    public:
        MiniWindowVideoPanel(QWidget* _parent);
        ~MiniWindowVideoPanel();

        bool isUnderMouse();

        void updatePosition(const QWidget& _parent) override;

        void showScreenBorder(std::string_view _uid);
        void hideScreenBorder();

    private:
        void updateVideoDeviceButtonsState();

    protected:
        void changeEvent(QEvent* _e) override;
        void enterEvent(QEvent* _e) override;
        void leaveEvent(QEvent* _e) override;
        void resizeEvent(QResizeEvent* _e) override;
        void mousePressEvent(QMouseEvent* _e) override;
        void mouseMoveEvent(QMouseEvent* _e) override;
        void mouseDoubleClickEvent(QMouseEvent* _e) override;
        void paintEvent(QPaintEvent* _e) override;

    private:
        std::vector<voip_manager::Contact> activeContact_;

        QWidget* parent_;
        QWidget* rootWidget_;

        TransparentPanelButton* resizeButton_;
        TransparentPanelButton* openCallButton_;
        PanelButton* micButton_;
        PanelButton* stopCallButton_;
        PanelButton* videoButton_;
        PanelButton* shareScreenButton_;
        QPointer<Ui::VideoPanelParts::ShareScreenFrame> shareScreenFrame_;

        bool localVideoEnabled_ : 1;
        bool isScreenSharingEnabled_ : 1;
        bool isCameraEnabled_ : 1;
        bool mouseUnderPanel_ : 1;
    };

    class DetachedVideoWindow : public QWidget
    {
        Q_OBJECT

    protected:
        void showEvent(QShowEvent*) override;
        void hideEvent(QHideEvent*) override;
        void changeEvent(QEvent* _e) override;
        void enterEvent(QEvent* _e) override;
        void leaveEvent(QEvent* _e) override;
        void paintEvent(QPaintEvent* _e) override;
        void mousePressEvent(QMouseEvent* _e) override;
        void mouseReleaseEvent(QMouseEvent* _e) override;
        void mouseMoveEvent(QMouseEvent* _e) override;
        void mouseDoubleClickEvent(QMouseEvent* _e) override;

    Q_SIGNALS:
        void windowWillDeminiaturize();
        void windowDidDeminiaturize();
        void needShowScreenPermissionsPopup(media::permissions::DeviceType type);
        void onMicrophoneClick();

    private Q_SLOTS:
        void onPanelMouseEnter();
        void onPanelMouseLeave();

        void onPanelClickedClose();
        void onPanelClickedResize();

        void onVoipCallDestroyed(const voip_manager::ContactEx& _contactEx);
        void onVoipWindowRemoveComplete(quintptr _winId);
        void onVoipWindowAddComplete(quintptr _winId);

    private:
        void onMousePress(const QMouseEvent& _e);
        void onMouseMove(const QMouseEvent& _e);

        quintptr getContentWinId();
        void updatePanels() const;

        void activateMainVideoWindow();

        enum class ResizeDirection
        {
            Minimize,
            Maximize,
        };
        void resizeAnimated(ResizeDirection _dir);

        void startTooltipTimer();
        void onTooltipTimer();

    public:
        DetachedVideoWindow(QWidget* _parent);
        ~DetachedVideoWindow();

        quintptr getVideoFrameId() const;
        bool closedManualy();

        void showFrame();
        void hideFrame();

        bool isMinimized() const;

    private:
        ResizeEventFilter* eventFilter_;
        QWidget* parent_;
        QPoint posDragBegin_;
        QPoint pressPos_;
        bool closedManualy_;
        MiniWindowVideoPanel* videoPanel_;

        std::unique_ptr<ShadowWindowParent> shadow_;
        platform_specific::GraphicsPanel* rootWidget_;

        QVariantAnimation* opacityAnimation_;
        QVariantAnimation* resizeAnimation_;

        QPoint tooltipPos_;
        QTimer* tooltipTimer_;
    };
}
