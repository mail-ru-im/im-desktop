#include "stdafx.h"
#include "History.h"
#include "Message.h"

#include "../../contact_list/RecentsModel.h"
#include "../../contact_list/UnknownsModel.h"
#include "../../friendly/FriendlyContainer.h"
#include "../../history_control/ChatEventInfo.h"
#include "../../history_control/VoipEventInfo.h"

#include "../../../core_dispatcher.h"
#include "../../../gui_settings.h"
#include "../../../my_info.h"
#include "../../../utils/log/log.h"
#include "../../../utils/utils.h"
#include "../../../utils/gui_coll_helper.h"
#include "../../../utils/InterConnector.h"
#include "../../../utils/gui_metrics.h"
#include "../../mediatype.h"

#include <boost/range/adaptor/map.hpp>
#include <boost/range/adaptor/reversed.hpp>
#include <boost/range.hpp>
#include <boost/range/algorithm.hpp>

Q_LOGGING_CATEGORY(history, "history")

namespace
{
    int getPendingIdx(const Data::MessageBuddies& _buddies)
    {
        const auto begin = _buddies.begin();
        const auto end = _buddies.end();
        const auto it = std::find_if(begin, end, [](const auto& msg) { return msg->IsPending(); });
        if (it != end)
            return std::distance(begin, it);
        return _buddies.size();
    }

    auto isKeyEqual = [](const auto& needle) noexcept
    {
        return [&needle](const auto& msgWithKey) noexcept
        {
            return needle == msgWithKey.key;
        };
    };

    auto hasId = [](auto id) noexcept
    {
        return [id](const auto& msg) noexcept
        {
            return msg->Id_ == id;
        };
    };

    auto msgWithKeyComparator = [](const auto& lhs, const auto& rhs) noexcept
    {
        return lhs.key < rhs.key;
    };

    auto msgComparator = [](const auto& lhs, const auto& rhs)
    {
        return lhs->ToKey() < rhs->ToKey();
    };

    auto hasExecutedFetchRequestParams = [](auto needle) noexcept
    {
        return [needle](auto params) noexcept
        {
            return needle.id == params.id && needle.direction == params.direction && needle.isContext == params.isContext;
        };
    };

    auto hasSeq = [](auto seq) noexcept
    {
        return [seq](auto param) noexcept
        {
            return param.seq == seq;
        };
    };

    constexpr size_t preloadCount() noexcept { return 30; }

    bool isInvisibleVoip(const Data::MessageBuddy& _buddy)
    {
        return _buddy.IsVoipEvent() && !_buddy.GetVoipEvent()->isVisible();
    }

    bool isInvisibleVoip(const Data::MessageBuddySptr& _buddy)
    {
        return _buddy && isInvisibleVoip(*_buddy);
    }

    bool isVisibleMessage(const Data::MessageBuddy& _buddy)
    {
        return !_buddy.IsDeleted() && !isInvisibleVoip(_buddy);
    }

    bool isVisibleMessage(const Data::MessageBuddySptr& _buddy)
    {
        return _buddy && isVisibleMessage(*_buddy);
    }

    template <typename T>
    T optionToStr(Ui::MessagesBuddiesOpt option) noexcept = delete;

    template <>
    std::string_view optionToStr<std::string_view>(Ui::MessagesBuddiesOpt option) noexcept
    {
        switch (option)
        {
        case Ui::MessagesBuddiesOpt::Requested:
            return "Requested option";
        case Ui::MessagesBuddiesOpt::RequestedByIds:
            return "RequestedByIds option";
        case Ui::MessagesBuddiesOpt::Updated:
            return "Updated option";
        case Ui::MessagesBuddiesOpt::FromServer:
            return "FromServer option";
        case Ui::MessagesBuddiesOpt::DlgState:
            return "DlgState option";
        case Ui::MessagesBuddiesOpt::Intro:
            return "Intro option";
        case Ui::MessagesBuddiesOpt::Pending:
            return "Pending option";
        case Ui::MessagesBuddiesOpt::Init:
            return "Init option";
        case Ui::MessagesBuddiesOpt::MessageStatus:
            return "MessageStatus option";
        case Ui::MessagesBuddiesOpt::Search:
            return "Search option";
        default:
            return "invalid option";
        }
    }

    template <>
    QLatin1String optionToStr<QLatin1String>(Ui::MessagesBuddiesOpt option) noexcept
    {
        const auto sw = optionToStr<std::string_view>(option);
        return QLatin1String(sw.data(), int(sw.size()));
    }

    constexpr bool isCommonDlgState(Ui::MessagesBuddiesOpt _option) noexcept
    {
        switch (_option)
        {
        case Ui::MessagesBuddiesOpt::DlgState:
        case Ui::MessagesBuddiesOpt::MessageStatus:
        case Ui::MessagesBuddiesOpt::Intro:
        case Ui::MessagesBuddiesOpt::Init:
            return true;

        default:
            return false;
        }
    }

    template <typename R>
    void traceMessageBuddiesImpl(const R& _buddies, qint64 _messageId)
    {
        if (_messageId != -1)
        {
            for (const auto& x : _buddies)
                qCDebug(history) << x->Id_ << " is " << (x->Id_ > _messageId ? "greater" : (x->Id_ == _messageId ? "equal" : "less")) << "  " << x->GetText() << "    prev is " << x->Prev_ << (x->IsDeleted() ? "   deleted" : "");
        }
        else
        {
            for (const auto& x : _buddies)
                qCDebug(history) << x->Id_ << "  " << x->GetText() << "    prev is " << x->Prev_ << (x->IsDeleted() ? "   deleted" : "");
        }
    }

    void traceMessageBuddies(const QString& _aimId,  const Data::MessageBuddies& _buddies, Ui::MessagesBuddiesOpt _option, qint64 _messageId = -1)
    {
        qCDebug(history) << _aimId << "buddies size" << _buddies.size() << "option:" << optionToStr<QLatin1String>(_option);
        traceMessageBuddiesImpl(_buddies, _messageId);
    }

    void traceMessageBuddies(const QString& _aimId, const std::map<qint64, Data::MessageBuddySptr>& _buddies, Ui::MessagesBuddiesOpt _option, qint64 _messageId = -1)
    {
        qCDebug(history) << _aimId << "buddies size" << _buddies.size() << "option:" << optionToStr<QLatin1String>(_option);
        traceMessageBuddiesImpl(boost::adaptors::values(_buddies), _messageId);
    }

    static QVector<qint64> incomingIdsGreaterThan(const Data::MessageBuddies& _buddies, qint64 _id)
    {
        QVector<qint64> res;
        res.reserve(_buddies.size());
        for (const auto& m : _buddies)
            if (!m->IsOutgoing() && m->Id_ > _id && isVisibleMessage(m))
                res.push_back(m->Id_);
        return res;
    }
}

namespace hist
{
    Data::DlgState getDlgState(const QString& _contact)
    {
        auto dlgState = Logic::getRecentsModel()->getDlgState(_contact);
        if (dlgState.AimId_ != _contact)
            dlgState = Logic::getUnknownsModel()->getDlgState(_contact);

        return dlgState;
    }

