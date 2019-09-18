#include "stdafx.h"

#include "MessageSearcher.h"

#include "core_dispatcher.h"
#include "utils/gui_coll_helper.h"

Q_LOGGING_CATEGORY(messageSearcher, "messageSearcher")

namespace Logic
{
    MessageSearcher::MessageSearcher(QObject* _parent)
        : AbstractSearcher(_parent)
        , totalServerEntries_(0)
    {
        connect(Ui::GetDispatcher(), &Ui::core_dispatcher::searchedMessagesLocal, this, &MessageSearcher::onLocalResults);
        connect(Ui::GetDispatcher(), &Ui::core_dispatcher::searchedMessagesLocalEmptyResult, this, &MessageSearcher::onEmptyLocalResults);
        connect(Ui::GetDispatcher(), &Ui::core_dispatcher::searchedMessagesServer, this, &MessageSearcher::onServerResults);
    }

    void MessageSearcher::searchByCursor(const QString& _cursor, const CursorMode _mode)
    {
        if (source_ & SearchDataSource::server && !_cursor.isEmpty())
        {
            reqReturnedCount_ = 0;
            reqReturnedNeeded_ = 1;

            if (_mode == CursorMode::new_search)
                serverResults_.clear();

            doServerSearchRequestCursor(_cursor);
        }
    }

    void MessageSearcher::requestMoreResults()
    {
        searchByCursor(cursorNext_, CursorMode::continue_search);
    }

    bool MessageSearcher::haveMoreResults() const
    {
        return !cursorNext_.isEmpty();
    }

    void MessageSearcher::setDialogAimId(const QString& _aimId)
    {
        dialogAimid_ = _aimId;
    }

    QString MessageSearcher::getDialogAimId() const
    {
        return dialogAimid_;
    }

    void MessageSearcher::setSearchInAllDialogs()
    {
        setDialogAimId(QString());
    }

    bool MessageSearcher::isSearchInSingleDialog() const
    {
        return !dialogAimid_.isEmpty();
    }

    void MessageSearcher::endLocalSearch()
    {
        Ui::GetDispatcher()->post_message_to_core("dialogs/search/local/end", nullptr);
    }

    int MessageSearcher::getTotalServerEntries() const
    {
        return totalServerEntries_;
    }

    void MessageSearcher::clear()
    {
        AbstractSearcher::clear();

        cursorNext_.clear();
        totalServerEntries_ = 0;
    }

    void MessageSearcher::doLocalSearchRequest()
    {
        Ui::gui_coll_helper collection(Ui::GetDispatcher()->create_collection(), true);
        collection.set_value_as_qstring("keyword", platform::is_apple() ? searchPattern_.toLower() : searchPattern_);
        if (isSearchInSingleDialog())
            collection.set_value_as_qstring("contact", dialogAimid_);

        localReqId_ = Ui::GetDispatcher()->post_message_to_core("dialogs/search/local", collection.get());
    }

    void MessageSearcher::onLocalResults(const Data::SearchResultsV& _localResults, const qint64 _reqId)
    {
        if (_reqId != localReqId_)
            return;

        const auto singleSearchRes = isSearchInSingleDialog();
        for (auto& res : _localResults)
        {
            res->dialogSearchResult_ = singleSearchRes;
            localResults_.append(res);
        }

        qCDebug(messageSearcher) << "got" << _localResults.size() << "items from local";

        emit localResults();
        onRequestReturned();
    }

    void MessageSearcher::doServerSearchRequest()
    {
        Ui::gui_coll_helper collection(Ui::GetDispatcher()->create_collection(), true);
        collection.set_value_as_qstring("keyword", searchPattern_);
        collection.set_value_as_qstring("contact", dialogAimid_);

        serverReqId_ = Ui::GetDispatcher()->post_message_to_core("dialogs/search/server", collection.get());

        qCDebug(messageSearcher) << "requested server with keyword=[" << searchPattern_ << "] contact=[" << dialogAimid_ << "]";
    }

    void MessageSearcher::doServerSearchRequestCursor(const QString& _cursor)
    {
        Ui::gui_coll_helper collection(Ui::GetDispatcher()->create_collection(), true);
        collection.set_value_as_qstring("cursor", _cursor);
        collection.set_value_as_qstring("contact", dialogAimid_);

        serverReqId_ = Ui::GetDispatcher()->post_message_to_core("dialogs/search/server", collection.get());
        serverTimeoutTimer_->start();

        qCDebug(messageSearcher) << "requested server with cursor=[" << _cursor << "] search_all=[" << dialogAimid_.isEmpty() << "]";
    }

    void MessageSearcher::onServerTimedOut()
    {
        qCDebug(messageSearcher) << "!!! server timed out";
    }

    void MessageSearcher::onEmptyLocalResults(const qint64 _reqId)
    {
        if (_reqId != localReqId_)
            return;

        qCDebug(messageSearcher) << "local results empty";

        emit localResults();
        onRequestReturned();
    }

    void MessageSearcher::onServerResults(const Data::SearchResultsV& _serverResults, const QString& _cursorNext, const int _totalEntries, const qint64 _reqId)
    {
        if (_reqId != serverReqId_)
            return;

        [[maybe_unused]] const int elapsedTime = serverTimeoutTimer_->interval() - serverTimeoutTimer_->remainingTime();
        serverTimeoutTimer_->stop();

        const auto singleSearchRes = isSearchInSingleDialog();
        for (auto& res : _serverResults)
        {
            res->dialogSearchResult_ = singleSearchRes;
            serverResults_.append(res);
        }

        cursorNext_ = _cursorNext;

        if (_totalEntries >= 0)
        {
            totalServerEntries_ = _totalEntries;
        }
        else // timeout, error 50000 or other
        {
            qCDebug(messageSearcher) << "!!! server returned error or timed out";
            serverReqId_ = -1;
            isServerTimedOut_ = true;
        }

        qCDebug(messageSearcher) << "got" << _serverResults.size() << "items from server in" << elapsedTime << "ms"
            << "\n results count " << serverResults_.size() - _serverResults.size() << "->" <<serverResults_.size()
            << "\n total ~" << _totalEntries << "cursorNext" << _cursorNext;

        emit serverResults();
        onRequestReturned();
    }
}
