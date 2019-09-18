#pragma once

#include "AbstractSearchModel.h"
#include "../../types/search_result.h"

Q_DECLARE_LOGGING_CATEGORY(searchModel)

namespace Logic
{
    class ContactSearcher;
    class MessageSearcher;

    using SearchPatterns = std::list<QString>;

    class SearchModel : public AbstractSearchModel
    {
        Q_OBJECT

    Q_SIGNALS:
        void suggests(QPrivateSignal) const;

    public Q_SLOTS:
        void endLocalSearch();
        void requestMore();

    private Q_SLOTS:
        void avatarLoaded(const QString& _aimId);
        void contactRemoved(const QString& _aimId);
        void doSearch();

        void onContactsLocal();
        void onContactsServer();
        void onContactsAll();

        void onMessagesLocal();
        void onMessagesServer();
        void onMessagesAll();

    public:
        SearchModel(QObject* _parent);

        void setSearchPattern(const QString& _p) override;
        int rowCount(const QModelIndex& _parent = QModelIndex()) const override;
        QVariant data(const QModelIndex& _index, int _role) const override;
        void setFocus() override;
        bool isServiceItem(const QModelIndex& _index) const override;
        bool isClickableItem(const QModelIndex& _index) const override;
        bool isLocalResult(const QModelIndex& _index) const;

        void clear();

        void setSearchInDialogs(const bool _enabled);
        bool isSearchInDialogs() const;

        bool isCategoriesEnabled() const noexcept;
        void setCategoriesEnabled(const bool _enabled);

        bool isSortByTime() const;
        void setSortByTime(bool _enabled);

        void setSearchInSingleDialog(const QString& _contact);
        bool isSearchInSingleDialog() const;
        void disableSearchInSingleDialog();
        QString getSingleDialogAimId() const;

        void setSearchInContacts(const bool _enabled);
        bool isSearchInContacts() const;

        void setExcludeChats(const bool _exclude);
        void setHideReadonly(const bool _hide);

        QModelIndex contactIndex(const QString& _aimId) const;
        int getTotalMessagesCount() const;
        bool isAllDataReceived() const;

        static const QString getContactsAndGroupsAimId();
        static const QString getMessagesAimId();

        static const bool simpleSort(const Data::AbstractSearchResultSptr& _first, const Data::AbstractSearchResultSptr& _second, bool _sort_by_time, const QString& _searchPattern);

    private:
        bool isServerMessagesNeeded() const;
        bool isServerContactsNeeded() const;

        void composeResults();
        void composeResultsChatsCategorized();
        void composeResultsChatsSimple();
        void composeResultsMessages();

        void composeSuggests();

        ContactSearcher* contactSearcher_;
        MessageSearcher* messageSearcher_;

        bool isCategoriesEnabled_;
        bool isSortByTime_;
        bool isSearchInDialogsEnabled_;
        bool isSearchInContactsEnabled_;

        bool localMsgSearchNoMore_;

        QTimer* timerSearch_;

        bool allContactResRcvd_;
        bool allMessageResRcvd_;
        bool messageResServerRcvd_;

        Data::SearchResultsV contactLocalRes_;
        Data::SearchResultsV contactServerRes_;
        Data::SearchResultsV msgLocalRes_;
        Data::SearchResultsV msgServerRes_;
        Data::SearchResultsV results_;
    };
}
