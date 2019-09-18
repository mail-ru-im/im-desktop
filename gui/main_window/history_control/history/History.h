#pragma once

#include "../../../types/message.h"
#include "../../../types/chatheads.h"
#include "../../mediatype.h"
#include "../../../utils/utils.h"
#include "Message.h"

Q_DECLARE_LOGGING_CATEGORY(history)

namespace core
{
    enum class message_type;
    enum class chat_event_type;
}

namespace HistoryControl
{
    using FileSharingInfoSptr = std::shared_ptr<class FileSharingInfo>;
    using StickerInfoSptr = std::shared_ptr<class StickerInfo>;
    using ChatEventInfoSptr = std::shared_ptr<class ChatEventInfo>;
}

namespace Ui
{
    class MessageItem;
    class ServiceMessageItem;
    class HistoryControlPageItem;
    enum class MessagesBuddiesOpt;
    enum class MessagesNetworkError;
}

namespace hist
{
    inline QString normalizeAimId(const QString& _aimId)
    {
        const int pos = _aimId.indexOf(ql1s("@uin.icq"));
        return pos == -1 ? _aimId : _aimId.left(pos);
    }

    Data::DlgState getDlgState(const QString& _contact);

    enum class scroll_mode_type
    {
        none,
        search,
        unread,
        to_deleted,
        background
    };

    enum class FetchDirection
    {
        toNewer,
        toOlder
    };

    enum class UnloadDirection
    {
        toTop,
        toBottom
    };

    class SimpleRecursiveLock
    {
    public:
        void lock() noexcept { assert(counter >= 0); ++counter; }
        void unlock() noexcept { --counter; assert(counter >= 0); }
        bool isLocked() const noexcept { return counter > 0; }

        SimpleRecursiveLock() = default;

        SimpleRecursiveLock(const SimpleRecursiveLock&) = delete;
        SimpleRecursiveLock& operator=(const SimpleRecursiveLock&) = delete;

    private:
        int counter = 0;
    };

    class History : public QObject
    {
        Q_OBJECT
    private:
        enum class RequestDirection
        {
            ToNewer,
            ToOlder,
            Bidirection,
        };
        enum class Prefetch
        {
            No,
            Yes
        };
        enum class JumpToBottom
        {
            No,
            Yes
        };
        enum class FirstRequest
        {
            No,
            Yes
        };
        enum class SearchMode
        {
            No,
            Yes
        };
        enum class NewPlateMode
        {
            No,
            Yes
        };

        enum class ServerResultsOnly
        {
            No,
            Yes
        };
        enum class Reason
        {
            Plain,
            ServerUpdate,
            HoleOnTheEdge // there is hole with last message in model and new message from dlg_state
        };

    public:

        explicit History(const QString& _aimId, QObject* _parent = nullptr);
        ~History();

        [[nodiscard]] Data::MessageBuddies fetchBuddies(FetchDirection direction);

        void initFor(qint64 messageId, hist::scroll_mode_type _type);

        [[nodiscard]] Data::MessageBuddies getInitMessages(std::optional<qint64> messageId) const;

        void scrollToBottom();

        [[nodiscard]] bool canViewFetch() const noexcept;
        [[nodiscard]] bool isViewLocked() const noexcept;
        [[nodiscard]] bool hasFetchRequest(FetchDirection direction) const;

        enum class CheckViewLock
        {
            No,
            Yes
        };

        [[nodiscard]] bool canUnload(UnloadDirection, CheckViewLock _mode = CheckViewLock::Yes) const noexcept;

        void setTopBound(const Logic::MessageKey&, CheckViewLock _mode = CheckViewLock::Yes);
        void setBottomBound(const Logic::MessageKey&, CheckViewLock _mode = CheckViewLock::Yes);

        bool isVisibleHoleBetween(qint64 _older, qint64 _newer) const;

    signals:
        void fetchedToNew(QPrivateSignal) const;
        void fetchedToOlder(QPrivateSignal) const;
        void clearAll(QPrivateSignal) const;

        void canFetch(hist::FetchDirection, QPrivateSignal) const;

        void readyInit(qint64 _id, scroll_mode_type _mode, QPrivateSignal) const;

        void updatedBuddies(const Data::MessageBuddies&, QPrivateSignal) const;
        void insertedBuddies(const Data::MessageBuddies&, QPrivateSignal) const;
        void dlgStateBuddies(const Data::MessageBuddies&, QPrivateSignal) const;
        void deletedBuddies(const Data::MessageBuddies&, QPrivateSignal) const;
        void pendingResolves(const Data::MessageBuddies& inView, const Data::MessageBuddies& all, QPrivateSignal) const;

        void updateLastSeen(const Data::DlgState&, QPrivateSignal) const;
        void newMessagesReceived(const QVector<qint64>&, QPrivateSignal) const;

    private:
        void initForImpl(qint64 _messageId, hist::scroll_mode_type _type, ServerResultsOnly _serverResultsOnly);
        void messageBuddies(const Data::MessageBuddies& _buddies, const QString& _aimId, Ui::MessagesBuddiesOpt _option, bool _havePending, qint64 _seq, qint64 _lastMsgid);
        void messageIdsFromServer(const QVector<qint64>& _ids, const QString& _aimId, qint64 _seq);
        void messageIdsUpdated(const QVector<qint64>& _ids, const QString& _aimId, qint64 _seq);
        void messagesDeleted(const QString& _aimId, const Data::MessageBuddies&);
        void messagesDeletedUpTo(const QString& _aimId, qint64 _id);

        void pendingAutoRemove(const QString& _aimId, const Logic::MessageKey& _key);

        void clearPendingsIfPossible();

