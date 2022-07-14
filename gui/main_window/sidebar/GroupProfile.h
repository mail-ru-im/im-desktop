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
    enum class MediaContentType;

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
        void initFor(const QString& _aimId, SidebarParams _params = {}) override;
        void setFrameCountMode(FrameCountMode _mode) override;
        void close() override;
        void setMembersVisible(bool _on) override;
        bool isMembersVisible() const override;
        QString getSelectedText() const override;
        void scrollToTop() override;

    protected:
        void resizeEvent(QResizeEvent* _event) override;
        void mousePressEvent(QMouseEvent* _event) override;
        void showEvent(QShowEvent* _event) override;

    private:
        void init();
        QWidget* initContent(QWidget* _parent);
        QWidget* initThreadInfo(QWidget* _parent);
        QWidget* initSettings(QWidget* _parent);
        QWidget* initContactList(QWidget* _parent);
        QWidget* initGallery(QWidget* _parent);
        void initTexts();
        QString pageTitle(int _pageIndex) const;

        void updateCloseIcon();

        void closeGallery();

        void changeTab(int _tab);
        void changeGalleryPage(MediaContentType _page);

        void updateRegim();
        void updateChatControls();
        void hideControls();

        void blockUser(const QString& aimId, ActionType _action);
        void readOnly(const QString& aimId, ActionType _action);
        void changeRole(const QString& aimId, ActionType _action);
        void removeInvite(const QString& aimId);
        void deleteUser(const QString& aimId);

        void requestNickSuggest();

        void setInfoPlaceholderVisible(bool _isVisible);
        void updateGalleryVisibility();
        void updateSettingsCheckboxes();

        bool isThread() const;
        bool isChannel() const;

    private Q_SLOTS:
        void refresh();
        void enableFading();
        void disableFading();
        void chatInfo(qint64, const std::shared_ptr<Data::ChatInfo>&, const int _requestMembersLimit);
        void chatInfoFailed(qint64 _seq, core::group_chat_info_errors _errorCode, const QString& _aimId);
        void threadInfo(qint64, const Data::ThreadInfo& _threadInfo);
        void dialogGalleryState(const QString& _aimId, const Data::DialogGalleryState& _state);
        void favoriteChanged(const QString& _aimid);
        void unimportantChanged(const QString& _aimid);
        void modChatParamsResult(int _error);
        void suggestGroupNickResult(const QString& _nick);
        void loadChatInfo();
        void loadGroupInfo();
        void loadThreadInfo(bool _subscription = true);
        void onThreadAutosubscribe(int _error);

        void share();
        void shareClicked();
        void copy(const QString& _text);
        void notificationsChecked(bool _checked);
        void threadSubscriptionChecked(bool _checked);
        void threadAllSubscriptionChecked(bool _checked);
        void settingsClicked();

        void galleryPhotoClicked();
        void galleryVideoClicked();
        void galleryFilesClicked();
        void galleryLinksClicked();
        void galleryPttClicked();
        void searchMembersClicked();
        void addToChatClicked();
        void pendingsClicked();
        void yourInvitesClicked();
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
        void memberActionResult();
        void chatEvents(const QString&);
        void onContactChanged(const QString& _aimid);
        void onRoleChanged(const QString& _aimId);
        void updateMuteState();

        void avatarClicked();
        void titleArrowClicked();
        void applyChatSettings();
        void checkApplyButton();
        void publicClicked();
        void opposePublicTrustRequired();

        void updateActiveChatMembersModel(const QString& _aimId);

        void switchToGallery(MediaContentType _type);

        bool validateTrustRequiredCheck(bool _desiredValue);

        void updatePinButton();

    private:
        QElapsedTimer elapsedTimer_;
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
        QWidget* infoSpacer_;
        TextLabel* groupStatus_;
        QWidget* buttonsWidget_;
        ColoredButton* mainActionButton_;
        StartCallButton* callButton_;
        std::unique_ptr<InfoBlock> about_;
        std::unique_ptr<InfoBlock> rules_;
        std::unique_ptr<InfoBlock> link_;
        SidebarButton* share_;
        SidebarCheckboxButton* notifications_;
        SidebarCheckboxButton* threadSubscription_;
        SidebarCheckboxButton* threadsAutoSubscribe_;
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
        MembersPlate* threadMembers_;
        SidebarButton* addToChat_;
        SidebarButton* pendings_;
        SidebarButton* yourInvites_;
        MembersWidget* shortMembersList_;
        MembersWidget* threadMembersList_;
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
        ContactListWidget* cl_ = nullptr;

        SidebarCheckboxButton* public_ = nullptr;
        SidebarCheckboxButton* trustRequired_ = nullptr;
        SidebarCheckboxButton* joinModeration_ = nullptr;
        SidebarCheckboxButton* threadsEnabled_ = nullptr;
        TextLabel* approvalLabel_ = nullptr;
        TextLabel* publicLabel_ = nullptr;
        TextLabel* trustLabel_ = nullptr;

        std::unique_ptr<InfoBlock> aboutLinkToChat_;
        QWidget* aboutLinkToChatBlock_;
        EditNicknameWidget* editGroupLinkWidget_;
        TextLabel* makeNewLink_;
        DialogButton* applyChatSettings_;

        GalleryList* gallery_;

        SearchWidget* searchWidget_;
        Logic::ContactListItemDelegate* delegate_;
        Logic::ContactListItemDelegate* threadDelegate_;
        GalleryPopup* galleryPopup_;

        FrameCountMode frameCountMode_;
        QString currentAimId_;
        Logic::ChatMembersModel* shortChatModel_;
        Logic::ChatMembersModel* listChatModel_;
        std::shared_ptr<Data::ChatInfo> chatInfo_;

        MediaContentType currentGalleryPage_;
        int currentListPage_;
        int mainAction_;

        bool galleryIsEmpty_;
        bool invalidNick_;
        bool infoPlaceholderVisible_;
        bool stateAllThreadsAutoSubscribe_ = false;
    };
}
