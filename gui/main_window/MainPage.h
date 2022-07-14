#pragma once

#include "../utils/InterConnector.h"
#include "sidebar/Sidebar.h"
#include "types/idinfo.h"

#include "EscapeCancellable.h"
#include "contact_list/Common.h"

class QStandardItemModel;

namespace voip_manager
{
    struct Contact;
    struct ContactEx;
}

namespace core
{
    enum class add_member_failure;
}

namespace Utils
{
    enum class CommonSettingsType;
    class CallLinkCreator;
}

namespace Ui
{
    class WidgetsNavigator;
    class RecentsTab;
    class VideoWindow;
    class VideoSettings;
    class IncomingCallWindow;
    class ContactDialog;
    class GeneralSettingsWidget;
    class HistoryControlPage;
    class TopPanelWidget;
    class SemitransparentWindowAnimated;
    class MyProfilePage;
    class Splitter;
    class TabWidget;
    class SettingsTab;
    class ContactsTab;
    class CallsTab;
    class ExpandButton;
    class HeaderTitleBarButton;
    class SettingsHeader;
    class LineLayoutSeparator;
    class LoaderOverlay;
    class UpdaterButton;
    class ContextMenu;
    class EmptyConnectionInfoPage;
    enum class CreateChatSource;
    enum class ConferenceType;
    enum class MicroIssue;

    enum class TabType
    {
        Calls,
        Contacts,
        Recents,
        Settings
    };

    namespace Stickers
    {
        class Store;
    }

    class UnreadsCounter : public QWidget
    {
        Q_OBJECT

    public:
        explicit UnreadsCounter(QWidget* _parent);

    protected:
        void paintEvent(QPaintEvent* e) override;
    };

    class BackButton;

    enum class LeftPanelState
    {
        min,

        normal,
        picture_only,
        spreaded,

        max
    };

    enum voip_call_type
    {
        audio_call,
        video_call
    };

    enum class SidebarStateFlag
    {
        Closed = 0,
        Opened = 1 << 1,
        Animated = 1 << 2,
        KeepState = 1 << 3,

        CloseAnimated = Closed | Animated,
        OpenAnimated = Opened | Animated,
    };
    Q_DECLARE_FLAGS(SidebarStateFlags, SidebarStateFlag)
    Q_DECLARE_OPERATORS_FOR_FLAGS(Ui::SidebarStateFlags)

    class MainPage : public QWidget
    {
        Q_OBJECT

    public Q_SLOTS:
        void startSearchInDialog(const QString& _aimid, const QString& _pattern = {});
        void setSearchFocus();
        void setSearchTabFocus();
        bool hasSearchFocus() const;
        void settingsClicked();
        void openRecents();
        void openCalls();
        void openContacts();
        void onSidebarVisibilityChanged(bool _visible);

        void tabBarCurrentChanged(int _index);
        void onTabClicked(TabType _tabType, bool _isSameTabClicked);
        void selectTab(TabType _tab, bool _needUpdateFocus = true);
        void closeThreadInfo(const QString& _threadId);

        void onContactFilterRemoved(const QString& _aimId);
        void onScrollToNewsFeedMesssage(const QString& _aimId, int64_t _msgId, const Ui::highlightsV& _highlights);

    private:
        void searchBegin();
        void searchEnd();
        void searchCancelled();
        void onSearchInputEmpty();
        void onContactSelected(const QString& _contact);
        void onMessageSent();
        void createGroupChat(const CreateChatSource _source);
        void createChannel(const CreateChatSource _source);
        void dialogBackClicked(const bool _needHighlightInRecents);
        void sidebarBackClicked();
        // settings
        void onProfileSettingsShow(const QString& _uin);
        void onSharedProfileShow(const QString& _uin);
        void onGeneralSettingsShow(int _type);

#ifndef STRIP_VOIP
        //voip
        void onVoipShowVideoWindow(bool);
        void onVoipCallIncoming(const std::string&, const std::string&, const std::string&);
        void onVoipCallIncomingAccepted(const std::string& _callId);
        void onVoipCallDestroyed(const voip_manager::ContactEx& _contactEx);
#endif

        void post_stats_with_settings();
        void popPagesToRoot();

        void spreadCL();
        void hideRecentsPopup();
        void searchActivityChanged(bool _isActive);

        void changeCLHeadToSearchSlot();
        void changeCLHeadToUnknownSlot();
        void recentsTopPanelBack();
        void addByNick();

        void createGroupCall();
        void createCallByLink();
        void createWebinar();
        void createCallLink(ConferenceType _type);
        void callContact(const std::vector<QString>& _aimids, const QString& _friendly, bool _video);

        void compactModeChanged();
        void tabChanged(int);
        void settingsHeaderBack();
        void showSettingsHeader(const QString&);
        void hideSettingsHeader();
        void currentPageChanged(int _index);

        void tabBarRightClicked(int _index);

        void onTabClicked(int _index);

        void showStickersStore();

        void onSearchEnd();

