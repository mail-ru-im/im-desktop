#pragma once
#include "VideoWindow.h"
#include "renders/OGLRender.h"
#include "CommonUI.h"

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

    private Q_SLOTS:
        void onVoipCallNameChanged(const voip_manager::ContactsList&);

        void onDeclineButtonClicked();
        void onAcceptVideoClicked();
        void onAcceptAudioClicked();

        void updateTitle();

    public:
        IncomingCallWindow(const std::string& call_id, const std::string& _contact, const std::string& call_type);
        ~IncomingCallWindow();

        void showFrame();
        void hideFrame();

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
        IRender* rootWidget_;
#endif
        std::string vcs_conference_name_;

        // Search for the best position for window related of other windows.
        QPoint findBestPosition(const QPoint& _windowPosition, const QPoint& _offset);

        void showEvent(QShowEvent*) override;
        void hideEvent(QHideEvent*) override;
        void changeEvent(QEvent *) override;
        void setFrameless();

        QTimer* incomingAlarmTimer_;
    };
}
