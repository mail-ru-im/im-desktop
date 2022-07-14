#pragma once

#include "history/History.h"
#include "styles/WallpaperId.h"
#include "../../types/chat.h"
#include "../../types/typing.h"
#include "../../types/filesharing_meta.h"
#include "types/reactions.h"
#include "PageBase.h"

Q_DECLARE_LOGGING_CATEGORY(historyPage)

namespace Utils
{
    struct FileSharingId;
}

namespace Data
{
    class SmartreplySuggest;
    struct ParentTopic;
}

namespace Logic
{
    class MessageItemDelegate;
    enum class scroll_mode_type;
}

namespace core
{
    enum class group_chat_info_errors;
}

namespace Heads
{
    class HeadContainer;
}

namespace hist
{
    class MessageReader;
    class History;
    enum class scroll_mode_type;
    enum class FetchDirection;
}

namespace Utils
{
    struct QStringHasher;
    struct FileSharingId;
}

namespace Ui
{
    class ServiceMessageItem;
    class VoipEventItem;
    class HistoryControlPage;
    class HistoryControlPageItem;
    class MessagesWidget;
    class MessagesScrollArea;
    class HistoryButton;
    class MentionCompleter;
    class PinnedMessageWidget;
    class TypingWidget;
    class SmartReplyWidget;
    class ShowHideButton;
    class ActiveCallPlate;
    class ChatPlaceholder;
    enum class PlaceholderState;
    class InputWidget;
    class SmartReplyForQuote;
    class TopWidget;
    class DialogHeaderPanel;
    class DragOverlayWindow;
    class BackgroundWidget;

    namespace Smiles
    {
        class SmilesMenu;
    }

    namespace Stickers
    {
        class StickersSuggest;
    }

    using highlightsV = std::vector<QString>;

    struct InsertHistMessagesParams;

    namespace ComplexMessage
    {
        class ComplexMessageItem;
    }

    class MessagesWidgetEventFilter : public QObject
    {
        Q_OBJECT

    public:
        MessagesWidgetEventFilter(
            MessagesScrollArea* _scrollArea,
            ServiceMessageItem* _overlay,
            HistoryControlPage* _dialog
        );

        void resetNewPlate();

    protected:
        bool eventFilter(QObject* _obj, QEvent* _event) override;

    private:
        MessagesScrollArea* ScrollArea_;
        MessagesWidget* MessagesWidget_;

        bool NewPlateShowed_;
        bool ScrollDirectionDown_;
        HistoryControlPage* Dialog_;
        QDate Date_;
        QPoint MousePos_;
        ServiceMessageItem* Overlay_;
    };

    struct ItemData
    {
        ItemData(
            const Logic::MessageKey& _key,
            QWidget* _widget,
            const unsigned _mode,
            const bool _isDeleted)
            : Key_(_key)
            , Widget_(_widget)
            , Mode_(_mode)
            , IsDeleted_(_isDeleted)
        {
        }

        Logic::MessageKey Key_;

        QWidget* Widget_;

        unsigned Mode_;

        bool IsDeleted_;
    };

    namespace themes
    {
        class theme;
    }

    class HistoryControlPage : public PageBase
    {
        enum class State;

        friend QTextStream& operator<<(QTextStream& _oss, const State _arg);

        Q_OBJECT

    Q_SIGNALS :
        void postponeFetch(hist::FetchDirection, QPrivateSignal);

        void needRemove(const Logic::MessageKey&);
        void quote(const Data::QuotesVec&);
        void messageIdsFetched(const QString&, const Data::MessageBuddies&, QPrivateSignal) const;
        void createTask(const Data::FString& _text, const Data::MentionMap& _mentions, const QString& _assignee, const bool _isThreadFeedMessage);

    public Q_SLOTS:
        void scrollMovedToBottom();

        void chatInfo(qint64, const std::shared_ptr<Data::ChatInfo>&, const int _requestMembersLimit);
        void chatInfoFailed(qint64 _seq, core::group_chat_info_errors, const QString& _aimId);

        void scrollToBottom();
        void scrollToBottomByButton();
        void onUpdateHistoryPosition(int32_t position, int32_t offset);

        void editBlockCanceled();

        void onWallpaperChanged(const QString& _aimId);
        void onNavigationKeyPressed(int _key, Qt::KeyboardModifiers _modifiers);