    History::History(const QString& _aimId, QObject* _parent)
        : QObject(_parent)
        , aimId_(_aimId)
    {
        assert(!aimId_.isEmpty());
        connect(Ui::GetDispatcher(), &Ui::core_dispatcher::messageBuddies, this, &History::messageBuddies);
        connect(Ui::GetDispatcher(), &Ui::core_dispatcher::messageIdsFromServer, this, &History::messageIdsFromServer);
        connect(Ui::GetDispatcher(), &Ui::core_dispatcher::messageIdsUpdated, this, &History::messageIdsUpdated);
        connect(Ui::GetDispatcher(), &Ui::core_dispatcher::messagesDeleted, this, &History::messagesDeleted);
        connect(Ui::GetDispatcher(), &Ui::core_dispatcher::messagesDeletedUpTo, this, &History::messagesDeletedUpTo);
        connect(Ui::GetDispatcher(), &Ui::core_dispatcher::messagesModified, this, &History::messagesModified);
        connect(Ui::GetDispatcher(), &Ui::core_dispatcher::messagesClear, this, &History::messagesClear);
        connect(Ui::GetDispatcher(), &Ui::core_dispatcher::messageContextError, this, &History::messageContextError);
        connect(Ui::GetDispatcher(), &Ui::core_dispatcher::messageLoadAfterSearchError, this, &History::messageLoadAfterSearchError);
        connect(Ui::GetDispatcher(), &Ui::core_dispatcher::removePending, this, &History::pendingAutoRemove);

        connect(&Utils::InterConnector::instance(), &Utils::InterConnector::logHistory, this, &History::dump);
    }

    History::~History() = default;

    Data::MessageBuddies History::fetchBuddies(FetchDirection _direction)
    {
        std::scoped_lock locker(lockForViewFetchCounter_);

        Data::MessageBuddies result;
        size_t visibleCount = 0;
        ViewBounds newViewBounds = viewBounds_;

        auto loadToOlder = [this, &result, &visibleCount, &newViewBounds]()
        {
            const auto resultSizeBefore = result.size();
            const auto bound = viewBounds_.top ? messages_.lower_bound(*viewBounds_.top) : messages_.end();
            const auto range = boost::make_iterator_range(messages_.begin(), bound);
            for (const auto& msg : boost::adaptors::values(boost::adaptors::reverse(range)))
            {
                result.push_back(msg);
                if (isVisibleMessage(msg))
                    ++visibleCount;
                if (visibleCount >= preloadCount())
                    break;
            }

            const auto distance = std::distance(range.begin(), range.end());

            const auto lastMessageId = messages_.empty() ? std::make_optional(qint64(-1)) : (messages_.begin()->second->Prev_ > 0 ? std::make_optional(messages_.begin()->first) : std::nullopt);

            if (size_t(distance - (result.size() - resultSizeBefore)) < preloadCount() && lastMessageId)
            {
                const auto searchMode = initParams_.centralMessage > 0  && initParams_._serverResultsOnly == ServerResultsOnly::Yes ? SearchMode::Yes : SearchMode::No;
                const auto isFirstInit = searchMode == SearchMode::Yes ? FirstRequest::No : getAndResetFirstInitRequest();

                RequestParams _params = { *lastMessageId, RequestDirection::ToOlder, JumpToBottom::No, isFirstInit, Reason::Plain, searchMode };
                requestMessages(_params);
            }

            if (resultSizeBefore != result.size())
            {
                newViewBounds.top = result.constLast()->Id_;
                if (!viewBounds_.bottom)
                    newViewBounds.bottom = result.at(resultSizeBefore)->Id_;
            }
        };


        if (_direction == FetchDirection::toOlder)
        {
            if (viewBounds_.top || pendings_.empty())
            {
                loadToOlder();
            }
            else
            {
                if (viewBounds_.topPending || !pendings_.empty())
                {
                    const auto bound = viewBounds_.topPending ? std::lower_bound(pendings_.begin(), pendings_.end(), MessageWithKey{ *viewBounds_.topPending, nullptr }, msgWithKeyComparator) : pendings_.end();
                    const auto range = boost::make_iterator_range(pendings_.begin(), bound);
                    for (const auto& msg : boost::adaptors::reverse(range))
                    {
                        result.push_back(msg.message);
                        if (isVisibleMessage(msg.message))
                            ++visibleCount;
                        if (visibleCount >= preloadCount())
                            break;
                    }
                }

                if (!result.isEmpty())
                {
                    newViewBounds.topPending = result.constLast()->ToKey();
                    if (!viewBounds_.bottomPending)
                        newViewBounds.bottomPending = result.constFirst()->ToKey();
                }

                if (visibleCount < preloadCount())
                    loadToOlder();
            }
        }
        else
        {
            assert(!messages_.empty() || !pendings_.empty());
            if (viewBounds_.bottom)
            {
                const auto it = messages_.upper_bound(*viewBounds_.bottom);
                const auto range = boost::make_iterator_range(it, messages_.end());
                for (const auto& msg : boost::adaptors::values(range))
                {
                    result.push_back(msg);
                    if (isVisibleMessage(msg))
                        ++visibleCount;
                    if (visibleCount >= preloadCount())
                        break;
                }

                if (!result.isEmpty())
                    newViewBounds.bottom = result.constLast()->Id_;

                const auto distance = std::distance(range.begin(), range.end());
                const auto lastMessageId = messages_.empty() ? std::nullopt : std::make_optional(messages_.crbegin()->first);
                if (size_t(distance - result.size()) < preloadCount() && lastMessageId)
                {
                    const auto searchMode = /*initParams_._mode == scroll_mode_type::search &&*/ initParams_._serverResultsOnly == ServerResultsOnly::Yes ? SearchMode::Yes : SearchMode::No;
                    const auto isFirstInit = searchMode == SearchMode::Yes ? FirstRequest::No : getAndResetFirstInitRequest();

                    RequestParams _params = { *lastMessageId, RequestDirection::ToNewer, JumpToBottom::No, isFirstInit, Reason::Plain, searchMode };
                    requestMessages(_params);
                }

                if (visibleCount < preloadCount())
                {
                    const auto resultSizeBefore = result.size();
                    const auto bound = viewBounds_.bottomPending ? std::upper_bound(pendings_.begin(), pendings_.end(), MessageWithKey{ *viewBounds_.bottomPending, nullptr }, msgWithKeyComparator) : pendings_.begin();
                    for (const auto& msg : boost::make_iterator_range(bound, pendings_.end()))
                    {
                        result.push_back(msg.message);
                        if (isVisibleMessage(msg.message))
                            ++visibleCount;
                        if (visibleCount >= preloadCount())
                            break;
                    }

                    if (resultSizeBefore != result.size())
                    {
                        newViewBounds.bottomPending = result.constLast()->ToKey();

                        if (!newViewBounds.topPending)
                            newViewBounds.topPending = result.at(resultSizeBefore)->ToKey();
                    }
                }
            }
            else
            {
                qCDebug(history) << aimId_ << "no bottom view bound";
            }
        }

        viewBounds_ = std::move(newViewBounds);

        return result;
    }

    void History::initFor(qint64 _messageId, hist::scroll_mode_type _type)
    {
        initForImpl(_messageId, _type, _type == hist::scroll_mode_type::search ? ServerResultsOnly::Yes : ServerResultsOnly::No);
    }

