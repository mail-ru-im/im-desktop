#pragma once

#include "history/History.h"
#include "styles/WallpaperId.h"
#include "../../types/chat.h"
#include "../../types/typing.h"
#include "../../types/filesharing_meta.h"
#include "types/reactions.h"
#include "../../controls/TextUnit.h"

Q_DECLARE_LOGGING_CATEGORY(historyPage)

namespace Data
{
    class SmartreplySuggest;
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
}

namespace Ui
{
    class ServiceMessageItem;
    class VoipEventItem;
    class HistoryControl;
    class HistoryControlPage;
    class HistoryControlPageItem;
    class AuthWidget;
    class LabelEx;
    class TextEmojiWidget;
    class MessagesWidget;
    class MessagesScrollArea;
    class HistoryControlPageThemePanel;
    class HistoryButton;
    class MentionCompleter;
    class CustomButton;
    class PinnedMessageWidget;
    class PictureWidget;
    class ClickableTextWidget;
    class ConnectionWidget;
    class TypingWidget;
    class SmartReplyWidget;
    class ShowHideButton;
    class ContactAvatarWidget;
    class ActiveCallPlate;
    enum class ConnectionState;
    enum class CallType;
    class ChatPlaceholder;
    enum class PlaceholderState;
    class StartCallButton;

    using highlightsV = std::vector<QString>;

    struct InsertHistMessagesParams;

    namespace ComplexMessage {
        class ComplexMessageItem;
    }

    class OverlayTopChatWidget : public QWidget
    {
        Q_OBJECT;

    public:
        explicit OverlayTopChatWidget(QWidget* _parent = nullptr);
        ~OverlayTopChatWidget();

        void setBadgeText(const QString& _text);
        void setPosition(const QPoint& _pos);

    protected:
        void paintEvent(QPaintEvent* _event) override;

    private:
        QString text_;
        TextRendering::TextUnitPtr textUnit_;
        QPoint pos_;
    };

    class TopWidget : public QStackedWidget
    {
        Q_OBJECT

    public:
        explicit TopWidget(QWidget* _parent);

        enum Widgets
        {
            Main = 0,
            Selection
        };

        void updateStyle();

    protected:
        void paintEvent(QPaintEvent* _event) override;

    private:
        int lastIndex_;

        QColor bg_;
        QColor border_;
    };

    class MessagesWidgetEventFilter : public QObject
    {
        Q_OBJECT

    public:
        MessagesWidgetEventFilter(
            QWidget* _buttonsWidget,
            const QString& _contactName,
            ClickableTextWidget* _contactNameWidget,
            MessagesScrollArea* _scrollArea,
            ServiceMessageItem* _overlay,
            HistoryControlPage* _dialog,
            const QString& _aimId
        );

        void resetNewPlate();

        void setContactName(const QString& _contactName);

    protected:
        bool eventFilter(QObject* _obj, QEvent* _event) override;

    private:
        QWidget* ButtonsWidget_;

        MessagesScrollArea* ScrollArea_;
        MessagesWidget* MessagesWidget_;

        bool NewPlateShowed_;
        bool ScrollDirectionDown_;
        HistoryControlPage* Dialog_;
        QDate Date_;
        QPoint MousePos_;
        QString ContactName_;
        ClickableTextWidget* ContactNameWidget_;
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

    class HistoryControlPage : public QWidget
    {
        enum class State;

        friend QTextStream& operator<<(QTextStream& _oss, const State _arg);

        Q_OBJECT

    Q_SIGNALS :
        void postponeFetch(hist::FetchDirection, QPrivateSignal);

        void needRemove(const Logic::MessageKey&);
        void quote(const Data::QuotesVec&);
        void messageIdsFetched(const QString&, const Data::MessageBuddies&, QPrivateSignal) const;
        void updateMembers();

        void switchToPrevDialog(const bool _byKeyboard, QPrivateSignal) const;

    public Q_SLOTS:
        void scrollMovedToBottom();

        void chatInfo(qint64, const std::shared_ptr<Data::ChatInfo>&, const int _requestMembersLimit);
        void chatInfoFailed(qint64 _seq, core::group_chat_info_errors, const QString& _aimId);