    private Q_SLOTS:
        void sourceReadyHist(int64_t _messId, hist::scroll_mode_type _scrollMode);
        void updatedBuddies(const Data::MessageBuddies&);
        void insertedBuddies(const Data::MessageBuddies&);
        void deleted(const Data::MessageBuddies&);
        void pendingResolves(const Data::MessageBuddies&, const Data::MessageBuddies&);
        void clearAll();
        void emptyHistory();
        void hideStickersAndSuggests(bool _animated = true);
        void fetch(hist::FetchDirection);

        void downPressed();
        void autoScroll(bool);
        void updateChatInfo(const QString& _aimId, const QVector<::HistoryControl::ChatEventInfoSptr>& _events);
        void onReachedFetchingDistance(bool _isMoveToBottomIfNeed);
        void canFetchMore(hist::FetchDirection);

        void onNewMessageAdded(const QString& _aimId);
        void onNewMessagesReceived(const QVector<qint64>&);

        void contactChanged(const QString&);
        void onChatRoleChanged(const QString&);
        void removeWidget(const Logic::MessageKey&);

        void copy();
        void quoteText(const Data::QuotesVec&);
        void forwardText(const Data::QuotesVec&);
        void addToFavorites(const Data::QuotesVec& _quotes);
        void pin(const QString& _chatId, const int64_t _msgId, const bool _isUnpin = false);

        void multiselectChanged();

        void multiselectDelete(const QString& _aimId);
        void multiselectFavorites(const QString& _aimId);
        void multiselectCopy(const QString& _aimId);
        void multiselectReply(const QString& _aimId);
        void multiselectForward(const QString& _aimId);

        void edit(const Data::MessageBuddySptr& _msg, MediaType _mediaType);

        void onItemLayoutChanged();

        void strangerClose();
        void strangerBlock();

        void authBlockContact(const QString& _aimId);
        void authDeleteContact(const QString& _aimId);

        void changeDlgState(const Data::DlgState& _dlgState);

        void avatarMenuRequest(const QString&);
        void avatarMenu(QAction* _action);

        void chatHeads(const Data::ChatHeads& _heads);

        void onButtonDownMove();
        void onButtonMentionsClicked();

        void onTypingTimer();
        void onLookingTimer();
        void mentionMe(const QString& _contact, Data::MessageBuddySptr _mention);
        void onMentionRead(const qint64 _messageId);

        void groupSubscribe();
        void cancelGroupSubscription();
        void groupSubscribeResult(const int64_t _seq, int _error, int _resubscribeIn);

        void onNeedUpdateRecentsText();
        void hideHeads();
        void mentionHeads(const QString& _aimId, const QString& _friendly);

        void onFontParamsChanged();

        void onRoleChanged(const QString& _aimId);
        void onMessageBuddies(const Data::MessageBuddies& _buddies, const QString& _aimId, Ui::MessagesBuddiesOpt _option, bool _havePending, qint64 _seq, int64_t _last_msgid);
        void onChatMemberInfo(const qint64 _seq, const std::shared_ptr<Data::ChatMembersPage>& _info);

        void onReadOnlyUser(const QString& _aimId, bool _isReadOnly);
        void onBlockUser(const QString& _aimId, bool _isBlock);

        void onCreateTask(const Data::FString& _text, const Data::MentionMap& _mentions, const QString& _assignee, const bool _isThreadFeedMessage);

        void onAddReactionPlateActivityChanged(const QString& _contact, bool _active);

        void onCallRoomInfo(const QString& _roomId, int64_t _membersCount, bool _failed);

        void updateSpellCheckVisibility();
        void updateFileStatuses();

        void onThreadAdded(int64_t _seq, const Data::MessageParentTopic& _parentTopic, const QString& _threadId, int _error);
        void openThread(const QString& _threadId, int64_t _msgId, const QString& _fromPage);

        void startPttRecording();

    public:
        HistoryControlPage(const QString& _aimId, QWidget* _parent, BackgroundWidget* _bgWidget);
        ~HistoryControlPage();

        void initFor(qint64 _id, hist::scroll_mode_type _type, FirstInit _firstInit = FirstInit::No) override;
        void resetMessageHighlights() override;
        void setHighlights(const highlightsV& _highlights) override;
        bool isInputActive() const;

