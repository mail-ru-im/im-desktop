#pragma once

#include "types/search_result.h"

namespace Logic
{
    enum SearchDataSource
    {
        none    = 0x0,
        local   = 0x1,
        server  = 0x2,
        localAndServer = local | server
    };

    class AbstractSearcher : public QObject
    {
        Q_OBJECT

    Q_SIGNALS:
        void localResults();
        void serverResults();
        void allResults();

    private Q_SLOTS:
        virtual void onServerTimeoutSlot();

    public:
        AbstractSearcher(QObject* _parent);

        virtual void search(const QString& _pattern);
        QString getSearchPattern() const;

        void setSearchSource(const SearchDataSource _source);
        SearchDataSource getSearchSource() const;

        Data::SearchResultsV getLocalResults() const;
        Data::SearchResultsV getServerResults() const;
        bool isResultsEmpty() const;

        void setServerTimeout(const std::chrono::milliseconds _timeoutMs);
        std::chrono::milliseconds getServerTimeout() const;

        bool isServerTimedOut() const;
        bool isDoingServerRequest() const;

        virtual void clear();

    protected:
        virtual void doLocalSearchRequest() = 0;
        virtual void doServerSearchRequest() = 0;

        virtual void onRequestReturned();
        virtual void onServerTimedOut() {}

        QString searchPattern_;

        int64_t localReqId_;
        int64_t serverReqId_;

        SearchDataSource source_;
        QTimer* serverTimeoutTimer_;
        bool isServerTimedOut_;

        int reqReturnedCount_;
        int reqReturnedNeeded_;

        Data::SearchResultsV localResults_;
        Data::SearchResultsV serverResults_;
    };
}