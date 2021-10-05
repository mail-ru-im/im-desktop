#pragma once
#include "Sidebar.h"
#include "../../types/chat.h"
#include "../../types/contact.h"

namespace Logic
{
    class CommonChatsModel;
    class CommonChatsSearchModel;
}

namespace Data
{
    class LastSeen;
}

namespace Ui
{
    class HeaderTitleBar;
    class HeaderTitleBarButton;
    class AvatarNameInfo;
    class AvatarNamePlaceholder;
    class InfoBlock;
    class SidebarButton;
    class SidebarCheckboxButton;
    class GalleryPreviewWidget;
    class GalleryList;
    class ColoredButton;
    class SearchWidget;
    class ContactListWidget;
    class GalleryPopup;
    enum class MediaContentType;

    class UserProfile : public SidebarPage
    {
        Q_OBJECT

    public:
        explicit UserProfile(QWidget* _parent);
        virtual ~UserProfile();

        UserProfile(QWidget* _parent, const QString& _phone, const QString& _aimid, const QString& _friendy);

        void initFor(const QString& _aimId, SidebarParams _params = {}) override;
        void setFrameCountMode(FrameCountMode _mode) override;
        void close() override;
        QString getSelectedText() const override;
        void scrollToTop() override;

    protected:
        void resizeEvent(QResizeEvent* _event) override;

    Q_SIGNALS:
        void saveClicked(QPrivateSignal);
        void copyClicked(QPrivateSignal);
        void sharingClicked(QPrivateSignal);

    private Q_SLOTS:
        void refresh();
        void enableFading();
        void disableFading();
        void editButtonClicked();
        void closeClicked();
        void avatarClicked();
        void share();
        void shareClicked();
        void copy(const QString&);
        void sharePhoneClicked();
        void notificationsChecked(bool _checked);
        void galleryPhotoClicked();
        void galleryVideoClicked();
        void galleryFilesClicked();
        void galleryLinksClicked();
        void galleryPttClicked();
        void createGroupClicked();
        void commonChatsClicked();
        void pinClicked();
        void themeClicked();
        void clearHistoryClicked();
        void blockClicked();
        void reportCliked();
        void removeContact();
        void favoriteChanged(const QString& _aimid);
        void unimportantChanged(const QString& _aimid);
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
        void changeGalleryPage(MediaContentType _page);
        void closeGallery();
        void loadInfo();
        void updateControls();
        void updateStatus(const Data::LastSeen& _lastseen);
        QString getShareLink() const;
        bool isFavorites() const;

        void setInfoPlaceholderVisible(bool _isVisible);
        void updateGalleryVisibility();

        enum class MessageOrCallMode
        {
            Message,
            Call
        };

        void switchMessageOrCallMode(MessageOrCallMode _mode);
        void hideControls();

        void switchToGallery(MediaContentType _type);

    private:
        QElapsedTimer elapsedTimer_;
        QStackedWidget* stackedWidget_;
        HeaderTitleBar* titleBar_;
        HeaderTitleBarButton* editButton_;
        HeaderTitleBarButton* closeButton_;
        QScrollArea* area_;
        QWidget* firstSpacer_;
        QWidget* secondSpacer_;
        AvatarNameInfo* info_;
        QWidget* infoContainer_;
        QWidget* infoSpacer_;
        QWidget* controlsWidget_;

        MessageOrCallMode messageOrCallMode_ = MessageOrCallMode::Message;
        ColoredButton* messageOrCall_;

        ColoredButton* audioCall_;
        ColoredButton* videoCall_;
        ColoredButton* unblock_;
        ColoredButton* openFavorites_;
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
        SidebarButton* remove_;
        SearchWidget* searchWidget_;
        ContactListWidget* cl_;
        GalleryPopup* galleryPopup_;

        QString currentAimId_;
        QString currentPhone_;
        FrameCountMode frameCountMode_;
        MediaContentType currentGalleryPage_;
        bool galleryIsEmpty_;
        bool shortView_;
        bool replaceFavorites_;
        bool isInfoPlaceholderVisible_;

        QTimer* lastseenTimer_;
        qint64 seq_;
        Data::UserInfo userInfo_;

        Logic::CommonChatsModel* commonChatsModel_;
        Logic::CommonChatsSearchModel* commonChatsSearchModel_;
    };
}