        void scrollToBottom();
        void scrollToBottomByButton();
        void onUpdateHistoryPosition(int32_t position, int32_t offset);
        void showNewMessageForce();

        void editBlockCanceled();

        void showMentionCompleter(const QString& _initialPattern, const QPoint& _position = QPoint());

    private Q_SLOTS:
        void searchButtonClicked();
        void pendingButtonClicked();
        void moreButtonClicked();
        void sourceReadyHist(int64_t _mess_id, hist::scroll_mode_type _scrollMode);
        void updatedBuddies(const Data::MessageBuddies&);
        void insertedBuddies(const Data::MessageBuddies&);
        void deleted(const Data::MessageBuddies&);
        void pendingResolves(const Data::MessageBuddies&, const Data::MessageBuddies&);
        void clearAll();
        void emptyHistory();

        void fetch(hist::FetchDirection);

        void downPressed();
        void autoScroll(bool);
        void updateChatInfo(const QString& _aimId, const QVector<::HistoryControl::ChatEventInfoSptr>& _events);
        void onReachedFetchingDistance(bool _isMoveToBottomIfNeed);
        void canFetchMore(hist::FetchDirection);
        void nameClicked();
        void editMembers();
        void updateContactNameTooltip(bool _show);

        void onNewMessageAdded(const QString& _aimId);
        void onNewMessagesReceived(const QVector<qint64>&);

        void contactChanged(const QString&);
        void onChatRoleChanged(const QString&);
        void removeWidget(const Logic::MessageKey&);

        void copy(const QString&);
        void quoteText(const Data::QuotesVec&);
        void forwardText(const Data::QuotesVec&);
        void addToFavorites(const Data::QuotesVec& _quotes);
        void pin(const QString& _chatId, const int64_t _msgId, const bool _isUnpin = false);

        void multiselectDelete();
        void multiselectFavorites();
        void multiselectChanged();
        void multiselectCopy();
        void multiselectReply();
        void multiselectForward(const QString&);

        void edit(const Data::MessageBuddySptr& _msg, MediaType _mediaType);

        void onItemLayoutChanged();

        void strangerClose();
        void strangerBlock();

        void authBlockContact(const QString& _aimId);
        void authDeleteContact(const QString& _aimId);

        void addMember();

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
        void groupSubscribeResult(int _error, int _resubscribeIn);

        void onNeedUpdateRecentsText();
        void hideHeads();
        void mentionHeads(const QString& _aimId, const QString& _friendly);
        void connectionStateChanged(const Ui::ConnectionState& _state);
        void lastseenChanged(const QString& _aimid);

        void onGlobalThemeChanged();
        void onFontParamsChanged();

        void onRoleChanged(const QString& _aimId);
        void onMessageBuddies(const Data::MessageBuddies& _buddies, const QString& _aimId, Ui::MessagesBuddiesOpt _option, bool _havePending, qint64 _seq, int64_t _last_msgid);
        void onChatMemberInfo(const qint64 _seq, const std::shared_ptr<Data::ChatMembersPage>& _info);

        void onReadOnlyUser(const QString& _aimId, bool _isReadOnly);
        void onBlockUser(const QString& _aimId, bool _isBlock);

        void onAddReactionPlateActivityChanged(const QString& _contact, bool _active);

        void onCallRoomInfo(const QString& _roomId, int64_t _membersCount, bool _failed);

        void updateSpellCheckVisibility();

    public:
        HistoryControlPage(QWidget* _parent, const QString& _aimId);
        ~HistoryControlPage();

        enum class FirstInit
        {
            No,
            Yes
        };

        void initFor(qint64 _id, hist::scroll_mode_type _type, FirstInit _firstInit = FirstInit::No);
        void resetMessageHighlights();
        void setHighlights(const highlightsV& _highlights);

        void updateState(bool);
        std::optional<qint64> getNewPlateId() const;
        void newPlateShowed();

        void insertNewPlate();

        void open();
        const QString& aimId() const;
        void cancelSelection();

        void setHistoryControl(HistoryControl* _control);

        bool touchScrollInProgress() const;
        void updateWidgetsTheme();
        void updateTopPanelStyle();
        void updateSmartreplyStyle();

        void showMainTopPanel();

        void scrollTo(const Logic::MessageKey& key, hist::scroll_mode_type _scrollMode);
        void updateItems();

