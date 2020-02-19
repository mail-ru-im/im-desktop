#pragma once

#include "../utils/InterConnector.h"
#include "types/idinfo.h"

class QStandardItemModel;

namespace voip_manager
{
    struct Contact;
    struct ContactEx;
}

namespace Utils
{
    enum class CommonSettingsType;
}

namespace Ui
{
    class WidgetsNavigator;
    class ContactList;
    class VideoWindow;
    class VideoSettings;
    class IncomingCallWindow;
    class ContactDialog;
    class GeneralSettingsWidget;
    class HistoryControlPage;
    class LiveChatHome;
    class LiveChats;
    class TopPanelWidget;
    class SemitransparentWindowAnimated;
    class MyProfilePage;
    class Splitter;
    class Sidebar;
    class TabWidget;
    class SettingsTab;
    class ContactsTab;
    class ExpandButton;
    class HeaderTitleBarButton;
    class SettingsHeader;
    class LineLayoutSeparator;
    class LoaderOverlay;
    class ContextMenu;
    class UpdaterButton;

    enum class Tabs;
    enum class CreateChatSource;

    namespace Stickers
    {
        class Store;
        class StickersSuggest;
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

    enum class FrameCountMode
    {
        _1,
        _2,
        _3
    };

    class MainPage : public QWidget
    {
        Q_OBJECT

    public Q_SLOTS:
        void startSearchInDialog(const QString& _aimid);
        void setSearchFocus();
        void setSearchTabFocus();
        void settingsClicked();
        void openRecents();

    private Q_SLOTS:
        void searchBegin();
        void searchEnd();
        void searchCancelled();
        void onSearchInputEmpty();
        void onContactSelected(const QString& _contact);
        void onMessageSent();
        void contactsClicked();
        void createGroupChat(const CreateChatSource _source);
        void createChannel(const CreateChatSource _source);
        void dialogBackClicked(const bool _needHighlightInRecents);
        void sidebarBackClicked();
        // settings
        void onProfileSettingsShow(const QString& _uin);
        void onSharedProfileShow(const QString& _uin);
        void onGeneralSettingsShow(int _type);

        //voip
        void onVoipShowVideoWindow(bool);
        void onVoipCallIncoming(const std::string&, const std::string&);
        void onVoipCallIncomingAccepted(const voip_manager::ContactEx& _contacEx);
        void onVoipCallDestroyed(const voip_manager::ContactEx& _contactEx);

        void showPlaceholder(Utils::PlaceholdersType _placeholdersType);

        void post_stats_with_settings();
        void myInfo();
        void popPagesToRoot();
        void liveChatSelected();

        void spreadCL();
        void hideRecentsPopup();
        void searchActivityChanged(bool _isActive);

        void changeCLHeadToSearchSlot();
        void changeCLHeadToUnknownSlot();
        void recentsTopPanelBack();
        void addByNick();

        void onShowWindowForTesters();
        void compactModeChanged();
        void tabChanged(int);
        void settingsHeaderBack();
        void showSettingsHeader(const QString&);
        void hideSettingsHeader();
        void currentPageChanged(int _index);

        void tabBarCurrentChanged(int _index);
        void tabBarRightClicked(int _index);

        void tabBarClicked(int _index);

        void showStickersStore();

        void onSearchEnd();

        void loggedIn();

        void chatUnreadChanged();
        void expandButtonClicked();

        void onDialogClosed(const QString&, bool);
        void onSelectedContactChanged(const QString&);
        void onContactRemoved(const QString&);

        void addContactClicked();
        void readAllClicked();

        void showLoaderOverlay();
        void hideLoaderOverlay();

    private:
        explicit MainPage(QWidget* _parent);

        static std::unique_ptr<MainPage> _instance;

        enum class OneFrameMode
        {
            Tab,
            Content,
            Sidebar
        };

    public:
        static MainPage* instance(QWidget* _parent = nullptr);
        static void reset();
        ~MainPage();
        void selectRecentChat(const QString& _aimId);
        void recentsTabActivate(bool _selectUnread = false);
        void settingsTabActivate(Utils::CommonSettingsType _item = Utils::CommonSettingsType::CommonSettingsType_None);
        void hideInput();
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
        void takeSidebarFromSplitter();

        void raiseVideoWindow(const voip_call_type _call_type);

        void nextChat();
        void prevChat();

        ContactDialog* getContactDialog() const;
        HistoryControlPage* getHistoryPage(const QString& _aimId) const;

        void showMembersInSidebar(const QString& _aimid);
        void showSidebar(const QString& _aimId, bool _selectionChanged = false, bool _shared_profile = false);
        bool isSidebarVisible() const;
        void setSidebarVisible(bool _visible);
        void setSidebarVisible(const Utils::SidebarVisibilityParams& _params);
        void restoreSidebar();
        int getSidebarWidth() const;