        void expandButtonClicked();

        void onDialogClosed(const QString&, bool);
        void onSelectedContactChanged(const QString&);
        void onContactRemoved(const QString&);

        void addContactClicked();
        void readAllClicked();

        void showLoaderOverlay();
        void hideLoaderOverlay();

        void onJoinChatResultBlocked(const QString& _stamp);

        void openThread(const QString& _threadAimId, int64_t _msgId, const QString& _fromPage);
        void onSizeChanged(QSize _oldSize = {}, bool _needUpdateFocus = true);

        void onItemSelected(const QString& _aimId, qint64 _messageId, qint64 /*_threadId*/, const highlightsV& _highlights, bool _ignoreScroll);

    private:
        explicit MainPage(QWidget* _parent);

        static std::unique_ptr<MainPage, QObjectDeleteLater> _instance;

        enum class OneFrameMode
        {
            Tab,
            Content,
            Sidebar
        };

    public:
        static const QString& callsTabAccessibleName();
        static const QString& settingsTabAccessibleName();

        static MainPage* instance(QWidget* _parent = nullptr);
        static bool hasInstance();
        static void reset();
        ~MainPage();

        static int splitterHandleWidth() noexcept;
        static int pageMinWidth() noexcept;
        static int getFrame2MinWidth() noexcept;
        static int getFrame3MinWidth() noexcept;

        void selectRecentChat(const QString& _aimId);
        void recentsTabActivate(bool _selectUnread = false);
        void selectRecentsTab(bool _needUpdateFocus = true);
        void settingsTabActivate(Utils::CommonSettingsType _item = Utils::CommonSettingsType::CommonSettingsType_None);
        void onSwitchedToEmpty();
        void cancelSelection();
        void clearSearchMembers();
        void openCreatedGroupChat();

        bool isSearchActive() const;
        bool isNavigatingInRecents() const;
        void stopNavigatingInRecents();

        void updateSidebarPos(int _width);
        void updateSplitterHandle(int _width);
        bool isSideBarInSplitter() const;
        void insertSidebarToSplitter();
        void takeSidebarFromSplitter(int _widthHint);
        void removeSidebarPlaceholder();
        void chatUnreadChanged();

#ifndef STRIP_VOIP
        void raiseVideoWindow(const voip_call_type _call_type);
#endif

        void nextChat();
        void prevChat();

        void saveSidebarState();
        void restoreSidebarState();

        ContactDialog* getContactDialog() const;
        VideoWindow* getVideoWindow() const;

        void showMembersInSidebar(const QString& _aimid);
        void showSidebar(const QString& _aimId, bool _selectionChanged = false);
        void showSidebarWithParams(const QString& _aimId, SidebarParams _params = {}, bool _selectionChanged = false, bool _animate = true);
        bool isSidebarVisible() const;
        void setSidebarState(SidebarStateFlags _flags);
        void restoreSidebar();
        int getSidebarWidth() const;
        QString getSidebarAimid(Sidebar::ResolveThread _resolve = Sidebar::ResolveThread::No) const;
        bool isSidebarWithThreadPage() const;
        bool isSidebarInputInFocus() const;
        bool isSidebarInFocus(bool _checkSearchFocus = false) const;

        bool isContactDialog() const;

        bool isSearchInDialog() const;

        Q_PROPERTY(int clWidth READ getCLWidth WRITE setCLWidth)

        void setCLWidth(int _val);
        void setCLWidthNew(int _val);
        int getCLWidth() const;

        void showVideoWindow();
        void minimizeVideoWindow();
        void maximizeVideoWindow();

        void notifyApplicationWindowActive(const bool isActive);
        void notifyUIActive(const bool _isActive);

        bool isVideoWindowActive() const;
        bool isVideoWindowOn() const;
        bool isVideoWindowMinimized() const;
        bool isVideoWindowMaximized() const;
        bool isVideoWindowFullscreen() const;

        void clearSearchFocus();

        void showSemiWindow();
        void hideSemiWindow();
        bool isSemiWindowVisible() const;

        LeftPanelState getLeftPanelState() const;

        bool isOneFrameMode() const;
        bool isOneFrameTab() const;
        bool isOneFrameSidebar() const;
        bool isInSettingsTab() const;
        bool isInRecentsTab() const;
        bool isInContactsTab() const;

        void openDialogOrProfileById(const QString& _id, bool _forceDialogOpen, std::optional<QString> _botParams);

        FrameCountMode getFrameCount() const;

        void closeAndHighlightDialog();
        void showChatMembersFailuresPopup(Utils::ChatMembersOperation _operation, QString _chatAimId, std::map<core::chat_member_failure, std::vector<QString>> _failures);

        QString getSidebarSelectedText() const;

        void setOpenedAs(PageOpenedAs _openedAs);
        void scrollRecentsToTop();

    protected:
        void resizeEvent(QResizeEvent* _event) override;

        bool eventFilter(QObject* _obj, QEvent* _event) override;