    Data::MessageBuddies History::getInitMessages(std::optional<qint64> messageId) const
    {
        std::scoped_lock locker(lockForViewFetchCounter_);

        Data::MessageBuddies result;
        const auto end = messages_.end();
        const auto centerIt = messageId.has_value() ? messages_.lower_bound(*messageId) : end;
        size_t visibleCount = 0;
        ViewBounds newViewBounds;
        if (centerIt == end)
        {
            for (const auto& msg : boost::adaptors::reverse(pendings_))
            {
                result.push_back(msg.message);
                if (isVisibleMessage(msg.message))
                    ++visibleCount;
                if (visibleCount >= preloadCount())
                    break;
            }

            if (!result.empty())
            {
                newViewBounds.topPending = result.back()->ToKey();
                newViewBounds.bottomPending = result.front()->ToKey();
            }

            if (visibleCount < preloadCount())
            {
                const auto prevVisibleCount = visibleCount;
                const auto prevResultSize = result.size();
                for (const auto& msg : boost::adaptors::reverse(boost::adaptors::values(messages_)))
                {
                    result.push_back(msg);
                    if (isVisibleMessage(msg))
                        ++visibleCount;
                    if (visibleCount >= preloadCount())
                        break;
                }

                if (prevVisibleCount < visibleCount)
                {
                    newViewBounds.bottom = result[prevResultSize]->Id_;
                    newViewBounds.top = result.back()->Id_;
                }
            }
        }
        else
        {
            for (const auto& msg : boost::adaptors::values(boost::adaptors::reverse(boost::make_iterator_range(messages_.begin(), centerIt))))
            {
                result.push_back(msg);
                if (isVisibleMessage(msg))
                    ++visibleCount;
                if (visibleCount >= preloadCount())
                    break;
            }
            visibleCount = 0;

            std::reverse(result.begin(), result.end());

            for (const auto& msg : boost::adaptors::values(boost::make_iterator_range(centerIt, end)))
            {
                result.push_back(msg);
                if (isVisibleMessage(msg))
                    ++visibleCount;
                if (visibleCount >= preloadCount())
                    break;
            }

            if (!result.isEmpty())
            {
                newViewBounds.top = result.constFirst()->Id_;
                newViewBounds.bottom = result.constLast()->Id_;
            }

            if (visibleCount < preloadCount() && !pendings_.empty())
            {
                const auto prevResultSize = result.size();
                for (const auto& msg : pendings_)
                {
                    result.push_back(msg.message);
                    if (isVisibleMessage(msg.message))
                        ++visibleCount;
                    if (visibleCount >= preloadCount())
                        break;
                }

                if (prevResultSize != result.size())
                {
                    newViewBounds.topPending = result.at(prevResultSize)->ToKey();
                    newViewBounds.bottomPending = result.constLast()->ToKey();
                }
            }
        }

        viewBounds_ = std::move(newViewBounds);
        viewBounds_.checkInvarinat();

        std::sort(result.begin(), result.end(), msgComparator);

        return result;
    }

    void History::scrollToBottom()
    {
        initFor(-1, hist::scroll_mode_type::none);
    }

    bool History::canViewFetch() const noexcept
    {
        return !lockForViewFetchCounter_.isLocked() && !waitingInitSeq_;
    }

    bool History::isViewLocked() const noexcept
    {
        return lockForViewFetchCounter_.isLocked();
    }

    bool History::hasFetchRequest(FetchDirection _direction) const
    {
        const auto dir = _direction == FetchDirection::toNewer ? RequestDirection::ToNewer : RequestDirection::ToOlder;
        return executedRequests.hasFetchRequests(dir);
    }

    bool History::canUnload(UnloadDirection _direction, CheckViewLock _mode) const noexcept
    {
        const auto dir = _direction == UnloadDirection::toBottom ? RequestDirection::ToNewer : RequestDirection::ToOlder;
        const auto canFetch = _mode == CheckViewLock::Yes ? canViewFetch() : true;
        return canFetch && !executedRequests.hasFetchRequests(dir);
    }

    void History::setTopBound(const Logic::MessageKey& _key, CheckViewLock _mode)
    {
        if (_mode == CheckViewLock::Yes)
            assert(canViewFetch());
        else
            assert(isViewLocked());
        assert(canUnload(UnloadDirection::toTop, _mode));
        qCDebug(history) << aimId_ << "setTopBound" << _key.getId() << _key.getInternalId();
        logCurrentIds(qsl("setTopBoundBefore"), LogType::Simple);
        if (_key.hasId())
        {
            assert(viewBounds_.top);
            if (viewBounds_.top)
                viewBounds_.top = _key.getId();
        }
        else
        {
            assert(viewBounds_.topPending);
            if (viewBounds_.topPending)
            {
                viewBounds_.topPending = _key;
                viewBounds_.top = std::nullopt;
                viewBounds_.bottom = std::nullopt;
            }
        }
        logCurrentIds(qsl("setTopBoundAfter"), LogType::Simple);
        viewBounds_.checkInvarinat();
    }

    void History::setBottomBound(const Logic::MessageKey& _key, CheckViewLock _mode)
    {
        if (_mode == CheckViewLock::Yes)
            assert(canViewFetch());
        else
            assert(isViewLocked());
        assert(canUnload(UnloadDirection::toBottom, _mode));
        qCDebug(history) << aimId_ << "setBottomBound" << _key.getId() << _key.getInternalId();
        logCurrentIds(qsl("setBottomBoundBefore"), LogType::Simple);
        if (_key.hasId())
        {
            assert(viewBounds_.bottom);
            if (viewBounds_.bottom)
            {
                viewBounds_.bottom = _key.getId();
                viewBounds_.bottomPending = std::nullopt;
                viewBounds_.topPending = std::nullopt;
            }
        }
        else
        {
            assert(viewBounds_.bottomPending);
            if (viewBounds_.bottomPending)
                viewBounds_.bottomPending = _key;
        }
        logCurrentIds(qsl("setBottomBoundAfter"), LogType::Simple);
        viewBounds_.checkInvarinat();
    }

    bool History::isVisibleHoleBetween(qint64 _older, qint64 _newer) const
    {
        if (_older == _newer)
            return false;

        if (_newer < _older)
            return true;

        const auto oIt = messages_.find(_older);
        const auto nIt = messages_.find(_newer);
        if (nIt != messages_.end() && oIt != messages_.end())
        {
            assert(isVisibleMessage(nIt->second) && isVisibleMessage(oIt->second));
            for (auto it = nIt; it != oIt; --it)
            {
                if (const auto prev = std::prev(it); prev == oIt)
                    return it->second->Prev_ != oIt->first;
                else if (!isVisibleMessage(prev->second))
                    continue;

                return true;
            }
            return false;
        }
        return true;
    }

    void History::initForImpl(qint64 _messageId, hist::scroll_mode_type _type, ServerResultsOnly _serverResultsOnly)
    {
        std::scoped_lock locker(lockForViewFetchCounter_);

        initParams_ = { _messageId, _type, _serverResultsOnly };

        if (hasAllMessagesForInit(_messageId, _type))
        {
            waitingInitSeq_ = std::nullopt;
            auto lastMessageIvView = [this]()
            {
                if (viewBounds_.inBounds(lastMessageId_))
                    return true;

                if (messages_.empty() && !pendings_.empty())
                    return viewBounds_.inBounds(pendings_.back().key);

                return false;
            };
            if (_type == hist::scroll_mode_type::none && _messageId == -1 && lastMessageIvView())
            {
                qCDebug(history) << "no need to request and emit init";
            }
            else
            {
                clearExecutedRequests();
                emit readyInit(initParams_.centralMessage, initParams_._mode, QPrivateSignal());
            }
            return;
        }

        clearExecutedRequests();

        viewBounds_ = {};

        // optimize me
        messages_.clear();

        if (messages_.empty())
        {
            if (/*_type == hist::scroll_mode_type::search*/ _messageId > 0 && _serverResultsOnly == ServerResultsOnly::Yes)
            {
                waitingInitSeq_ = requestMessageContext(_messageId);
                qCDebug(history) << *waitingInitSeq_ << "initfor requestMessageContext: " << _messageId;
            }
            else
            {
                const auto direction = _messageId > 0 ? RequestDirection::Bidirection : RequestDirection::ToOlder;
                RequestParams _params = { _messageId, direction, JumpToBottom::Yes, getAndResetFirstInitRequest(), Reason::Plain, SearchMode::No };
                waitingInitSeq_ = requestMessages(_params);
                qCDebug(history) << *waitingInitSeq_ << "initfor message id: " << _messageId;
            }
        }
        else
        {
            if (_messageId > 0)
            {

            }
            else
            {

            }
        }
    }

