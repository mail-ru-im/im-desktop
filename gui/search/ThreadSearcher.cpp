#include "stdafx.h"

#include "ThreadSearcher.h"

#include "core_dispatcher.h"
#include "utils/gui_coll_helper.h"

Q_LOGGING_CATEGORY(threadSearcher, "ThreadSearcher")

namespace Logic
{
    ThreadSearcher::ThreadSearcher(QObject* _parent)
        : AbstractSearcher(_parent)
        , totalServerEntries_(0)
    {
        connect(Ui::GetDispatcher(), &Ui::core_dispatcher::searchedThreadsLocal, this, &ThreadSearcher::onLocalResults);
        connect(Ui::GetDispatcher(), &Ui::core_dispatcher::searchedThreadsLocalEmptyResult, this, &ThreadSearcher::onEmptyLocalResults);
        connect(Ui::GetDispatcher(), &Ui::core_dispatcher::searchedThreadsServer, this, &ThreadSearcher::onServerResults);
    }

    void ThreadSearcher::searchByCursor(const QString& _cursor, const CursorMode _mode)
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

    void ThreadSearcher::requestMoreResults()
    {
        searchByCursor(cursorNext_, CursorMode::continue_search);
    }

    bool ThreadSearcher::haveMoreResults() const
    {
        return !cursorNext_.isEmpty();
    }

    void ThreadSearcher::setDialogAimId(const QString& _aimId)
    {
        dialogAimid_ = _aimId;
    }

    QString ThreadSearcher::getDialogAimId() const
    {
        return dialogAimid_;
    }

    void ThreadSearcher::endLocalSearch()
    {
        Ui::GetDispatcher()->post_message_to_core("threads/search/local/end", nullptr);
    }

    int ThreadSearcher::getTotalServerEntries() const
    {
        return totalServerEntries_;
    }

    void ThreadSearcher::clear()
    {
        AbstractSearcher::clear();

        cursorNext_.clear();
        totalServerEntries_ = 0;
    }

    void ThreadSearcher::doLocalSearchRequest()
    {
        Ui::gui_coll_helper collection(Ui::GetDispatcher()->create_collection(), true);
        collection.set_value_as_qstring("keyword", platform::is_apple() ? searchPattern_.toLower() : searchPattern_);
        collection.set_value_as_qstring("contact", dialogAimid_);

        localReqId_ = Ui::GetDispatcher()->post_message_to_core("threads/search/local", collection.get());
    }

    void ThreadSearcher::onLocalResults(const Data::SearchResultsV& _localResults, const qint64 _reqId)
    {
        if (_reqId != localReqId_)
            return;

        for (auto& res : _localResults)
        {
            res->dialogSearchResult_ = true;
            localResults_.append(res);
        }

        qCDebug(threadSearcher) << "got" << _localResults.size() << "items from local";

        Q_EMIT localResults();
        onRequestReturned();
    }

    void ThreadSearcher::doServerSearchRequest()
    {
        Ui::gui_coll_helper collection(Ui::GetDispatcher()->create_collection(), true);
        collection.set_value_as_qstring("keyword", searchPattern_);
        collection.set_value_as_qstring("contact", dialogAimid_);

        serverReqId_ = Ui::GetDispatcher()->post_message_to_core("threads/search/server", collection.get());

        qCDebug(threadSearcher) << "requested server with keyword=[" << searchPattern_ << "] contact=[" << dialogAimid_ << "]";
    }

    void ThreadSearcher::doServerSearchRequestCursor(const QString& _cursor)
    {
        Ui::gui_coll_helper collection(Ui::GetDispatcher()->create_collection(), true);
        collection.set_value_as_qstring("cursor", _cursor);
        collection.set_value_as_qstring("contact", dialogAimid_);

        serverReqId_ = Ui::GetDispatcher()->post_message_to_core("threads/search/server", collection.get());
        serverTimeoutTimer_->start();

        qCDebug(threadSearcher) << "requested server with cursor=[" << _cursor << "] search_all=[" << dialogAimid_.isEmpty() << "]";
    }

    void ThreadSearcher::onServerTimedOut()
    {
        qCDebug(threadSearcher) << "!!! server timed out";
    }

    void ThreadSearcher::onEmptyLocalResults(const qint64 _reqId)
    {
        if (_reqId != localReqId_)
            return;

        qCDebug(threadSearcher) << "local results empty";

        Q_EMIT localResults();
        onRequestReturned();
    }

    void ThreadSearcher::onServerResults(const Data::SearchResultsV& _serverResults, const QString& _cursorNext, const int _totalEntries, const qint64 _reqId)
    {
        if (_reqId != serverReqId_)
            return;

        [[maybe_unused]] const int elapsedTime = serverTimeoutTimer_->interval() - serverTimeoutTimer_->remainingTime();
        serverTimeoutTimer_->stop();

        for (auto& res : _serverResults)
        {
            res->dialogSearchResult_ = true;
            serverResults_.append(res);
        }

        cursorNext_ = _cursorNext;

        if (_totalEntries >= 0)
        {
            totalServerEntries_ = _totalEntries;
        }
        else // timeout, error 50000 or other
        {
            qCDebug(threadSearcher) << "!!! server returned error or timed out";
            serverReqId_ = -1;
            isServerTimedOut_ = true;
        }

        qCDebug(threadSearcher) << "got" << _serverResults.size() << "items from server in" << elapsedTime << "ms"
            << "\n results count " << serverResults_.size() - _serverResults.size() << "->" <<serverResults_.size()
            << "\n total ~" << _totalEntries << "cursorNext" << _cursorNext;

        Q_EMIT serverResults();
        onRequestReturned();
    }
}
