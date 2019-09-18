#pragma once
#include "Sidebar.h"
#include "../../types/chat.h"
#include "../../types/contact.h"

namespace Logic
{
    class CommonChatsModel;
    class CommonChatsSearchModel;
}

namespace Ui
{
    class HeaderTitleBar;
    class HeaderTitleBarButton;
    class AvatarNameInfo;
    class InfoBlock;
    class SidebarButton;
    class SidebarCheckboxButton;
    class GalleryPreviewWidget;
    class GalleryList;
    class ColoredButton;
    class SearchWidget;
    class ContactListWidget;
    class GalleryPopup;

    class UserProfile : public SidebarPage
    {
        Q_OBJECT
    Q_SIGNALS:
        void headerBackButtonClicked();

        void saveClicked();
        void copyClicked();
        void sharingClicked();

    public:
        explicit UserProfile(QWidget* parent);
        virtual ~UserProfile();

        UserProfile(QWidget* _parent, const QString& _phone, const QString& _aimid, const QString& _friendy);

        void initFor(const QString& aimId) override;
        void setSharedProfile(bool _sharedProfile) override;
        void setFrameCountMode(FrameCountMode _mode) override;
        void close() override;

    protected:
        void resizeEvent(QResizeEvent* _event) override;

    private Q_SLOTS:
        void editButtonClicked();
        void closeClicked();
        void avatarClicked();
        void share();
        void shareClicked();
        void copy(const QString&);
        void sharePhoneClicked();
        void notificationsChecked(bool _checked);
        void galleryPhotoCLicked();
        void galleryVideoCLicked();
        void galleryFilesCLicked();
        void galleryLinksCLicked();
        void galleryPttCLicked();
        void createGroupClicked();
        void commonChatsClicked();
        void pinClicked();
        void themeClicked();
        void clearHistoryClicked();
        void blockClicked();
        void reportCliked();
        void favoriteChanged(const QString _aimid);
        void dialogGalleryState(const QString& _aimId, const Data::DialogGalleryState& _state);
        void userInfo(const int64_t, const QString& _aimid, const Data::UserInfo& _info);
        void menuAction(QAction*);
        void messageClicked();
        void audioCallClicked();
        void videoCallClicked();
        void unblockClicked();
        void updateLastSeen();
        void updateNick();
        void connectionStateChanged();
        void shortViewAvatarClicked();
        void chatSelected(const QString& _aimid);
        void recvPermitDeny(bool);
        void emailClicked();
        void emailCopy(const QString&);
        void nickCopy(const QString&);
        void nickShare();
        void nickClicked();
        void phoneCopy(const QString&);
        void phoneClicked();
        void contactChanged(const QString&);
        void titleArrowClicked();
        void lastseenChanged(const QString& _aimid);
        void updateMuteState();
        void friendlyChanged(const QString& _aimid, const QString& _friendly);

    private:
        void init();
        void initShort();
        QWidget* initContent(QWidget* _parent);
        QWidget* initGallery(QWidget* _parent);
        QWidget* initCommonChats(QWidget* _parent);
        void updateCloseButton();
        void updatePinButton();
        void changeTab(int _tab);
        void changeGalleryPage(int _page);
        void closeGallery();
        void loadInfo();
        void updateControls();
        void updateStatus(int32_t _lastseen);

    private:
        QStackedWidget* stackedWidget_;
        HeaderTitleBar* titleBar_;
        HeaderTitleBarButton* editButton_;
        HeaderTitleBarButton* closeButton_;
        QScrollArea* area_;
        QWidget* firstSpacer_;
        QWidget* secondSpacer_;
        AvatarNameInfo* info_;
        QWidget* infoSpacer_;
        QWidget* controlsWidget_;
        ColoredButton* message_;
        ColoredButton* audioCall_;
        ColoredButton* videoCall_;
        ColoredButton* unblock_;
        std::unique_ptr<InfoBlock> about_;
        std::unique_ptr<InfoBlock> nick_;
        std::unique_ptr<InfoBlock> email_;
        std::unique_ptr<InfoBlock> phone_;
        SidebarButton* share_;
        SidebarCheckboxButton* notifications_;
        QWidget* galleryWidget_;
        GalleryList* gallery_;
        SidebarButton* galleryPhoto_;
        GalleryPreviewWidget* galleryPreview_;
        SidebarButton* galleryVideo_;
        SidebarButton* galleryFiles_;
        SidebarButton* galleryLinks_;
        SidebarButton* galleryPtt_;
        SidebarButton* generalGroups_;
        SidebarButton* createGroup_;
        SidebarButton* pin_;
        SidebarButton* theme_;
        SidebarButton* clearHistory_;
        SidebarButton* block_;
        SidebarButton* report_;
        SearchWidget* searchWidget_;
        ContactListWidget* cl_;
        GalleryPopup* galleryPopup_;

        QString currentAimId_;
        QString currentPhone_;
        FrameCountMode frameCountMode_;
        int currentGalleryPage_;
        bool galleryIsEmpty_;
        bool shortView_;
        bool sharedProfile_;

        QTimer* lastseenTimer_;
        qint64 seq_;
        Data::UserInfo userInfo_;

        Logic::CommonChatsModel* commonChatsModel_;
        Logic::CommonChatsSearchModel* commonChatsSearchModel_;
    };
}