    void History::messageBuddies(const Data::MessageBuddies& _buddies, const QString& _aimId, Ui::MessagesBuddiesOpt _option, bool _havePending, qint64 _seq, qint64 _lastMsgid)
    {
        if (_aimId != aimId_)
            return;

        std::unique_lock locker(lockForViewFetchCounter_);

        const bool needWaitingForSeq = waitingInitSeq_.has_value();
        const bool isRequiredWaitingSeq = needWaitingForSeq && (_seq == *waitingInitSeq_);

        if constexpr (build::is_debug())
            traceMessageBuddies(_aimId, _buddies, _option);

        const auto scopedExit = Utils::ScopeExitT([this, _seq]() { removeSeq(_seq); viewBounds_.checkInvarinat(); });

        const auto prevLastId = std::exchange(lastMessageId_, std::max({ lastMessageId_, _lastMsgid, !_buddies.isEmpty() ? _buddies.constLast()->Id_ : -1 }));

        processChatEvents(_buddies, _option, _seq);

        if (needIgnoreMessages(_buddies, _option, _havePending, _seq, _lastMsgid))
        {
            const auto newIds = incomingIdsGreaterThan(_buddies, prevLastId);
            if (!newIds.isEmpty())
                emit newMessagesReceived(newIds, QPrivateSignal());

            logCurrentIds(qsl("needIgnoreMessages"), LogType::Simple);
            return;
        }

        if (_option == Ui::MessagesBuddiesOpt::Updated)
        {
            processUpdatedMessage(_buddies);
            return;
        }
        else if (_option == Ui::MessagesBuddiesOpt::RequestedByIds || _option == Ui::MessagesBuddiesOpt::Search)
        {
            processRequestedByIds(_buddies);
            return;
        }

        if (isRequiredWaitingSeq)
        {
            messages_.clear();
            viewBounds_ = {};
        }

        const int buddiesSize = _buddies.size();
        const int pendingIdx = _havePending || _option == Ui::MessagesBuddiesOpt::Pending ? getPendingIdx(_buddies) : buddiesSize;

        const auto pendingRange = boost::make_iterator_range(std::next(_buddies.begin(), pendingIdx), _buddies.end());
        const auto buddiesRange = boost::make_iterator_range(_buddies.begin(), std::next(_buddies.begin(), pendingIdx));

        auto pendingResult = insertPendings(pendingRange);
        auto buddiesResult = insertBuddies(buddiesRange, _option);

        const auto containsLastMessage = std::any_of(buddiesRange.begin(), buddiesRange.end(), hasId(_lastMsgid));

        if (containsLastMessage)
            addToTail(buddiesRange);

        std::sort(tail_.begin(), tail_.end(), msgWithKeyComparator);

        if (isRequiredWaitingSeq)
        {
            if (const auto fixed = fixIdAndMode(initParams_.centralMessage, initParams_._mode); fixed.id && fixed.mode)
            {
                qCDebug(history) << "emit ready fixed" << *fixed.id;
                emit readyInit(*fixed.id, *fixed.mode, QPrivateSignal());
            }
            else
            {
                emit readyInit(initParams_.centralMessage, initParams_._mode, QPrivateSignal());
            }
            waitingInitSeq_ = std::nullopt;

            statistic::getGuiMetrics().eventChatLoaded(_aimId);
        }
        else
        {
            proccesInserted(pendingResult, buddiesResult, _option, _seq, prevLastId);
        }

        emit updateLastSeen(getDlgState(aimId_), QPrivateSignal());
    }

    template <typename R>
    inline void History::addToTail(const R& range)
    {
     /*   for (const auto& msg : range)
            tail_.push_back({ msg->ToKey(), msg });
            */
    }

    template<typename R>
    [[nodiscard]] History::PendingsInsertResult History::insertPendings(const R& range)
    {
        Data::MessageBuddies inserted;
        Data::MessageBuddies updated;

        for (const auto& msg : range)
        {
            assert(msg->IsPending());
            auto key = msg->ToKey();
            if (auto it = std::find_if(pendings_.begin(), pendings_.end(), isKeyEqual(key)); it != pendings_.end())
            {
                if (!msg->GetUpdatePatchVersion().is_empty() || msg->GetUpdatePatchVersion().get_offline_version() > 0)
                {
                    *it = { std::move(key), msg };
                    updated.push_back(msg);
                }
            }
            else
            {
                if (!containsInternalId(msg->InternalId_))
                {
                    pendings_.push_back({ std::move(key), msg });
                    inserted.push_back(msg);
                }
            }
        }

        if (!inserted.isEmpty())
            std::sort(pendings_.begin(), pendings_.end(), msgWithKeyComparator);

        return { std::move(inserted), std::move(updated) };
    }

    template<typename R>
    [[nodiscard]] History::BuddiesInsertResult History::insertBuddies(const R& range, Ui::MessagesBuddiesOpt option)
    {
        Data::MessageBuddies inserted;
        Data::MessageBuddies updated;
        Data::MessageBuddies pendingResolves;
        bool canFetch = false;

        for (const auto& msg : range)
        {
            bool isPendingResolve = false;
            if (msg->IsOutgoing())
            {
                if (const auto it = std::find_if(pendings_.begin(), pendings_.end(), isKeyEqual(msg->ToKey())); it != pendings_.end())
                {
                    pendings_.erase(it);
                    addToTail(msg);
                    pendingResolves.push_back(msg);
                    isPendingResolve = true;
                }
            }

            auto& currentMsg = messages_[msg->Id_];

            if (currentMsg)
            {
                if (currentMsg->needUpdateWith(*msg))
                {
                    currentMsg = msg;
                    if (!isPendingResolve)
                        updated.push_back(msg);
                }

                // skip, no need to update
            }
            else
            {
                if (isCommonDlgState(option))
                {
                    if (msg->Prev_ != -1 && !containsMsgId(msg->Prev_))
                    {
                        messages_.erase(msg->Id_);
                        canFetch = true;
                        continue;
                    }
                }
                currentMsg = msg;
                if (!isPendingResolve)
                    inserted.push_back(msg);
            }
        }
        return { std::move(inserted), std::move(updated), std::move(pendingResolves), canFetch };
    }

    enum class IdsMessages
    {
        Updated,
        New
    };

    template<typename R>
    [[nodiscard]] static qint64 requestMessagesByIds(const QString& aimId, IdsMessages mode, const R& range)
    {
        assert(std::distance(range.begin(), range.end()) > 0);
        Ui::gui_coll_helper collection(Ui::GetDispatcher()->create_collection(), true);
        collection.set_value_as_qstring("contact", aimId);
        core::ifptr<core::iarray> idsArray(collection->create_array());
        idsArray->reserve(int32_t(std::distance(range.begin(), range.end())));
        for (auto id : range)
        {
            core::ifptr<core::ivalue> val = collection->create_value();
            val->set_as_int64(id);
            idsArray->push_back(val.get());
        }
        if (mode == IdsMessages::Updated)
            collection.set_value_as_bool("updated", true);
        collection.set_value_as_array("ids", idsArray.get());
        return Ui::GetDispatcher()->post_message_to_core("archive/buddies/get", collection.get());
    }