        bool contains(const QString& _aimId) const;
        bool containsWidgetWithKey(const Logic::MessageKey& _key) const;

        void resumeVisibleItems();
        void suspendVisisbleItems();

        /// set buttonDown_ position from resize
        void updateFooterButtonsPositions();
        void setButtonDownPositions(int x_showed, int y_showed, int y_hided);
        void setButtonMentionPositions(int x_showed, int y_showed, int y_hided);
        void positionMentionCompleter(const QPoint& _atPos);
        void updatePlaceholderPosition();

        void setUnreadBadgeText(const QString& _text);

        MentionCompleter* getMentionCompleter();

        void setPrevChatButtonVisible(const bool _visible);
        void setOverlayTopWidgetVisible(const bool _visible);

        void inputTyped();
        void pageOpen();
        void pageLeave();
        void notifyApplicationWindowActive(const bool _active);
        void notifyUIActive(const bool _active);

        void setPinnedMessage(Data::MessageBuddySptr _msg);
        void clearSelection();
        void resetShiftingParams();

        void drawDebugRectForHole(QWidget*);

        void updateFonts();

        QWidget* getTopWidget() const;

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

        std::optional<Data::FileSharingMeta> getMeta(const QString& _id) const;

    protected:
        void focusOutEvent(QFocusEvent* _event) override;
        void wheelEvent(QWheelEvent *_event) override;
        void showEvent(QShowEvent* _event) override;
        void resizeEvent(QResizeEvent* _event) override;
        bool eventFilter(QObject* _object, QEvent* _event) override;

    private:
        void changeDlgStateContact(const Data::DlgState& _dlgState);
        void changeDlgStateChat(const Data::DlgState& _dlgState);

        void updateName();
        void updateName(const QString& _friendly);
        bool blockUser(const QString& _aimId, bool _blockUser);
        bool changeRole(const QString& _aimId, bool _moder);
        bool readonly(const QString& _aimId, bool _readonly);

        void setRecvLastMessage(bool _value);

        bool connectToComplexMessageItemImpl(const Ui::ComplexMessage::ComplexMessageItem*) const;
        void connectToComplexMessageItem(const QWidget*) const;
        bool connectToVoipItem(const Ui::VoipEventItem* _item) const;

        void connectToMessageBuddies();

        void insertMessages(InsertHistMessagesParams&&);

        void unloadAfterInserting();

        enum class FromInserting
        {
            No,
            Yes
        };

        void unloadTop(FromInserting _mode = FromInserting::No);
        void unloadBottom(FromInserting _mode = FromInserting::No);

        void updateMessageItems();

        void removeNewPlateItem();

        void initButtonDown();
        void initMentionsButton();
        void initTopWidget();
        void updateMentionsButton();
        void updateMentionsButtonDelayed();

        void updateOverlaySizes();

        void mention(const QString& _aimId);

        bool isPageOpen() const;

        bool isInBackground() const;

        void updatePendingButtonPosition();
        void updateCallButtonsVisibility();

        void sendBotCommand(const QString& _command);
        void startBot();

        class PositionInfo;

        Styling::WallpaperId wallpaperId_;
        using PositionInfoSptr = std::shared_ptr<PositionInfo>;
        using PositionInfoList = std::list<PositionInfoSptr>;
        using PositionInfoListIter = PositionInfoList::iterator;

        enum class WidgetRemovalResult;

        TypingWidget* typingWidget_;
        SmartReplyWidget* smartreplyWidget_;
        ShowHideButton* smartreplyButton_;

        void initStatus();

        void showStatusBannerIfNeeded();
        void showStrangerIfNeeded();

        bool isScrolling() const;
        QWidget* getWidgetByKey(const Logic::MessageKey& _key) const;
        QWidget* extractWidgetByKey(const Logic::MessageKey& _key);
        HistoryControlPageItem* getPageItemByKey(const Logic::MessageKey& _key) const;
        WidgetRemovalResult removeExistingWidgetByKey(const Logic::MessageKey& _key);
        void cancelWidgetRequests(const QVector<Logic::MessageKey>&);
        void removeWidgetByKeys(const QVector<Logic::MessageKey>&);
        void replaceExistingWidgetByKey(const Logic::MessageKey& _key, std::unique_ptr<QWidget> _widget);