        std::optional<qint64> getNewPlateId() const;
        void newPlateShowed();

        void insertNewPlate();

        void open() override;
        const QString& aimId() const override;
        void cancelSelection() override;

        void updateWidgetsTheme() override;
        void updateTopPanelStyle();
        void updateSmartreplyStyle();

        void scrollToMessage(int64_t _msgId);
        void scrollTo(const Logic::MessageKey& key, hist::scroll_mode_type _scrollMode);
        void updateItems() override;

        bool contains(const QString& _aimId) const;

        void resumeVisibleItems();
        void suspendVisisbleItems() override;

        void setPrevChatButtonVisible(bool _visible) override;
        void setOverlayTopWidgetVisible(bool _visible) override;

        /// set buttonDown_ position from resize
        void updateFooterButtonsPositions();
        void setButtonDownPositions(int x_showed, int y_showed, int y_hided);
        void setButtonMentionPositions(int x_showed, int y_showed, int y_hided);
        void positionMentionCompleter(const QPoint& _atPos);
        void updatePlaceholderPosition();

        void setUnreadBadgeText(const QString& _text) override;

        MentionCompleter* getMentionCompleter();

        void inputTyped();

        void pageOpen() override;
        void pageLeave() override;
        void mainPageChanged() override;
        bool isPageOpen() const;

        void notifyApplicationWindowActive(const bool _active);
        void notifyUIActive(const bool _active);

        void setPinnedMessage(Data::MessageBuddySptr _msg) override;
        void clearSelection();
        void resetShiftingParams();

        void drawDebugRectForHole(QWidget*);

        void updateFonts();

        QWidget* getTopWidget() const override;

        bool hasMessageUnderCursor() const;

        int getMessagesAreaHeight() const;

        void setFocusOnArea();
        void clearPartialSelection();

        void showSmartreplies();
        void hideSmartreplies();
        bool isSmartrepliesVisible() const;
        void showSmartrepliesButton();
        void hideSmartrepliesButton();
        void setSmartrepliesSemitransparent(bool _semi);
        void notifyQuotesResize();

        bool hasMessages() const noexcept;

        void setBotParameters(std::optional<QString> _parameters);

        bool needStartButton() const noexcept { return botParams_.has_value(); }

        int chatRoomCallParticipantsCount() const;

        void clearPttProgress();

        MessagesScrollArea* scrollArea() const;

        int getInputHeight() const;
        bool isInputWidgetActive() const; // for update check
        bool canSetFocusOnInput() const;
        void setFocusOnInputFirstFocusable();

        bool isRecordingPtt() const;
        bool isPttRecordingHold() const;
        bool isPttRecordingHoldByKeyboard() const;
        bool isPttRecordingHoldByMouse() const;
        bool tryPlayPttRecord();
        bool tryPausePttRecord();
        void closePttRecording();
        void sendPttRecord();
        void startPttRecordingLock();
        void stopPttRecording();

        const QElapsedTimer& getInitForTimer() const noexcept { return initForTimer_; }

        void showDragOverlay();
        void hideDragOverlay();
        DragOverlayWindow* getDragOverlayWindow() const noexcept { return dragOverlayWindow_; }

        bool isInputWidgetInFocus() const;
        bool isInputOrHistoryInFocus() const;

    protected:
        void wheelEvent(QWheelEvent *_event) override;
        void showEvent(QShowEvent* _event) override;
        void resizeEvent(QResizeEvent* _event) override;
        bool eventFilter(QObject* _object, QEvent* _event) override;
        void dragEnterEvent(QDragEnterEvent* _event) override;
        void dragLeaveEvent(QDragLeaveEvent* _event) override;
        void dragMoveEvent(QDragMoveEvent* _event) override;

    private:
        void updatePinMessage(const Data::MessageBuddies& _buddies);
        void changeDlgStateContact(const Data::DlgState& _dlgState);
        void changeDlgStateChat(const Data::DlgState& _dlgState);

        bool blockUser(const QString& _aimId, bool _blockUser);
        bool changeRole(const QString& _aimId, bool _moder);
        bool readonly(const QString& _aimId, bool _readonly);