    void History::messageIdsFromServer(const QVector<qint64>& _ids, const QString& _aimId, qint64 _seq)
    {
        if (_ids.isEmpty() || _aimId != aimId_)
            return;

        qCDebug(history) << _aimId << "messageIdsFromServer" << _ids;

        logCurrentIds(qsl("messageIdsFromServer"), LogType::Simple);

        if (initParams_.centralMessage > 0 && _ids.contains(initParams_.centralMessage))
        {
            // reinit to scroll
            initFor(initParams_.centralMessage, initParams_._mode);
            return;
        }

        assert(std::is_sorted(_ids.begin(), _ids.end()));

        std::scoped_lock locker(lockForViewFetchCounter_);

        if (messages_.empty())
        {
            // no messages in dialog. need to fetch from last
            emit canFetch(FetchDirection::toOlder, QPrivateSignal());
            return;
        }

        qCDebug(history) << _aimId << "bounds " << messages_.cbegin()->first << messages_.crbegin()->first;

        std::vector<qint64> inBounds;
        std::copy_if(_ids.begin(), _ids.end(), std::back_inserter(inBounds),
            [tb = messages_.cbegin()->first, bb = messages_.crbegin()->first](auto id)
        {
            return id > tb && id < bb;
        });

        if (!inBounds.empty())
        {
            // request only messages between first and last messages in dialog
            ExecutedByIdRequestParams req;
            req.ids = std::move(inBounds);
            req.seq = requestMessagesByIds(aimId_, IdsMessages::New, req.ids);

            qCDebug(history) << _aimId << req.seq << "request by ids" << QVector<qint64>::fromStdVector(req.ids);

            executedRequests.byIds.push_back(std::move(req));
        }

        const bool canFetchToOlder = std::any_of(_ids.begin(), _ids.end(), [tb = messages_.cbegin()->first](auto x) { return x < tb; });
        const bool canFetchToNewer = std::any_of(_ids.begin(), _ids.end(), [bb = messages_.crbegin()->first](auto x) { return x > bb; });

        if (canFetchToOlder)
            emit canFetch(FetchDirection::toOlder, QPrivateSignal());
        else if (canFetchToNewer)
            emit canFetch(FetchDirection::toNewer, QPrivateSignal());
    }

    void History::messageIdsUpdated(const QVector<qint64>& _ids, const QString& _aimId, qint64 _seq)
    {
        if (_ids.isEmpty() || messages_.empty() || _aimId != aimId_)
            return;

        std::vector<qint64> inBounds;
        std::copy_if(_ids.begin(), _ids.end(), std::back_inserter(inBounds),
            [tb = messages_.cbegin()->first, bb = messages_.crbegin()->first](auto id)
        {
            return id >= tb && id <= bb;
        });

        if (!inBounds.empty())
        {
            ExecutedByIdRequestParams req;
            req.seq = requestMessagesByIds(aimId_, IdsMessages::Updated, inBounds);
            req.ids = std::move(inBounds);
            qCDebug(history) << _aimId << req.seq << "messageIdsUpdated req" << QVector<qint64>::fromStdVector(req.ids);
            executedRequests.byIds.push_back(std::move(req));
        }
    }

    void History::messagesDeleted(const QString& _aimId, const Data::MessageBuddies& _buddies)
    {
        if (_buddies.isEmpty() || (messages_.empty() && pendings_.empty()) || _aimId != aimId_)
            return;

        std::scoped_lock locker(lockForViewFetchCounter_);

        Data::MessageBuddies deleted;

        for (const auto& msg : _buddies)
        {
            if (msg->IsPending())
            {
                const auto key = msg->ToKey();
                const auto it = std::find_if(pendings_.begin(), pendings_.end(), isKeyEqual(key));
                if (it != pendings_.end())
                {
                    it->message->SetDeleted(true);
                    deleted.push_back(msg);
                }
            }
            else
            {
                if (const auto it = messages_.find(msg->Id_); it != messages_.end())
                {
                    it->second->SetDeleted(true);
                    deleted.push_back(msg);
                }
            }
        }

        clearPendingsIfPossible();

        if (!deleted.isEmpty())
            emit deletedBuddies(deleted, QPrivateSignal());
    }

    void History::messagesDeletedUpTo(const QString& _aimId, qint64 _id)
    {
        if (_aimId != aimId_)
            return;

        std::scoped_lock locker(lockForViewFetchCounter_);

        const auto it = messages_.upper_bound(_id);
        const auto rangeForDelete = it != messages_.end() ? boost::make_iterator_range(messages_.begin(), it) : boost::make_iterator_range(messages_.begin(), messages_.end());

        Data::MessageBuddies deleted;

        if (!viewBounds_.isEmpty())
        {
            for (const auto& [id, msg] : rangeForDelete)
            {
                if (id >= viewBounds_.top && id <= viewBounds_.bottom)
                    deleted.push_back(msg);
            }
        }

        messages_.erase(rangeForDelete.begin(), rangeForDelete.end());

        if (messages_.empty())
        {
            viewBounds_.top = std::nullopt;
            viewBounds_.bottom = std::nullopt;
        }

        if (!deleted.isEmpty())
            emit deletedBuddies(deleted, QPrivateSignal());
    }

    void History::pendingAutoRemove(const QString& _aimId, const Logic::MessageKey& _key)
    {
        if (_aimId != aimId_)
            return;

        assert(_key.isPending());

        const auto it = std::lower_bound(pendings_.begin(), pendings_.end(), MessageWithKey{ _key, nullptr }, msgWithKeyComparator);
        if (it != pendings_.end() && it->key == _key)
        {
            it->message->SetDeleted(true);
            Data::MessageBuddies deleted;
            deleted.push_back(it->message);

            clearPendingsIfPossible();

            emit deletedBuddies(deleted, QPrivateSignal());
        }
    }

    void History::clearPendingsIfPossible()
    {
        if (pendings_.empty() || std::all_of(pendings_.begin(), pendings_.end(), [](const auto& x) { return x.message->IsDeleted(); }))
        {
            pendings_ = {};
            viewBounds_.topPending = std::nullopt;
            viewBounds_.bottomPending = std::nullopt;
        }
    }

    void History::messageContextError(const QString& _aimId, qint64 _seq, qint64 _id, Ui::MessagesNetworkError _error)
    {
        if (_aimId != aimId_)
            return;

        const auto scopedExit = Utils::ScopeExitT([this, _seq]() { removeSeq(_seq); });

        const bool needWaitingForSeq = waitingInitSeq_.has_value();
        const bool isRequiredWaitingSeq = needWaitingForSeq && (_seq == *waitingInitSeq_);

        if (isRequiredWaitingSeq)
        {
            if (_error == Ui::MessagesNetworkError::Yes)
            {
                initForImpl(_id, scroll_mode_type::search, ServerResultsOnly::No);
            }
            else
            {
                initForImpl(_id, scroll_mode_type::search, ServerResultsOnly::Yes);
            }
        }
    }