        void loadChatInfo();
        qint64 loadChatMembersInfo(const std::vector<QString>& _members);
        void renameContact();

        void updateSendersInfo();

        void setState(const State _state, const char* _dbgWhere);
        bool isState(const State _state) const;
        bool isStateFetching() const;
        bool isStateIdle() const;
        bool isStateInserting() const;
        void postponeMessagesRequest(const char* _dbgWhere, bool _isDown);
        void switchToIdleState(const char* _dbgWhere);
        void switchToInsertingState(const char* _dbgWhere);
        void switchToFetchingState(const char* _dbgWhere);
        void setContactStatusClickable(bool _isEnabled);

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
        void setEmptyAreaVisible(bool _visible);

    private:
        bool isContactStatusClickable_;
        bool isMessagesRequestPostponed_;
        bool isMessagesRequestPostponedDown_;
        bool isPublicChat_;
        int32_t chatMembersCount_;
        char const* dbgWherePostponed_;
        ClickableTextWidget* contactStatus_;
        MessagesScrollArea* messagesArea_;
        MessagesWidgetEventFilter* eventFilter_;
        qint32 nextLocalPosition_;
        QWidget* topWidgetLeftPadding_;
        QWidget* prevChatButtonWidget_;
        CustomButton* searchButton_;
        CustomButton* addMemberButton_;
        CustomButton* pendingButton_;
        StartCallButton* callButton_;
        QWidget* buttonsWidget_;
        QSpacerItem* verticalSpacer_;
        QString aimId_;
        Data::DlgState dlgState_;
        QTimer* contactStatusTimer_;

        QWidget* contactWidget_;
        ClickableTextWidget* contactName_;
        ServiceMessageItem* messagesOverlay_;
        State state_;
        std::list<ItemData> itemsData_;
        TopWidget* topWidget_;
        OverlayTopChatWidget* overlayTopChatWidget_;
        OverlayTopChatWidget* overlayPendings_;
        MentionCompleter* mentionCompleter_;
        PinnedMessageWidget* pinnedWidget_;
        ConnectionWidget* connectionWidget_;

        /// button to move history at last messages
        HistoryButton* buttonDown_;
        HistoryButton* buttonMentions_;
        /// button position
        QPoint buttonDownShowPosition_;
        QPoint buttonDownHidePosition_;
        /// -1 hide animation, 0 - show animation, 1 show animation
        int buttonDir_;
        /// time of buttonDown 0..1 show 1..0 hide
        float buttonDownTime_;

        QTimer* buttonDownTimer_;
        bool isFetchBlocked_;
        bool isPageOpen_;

        bool fontsHaveChanged_;

        /// new message plate force show
        bool bNewMessageForceShow_;
        /// current time in ms for timer function
        qint64 buttonDownCurrentTime_;

        std::optional<qint64> newPlateId_;

        std::vector<qint64> newMessageIds_;

        // typing
        qint64 prevTypingTime_;
        QTimer* typedTimer_;
        QTimer* lookingTimer_;
        QTimer* mentionTimer_;
        QTimer* groupSubscribeTimer_;
        QString lookingId_;
        bool chatscrBlockbarActionTracked_;

        // heads
        Heads::HeadContainer* heads_;

        hist::MessageReader* reader_;
        hist::History* history_;

        highlightsV highlights_;

        HistoryControl* control_;

        QMetaObject::Connection connectMessageBuddies_;
        std::unordered_map<QString, std::unique_ptr<Data::ChatMemberInfo>, Utils::QStringHasher> chatSenders_;
        bool isAdminRole_;
        bool isChatCreator_;
        bool isMember_ = false;
        QString chatInviter_;
        qint64 requestWithLoaderSequence_;
        QString requestWithLoaderAimId_;
        ContactAvatarWidget* avatar_;
        ActiveCallPlate* activeCallPlate_;

        std::optional<QString> botParams_;

        QWidget* emptyArea_;
        ChatPlaceholder* chatPlaceholder_;
        QTimer* chatPlaceholderTimer_;

        bool addReactionPlateActive_ = false;

        friend class MessagesWidgetEventFilter;
    };
}