        void messageContextError(const QString&, qint64 _seq, qint64 _id, Ui::MessagesNetworkError _error);
        void messageLoadAfterSearchError(const QString&, qint64 _seq, qint64 _from, qint64 _countLater, qint64 _countEarly, Ui::MessagesNetworkError _error);

        void messagesClear(const QString& _aimId, qint64 _seq);

        void messagesModified(const QString&, const Data::MessageBuddies&);

        template <typename R>
        void addToTail(const R& range);
        void addToTail(const Data::MessageBuddySptr& _msg);

        // refactor me. move to core
        void processChatEvents(const Data::MessageBuddies& _msgs, Ui::MessagesBuddiesOpt _option, qint64 _seq) const;

        bool hasAllMessagesForInit(qint64 messageId, hist::scroll_mode_type _type) const;

        bool hasHoleAtBottom() const;

        struct PendingsInsertResult
        {
            Data::MessageBuddies inserted;
            Data::MessageBuddies updated;
        };
        template <typename R>
        PendingsInsertResult insertPendings(const R& range);

        struct BuddiesInsertResult
        {
            Data::MessageBuddies inserted;
            Data::MessageBuddies updated;
            Data::MessageBuddies pendingResolves;
            bool canFetch = false;
        };
        template <typename R>
        BuddiesInsertResult insertBuddies(const R& range, Ui::MessagesBuddiesOpt option);

        void proccesInserted(const PendingsInsertResult& _pendingResult, const BuddiesInsertResult& _buddiesResult, Ui::MessagesBuddiesOpt _option, qint64 _seq, qint64 _prevLastId);

        struct RequestParams
        {
            qint64 messageId = -1;
            RequestDirection direction = RequestDirection::ToOlder;
            JumpToBottom jump_to_bottom = JumpToBottom::Yes;
            FirstRequest firstRequest = FirstRequest::Yes;
            Reason reason = Reason::Plain;
            SearchMode search = SearchMode::No;
        };

        std::optional<qint64> requestMessages(RequestParams _params);
        qint64 requestMessageContext(qint64 _messageId);

        void processUpdatedMessage(const Data::MessageBuddies& _buddies);
        void processRequestedByIds(const Data::MessageBuddies& _buddies);

        struct FixedIdAndMode
        {
            std::optional<qint64> id;
            std::optional<hist::scroll_mode_type> mode;
        };

        FixedIdAndMode fixIdAndMode(qint64 _id, hist::scroll_mode_type _mode) const;

        bool containsFirstMessage() const;

        bool needSkipSeq(qint64 seq) const;

        void removeSeq(qint64 seq);

        FirstRequest getAndResetFirstInitRequest();

        void clearExecutedRequests();

        bool needIgnoreMessages(const Data::MessageBuddies& _buddies, Ui::MessagesBuddiesOpt _option, bool _havePending, qint64 _seq, qint64 _lastMsgid) const;

        bool containsMsgId(qint64 id) const;
        bool containsInternalId(const QString& _id) const;

        enum class LogType
        {
            Simple,
            Full
        };
        void logCurrentIds(const QString& _hint, LogType _type);

        void dump(const QString&);

    private:

        const QString aimId_;

        FirstRequest firstInitRequest_ = FirstRequest::Yes;

        struct
        {
            qint64 centralMessage = -1;
            scroll_mode_type _mode  = scroll_mode_type::none;
            ServerResultsOnly _serverResultsOnly = ServerResultsOnly::No;

        } initParams_;

        struct MessageWithKey
        {
            Logic::MessageKey key;
            Data::MessageBuddySptr message;
        };

        std::optional<qint64> waitingInitSeq_;

        qint64 lastMessageId_ = -1;

        mutable SimpleRecursiveLock lockForViewFetchCounter_;

        mutable struct ViewBounds
        {
            std::optional<qint64> top;
            std::optional<qint64> bottom;
            std::optional<Logic::MessageKey> topPending;
            std::optional<Logic::MessageKey> bottomPending;

            bool hasPendings() const noexcept { return topPending && bottomPending; }

            bool isEmpty() const noexcept { return !top && !bottom; }

            void checkInvarinat() const { assert(top <= bottom && topPending <= bottomPending); }

            bool inBounds(qint64 _id) const noexcept { return !isEmpty() && _id <= *bottom && _id >= *top; }
            bool inBounds(const Logic::MessageKey& _key) const noexcept { return hasPendings() && *bottomPending <= _key && *topPending <= _key; }

        } viewBounds_;

        std::vector<MessageWithKey> tail_;
        std::vector<MessageWithKey> pendings_;

        using MessagesMap = std::map<qint64, Data::MessageBuddySptr>;
        MessagesMap messages_;

        struct ExecutedFetchRequestParams
        {
            qint64 seq = 0;
            qint64 id = -1;
            RequestDirection direction = RequestDirection::ToOlder;
            bool isContext = false;
        };

        struct ExecutedByIdRequestParams
        {
            qint64 seq = 0;
            std::vector<qint64> ids;
        };

        struct
        {
            std::vector<ExecutedByIdRequestParams> byIds;
            std::vector<ExecutedFetchRequestParams> fetch;

            bool hasRequests() const noexcept { return !byIds.empty() || !fetch.empty(); }
            bool hasFetchRequests(RequestDirection _direction) const noexcept { return std::any_of(fetch.begin(), fetch.end(), [_direction](auto x) { return x.direction == _direction; }); }

        } executedRequests;

        struct
        {
            qint64 toNew;
            qint64 toOlder;
        } fetchRequestId = {};
    };
}

Q_DECLARE_METATYPE(hist::scroll_mode_type);
