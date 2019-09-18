#ifndef __VIDEO_PANEL_HEADER_H__
#define __VIDEO_PANEL_HEADER_H__

#include "NameAndStatusWidget.h"
#include "WindowHeaderFormat.h"
#include "CommonUI.h"

namespace voip_manager
{
    struct ContactEx;
    struct Contact;
}

namespace Ui
{
    std::string getFotmatedTime(unsigned _ts);

    //class videoPanelHeader;
    class VideoPanelHeader : public BaseTopVideoPanel
    {
    Q_OBJECT

    Q_SIGNALS:
        void onMouseEnter();
        void onMouseLeave();
        void onSecureCallClicked(const QRect& _rc);
        void updateConferenceMode(voip_manager::VideoLayout layout);
        void addUserToConference();
        void onPlaybackClick();

    private Q_SLOTS:

        void _onSecureCallClicked();
        void onChangeConferenceMode();        
        void onPlaybackAudioOnOffClicked();
        void onClickSettings();
        void onVoipVolumeChanged(const std::string& _deviceType, int _vol);
        void onAddUserClicked();

    public:
        VideoPanelHeader(QWidget* _parent, int offset);
        virtual ~VideoPanelHeader();

        void setSecureWndOpened(const bool _opened);
        void enableSecureCall(bool _enable);
        void changeConferenceMode(voip_manager::VideoLayout layout);
        void switchConferenceMode();

        virtual void fadeIn(unsigned int duration)  override;
        virtual void fadeOut(unsigned int duration) override;
         
        void setContacts(const std::vector<voip_manager::Contact>&);
        void setTopOffset(int offset);

    public Q_SLOTS:
        
        void onCreateNewCall();
        void onStartedTalk();

    private:

        QWidget*     lowWidget_;
        bool         secureCallEnabled_;
        bool         isVideoConference_;
        bool         startTalking_;

        QPushButton* _backToVideoCode;
        int          topOffset_;
        QPushButton* conferenceMode_;
        QPushButton* security_;
        QPushButton* speaker_;
        QPushButton* addUsers_;

        //std::unique_ptr<UIEffects> rootEffect_;

        void enterEvent(QEvent* _e) override;
        void leaveEvent(QEvent* _e) override;
        void updatePosition(const QWidget& parent) override;

        void updateSecurityButtonState();
    };

}

#endif//__VIDEO_PANEL_HEADER_H__