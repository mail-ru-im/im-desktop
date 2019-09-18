#ifndef __VOIP_SYS_PANEL_HEADER_H__
#define __VOIP_SYS_PANEL_HEADER_H__
#include "VideoPanelHeader.h"
#include "AvatarContainerWidget.h"
#include "NameAndStatusWidget.h"

namespace Ui
{
    class IncomingCallControls : public BaseBottomVideoPanel
    {
        Q_OBJECT
    public:
        IncomingCallControls(QWidget* _parent);
        virtual ~IncomingCallControls();

    private Q_SLOTS:
        void _onDecline();
        void _onAudio();
        void _onVideo();


    Q_SIGNALS:
        void onDecline();
        void onAudio();
        void onVideo();

    private:
        QWidget* parent_;
        QWidget* rootWidget_;
        void changeEvent(QEvent*) override;
    };

    class VoipSysPanelHeader : public MoveablePanel
    {
        Q_OBJECT
    public:
        VoipSysPanelHeader(QWidget* _parent, int maxAvalibleWidth);
        virtual ~VoipSysPanelHeader();

        void setTitle  (const char*);
        void setStatus (const char*);

        virtual void fadeIn(unsigned int duration) override;
        virtual void fadeOut(unsigned int duration) override;

    private Q_SLOTS:
    Q_SIGNALS:
        void onMouseEnter();
        void onMouseLeave();

    private:
        bool uiWidgetIsActive() const override;

        NameAndStatusWidget* nameAndStatusContainer_;
        QWidget* rootWidget_;

        void enterEvent(QEvent* _e) override;
        void leaveEvent(QEvent* _e) override;
    };

}

#endif