        bool connectToComplexMessageItemImpl(const Ui::ComplexMessage::ComplexMessageItem*) const;
        void connectToComplexMessageItem(const QWidget*) const;
        bool connectToVoipItem(const Ui::VoipEventItem* _item) const;
        void connectToPageItem(const HistoryControlPageItem* _item) const;

        void connectToMessageBuddies();

        void insertMessages(InsertHistMessagesParams&&);

        void unloadAfterInserting();

        enum class FromInserting
        {
            No,
            Yes
        };

        enum class UnloadDirection
        {
            TOP,
            BOTTOM,
        };

        void unload(FromInserting _mode, UnloadDirection _direction);
        void unloadTop(FromInserting _mode = FromInserting::No);
        void unloadBottom(FromInserting _mode = FromInserting::No);

        void removeNewPlateItem();
        bool hasNewPlate();

        void initButtonDown();
        void initMentionsButton();
        void initTopWidget();
        void updateMentionsButton();
        void updateMentionsButtonDelayed();

        void updateOverlaySizes();

        void mention(const QString& _aimId);

        bool isInBackground() const;

        void updateCallButtonsVisibility();

        void sendBotCommand(const QString& _command);
        void startBot();

        void pageLeaveStuff();

        class PositionInfo;

        Styling::WallpaperId wallpaperId_;
        Styling::ThemeChecker themeChecker_;
        using PositionInfoSptr = std::shared_ptr<PositionInfo>;
        using PositionInfoList = std::list<PositionInfoSptr>;
        using PositionInfoListIter = PositionInfoList::iterator;

        enum class WidgetRemovalResult;

        TypingWidget* typingWidget_;
        SmartReplyWidget* smartreplyWidget_ = nullptr;
        ShowHideButton* smartreplyButton_ = nullptr;

        void initStatus();

        void showStatusBannerIfNeeded();
        void showStrangerIfNeeded();

        bool isScrolling() const;
        QWidget* getWidgetByKey(const Logic::MessageKey& _key) const;
        WidgetRemovalResult removeExistingWidgetByKey(const Logic::MessageKey& _key);
        void cancelWidgetRequests(const QVector<Logic::MessageKey>&);
        void removeWidgetByKeys(const QVector<Logic::MessageKey>&);

        void loadChatInfo();
        qint64 loadChatMembersInfo(const std::vector<QString>& _members);

        void updateSendersInfo();

        void setState(const State _state, const char* _dbgWhere);
        bool isState(const State _state) const;
        bool isStateIdle() const;
        void switchToIdleState(const char* _dbgWhere);
        void switchToInsertingState(const char* _dbgWhere);

        void showAvatarMenu(const QString& _aimId);

        QRect getScrollAreaGeometry() const;

        void onSmartreplies(const std::vector<Data::SmartreplySuggest>& _suggests);
        void hideAndClearSmartreplies();
        void clearSmartrepliesForMessage(const qint64 _msgId);
        void onSmartreplyButtonClicked();
        void onSmartreplyHide();
        void sendSmartreply(const Data::SmartreplySuggest& _suggest);

        void showActiveCallPlate();

        void startShowButtonDown();
        void startHideButtonDown();

        void initChatPlaceholder();
        void showChatPlaceholder(PlaceholderState _state);
        void hideChatPlaceholder();
        PlaceholderState updateChatPlaceholder();
        PlaceholderState getPlaceholderState() const;
        void initEmptyArea();
        void setEmptyAreaVisible(bool _visible);

        void setFocusOnInputWidget();

        void initStickerPicker();

        bool needShowSuggests();
        void onShowDragOverlay();
        void onHideDragOverlay();

        enum class ShowHideSource
        {
            EmojiButton,
            MenuButton,
        };
        enum class ShowHideInput
        {
            Keyboard,
            Mouse,
        };
        void showHideStickerPicker(ShowHideInput _input = ShowHideInput::Mouse, ShowHideSource _source = ShowHideSource::EmojiButton);
        void onStickerPickerVisibilityChanged(bool _visible);

        void onSmartrepliesForQuote(const std::vector<Data::SmartreplySuggest>& _suggests);

        void hideSmartrepliesForQuoteAnimated();
        void hideSmartrepliesForQuoteForce();

        struct SmartreplyForQuoteParams
        {
            QPoint pos_;
            QRect areaRect_;
            QSize maxSize_;
        };
        SmartreplyForQuoteParams getSmartreplyForQuoteParams() const;