    void History::messageLoadAfterSearchError(const QString& _aimId, qint64 _seq, qint64 _from, qint64 _countLater, qint64 _countEarly, Ui::MessagesNetworkError _error)
    {
        if (_aimId != aimId_)
            return;

        const auto scopedExit = Utils::ScopeExitT([this, _seq]() { removeSeq(_seq); });
        const auto it = std::find_if(executedRequests.fetch.begin(), executedRequests.fetch.end(), hasSeq(_seq));
        if (it != executedRequests.fetch.end())
        {
            if (_error == Ui::MessagesNetworkError::Yes)
            {
                initParams_._serverResultsOnly = ServerResultsOnly::No;

                const auto searchMode = /*initParams_._mode == scroll_mode_type::search &&*/ initParams_._serverResultsOnly == ServerResultsOnly::Yes ? SearchMode::Yes : SearchMode::No;
                const auto isFirstInit = searchMode == SearchMode::Yes ? FirstRequest::No : getAndResetFirstInitRequest();
                auto params = *it;
                Ui::gui_coll_helper collection(Ui::GetDispatcher()->create_collection(), true);
                collection.set_value_as_qstring("contact", aimId_);
                collection.set_value_as_int64("from", _from); // this id is included
                collection.set_value_as_int64("count_early", _countEarly);
                collection.set_value_as_int64("count_later", _countLater);
                collection.set_value_as_bool("need_prefetch", true);
                collection.set_value_as_bool("is_first_request", isFirstInit == FirstRequest::Yes);
                collection.set_value_as_bool("after_search", searchMode == SearchMode::Yes);

                const auto seq = Ui::GetDispatcher()->post_message_to_core("archive/messages/get", collection.get());

                params.seq = seq;

                executedRequests.fetch.push_back(params);
            }
        }
    }

    void History::messagesClear(const QString& _aimId, qint64 _seq)
    {
        if (_aimId != aimId_)
            return;

        std::scoped_lock locker(lockForViewFetchCounter_);

        messages_.clear();
        pendings_.clear();
        tail_.clear();

        viewBounds_ = {};

        clearExecutedRequests();

        emit clearAll(QPrivateSignal());
    }

    void History::messagesModified(const QString& _aimId, const Data::MessageBuddies& _modifications)
    {
        if (_aimId != aimId_)
            return;

        Data::MessageBuddies updated;
        for (const auto &modification : _modifications)
        {
            if (!modification->HasText() &&
                !modification->IsChatEvent())
            {
                assert(!"unexpected modification");
                continue;
            }

            if (modification->HasId())
            {
                if (auto it = messages_.find(modification->Id_); it != messages_.end())
                {
                    auto& currentMsg = it->second;
                    // core send empty date. restore it
                    const auto currentTime = currentMsg->GetTime();
                    const auto currentDate = currentMsg->GetDate();
                    currentMsg = modification;
                    currentMsg->SetTime(currentTime);
                    currentMsg->SetDate(currentDate);
                    updated.push_back(currentMsg);
                }
            }
            else if (modification->IsPending())
            {
                auto key = modification->ToKey();
                if (auto it = std::find_if(pendings_.begin(), pendings_.end(), isKeyEqual(key)); it != pendings_.end())
                {
                    it->key = std::move(key);
                    const auto currentTime = it->message->GetTime();
                    const auto currentDate = it->message->GetDate();
                    it->message = modification;
                    it->message->SetTime(currentTime);
                    it->message->SetDate(currentDate);
                    updated.push_back(it->message);
                }
            }
        }

        if (!updated.isEmpty())
            emit updatedBuddies(updated, QPrivateSignal());
    }

    void History::addToTail(const Data::MessageBuddySptr& _msg)
    {
        const auto r = { _msg };
        addToTail(boost::make_iterator_range(r.begin(), r.end()));
    }

    void History::processChatEvents(const Data::MessageBuddies & _msgs, Ui::MessagesBuddiesOpt _option, qint64 _seq) const
    {
        if (isCommonDlgState(_option) || _option == Ui::MessagesBuddiesOpt::RequestedByIds)
        {
            QVector<HistoryControl::ChatEventInfoSptr> events;
            for (const auto& msg : _msgs)
            {
                if (msg->IsChatEvent())
                    events.push_back(msg->GetChatEvent());
            }

            if (!events.isEmpty())
                emit Utils::InterConnector::instance().chatEvents(aimId_, events);
        }
    }

    bool History::hasAllMessagesForInit(qint64 messageId, hist::scroll_mode_type _type) const
    {
        if (_type == hist::scroll_mode_type::none && messageId == -1)
        {
            if (lastMessageId_ > 0)
            {
                if (messages_.find(lastMessageId_) != messages_.end())
                    return !hasHoleAtBottom();
            }
            else if (messages_.empty() && !pendings_.empty())
            {
                return true;
            }
        }
        return false;
    }

    bool History::hasHoleAtBottom() const
    {
        std::vector<qint64> visibleItems;
        for (const auto& msg : boost::adaptors::reverse(boost::adaptors::values(messages_)))
        {
            if (isVisibleMessage(msg))
            {
                visibleItems.push_back(msg->Id_);
                if (visibleItems.size() >= preloadCount())
                    break;
            }
        }

        if (visibleItems.size() < 2)
            return false;

        std::reverse(visibleItems.begin(), visibleItems.end());

        const auto it = std::adjacent_find(visibleItems.begin(), visibleItems.end(), [this](auto older, auto newer) { return isVisibleHoleBetween(older, newer); });
        return it != visibleItems.end();
    }

