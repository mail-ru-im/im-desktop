#pragma once

#include <unordered_map>
#include "../utils/keyboard.h"
#include "../utils/utils.h"

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
}

namespace Data
{
    class ChatInfo;
    class DlgState;
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
        {
        }

        GalleryData(const QString& _aimid, const QString& _link, int64_t _msgId, const QString& _sender, time_t _time)
            : aimId_(_aimid)
            , link_(_link)
            , sender_(_sender)
            , msgId_(_msgId)
            , attachedPlayer_(nullptr)
            , time_(_time)
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

        void openDialogOrProfileById(const QString& _id, bool _forceDialogOpen = false, std::optional<QString> _botParams = {});
        void phoneAttached(bool);
        void phoneAttachmentCancelled();

        void loaderOverlayShow();
        void loaderOverlayHide();
        void loaderOverlayCancelled();
        void multiselectChanged();

        void multiselectDelete();
        void multiselectFavorites();
        void multiselectCopy();
        void multiselectReply();
        void multiselectForward(QString);

        void multiSelectCurrentElementChanged();
        void multiSelectCurrentMessageChanged();

        void multiSelectCurrentMessageUp(bool);
        void multiSelectCurrentMessageDown(bool);

        void multiselectSpaceClicked();

        void updateSelectedCount();
        void messageSelected(qint64, const QString&);
        void selectedCount(int, int);
        void multiselectAnimationUpdate();
        void selectionStateChanged(qint64, const QString&, bool);

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

        void pttProgressChanged(qint64, const QString&, int);

    public:
        static InterConnector& instance();
        ~InterConnector();

        void setMainWindow(Ui::MainWindow* window);
        void startDestroying();
        Ui::MainWindow* getMainWindow(bool _check_destroying = false) const;
        Ui::HistoryControlPage* getHistoryPage(const QString& aimId) const;
        Ui::ContactDialog* getContactDialog() const;
        Ui::AppsPage* getAppsPage() const;
        Ui::MainPage* getMessengerPage() const;

        bool isInBackground() const;

        void showSidebar(const QString& aimId);
        void showSidebarWithParams(const QString& aimId, Ui::SidebarParams _params);
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

        void setMultiselect(bool _enable, const QString& _contact = QString(), bool _fromKeyboard = false);
        bool isMultiselect(const QString& _contact = QString()) const;

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

        void disableInMultiselect(QWidget* _w, const QString& _aimid = QString());
        void detachFromMultiselect(QWidget* _w);

        void clearPartialSelection(const QString& _aimid);

        void openGallery(const GalleryData& _data);

        bool isRecordingPtt() const;
        void showChatMembersFailuresPopup(ChatMembersOperation _operation, QString _chatAimId, std::map<core::chat_member_failure, std::vector<QString>> _failures);

    private:
        InterConnector();

        InterConnector(InterConnector&&);
        InterConnector(const InterConnector&);
        InterConnector& operator=(const InterConnector&);

        std::unordered_map<KeyCombination, qint64> keyCombinationPresses_;
        Ui::MainWindow* MainWindow_;
        bool dragOverlay_;
        bool destroying_;

        std::map<QString, bool> multiselectStates_;
        MultiselectCurrentElement currentElement_;
        qint64 currentMessage_;
        QVariantAnimation* multiselectAnimation_;

        std::map<QWidget*, QString> disabledWidgets_;
    };
}
