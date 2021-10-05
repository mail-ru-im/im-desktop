#pragma once

#include "../utils/keyboard.h"
#include "../utils/utils.h"
#include "../main_window/history_control/complex_message/FileSharingUtils.h"

namespace core
{
    enum class add_member_failure;
}

namespace Ui
{
    class MainWindow;
    class HistoryControlPage;
    class ContactDialog;
    class MainPage;
    class AppsPage;
    struct SidebarParams;
    class DialogPlayer;
    class core_dispatcher;

    enum class FrameCountMode
    {
        _1,
        _2,
        _3
    };
}

namespace Data
{
    class ChatInfo;
    class DlgState;
    class AuthParams;
}

namespace Statuses
{
    class Status;
}

namespace Logic
{
    class SearchModel;
}

namespace Statuses
{
    class Status;
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
        CommonSettingsType_Favorites,
        CommonSettingsType_Debug,
        CommonSettingsType_Sessions,

        max
    };

    enum class MultiselectCurrentElement
    {
        Cancel = 0,
        Message,
        Delete,
        Favorites,
        Copy,
        Reply,
        Forward
    };

    enum class MacUpdateState
    {
        Ready = 0,
        Requested,
        LoadError,
        NotFound
    };

    enum class ChatMembersOperation
    {
        Add,
        Remove,
        Block,
        Unblock
    };

    struct GalleryData
    {
        GalleryData(const QString& _aimid, const QString& _link, int64_t _msgId, Ui::DialogPlayer* _attachedPlayer = nullptr, QPixmap _preview = QPixmap(), QSize _originalSize = QSize())
            : aimId_(_aimid)
            , link_(_link)
            , msgId_(_msgId)
            , attachedPlayer_(_attachedPlayer)
            , preview_(std::move(_preview))
            , originalSize_(_originalSize)
            , time_(-1)
            , fromThreadFeed_(false)
        {
        }

        GalleryData(const QString& _aimid, const QString& _link, int64_t _msgId, const QString& _sender, time_t _time)
            : aimId_(_aimid)
            , link_(_link)
            , sender_(_sender)
            , msgId_(_msgId)
            , attachedPlayer_(nullptr)
            , time_(_time)
            , fromThreadFeed_(false)
        {
        }

        QString aimId_;
        QString link_;
        QString sender_;
        QString caption_;
        int64_t msgId_;
        Ui::DialogPlayer* attachedPlayer_;
        QPixmap preview_;
        QSize originalSize_;
        time_t time_;
        bool fromThreadFeed_;
    };

    class InterConnector : public QObject
    {
        Q_OBJECT

    Q_SIGNALS:
        void profileSettingsShow(const QString& uin);
        void sharedProfileShow(const QString& uin);
        void profileSettingsBack();

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

        void showContactListPlaceholder();
        void hideContactListPlaceholder();

        void showRecentsPlaceholder();
        void hideRecentsPlaceholder();

        void disableSearchInDialog();
        void repeatSearch();
        void dialogClosed(const QString& _aimid, bool _isCurrent);

        void resetSearchResults();

        void closeAnyPopupWindow(const Utils::CloseWindowInfo& _info);
        void closeAnySemitransparentWindow(const Utils::CloseWindowInfo& _info);
        void closeSidebar();
        void closeAnyPopupMenu();
        void acceptGeneralDialog();
        void mainWindowResized();

        void forceRefreshList(QAbstractItemModel *, bool);

        void schemeUrlClicked(const QString&);

        void setAvatar(qint64 _seq, int error);
        void setAvatarId(const QString&);

        void historyControlPageFocusIn(const QString&);

        void unknownsGoSeeThem();
        void unknownsGoBack();
        void unknownsDeleteThemAll();

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
        void setContactSearchFocus();

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

        void hideMentionCompleter();
        void showStickersStore();
        void showStickersPicker();

        void hideSearchDropdown();
        void showSearchDropdownAddContact();
        void showSearchDropdownFull();

        void addPageToDialogHistory(const QString& _aimid);
        void clearDialogHistory();
        void closeLastDialog(const bool _needHighlightInRecents);
        void pageAddedToDialogHistory();
        void noPagesInDialogHistory();

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

        void openDialogOrProfileById(const QString& _id, bool _forceDialogOpen = false, std::optional<QString> _botParams = {});
        void phoneAttached(bool);
        void phoneAttachmentCancelled();

        void loaderOverlayShow();
        void loaderOverlayHide();
        void loaderOverlayCancelled();
        void multiselectChanged();

        void multiselectDelete(const QString& _aimid);
        void multiselectFavorites(const QString& _aimid);
        void multiselectCopy(const QString& _aimid);
        void multiselectReply(const QString& _aimid);
        void multiselectForward(const QString& _aimid);

        void multiSelectCurrentElementChanged();
        void multiSelectCurrentMessageChanged();

        void multiSelectCurrentMessageUp(bool);
        void multiSelectCurrentMessageDown(bool);

        void multiselectSpaceClicked();

        void updateSelectedCount(const QString& _aimid);
        void messageSelected(qint64, const QString&);
        void selectedCount(const QString& _aimId, int _totalCount, int _unsupported, int _plainFiles);
        void multiselectAnimationUpdate();
        void selectionStateChanged(const QString& _aimid, qint64, const QString&, bool);

        void clearInputText();
        void showPendingMembers();

        void fullPhoneNumber(const QString& _code, const QString& _number);
        void addByNick();

        void showDebugSettings();

        void sendBotCommand(const QString& _command);
        void startBot();

        void historyInsertedMessages(const QString& _aimid);
        void historyCleared(const QString& _aimid);
        void historyReady(const QString& _aimid);

        void workspaceChanged();

        void updateActiveChatMembersModel(const QString& _aimId);

        void createGroupCall();
        void createCallByLink();
        void createWebinar();
        void callContact(const std::vector<QString>& _aimids, const QString& _frienly, bool _video);

        void omicronUpdated();

        void openStatusPicker();
        void changeMyStatus(const Statuses::Status& _status);
        void updateWhenUserInactive();
        void onMacUpdateInfo(MacUpdateState _state);
        void trayIconThemeChanged();

        void addReactionPlateActivityChanged(const QString& _contact, bool _active);

        void showAddToDisallowedInvitersDialog();

        void pttProgressChanged(qint64, const Utils::FileSharingId&, int);

        void authParamsChanged(bool _aimsidChanged);

        void setFocusOnInput(const QString& _contact = {});
        void clearInputQuotes(const QString& _contact);

        void messageSent(const QString& _contact);
        void inputTextUpdated(const QString& _contact);
        void reloadDraft(const QString& _contact);

        void filesWidgetOpened(const QString& _contact);

        void openThread(const QString& _aimid, int64_t _msgId, const QString& _fromPage = QString());
        void pinnedItemClicked(Data::DlgState::PinnedServiceItemType _type);

        void webAppTasksSelected();
        void sidebarVisibilityChanged(bool _visible);

        void openDialogGallery(const QString& _aimid);

        void scrollThreadFeedToMsg(const QString& _threadId, int64_t _msgId);
        void scrollThreadToMsg(const QString& _threadId, int64_t _msgId);

    public:
        static InterConnector& instance();
        ~InterConnector();

        void clearInternalCaches();

        void setMainWindow(Ui::MainWindow* window);
        void startDestroying();
        Ui::MainWindow* getMainWindow(bool _check_destroying = false) const;

        Ui::HistoryControlPage* getHistoryPage(const QString& _aimId) const;
        Ui::PageBase* getPage(const QString& _aimId) const;
        void addPage(Ui::PageBase* _page);
        void removePage(Ui::PageBase* _page);
        std::vector<QPointer<Ui::HistoryControlPage>> getVisibleHistoryPages() const;
        bool isHistoryPageVisible(const QString& _aimId) const;

        Ui::ContactDialog* getContactDialog() const;
        Ui::AppsPage* getAppsPage() const;
        Ui::MainPage* getMessengerPage() const;

        bool isInBackground() const;

        void showSidebar(const QString& aimId);
        void showSidebarWithParams(const QString& aimId, Ui::SidebarParams _params);
        void showMembersInSidebar(const QString& aimId);
        void setSidebarVisible(bool _visible);
        bool isSidebarVisible() const;
        void restoreSidebar();
        QString getSidebarAimid() const;
        QString getSidebarSelectedText() const;

        void setDragOverlay(bool enable);
        bool isDragOverlay() const;

        void registerKeyCombinationPress(QKeyEvent* event, qint64 time = QDateTime::currentMSecsSinceEpoch());
        void registerKeyCombinationPress(KeyCombination keyComb, qint64 time = QDateTime::currentMSecsSinceEpoch());
        qint64 lastKeyEventMsecsSinceEpoch(KeyCombination keyCombination) const;

        void setMultiselect(bool _enable, const QString& _contact = QString(), bool _fromKeyboard = false);
        bool isMultiselect(const QString& _contact) const;
        bool isMultiselect() const;

        MultiselectCurrentElement currentMultiselectElement() const;
        void multiselectNextElementTab();
        void multiselectNextElementRight();
        void multiselectNextElementLeft();

        void multiselectNextElementUp(bool _shift);
        void multiselectNextElementDown(bool _shift);

        void multiselectSpace();
        void multiselectEnter();

        qint64 currentMultiselectMessage() const;
        void setCurrentMultiselectMessage(qint64 _id);

        double multiselectAnimationCurrent() const;

        void disableInMultiselect(QWidget* _w, const QString& _aimid);
        void detachFromMultiselect(QWidget* _w);

        void clearPartialSelection(const QString& _aimid);

        void openGallery(const GalleryData& _data);

        bool isRecordingPtt(const QString& _aimId) const;
        bool isRecordingPtt() const;

        void showChatMembersFailuresPopup(ChatMembersOperation _operation, QString _chatAimId, std::map<core::chat_member_failure, std::vector<QString>> _failures);

        void connectTo(Ui::core_dispatcher*);

        std::pair<QUrl, bool> signUrl(const QString& _miniappId, QUrl _url) const;
        bool isAuthParamsValid() const;
        bool isMiniAppAuthParamsValid(const QString& _miniappId) const;

        void openDialog(const QString& _aimId, qint64 _id = -1, bool _select = true, std::function<void(Ui::PageBase*)> _getPageCallback = {}, bool _ignoreScroll = false);
        void closeDialog();

        void closeAndHighlightDialog();

    private:
        InterConnector();

        InterConnector(InterConnector&&);
        InterConnector(const InterConnector&);
        InterConnector& operator=(const InterConnector&);

        void onAuthParamsChanged(const Data::AuthParams& _params);

        std::unordered_map<KeyCombination, qint64> keyCombinationPresses_;
        Ui::MainWindow* MainWindow_;
        bool dragOverlay_;
        bool destroying_;

        std::unordered_set<QString> multiselectStates_;
        MultiselectCurrentElement currentElement_;
        qint64 currentMessage_;
        QVariantAnimation* multiselectAnimation_;

        std::map<QPointer<QWidget>, QString> disabledWidgets_;

        std::unique_ptr<Data::AuthParams> authParams_;

        std::unordered_map<QString, QPointer<Ui::PageBase>> historyPages_;
    };
}
