#pragma once
#include "VideoFrame.h"
#include "VideoWindow.h"

namespace Ui
{
    class ShadowWindow;
    class ResizeEventFilter;
    class VoipSysPanelHeader;
    class IncomingCallControls;

    class IncomingCallWindow : public QWidget
    {
        Q_OBJECT
        std::string contact_;
        std::string call_id_;

    private:
        typedef platform_specific::GraphicsPanel FrameControl_t;

    private Q_SLOTS:
        void onVoipCallNameChanged(const voip_manager::ContactsList&);
        void onVoipWindowRemoveComplete(quintptr _winId);
        void onVoipWindowAddComplete(quintptr _winId);

        void onDeclineButtonClicked();
        void onAcceptVideoClicked();
        void onAcceptAudioClicked();

        void updateTitle();

    public:
        IncomingCallWindow(const std::string& call_id, const std::string& _contact, const std::string& call_type);
        ~IncomingCallWindow();

        void showFrame();
        void hideFrame();

    protected:
        void mouseMoveEvent(QMouseEvent* _e) override;
        void mousePressEvent(QMouseEvent * event) override;
#ifndef _WIN32
        void mouseReleaseEvent(QMouseEvent *event) override;
        // @return true if we resend message to any transparent panel
        void resendMouseEventToPanel(QMouseEvent *event_);
#endif

    private:
        std::string call_type_;
        std::unique_ptr<VoipSysPanelHeader>  header_;
        std::unique_ptr<IncomingCallControls> controls_;
        std::unique_ptr<TransparentPanel>   transparentPanelOutgoingWidget_;

        ResizeEventFilter* eventFilter_;

        // List of currect contacts
        std::vector<voip_manager::Contact> contacts_;
        // List of instances, needs to make cascade of these windows
        static QList<IncomingCallWindow*> instances_;

        // Shadow of this widnow
        ShadowWindowParent shadow_;
#ifndef STRIP_VOIP
        FrameControl_t* rootWidget_;
#endif
        std::string vcs_conference_name_;

        QPoint posDragBegin_;
        // Search for the best position for window related of other windows.
        QPoint findBestPosition(const QPoint& _windowPosition, const QPoint& _offset);

        void showEvent(QShowEvent*) override;
        void hideEvent(QHideEvent*) override;
        void changeEvent(QEvent *) override;
    };
}
