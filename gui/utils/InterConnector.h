#pragma once

#include "../utils/keyboard.h"
#include "../utils/utils.h"

namespace Ui
{
    class MainWindow;
    class HistoryControlPage;
    class ContactDialog;
    class MainPage;
}

namespace Data
{
    class ChatInfo;
    class DlgState;
}

namespace Logic
{
    class SearchModel;
}

namespace Utils
{
    enum class CommonSettingsType
    {
        min = -1,

        CommonSettingsType_None,
        CommonSettingsType_Profile,
        CommonSettingsType_General,
        CommonSettingsType_VoiceVideo,
        CommonSettingsType_Notifications,
        CommonSettingsType_Security,
        CommonSettingsType_Appearance,
        CommonSettingsType_About,
        CommonSettingsType_ContactUs,
        CommonSettingsType_AttachPhone,
        CommonSettingsType_Language,
        CommonSettingsType_Updater,
        CommonSettingsType_Shortcuts,
        CommonSettingsType_Stickers,

        max
    };

    enum class PlaceholdersType
    {
        min = -1,

        PlaceholdersType_IntroduceYourself,
        PlaceholdersType_SetExistanseOnIntroduceYourself,
        PlaceholdersType_SetExistanseOffIntroduceYourself,

        max
    };

    class InterConnector : public QObject
    {
        Q_OBJECT

Q_SIGNALS:
        void profileSettingsShow(const QString& uin);
        void sharedProfileShow(const QString& uin);
        void profileSettingsBack();

        void needJoinLiveChatByStamp(const QString& _stamp);
        void needJoinLiveChatByAimId(const QString& _aimId);

        void themesSettingsOpen();

        void generalSettingsShow(int type);
        void generalSettingsBack();

        void profileSettingsUpdateInterface();

        void generalSettingsContactUsShown();

        void attachPhoneBack();
        void attachUinBack();

        void makeSearchWidgetVisible(bool);
        void showIconInTaskbar(bool _show);

        void popPagesToRoot();

        void showPlaceholder(PlaceholdersType);
        void showContactListPlaceholder();
        void hideContactListPlaceholder();

        void showRecentsPlaceholder();
        void hideRecentsPlaceholder();

        void showDialogPlaceholder();
        void hideDialogPlaceholder();

        void disableSearchInDialog();
        void repeatSearch();
        void dialogClosed(const QString& _aimid, bool _isCurrent);

        void resetSearchResults();

        void closeAnyPopupWindow(const Utils::CloseWindowInfo& _info);
        void closeAnySemitransparentWindow(const Utils::CloseWindowInfo& _info);
        void closeAnyPopupMenu();
        void acceptGeneralDialog();
        void mainWindowResized();

        void forceRefreshList(QAbstractItemModel *, bool);
        void updateFocus();
        void liveChatsShow();

        void schemeUrlClicked(const QString&);

        void setAvatar(qint64 _seq, int error);
        void setAvatarId(const QString&);

        void historyControlPageFocusIn(const QString&);

        void unknownsGoSeeThem();
        void unknownsGoBack();
        void unknownsDeleteThemAll();

        void liveChatSelected();
        void showLiveChat(std::shared_ptr<Data::ChatInfo> _info);

        void activateNextUnread();

        void clSortChanged();

        void historyControlReady(const QString&, qint64 _message_id, const Data::DlgState&, qint64 _last_read_msg, bool _isFirstRequest);
        void logHistory(const QString&);
        void chatEvents(const QString&, const QVector<HistoryControl::ChatEventInfoSptr>&) const;

        void imageCropDialogIsShown(QWidget *);
        void imageCropDialogIsHidden(QWidget *);
        void imageCropDialogMoved(QWidget *);
        void imageCropDialogResized(QWidget *);

        void startSearchInDialog(const QString&);
        void setSearchFocus();

        void searchEnd();
        void searchClosed();
        void myProfileBack();

        void compactModeChanged();
        void mailBoxOpened();

        void logout();
        void authError(const int _error);
        void contacts();
        void showSettingsHeader(const QString&);
        void hideSettingsHeader();
        void headerBack();

        void currentPageChanged();

        void showHeads();
        void hideHeads();

        void updateTitleButtons();
        void hideTitleButtons();
        void titleButtonsUpdated();

        void attachFilePopupVisiblityChanged(const bool _isVisible);

        void hideMentionCompleter();
        void showStickersStore();

        void hideSearchDropdown();
        void showSearchDropdownAddContact();
        void showSearchDropdownFull();

        void addPageToDialogHistory(const QString& _aimid);
        void clearDialogHistory();
        void closeLastDialog(const bool _needHighlightInRecents);
        void pageAddedToDialogHistory();
        void noPagesInDialogHistory();

        void cancelEditing();
        void clearSelecting();

        void gotServiceUrls(const QVector<QString>& _urlsVector);

        void activationChanged(bool _isActiveNow) const;
        void applicationDeactivated() const;
        void applicationActivated() const; // currently os x only

        void applicationLocked();
        void downloadPathUpdated();

        void chatFontParamsChanged();
        void emojiSizeChanged();

        void stopPttRecord();

        void openDialogOrProfileById(const QString& _id);

        void loaderOverlayShow();
        void loaderOverlayHide();
        void loaderOverlayCancelled();

    public:
        static InterConnector& instance();
        ~InterConnector();

        void setMainWindow(Ui::MainWindow* window);
        void startDestroying();
        Ui::MainWindow* getMainWindow(bool _check_destroying = false) const;
        Ui::HistoryControlPage* getHistoryPage(const QString& aimId) const;
        Ui::ContactDialog* getContactDialog() const;
        Ui::MainPage* getMainPage() const;

        bool isInBackground() const;

        void showSidebar(const QString& aimId);
        void showMembersInSidebar(const QString& aimId);
        void setSidebarVisible(bool _visible);
        void setSidebarVisible(const SidebarVisibilityParams& _params);
        bool isSidebarVisible() const;
        void restoreSidebar();

        void setDragOverlay(bool enable);
        bool isDragOverlay() const;

        void setFocusOnInput();
        void onSendMessage(const QString&);

        void registerKeyCombinationPress(QKeyEvent* event, qint64 time = QDateTime::currentMSecsSinceEpoch());
        void registerKeyCombinationPress(KeyCombination keyComb, qint64 time = QDateTime::currentMSecsSinceEpoch());
        qint64 lastKeyEventMsecsSinceEpoch(KeyCombination keyCombination) const;

    private:
        InterConnector();

        InterConnector(InterConnector&&);
        InterConnector(const InterConnector&);
        InterConnector& operator=(const InterConnector&);

        std::unordered_map<KeyCombination, qint64> keyCombinationPresses_;
        Ui::MainWindow* MainWindow_;
        bool dragOverlay_;
        bool destroying_;
    };
}