        bool isContactDialog() const;

        bool isSearchInDialog() const;

        Q_PROPERTY(int clWidth READ getCLWidth WRITE setCLWidth)

        void setCLWidth(int _val);
        void setCLWidthNew(int _val);
        int getCLWidth() const;

        void showVideoWindow();
        void minimizeVideoWindow();
        void maximizeVideoWindow();
        void closeVideoWindow();

        void notifyApplicationWindowActive(const bool isActive);
        void notifyUIActive(const bool _isActive);

        bool isVideoWindowActive() const;
        bool isVideoWindowOn() const;
        bool isVideoWindowMinimized() const;
        bool isVideoWindowMaximized() const;
        bool isVideoWindowFullscreen() const;

        void setFocusOnInput();
        void clearSearchFocus();

        void onSendMessage(const QString& contact);

        bool isSuggestVisible() const;
        void hideSuggest();
        Stickers::StickersSuggest* getStickersSuggest();

        void showSemiWindow();
        void hideSemiWindow();
        bool isSemiWindowVisible() const;

        LeftPanelState getLeftPanelState() const;

        bool isOneFrameTab() const;

        void openDialogOrProfileById(const QString& _id);

        FrameCountMode getFrameCount() const;

        void closeAndHighlightDialog();

    protected:
        void resizeEvent(QResizeEvent* _event) override;

        bool eventFilter(QObject* _obj, QEvent* _event) override;

    private:

        QWidget* showIntroduceYourselfSuggestions(QWidget* _parent);
        void animateVisibilityCL(int _newWidth, bool _withAnimation);
        void setLeftPanelState(LeftPanelState _newState, bool _withAnimation, bool _for_search = false, bool _force = false);

        void saveSplitterState();
        void updateSplitterStretchFactors();

        void initAccessability();

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

    private:
        ContactList* contactListWidget_;
        VideoWindow* videoWindow_;
        VideoSettings* videoSettings_;
        WidgetsNavigator* pages_;
        ContactDialog* contactDialog_;
        QVBoxLayout* pagesLayout_;
        GeneralSettingsWidget* generalSettings_;
        Stickers::Store* stickersStore_;
        LiveChatHome* liveChatsPage_;
        QHBoxLayout* horizontalLayout_;
        QWidget* introduceYourselfSuggestions_;
        bool needShowIntroduceYourself_;
        QTimer* settingsTimer_;
        bool recvMyInfo_;
        QPropertyAnimation* animCLWidth_;
        QWidget* clSpacer_;
        QVBoxLayout* contactsLayout;
        QWidget* contactsWidget_;
        QVBoxLayout* clHostLayout_;
        LeftPanelState leftPanelState_;
        TopPanelWidget* topWidget_;
        QWidget* spacerBetweenHeaderAndRecents_;
        SemitransparentWindowAnimated* semiWindow_;
        UnreadsCounter* counter_;
        SettingsHeader* settingsHeader_;
        MyProfilePage* profilePage_;
        Splitter* splitter_;
        QWidget* clContainer_;
        QWidget* pagesContainer_;
        Sidebar* sidebar_;
        TabWidget* tabWidget_;
        ExpandButton* expandButton_;
        SettingsTab* settingsTab_;
        ContactsTab* contactsTab_;
        LineLayoutSeparator* spreadedModeLine_;
        UpdaterButton* updaterButton_;
        bool NeedShowUnknownsHeader_;
        int currentTab_;
        bool isManualRecentsMiniMode_;
        FrameCountMode frameCountMode_;
        OneFrameMode oneFrameMode_;

        LoaderOverlay* loaderOverlay_;
        qint64 idInfoSeq_;
        QString requestedId_;

        struct UnreadCounters
        {
            qint64 unreadMessages_ = 0;
            qint64 unreadEmails_ = 0;
            bool attention_ = false;
        } unreadCounters_;

        struct RecentsHeaderButtons
        {
            HeaderTitleBarButton* more_ = nullptr;
            HeaderTitleBarButton* newContacts_ = nullptr;
            HeaderTitleBarButton* newEmails_ = nullptr;
        } recentsHeaderButtons_;

        QString prevSearchInput_;

        std::vector<std::pair<int, Tabs>> indexToTabs_;

        ContextMenu* moreMenu_;

        Tabs getTabByIndex(int _index) const;
        int getIndexByTab(Tabs _tab) const;

        Tabs getCurrentTab() const;

        void selectSettings(Utils::CommonSettingsType _type);

        void selectTab(Tabs _tab);

        std::map<std::string, std::shared_ptr<IncomingCallWindow> > incomingCallWindows_;
        LiveChats* liveChats_;
        void destroyIncomingCallWindow(const std::string& _account, const std::string& _contact);
    };
}