        void sendSmartreplyForQuote(const Data::SmartreplySuggest& _suggest);

        void onSuggestShow(const QString& _text, const QPoint& _pos);
        void onSuggestHide();
        void onSuggestedStickerSelected(const Utils::FileSharingId& _stickerId);
        void hideSuggest(bool _animated = true);
        void sendSuggestedStickerStats(const Utils::FileSharingId& _stickerId);

        void hideMentionCompleter();
        void onMentionCompleterVisibilityChanged(bool _visible);

        void updateWidgetsThemeImpl();

        bool groupSubscribeNeeded() const;
        void updateDragOverlayGeometry();
        void updateDragOverlay();

        bool isThread() const;
        void showStickersSuggest();

    private:
        bool isMessagesRequestPostponed_ = false;
        bool isMessagesRequestPostponedDown_ = false;
        bool needShowSuggests_ = false;

        char const* dbgWherePostponed_;
        MessagesScrollArea* messagesArea_ = nullptr;
        MessagesWidgetEventFilter* eventFilter_ = nullptr;
        QString aimId_;
        Data::DlgState dlgState_;
        ServiceMessageItem* messagesOverlay_ = nullptr;
        State state_;
        TopWidget* topWidget_ = nullptr;
        DialogHeaderPanel* header_ = nullptr;
        MentionCompleter* mentionCompleter_ = nullptr;
        PinnedMessageWidget* pinnedWidget_ = nullptr;

        /// button to move history at last messages
        HistoryButton* buttonDown_ = nullptr;
        HistoryButton* buttonMentions_ = nullptr;
        /// button position
        QPoint buttonDownShowPosition_;
        QPoint buttonDownHidePosition_;
        /// -1 hide animation, 0 - show animation, 1 show animation
        int buttonDir_ = 0;
        /// time of buttonDown 0..1 show 1..0 hide
        float buttonDownTime_ = 0.;

        QTimer* buttonDownTimer_ = nullptr;
        bool isPageOpen_ = false;

        bool fontsHaveChanged_ = false;

        /// current time in ms for timer function
        qint64 buttonDownCurrentTime_ = 0;

        std::optional<qint64> newPlateId_;

        struct SuggestsCache
        {
            QString text_;
            QPoint pos_;
        } lastShownSuggests_;

        // typing
        qint64 prevTypingTime_ = 0;
        QTimer* typedTimer_ = nullptr;
        QTimer* lookingTimer_ = nullptr;
        QTimer* mentionTimer_ = nullptr;
        QTimer* groupSubscribeTimer_ = nullptr;
        QString lookingId_;
        bool chatscrBlockbarActionTracked_ = false;

        // heads
        Heads::HeadContainer* heads_ = nullptr;

        hist::MessageReader* reader_ = nullptr;
        hist::History* history_ = nullptr;

        highlightsV highlights_;

        QMetaObject::Connection connectMessageBuddies_;
        std::unordered_map<QString, std::unique_ptr<Data::ChatMemberInfo>, Utils::QStringHasher> chatSenders_;
        bool isAdminRole_ = false;
        bool isChatCreator_ = false;
        bool isMember_ = false;
        QString chatInviter_;
        qint64 requestWithLoaderSequence_ = -1;
        QString requestWithLoaderAimId_;

        ActiveCallPlate* activeCallPlate_ = nullptr;

        std::optional<QString> botParams_;

        QWidget* emptyArea_ = nullptr;
        ChatPlaceholder* chatPlaceholder_ = nullptr;
        QPointer<QTimer> chatPlaceholderTimer_;

        Smiles::SmilesMenu* stickerPicker_ = nullptr;
        InputWidget* inputWidget_ = nullptr;
        SmartReplyForQuote* smartreplyForQuotePopup_ = nullptr;

        Stickers::StickersSuggest* suggestsWidget_ = nullptr;
        bool suggestWidgetShown_ = false;

        bool addReactionPlateActive_ = false;

        QElapsedTimer initForTimer_;

        DragOverlayWindow* dragOverlayWindow_ = nullptr;
        QTimer* overlayUpdateTimer_ = nullptr;

        friend class MessagesWidgetEventFilter;
    };
}