    void History::proccesInserted(const PendingsInsertResult& _pendingResult, const BuddiesInsertResult& _buddiesResult, Ui::MessagesBuddiesOpt _option, qint64 _seq, qint64 _prevLastId)
    {
        const bool needLog = !_pendingResult.inserted.isEmpty() || !_pendingResult.updated.isEmpty() || !_buddiesResult.inserted.isEmpty() || !_buddiesResult.pendingResolves.isEmpty() || !_buddiesResult.updated.isEmpty();
        if (needLog)
            logCurrentIds(qsl("proccesInsertedBefore"), LogType::Simple);

        auto hasMessagesInBounds = [](const auto& messages, const auto& bounds)
        {
            return std::any_of(messages.begin(), messages.end(), [&bounds](const auto& msg)
            {
                if (!bounds.isEmpty())
                    return msg->Id_ < bounds.bottom && msg->Id_ > bounds.top;
                return false;
            });
        };

        if (!_buddiesResult.updated.isEmpty())
            emit updatedBuddies(_buddiesResult.updated, QPrivateSignal());

        const auto hasRequestedInside = (_option == Ui::MessagesBuddiesOpt::Requested) && hasMessagesInBounds(_buddiesResult.inserted, viewBounds_);

        if (isCommonDlgState(_option) || _option == Ui::MessagesBuddiesOpt::Pending || hasRequestedInside)
        {
            Data::MessageBuddies pendingResolvesInViewBounds;
            if (!_buddiesResult.pendingResolves.isEmpty())
            {
                if (viewBounds_.hasPendings())
                {
                    for (const auto& msg : _buddiesResult.pendingResolves)
                    {
                        const auto id = msg->Id_;
                        const auto key = msg->ToKey().toPurePending();
                        if (viewBounds_.topPending <= key && key <= viewBounds_.bottomPending)
                        {
                            if (pendings_.empty() || (viewBounds_.topPending == key && key == viewBounds_.bottomPending))
                            {
                                viewBounds_.topPending = std::nullopt;
                                viewBounds_.bottomPending = std::nullopt;
                            }
                            else if (viewBounds_.topPending == key)
                            {
                                assert(!pendings_.empty());
                                const auto it = std::lower_bound(pendings_.begin(), pendings_.end(), MessageWithKey{ *viewBounds_.topPending, nullptr }, msgWithKeyComparator);
                                assert(it != pendings_.end());
                                if (it != pendings_.end())
                                {
                                    assert(it->key != key);
                                    viewBounds_.topPending = it->key;
                                }
                            }
                            else if (viewBounds_.bottomPending == key)
                            {
                                assert(!pendings_.empty());
                                const auto it = std::find_if(pendings_.crbegin(), pendings_.crend(), [&key](const auto& x){
                                    return x.key < key;
                                });
                                assert(it != pendings_.crend());
                                if (it != pendings_.crend())
                                {
                                    assert(it->key != key);
                                    viewBounds_.bottomPending = it->key;
                                }
                            }
                            else
                            {
                                // this msg is not on the edge
                            }

                            pendingResolvesInViewBounds.push_back(msg);
                            viewBounds_.top = std::min(id, viewBounds_.top.value_or(id));
                            viewBounds_.bottom = std::max(id, viewBounds_.bottom.value_or(id));
                        }
                        else
                        {
                            // this msg is not in view bounds
                        }
                    }
                }

                emit pendingResolves(pendingResolvesInViewBounds, _buddiesResult.pendingResolves, QPrivateSignal());
            }

            if (!_pendingResult.updated.isEmpty())
                emit updatedBuddies(_pendingResult.updated, QPrivateSignal());

            Data::MessageBuddies needToInsert;
            if (!_buddiesResult.inserted.isEmpty())
            {
                auto updateViewBound = [&needToInsert, this]()
                {
                    const auto frontId = std::as_const(needToInsert).front()->Id_;
                    const auto backId = std::as_const(needToInsert).back()->Id_;
                    viewBounds_.top = std::min(frontId, viewBounds_.top.value_or(frontId));
                    viewBounds_.bottom = std::max(backId, viewBounds_.bottom.value_or(backId));
                };

                if (viewBounds_.isEmpty() || (*viewBounds_.bottom == _buddiesResult.inserted.front()->Prev_))
                {
                    needToInsert += _buddiesResult.inserted;
                    updateViewBound();
                }
                else
                {
                    for (const auto& msg : _buddiesResult.inserted)
                    {
                        // msg->Prev_ <= viewBounds_.bottom - because we don't fix prev id of deleted message
                        if ((msg->Id_ >= *viewBounds_.top && (msg->Id_ <= viewBounds_.bottom || msg->Prev_ <= viewBounds_.bottom)) || (*viewBounds_.bottom == msg->Prev_))
                        {
                            needToInsert += msg;
                            updateViewBound();
                        }
                    }
                }

                if (!needToInsert.isEmpty())
                {
                    qCDebug(history) << "insert dlg state";
                }
                else
                {
                    qCDebug(history) << "skip dlg state";
                    logCurrentIds(qsl("skip dlg state"), LogType::Simple);
                }
            }

            if (!_pendingResult.inserted.isEmpty())
            {
                needToInsert += _pendingResult.inserted;

                const auto topInsertedKey = _pendingResult.inserted.front()->ToKey();
                const auto bottomInsertedKey = _pendingResult.inserted.back()->ToKey();
                viewBounds_.topPending = std::min(topInsertedKey, viewBounds_.topPending.value_or(topInsertedKey));
                viewBounds_.bottomPending = std::max(bottomInsertedKey, viewBounds_.bottomPending.value_or(bottomInsertedKey));
            }

            std::sort(needToInsert.begin(), needToInsert.end());

            if (!needToInsert.isEmpty())
            {
                const auto newIds = incomingIdsGreaterThan(needToInsert, _prevLastId);
                if (!newIds.isEmpty())
                    emit newMessagesReceived(newIds, QPrivateSignal());
                emit dlgStateBuddies(needToInsert, QPrivateSignal());
            }
        }
        else
        {
            if (!_buddiesResult.inserted.isEmpty())
            {
                if (const auto it = std::find_if(executedRequests.fetch.cbegin(), executedRequests.fetch.cend(), hasSeq(_seq)); it != executedRequests.fetch.cend())
                {
                    if (it->direction == RequestDirection::ToNewer)
                        emit canFetch(hist::FetchDirection::toNewer, QPrivateSignal());
                    else if (it->direction == RequestDirection::ToOlder)
                        emit canFetch(hist::FetchDirection::toOlder, QPrivateSignal());
                }
            }
        }

        if (needLog)
            logCurrentIds(qsl("proccesInsertedAfter"), LogType::Simple);

        viewBounds_.checkInvarinat();
    }

    std::optional<qint64> History::requestMessages(RequestParams _params)
    {
        auto findExecutedRequest = [](auto first, auto last, auto params)
        {
            return std::find_if(first, last, [params](auto x) {
                return x.id == params.id && x.direction == params.direction;
            });
        };

        auto currentReq = ExecutedFetchRequestParams{ 0, _params.messageId, _params.direction };

        const auto it = findExecutedRequest(executedRequests.fetch.cbegin(), executedRequests.fetch.cend(), currentReq);

        qint64 countEarly = 0;
        qint64 countLater = 0;

        switch (_params.direction)
        {
        case RequestDirection::Bidirection:
        {
            countEarly = countLater = preloadCount();
            if (it != executedRequests.fetch.cend())
            {
                qCDebug(history) << aimId_ << "erase req " << it->seq;
                executedRequests.fetch.erase(it);
            }
        }
        break;
        case RequestDirection::ToNewer:
        {
            if (it != executedRequests.fetch.cend() && _params.firstRequest != FirstRequest::Yes)
                return std::make_optional(it->seq);

            countLater = preloadCount();
        }
        break;
        case RequestDirection::ToOlder:
        {
            if (it != executedRequests.fetch.cend() && _params.firstRequest != FirstRequest::Yes)
                return std::make_optional(it->seq);

            countEarly = preloadCount();
        }
        break;
        }

        Ui::gui_coll_helper collection(Ui::GetDispatcher()->create_collection(), true);
        collection.set_value_as_qstring("contact", aimId_);
        collection.set_value_as_int64("from", _params.messageId); // this id is included
        collection.set_value_as_int64("count_early", countEarly);
        collection.set_value_as_int64("count_later", countLater);
        collection.set_value_as_bool("need_prefetch", true);
        collection.set_value_as_bool("is_first_request", _params.firstRequest == FirstRequest::Yes);
        collection.set_value_as_bool("after_search", _params.search == SearchMode::Yes);

        const auto seq = Ui::GetDispatcher()->post_message_to_core("archive/messages/get", collection.get());

        currentReq.seq = seq;

        executedRequests.fetch.push_back(currentReq);
        return std::make_optional(seq);
    }

    qint64 History::requestMessageContext(qint64 _messageId)
    {
        auto currentParams = ExecutedFetchRequestParams{ 0, _messageId, RequestDirection::Bidirection, true };
        auto& fetchs = executedRequests.fetch;
        fetchs.erase(std::remove_if(fetchs.begin(), fetchs.end(), hasExecutedFetchRequestParams(currentParams)), fetchs.end());
        Ui::gui_coll_helper collection(Ui::GetDispatcher()->create_collection(), true);
        collection.set_value_as_qstring("contact", aimId_);
        collection.set_value_as_int64("msgid", _messageId);

        const auto seq = Ui::GetDispatcher()->post_message_to_core("messages/context/get", collection.get());
        currentParams.seq = seq;

        executedRequests.fetch.push_back(currentParams);
        return seq;
    }

    void History::processUpdatedMessage(const Data::MessageBuddies& _buddies)
    {
        Data::MessageBuddies updated;
        Data::MessageBuddies inserted;
        for (const auto& msg : _buddies)
        {
            if (auto it = messages_.find(msg->Id_); it != messages_.end())
            {
                auto& currentMsg = it->second;
                if (msg->GetUpdatePatchVersion() < currentMsg->GetUpdatePatchVersion())
                {
                    continue;
                }
                else
                {
                    currentMsg = msg;
                    updated.push_back(msg);
                }
            }
            else
            {
                inserted.push_back(msg);
            }
        }

        if (!updated.isEmpty())
            emit updatedBuddies(updated, QPrivateSignal());

        if (!inserted.isEmpty())
            processRequestedByIds(inserted);
    }

