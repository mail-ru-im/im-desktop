#ifndef __DETACHED_VIDEO_WND_H__
#define __DETACHED_VIDEO_WND_H__

#include "VideoFrame.h"
#include "CommonUI.h"
#include "VoipProxy.h"

namespace voip_manager
{
    struct ContactEx;
    struct FrameSize;
}

class MiniWindowVideoPanel;

namespace Ui
{
    class ResizeEventFilter;
    class ShadowWindowParent;

    class MiniWindowVideoPanel : public Ui::BaseBottomVideoPanel
    {
        Q_OBJECT

        Q_SIGNALS :
            void onMouseEnter();
            void onMouseLeave();


        private Q_SLOTS:
        void onHangUpButtonClicked();
        void onAudioOnOffClicked();
        void onVideoOnOffClicked();

        void onVoipMediaLocalAudio(bool _enabled);
        void onVoipMediaLocalVideo(bool _enabled);
        void onShareScreen();
        void onVoipVideoDeviceSelected(const voip_proxy::device_desc& desc);

    public:
        MiniWindowVideoPanel(QWidget* _parent);
        ~MiniWindowVideoPanel() = default;

        void fadeIn(unsigned int kAnimationDefDuration) override;
        void fadeOut(unsigned int kAnimationDefDuration) override;
        bool isFadedIn() override;
        bool isUnderMouse();

        void updatePosition(const QWidget& parent) override;

    private:
        void resetHangupText();
        void updateVideoDeviceButtonsState();

    protected:
        void changeEvent(QEvent* _e) override;
        void enterEvent(QEvent* _e) override;
        void leaveEvent(QEvent* _e) override;
        void resizeEvent(QResizeEvent* _e) override;

    private:

        bool mouseUnderPanel_;

        QWidget* parent_;
        QWidget* rootWidget_;

        std::vector<voip_manager::Contact> activeContact_;
        QPushButton* micButton_;
        QPushButton* stopCallButton_;
        QPushButton* videoButton_;
        QPushButton* shareScreenButton_;

        std::unique_ptr<Ui::UIEffects> videoPanelEffect_;

        bool isTakling;
        bool isFadedVisible;
        bool localVideoEnabled_;
        bool isScreenSharingEnabled_;
        bool isCameraEnabled_;
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
        
    Q_SIGNALS:

        void windowWillDeminiaturize();
        void windowDidDeminiaturize();

    private Q_SLOTS:
        void checkPanelsVis();
        void onPanelMouseEnter();
        void onPanelMouseLeave();

        void onPanelClickedClose();
        void onPanelClickedMinimize();
        void onPanelClickedMaximize();

        void onVoipCallDestroyed(const voip_manager::ContactEx& _contactEx);
        void onVoipWindowRemoveComplete(quintptr _winId);
        void onVoipWindowAddComplete(quintptr _winId);

    public:
        DetachedVideoWindow(QWidget* _parent);
        ~DetachedVideoWindow();

        quintptr getVideoFrameId() const;
        bool closedManualy();

        void showFrame();
        void hideFrame();

        bool isMinimized() const;

    private:
        QTimer showPanelTimer_;
        ResizeEventFilter* eventFilter_;
        QWidget* parent_;
        QPoint posDragBegin_;
        bool closedManualy_;
        QHBoxLayout *horizontalLayout_;
        MiniWindowVideoPanel* videoPanel_;

        std::unique_ptr<ShadowWindowParent> shadow_;
        platform_specific::GraphicsPanel* rootWidget_;

        void mousePressEvent(QMouseEvent* _e) override;
        void mouseMoveEvent(QMouseEvent* _e) override;
        void mouseDoubleClickEvent(QMouseEvent* _e) override;
        quintptr getContentWinId();
        void updatePanels() const;
    };
}

#endif//__DETACHED_VIDEO_WND_H__