    private:
        void sidebarPosWithRecents(int _width);
        void sidebarPosWithoutRecents(int _width);

        void animateVisibilityCL(int _newWidth, bool _withAnimation);
        void setLeftPanelState(LeftPanelState _newState, bool _withAnimation, bool _for_search = false, bool _force = false);

        void saveSplitterState();
        void updateSplitterStretchFactors();

        bool isUnknownsOpen() const;

        bool isPictureOnlyView() const;

        void setFrameCountMode(FrameCountMode _mode);
        void setOneFrameMode(OneFrameMode _mode);

        void updateSettingHeader();
        void updateNewMailButton();

        void switchToContentFrame();
        void switchToTabFrame();
        void switchToSidebarFrame();

        void onIdInfo(const int64_t _seq, const Data::IdInfo& _info);
        void onIdLoadingCancelled();

        void updateLoaderOverlayPosition();
        bool canSetSearchFocus() const;

        void onMoreClicked();

        void initUpdateButton();

        void resizeUpdateFocus(QWidget* _prevFocused);

        void updateSidebarWidth(int _desiredWidth);
        void saveContactListWidth();
        void restoreContactListWidth();

        bool isOpenedAsMain() const;
        bool canShowRecents() const;

    private:
        RecentsTab* recentsTab_ = nullptr;
        SettingsTab* settingsTab_  = nullptr;
        ContactsTab* contactsTab_  = nullptr;
        CallsTab* callsTab_  = nullptr;
        VideoWindow* videoWindow_  = nullptr;
        VideoSettings* videoSettings_  = nullptr;
        WidgetsNavigator* pages_  = nullptr;
        ContactDialog* contactDialog_  = nullptr;
        EmptyConnectionInfoPage* callsHistoryPage_  = nullptr;
        QVBoxLayout* pagesLayout_  = nullptr;
        GeneralSettingsWidget* generalSettings_  = nullptr;
        Stickers::Store* stickersStore_  = nullptr;
        QTimer* settingsTimer_  = nullptr;
        QPropertyAnimation* animCLWidth_  = nullptr;
        QWidget* clSpacer_  = nullptr;
        QVBoxLayout* contactsLayout_  = nullptr;
        QWidget* contactsWidget_  = nullptr;
        QVBoxLayout* clHostLayout_  = nullptr;
        LeftPanelState leftPanelState_;
        TopPanelWidget* topWidget_  = nullptr;
        QWidget* spacerBetweenHeaderAndRecents_  = nullptr;
        SemitransparentWindowAnimated* semiWindow_;
        UnreadsCounter* counter_  = nullptr;
        SettingsHeader* settingsHeader_  = nullptr;
        MyProfilePage* profilePage_  = nullptr;
        Splitter* splitter_  = nullptr;
        QWidget* clContainer_  = nullptr;
        QWidget* pagesContainer_  = nullptr;
        QWidget* sidebarPlaceholderWidget_  = nullptr;
        QPointer<Sidebar> sidebar_;
        bool sidebarVisibilityOnRecentsFrame_;
        TabWidget* tabWidget_  = nullptr;
        ExpandButton* expandButton_  = nullptr;
        LineLayoutSeparator* spreadedModeLine_  = nullptr;
        UpdaterButton* updaterButton_  = nullptr;
        bool NeedShowUnknownsHeader_;
        int currentTab_;
        bool isManualRecentsMiniMode_;
        FrameCountMode frameCountMode_;
        OneFrameMode oneFrameMode_;
        OneFrameMode prevOneFrameMode_;

        ContextMenu* moreMenu_  = nullptr;
        Utils::CallLinkCreator* callLinkCreator_  = nullptr;
        int currentCLWidth_{0};

        QString threadParentPageId_;

        struct DialogByIdLoader
        {
            LoaderOverlay* loaderOverlay_ = nullptr;
            qint64 idInfoSeq_ = -1;
            QString id_;
            std::optional<QString> botParams_;
            bool forceDialogOpen_ = false;

            void clear()
            {
                idInfoSeq_ = -1;
                id_.clear();
                botParams_ = {};
                forceDialogOpen_ = false;
            }

        } dialogIdLoader_;

        struct RecentsHeaderButtons
        {
            HeaderTitleBarButton* more_ = nullptr;
            HeaderTitleBarButton* newContacts_ = nullptr;
            HeaderTitleBarButton* newEmails_ = nullptr;
        } recentsHeaderButtons_;

        QString prevSearchInput_;

        std::vector<std::pair<int, TabType>> indexToTabs_;

        TabType getTabByIndex(int _index) const;
        int getIndexByTab(TabType _tab) const;

        TabType getCurrentTab() const;

        void selectSettings(Utils::CommonSettingsType _type);


        std::map<std::string, std::shared_ptr<IncomingCallWindow> > incomingCallWindows_;
        void destroyIncomingCallWindow(const std::string& _callId);

        PageOpenedAs openedAs_ = PageOpenedAs::MainPage;
    };
}