    void History::processRequestedByIds(const Data::MessageBuddies& _buddies)
    {
        assert(!messages_.empty());
        if (messages_.empty())
            return;

        std::scoped_lock locker(lockForViewFetchCounter_);

        const auto minId = messages_.cbegin()->first;
        const auto maxId = messages_.crbegin()->first;
        Data::MessageBuddies inserted;
        inserted.reserve(_buddies.size());
        for (const auto& msg : _buddies)
        {
            const auto currentId = msg->Id_;
            if (currentId >= minId && currentId <= maxId)
            {
                messages_[currentId] = msg;
                if (!viewBounds_.isEmpty())
                {
                    if (currentId >= *viewBounds_.top && currentId <= *viewBounds_.bottom)
                        inserted.push_back(msg);
                }
            }
            else
            {
                qCDebug(history) << aimId_ << __FUNCLINEA__ << "skip message" << currentId;
            }
        }

        if (!inserted.isEmpty())
            emit insertedBuddies(inserted, QPrivateSignal());
    }

    History::FixedIdAndMode History::fixIdAndMode(qint64 _id, hist::scroll_mode_type _mode) const
    {
        if (_id <= 0 || _mode == hist::scroll_mode_type::unread)
            return {};

        std::optional<qint64> fixedId;
        std::optional<hist::scroll_mode_type> fixedMode;

        auto firstVisible = [&fixedId, &fixedMode, &messages_ = messages_](auto it)
        {
            auto pred = [](const auto& x) { return isVisibleMessage(x.second); };
            if (const auto greaterIt = std::find_if(std::next(it), messages_.end(), pred); greaterIt == messages_.end())
            {
                if (const auto lessIt = std::find_if(std::make_reverse_iterator(it), messages_.rend(), pred); lessIt != messages_.rend())
                {
                    fixedId = lessIt->first;
                    fixedMode = hist::scroll_mode_type::to_deleted;
                }
            }
            else
            {
                fixedId = greaterIt->first;
                fixedMode = hist::scroll_mode_type::to_deleted;
            }
        };

        if (const auto it = messages_.find(_id); it != messages_.end())
        {
            if (!isVisibleMessage(it->second)) // needed msgid is deleted
                firstVisible(it);
        }
        else // there is no needed msgid -> go to bound
        {
            if (const auto lowerBound = messages_.lower_bound(_id); lowerBound != messages_.end())
            {
                if (!isVisibleMessage(lowerBound->second))
                {
                    firstVisible(lowerBound);
                }
                else
                {
                    fixedId = lowerBound->first;
                    fixedMode = hist::scroll_mode_type::to_deleted;
                }
            }
            else
            {
                if (!messages_.empty())
                    firstVisible(std::prev(messages_.end()));
            }
        }
        return { fixedId, fixedMode };
    }

    bool History::containsFirstMessage() const
    {
        if (!messages_.empty())
        {
            const auto& firstKey = *messages_.cbegin();
            if (firstKey.second->Prev_ == -1)
                return true;
        }
        return false;
    }

    bool History::needSkipSeq(qint64 seq) const
    {
        return std::none_of(executedRequests.byIds.cbegin(), executedRequests.byIds.cend(), hasSeq(seq)) &&
            std::none_of(executedRequests.fetch.cbegin(), executedRequests.fetch.cend(), hasSeq(seq));
    }

    void History::removeSeq(qint64 seq)
    {
        if (const auto fIt = std::find_if(executedRequests.fetch.cbegin(), executedRequests.fetch.cend(), hasSeq(seq)); fIt != executedRequests.fetch.cend())
            executedRequests.fetch.erase(fIt);
        else if (const auto iIt = std::find_if(executedRequests.byIds.cbegin(), executedRequests.byIds.cend(), hasSeq(seq)); iIt != executedRequests.byIds.cend())
            executedRequests.byIds.erase(iIt);
    }

    History::FirstRequest History::getAndResetFirstInitRequest()
    {
        return std::exchange(firstInitRequest_, FirstRequest::No);
    }

    void History::clearExecutedRequests()
    {
        executedRequests.byIds.clear();
        executedRequests.fetch.clear();
    }

    bool History::needIgnoreMessages(const Data::MessageBuddies& _buddies, Ui::MessagesBuddiesOpt _option, bool _havePending, qint64 _seq, qint64 _lastMsgid) const
    {
        const bool needWaitingForSeq = waitingInitSeq_.has_value();
        const bool isRequiredWaitingSeq = needWaitingForSeq && (_seq == *waitingInitSeq_);
        if (needWaitingForSeq && !isRequiredWaitingSeq)
        {
            qCDebug(history) << aimId_ << "seq" << _seq << "is not equal waiting seq" << *waitingInitSeq_;
            traceMessageBuddies(aimId_,_buddies, _option);
            return true;
        }

        if (_option != Ui::MessagesBuddiesOpt::Pending && _seq > 0 && needSkipSeq(_seq))
        {
            qCDebug(history) << aimId_ << "seq was removed" << _seq;
            traceMessageBuddies(aimId_, _buddies, _option);
            return true;
        }
        return false;
    }

    bool History::containsMsgId(qint64 id) const
    {
        return messages_.find(id) != messages_.end();
    }

    bool History::containsInternalId(const QString& _id) const
    {
        return std::any_of(messages_.cbegin(), messages_.cend(), [&_id](const auto& x) { return x.second->InternalId_ == _id; });
    }

    void History::logCurrentIds(const QString& _hint, LogType _type)
    {
        Ui::gui_coll_helper collection(Ui::GetDispatcher()->create_collection(), true);
        collection.set_value_as_qstring("contact", aimId_);
        if (!_hint.isEmpty())
            collection.set_value_as_qstring("hint", _hint);

        if (_type == LogType::Full)
        {
            core::ifptr<core::iarray> idsArray(collection->create_array());
            idsArray->reserve(int32_t(messages_.size()));
            for (const auto& x : messages_)
            {
                core::ifptr<core::ivalue> val = collection->create_value();
                val->set_as_int64(x.first);
                idsArray->push_back(val.get());
            }
            collection.set_value_as_array("ids", idsArray.get());
        }
        else
        {
            if (!messages_.empty())
            {
                collection.set_value_as_int64("first_id", messages_.cbegin()->first);
                collection.set_value_as_int64("last_id", messages_.crbegin()->first);
            }
        }

        if (viewBounds_.bottom)
            collection.set_value_as_int64("view_bottom", *viewBounds_.bottom);
        if (viewBounds_.top)
            collection.set_value_as_int64("view_top", *viewBounds_.top);
        if (viewBounds_.bottomPending)
            collection.set_value_as_qstring("view_bottom_pending", (*viewBounds_.bottomPending).getInternalId());
        if (viewBounds_.topPending)
            collection.set_value_as_qstring("view_top_pending", (*viewBounds_.topPending).getInternalId());


        const auto seq = Ui::GetDispatcher()->post_message_to_core("archive/log/model", collection.get());
        Q_UNUSED(seq);

        if constexpr (build::is_debug())
            traceMessageBuddies(aimId_, messages_, Ui::MessagesBuddiesOpt::Min);
    }

    void History::dump(const QString& _aimId)
    {
        if (_aimId != aimId_)
            return;

        logCurrentIds(qsl("dump"), LogType::Full);
    }
}
