#pragma once

#include "AbstractSearcher.h"

Q_DECLARE_LOGGING_CATEGORY(messageSearcher)

namespace Logic
{
   class MessageSearcher : public AbstractSearcher
   {
       Q_OBJECT

    private Q_SLOTS:
       void onLocalResults(const Data::SearchResultsV& _localResults, const qint64 _reqId);
       void onEmptyLocalResults(const qint64 _reqId);
       void onServerResults(const Data::SearchResultsV& _serverResults, const QString& _cursorNext, const int _totalEntries, const qint64 _reqId);

   public:
       MessageSearcher(QObject* _parent);

       enum class CursorMode
       {
           new_search,
           continue_search
       };
       void searchByCursor(const QString& _cursor, const CursorMode _mode = CursorMode::new_search);
       void requestMoreResults();
       bool haveMoreResults() const;

       void setDialogAimId(const QString& _aimId);
       QString getDialogAimId() const;
       void setSearchInAllDialogs();

       bool isSearchInSingleDialog() const;

       void endLocalSearch();

       int getTotalServerEntries() const;

       void clear() override;

   private:
       void doLocalSearchRequest() override;
       void doServerSearchRequest() override;
       void doServerSearchRequestCursor(const QString& _cursor);

       void onServerTimedOut() override;

       QString dialogAimid_;
       QString cursorNext_;
       int totalServerEntries_;
   };
}