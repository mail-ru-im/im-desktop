#pragma once

#include <QString>
#include "AbstractSearcher.h"

namespace Logic
{
    class ContactSearcher : public AbstractSearcher
    {
        Q_OBJECT

    private Q_SLOTS:
        void onLocalResults(const Data::SearchResultsV& _localResults, qint64 _reqId);
        void onServerResults(const Data::SearchResultsV& _serverResults, qint64 _reqId);

    public:
        ContactSearcher(QObject* _parent);

        void setExcludeChats(SearchDataSource _exclude);
        SearchDataSource getExcludeChats() const;

        void setHideReadonly(const bool _hide);
        bool getHideReadonly() const;

        const QString& getPhoneNumber() const;
        void setPhoneNumber(const QString& _phoneNumber);

        void setForceAddFavorites(bool _enable);

    private:
        void doLocalSearchRequest() override;
        void doServerSearchRequest() override;

        QString phoneNumber_;

        SearchDataSource excludeChats_;
        bool hideReadonly_;
        bool forceAddFavorites_;
    };
}
