#pragma once

#include "AbstractSearchModel.h"
#include "../../search/AbstractSearcher.h"
#include "../../types/search_result.h"

Q_DECLARE_LOGGING_CATEGORY(searchModel)

namespace Logic
{
    class ContactSearcher;
    class MessageSearcher;
    class ThreadSearcher;

    using SearchPatterns = std::list<QString>;

    enum class SearchFormat
    {
        ContactsAndMessages,
        Threads
    };

    class SearchModel : public AbstractSearchModel
    {
        Q_OBJECT

    Q_SIGNALS:
        void suggests(QPrivateSignal) const;

    public Q_SLOTS:
        void endLocalSearch();
        void endThreadLocalSearch();
        void requestMore();
        void onDataChanged(const QString& _aimId);

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

        void onThreadsLocal();
        void onThreadsServer();
        void onThreadsAll();

    public:
        SearchModel(QObject* _parent);

        void setSearchPattern(const QString& _p) override;
        int rowCount(const QModelIndex& _parent = QModelIndex()) const override;
        QVariant data(const QModelIndex& _index, int _role) const override;
        void setFocus() override;
        bool isServiceItem(const QModelIndex& _index) const override;
        bool isDropableItem(const QModelIndex& _index) const override;
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

        void setExcludeChats(SearchDataSource _exclude);
        void setHideReadonly(const bool _hide);

        QModelIndex contactIndex(const QString& _aimId) const;
        int getTotalMessagesCount() const;
        bool isAllDataReceived() const;

        static QString getContactsAndGroupsAimId();
        static QString getMessagesAimId();
        static QString getThreadsAimId();
        static QString getSingleThreadAimId();

        static const bool simpleSort(const Data::AbstractSearchResultSptr& _first,
                                     const Data::AbstractSearchResultSptr& _second,
                                     bool _sort_by_time,
                                     const QString& _searchPattern,
                                     bool _favorites_on_top = false);

        void setFavoritesOnTop(bool _enable);
        void setForceAddFavorites(bool _enable);

        void addTemporarySearchData(const QString& _data) override;
        void removeAllTemporarySearchData() override;

        void setSearchFormat(SearchFormat _format);
        bool isSearchInThreads(bool _checkMessages = false) const;
        bool hasOnlyThreadResults() const;

    protected:
        virtual void modifyResultsBeforeEmit(Data::SearchResultsV& _results) {}
        void refreshComposeResults();

    private:
        bool isServerMessagesNeeded() const;
        bool isServerContactsNeeded() const;

        void composeResults();
        void composeResultsChatsCategorized();
        void composeResultsChatsSimple();
        void composeResultsMessages();
        void composeResultsThreads();

        void composeSuggests();

        void setMessagesSearchFormat();
        void setThreadsSearchFormat();

        void initThreadSearcher();

    private:
        ContactSearcher* contactSearcher_;
        MessageSearcher* messageSearcher_;
        ThreadSearcher* threadSearcher_;

        bool isCategoriesEnabled_;
        bool isSortByTime_;
        bool isSearchInDialogsEnabled_;
        bool isSearchInContactsEnabled_;

        bool localMsgSearchNoMore_;
        bool localThreadSearchNoMore_;

        QTimer* timerSearch_;

        bool allContactResultsReceived_;
        bool allMessageResultsReceived_;
        bool messageServerResultsReceived_;
        bool allThreadsResultsReceived_;
        bool threadsServerResultsReceived_;
        bool favoritesOnTop_;
        SearchFormat format_{SearchFormat::ContactsAndMessages};

        Data::SearchResultsV contactLocalRes_;
        Data::SearchResultsV contactServerRes_;
        Data::SearchResultsV msgLocalRes_;
        Data::SearchResultsV msgServerRes_;
        Data::SearchResultsV threadsLocalRes_;
        Data::SearchResultsV threadsServerRes_;
        Data::SearchResultsV results_;
        Data::SearchResultsV messagesResults_;
        Data::SearchResultsV threadsResults_;

        Data::SearchResultsV temporaryLocalContacts_;
    };
}
