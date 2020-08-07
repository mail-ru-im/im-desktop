#pragma once
#include "Sidebar.h"
#include "../../types/chat.h"

namespace Logic
{
    class ChatMembersModel;
    class ContactListItemDelegate;
}

namespace Ui
{
    class AvatarNameInfo;
    class AvatarNamePlaceholder;
    class HeaderTitleBar;
    class HeaderTitleBarButton;
    class TextLabel;
    class InfoBlock;
    class SidebarButton;
    class SidebarCheckboxButton;
    class GalleryPreviewWidget;
    class MembersPlate;
    class MembersWidget;
    class SearchWidget;
    class GalleryList;
    class ColoredButton;
    class ContactListWidget;
    class GalleryPopup;
    class EditNicknameWidget;
    class DialogButton;
    class StartCallButton;

    enum class CallType;

    enum class ActionType
    {
        POSITIVE = 0,
        NEGATIVE = 1,
    };


    class GroupProfile : public SidebarPage
    {
        Q_OBJECT
    public:
        explicit GroupProfile(QWidget* parent);
        virtual ~GroupProfile();
        void initFor(const QString& aimId, SidebarParams _params = {}) override;
        void setFrameCountMode(FrameCountMode _mode) override;
        void close() override;
        void toggleMembersList() override;
        QString getSelectedText() const override;

    protected:
        void resizeEvent(QResizeEvent* _event) override;
        void mousePressEvent(QMouseEvent* _event) override;

    private:
        void init();
        QWidget* initContent(QWidget* _parent);
        QWidget* initSettings(QWidget* _parent);
        QWidget* initContactList(QWidget* _parent);
        QWidget* initGallery(QWidget* _parent);
        void initTexts();

        void updateCloseIcon();
        void updatePinButton();

        void closeGallery();

        void changeTab(int _tab);
        void changeGalleryPage(int _page);

        void updateRegim();
        void updateChatControls();
        void hideControls();

        void blockUser(const QString& aimId, ActionType _action);
        void readOnly(const QString& aimId, ActionType _action);
        void changeRole(const QString& aimId, ActionType _action);

        void requestNickSuggest();

        void setInfoPlaceholderVisible(bool _isVisible);
        void updateGalleryVisibility();

    private Q_SLOTS:
        void chatInfo(qint64, const std::shared_ptr<Data::ChatInfo>&, const int _requestMembersLimit);
        void chatInfoFailed(qint64 _seq, core::group_chat_info_errors _errorCode, const QString& _aimId);
        void dialogGalleryState(const QString& _aimId, const Data::DialogGalleryState& _state);
        void favoriteChanged(const QString& _aimid);
        void unimportantChanged(const QString& _aimid);
        void modChatParamsResult(int _error);
        void suggestGroupNickResult(const QString& _nick);
        void loadChatInfo();

        void share();
        void shareClicked();
        void copy(const QString& _text);
        void notificationsChecked(bool _checked);
        void settingsClicked();

        void galleryPhotoCLicked();
        void galleryVideoCLicked();
        void galleryFilesCLicked();
        void galleryLinksCLicked();
        void galleryPttCLicked();
        void searchMembersClicked();
        void addToChatClicked();
        void pendingsClicked();
        void allMembersClicked();
        void adminsClicked();
        void blockedMembersClicked();
        void pinClicked();
        void themeClicked();
        void clearHistoryClicked();
        void blockClicked();
        void reportCliked();
        void leaveClicked();
        void linkClicked();
        void aboutLinkClicked();
        void linkCopy();
        void linkShare();
        void mainActionClicked();
        void makeNewLink();

        void contactSelected(const QString& _aimid);
        void contactRemoved(const QString& _aimid);
        void contactMenuRequested(const QString& _aimid);
        void contactMenuApproved(const QString& _aimid, bool _approve);

        void closeClicked();
        void editButtonClicked();

        void approveAllClicked();
        void memberMenu(QAction*);
        void menuAction(QAction*);
        void memberActionResult(int);
        void chatEvents(const QString&);
        void onContactChanged(const QString& _aimid);
        void updateMuteState();

        void avatarClicked();
        void titleArrowClicked();
        void applyChatSettings();
        void checkApplyButton();
        void publicClicked();

        void updateActiveChatMembersModel(const QString& _aimId);

    private:
        QStackedWidget* stackedWidget_;

        HeaderTitleBar* titleBar_;
        HeaderTitleBarButton* editButton_;
        HeaderTitleBarButton* closeButton_;
        QScrollArea* area_;

        QWidget* secondSpacer_;
        QWidget* thirdSpacer_;
        QWidget* fourthSpacer_;
        QWidget* fifthSpacer_;

        AvatarNameInfo* info_;
        AvatarNamePlaceholder* infoPlaceholder_;
        QWidget* infoSpacer_;
        TextLabel* groupStatus_;
        ColoredButton* mainActionButton_;
        StartCallButton* callButton_;
        std::unique_ptr<InfoBlock> about_;
        std::unique_ptr<InfoBlock> rules_;
        std::unique_ptr<InfoBlock> link_;
        SidebarButton* share_;
        SidebarCheckboxButton* notifications_;
        SidebarButton* settings_;
        QWidget* galleryWidget_;
        SidebarButton* galleryPhoto_;
        GalleryPreviewWidget* galleryPreview_;
        SidebarButton* galleryVideo_;
        SidebarButton* galleryFiles_;
        SidebarButton* galleryLinks_;
        SidebarButton* galleryPtt_;
        QWidget* membersWidget_;
        MembersPlate* members_;
        SidebarButton* addToChat_;
        SidebarButton* pendings_;
        MembersWidget* shortMembersList_;
        SidebarButton* allMembers_;
        SidebarButton* admins_;
        SidebarButton* blockedMembers_;
        SidebarButton* pin_;
        SidebarButton* approveAll_;
        SidebarButton* theme_;
        SidebarButton* clearHistory_;
        SidebarButton* block_;
        SidebarButton* report_;
        SidebarButton* leave_;
        ContactListWidget* cl_;

        SidebarCheckboxButton* public_;
        SidebarCheckboxButton* joinModeration_;
        TextLabel* publicLabel_;

        std::unique_ptr<InfoBlock> aboutLinkToChat_;
        QWidget* aboutLinkToChatBlock_;
        EditNicknameWidget* editGroupLinkWidget_;
        TextLabel* makeNewLink_;
        DialogButton* applyChatSettings_;

        GalleryList* gallery_;

        SearchWidget* searchWidget_;
        Logic::ContactListItemDelegate* delegate_;
        GalleryPopup* galleryPopup_;

        FrameCountMode frameCountMode_;
        QString currentAimId_;
        Logic::ChatMembersModel* shortChatModel_;
        Logic::ChatMembersModel* listChatModel_;
        std::shared_ptr<Data::ChatInfo> chatInfo_;

        int currentGalleryPage_;
        int currentListPage_;
        int mainAction_;

        bool galleryIsEmpty_;
        bool forceMembersRefresh_;
        bool invalidNick_;
        bool infoPlaceholderVisible_;
    };
}
