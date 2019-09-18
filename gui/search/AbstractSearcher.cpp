#include "stdafx.h"

#include "AbstractSearcher.h"

namespace
{
    constexpr std::chrono::milliseconds serverResultsTimeoutDef = std::chrono::seconds(4);
}

namespace Logic
{
    AbstractSearcher::AbstractSearcher(QObject* _parent)
        : QObject(_parent)
        , localReqId_(-1)
        , serverReqId_(-1)
        , source_(SearchDataSource::localAndServer)
        , serverTimeoutTimer_(new QTimer(this))
        , isServerTimedOut_(false)
        , reqReturnedCount_(0)
        , reqReturnedNeeded_(0)
    {
        serverTimeoutTimer_->setSingleShot(true);
        serverTimeoutTimer_->setInterval(serverResultsTimeoutDef.count());
        connect(serverTimeoutTimer_, &QTimer::timeout, this, &AbstractSearcher::onServerTimeoutSlot);

        localResults_.reserve(common::get_limit_search_results());
        serverResults_.reserve(common::get_limit_search_results());
    }

    void AbstractSearcher::search(const QString& _pattern)
    {
        searchPattern_ = _pattern;

        clear();

        if (source_ & SearchDataSource::local)
        {
            reqReturnedNeeded_++;
            doLocalSearchRequest();
        }

        if (source_ & SearchDataSource::server)
        {
            reqReturnedNeeded_++;
            doServerSearchRequest();
            serverTimeoutTimer_->start();
        }
    }

    QString AbstractSearcher::getSearchPattern() const
    {
        return searchPattern_;
    }

    void AbstractSearcher::setSearchSource(const SearchDataSource _source)
    {
        source_ = _source;
    }

    SearchDataSource AbstractSearcher::getSearchSource() const
    {
        return source_;
    }

    Data::SearchResultsV AbstractSearcher::getLocalResults() const
    {
        return localResults_;
    }

    Data::SearchResultsV AbstractSearcher::getServerResults() const
    {
        return serverResults_;
    }

    bool AbstractSearcher::isResultsEmpty() const
    {
        return localResults_.isEmpty() && serverResults_.isEmpty();
    }

    void AbstractSearcher::setServerTimeout(const std::chrono::milliseconds _timeoutMs)
    {
        serverTimeoutTimer_->setInterval(_timeoutMs.count());
    }

    std::chrono::milliseconds AbstractSearcher::getServerTimeout() const
    {
        return std::chrono::milliseconds(serverTimeoutTimer_->interval());
    }

    bool AbstractSearcher::isServerTimedOut() const
    {
        return isServerTimedOut_;
    }

    bool AbstractSearcher::isDoingServerRequest() const
    {
        return serverTimeoutTimer_->isActive();
    }

    void AbstractSearcher::clear()
    {
        serverTimeoutTimer_->stop();
        localReqId_ = -1;
        serverReqId_ = -1;
        reqReturnedCount_ = 0;
        reqReturnedNeeded_ = 0;
        localResults_.clear();
        serverResults_.clear();
        isServerTimedOut_ = false;
    }

    void AbstractSearcher::onRequestReturned()
    {
        reqReturnedCount_++;

        if (reqReturnedCount_ >= reqReturnedNeeded_)
            emit allResults();
    }

    void AbstractSearcher::onServerTimeoutSlot()
    {
        serverReqId_ = -1;
        isServerTimedOut_ = true;

        onServerTimedOut();

        emit serverResults();
        onRequestReturned();
    }
}