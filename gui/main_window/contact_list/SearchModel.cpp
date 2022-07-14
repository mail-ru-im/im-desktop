#include "stdafx.h"

#include "SearchModel.h"
#include "ContactListModel.h"
#include "RecentsModel.h"
#include "FavoritesUtils.h"

#include "../../search/ContactSearcher.h"
#include "../../search/MessageSearcher.h"
#include "../../search/ThreadSearcher.h"

#include "../../utils/InterConnector.h"
#include "../../utils/features.h"
#include "../../core_dispatcher.h"
#include "../../utils/SearchPatternHistory.h"
#include "../../cache/avatars/AvatarStorage.h"
#include "../../gui_settings.h"
#include "../containers/StatusContainer.h"

#include "../../../common.shared/config/config.h"
#include "ServiceContacts.h"

Q_LOGGING_CATEGORY(searchModel, "searchModel")

namespace
{
    constexpr std::chrono::milliseconds typingTimeout = std::chrono::milliseconds(300);

    Data::ServiceSearchResultSptr makeServiceResult(const QString& _aimid, const QString& _text, const QString& _accessName)
    {
        auto sr = std::make_shared<Data::SearchResultService>();
        sr->serviceAimId_ = _aimid;
        sr->serviceText_ = _text;
        sr->accessibleName_ = _accessName;
        return sr;
    }
}

namespace Logic
{
    SearchModel::SearchModel(QObject* _parent)
        : AbstractSearchModel(_parent)
        , contactSearcher_(new ContactSearcher(this))
        , messageSearcher_(new MessageSearcher(this))
        , threadSearcher_(nullptr)
        , isCategoriesEnabled_(true)
        , isSortByTime_(false)
        , isSearchInDialogsEnabled_(true)
        , isSearchInContactsEnabled_(true)
        , localMsgSearchNoMore_(false)
        , timerSearch_(new QTimer(this))
        , allContactResultsReceived_(false)
        , allMessageResultsReceived_(false)
        , favoritesOnTop_(false)
    {
        timerSearch_->setSingleShot(true);
        timerSearch_->setInterval(typingTimeout);
        connect(timerSearch_, &QTimer::timeout, this, &SearchModel::doSearch);

        connect(contactSearcher_, &ContactSearcher::localResults, this, &SearchModel::onContactsLocal);
        connect(contactSearcher_, &ContactSearcher::serverResults, this, &SearchModel::onContactsServer);
        connect(contactSearcher_, &ContactSearcher::allResults, this, &SearchModel::onContactsAll);

        connect(messageSearcher_, &MessageSearcher::localResults, this, &SearchModel::onMessagesLocal);
        connect(messageSearcher_, &MessageSearcher::serverResults, this, &SearchModel::onMessagesServer);
        connect(messageSearcher_, &MessageSearcher::allResults, this, &SearchModel::onMessagesAll);

        allThreadsResultsReceived_ = !Features::isThreadsEnabled();
        initThreadSearcher();

        connect(&Utils::InterConnector::instance(), &Utils::InterConnector::repeatSearch, this, &SearchModel::repeatSearch);
        connect(&Utils::InterConnector::instance(), &Utils::InterConnector::disableSearchInDialog, this, [this]()
            {
                if (!Logic::getContactListModel()->isThread(messageSearcher_->getDialogAimId()))
                    setSearchInSingleDialog(QString());
            });

        connect(getContactListModel(), &ContactListModel::contact_removed, this, &SearchModel::contactRemoved);

        connect(GetAvatarStorage(), &Logic::AvatarStorage::avatarChanged, this, &SearchModel::avatarLoaded);
        connect(Logic::GetStatusContainer(), &Logic::StatusContainer::statusChanged, this, [this](const QString& _aimid)
        {
            avatarLoaded(_aimid);
        });

        connect(Logic::getContactListModel(), &Logic::ContactListModel::contactChanged, this, &SearchModel::onDataChanged);

        for (auto v : { &results_, &messagesResults_, &threadsResults_, &contactLocalRes_, &contactServerRes_, &msgLocalRes_, &msgServerRes_, &threadsLocalRes_, &threadsServerRes_})
            v->reserve(common::get_limit_search_results());

        Logic::getLastSearchPatterns(); // init
    }

    void SearchModel::endLocalSearch()
    {
        messageSearcher_->endLocalSearch();
    }

    void SearchModel::avatarLoaded(const QString& _aimId)
    {
        qCDebug(searchModel) << this << "avatarLoaded" << _aimId;
        if (results_.isEmpty() || _aimId.isEmpty())
            return;

        int i = 0;
        for (const auto& item : std::as_const(results_))
        {
            if (!item->isService() && !item->isSuggest() && (item->getAimId() == _aimId || item->getSenderAimId() == _aimId))
            {
                qCDebug(searchModel) << this << "updated avatar for" << _aimId << "at index" << i;

                const auto idx = index(i);
                Q_EMIT dataChanged(idx, idx);
            }
            ++i;
        }
    }

    void SearchModel::endThreadLocalSearch()
    {
        if (Features::isThreadsEnabled() && threadSearcher_)
            threadSearcher_->endLocalSearch();
    }

    void SearchModel::requestMore()
    {
        if (format_ == SearchFormat::ContactsAndMessages)
        {
            if (messageSearcher_->haveMoreResults() && !messageSearcher_->isDoingServerRequest())
            {
                qCDebug(searchModel) << this << "requested more results from server, currently have" << msgServerRes_.size() << "items";
                messageSearcher_->requestMoreResults();
            }
        }
        else if (Features::isThreadsEnabled() && threadSearcher_ && threadSearcher_->haveMoreResults() && !threadSearcher_->isDoingServerRequest())
        {
            qCDebug(searchModel) << this << "threads requested more results from server, currently have" << threadsServerRes_.size() << "items";
            threadSearcher_->requestMoreResults();
        }
    }

    void SearchModel::contactRemoved(const QString& _aimId)
    {
        if (rowCount() == 0)
            return;

        auto findItem = [&_aimId](auto& _v)
        {
            return std::find_if(_v.begin(), _v.end(), [&_aimId](const auto& item)
            {
                return item->isLocalResult_ && item->isContact() && item->getAimId() == _aimId;
            });
        };

        const auto it = findItem(results_);
        if (it != results_.end())
        {
            results_.erase(it);

            const auto localIt = findItem(contactLocalRes_);
            if (localIt != contactLocalRes_.end())
                contactLocalRes_.erase(localIt);

            const auto newCount = rowCount();
            Q_EMIT dataChanged(index(0), index(newCount));
            if (newCount == 0)
                Q_EMIT showNoSearchResults();
        }
    }

    void SearchModel::doSearch()
    {
        qCDebug(searchModel) << this << "searching for" << searchPattern_;
        if (isSearchInContacts())
        {
            if (isServerContactsNeeded())
                contactSearcher_->setSearchSource(SearchDataSource::localAndServer);
            else
                contactSearcher_->setSearchSource(SearchDataSource::local);

            contactSearcher_->search(searchPattern_);
        }

        if (isSearchInDialogs())
        {
            im_assert(!searchPattern_.isEmpty());
            uint32_t src = SearchDataSource::none;

            if (isServerMessagesNeeded() || ServiceContacts::isServiceContact(messageSearcher_->getDialogAimId()))
                src |= SearchDataSource::server;

            if (!localMsgSearchNoMore_)
                src |= SearchDataSource::local;

            messageSearcher_->setSearchSource(static_cast<SearchDataSource>(src));
            messageSearcher_->search(searchPattern_);

            if (Features::isThreadsEnabled() && !messageSearcher_->getDialogAimId().isEmpty())
            {
                initThreadSearcher();
                if(!Logic::getContactListModel()->isThread(threadSearcher_->getDialogAimId())
                && messageSearcher_->getDialogAimId() != ServiceContacts::getThreadsName())
                {
                    threadSearcher_->setSearchSource(SearchDataSource::localAndServer);
                    threadSearcher_->search(searchPattern_);
                }
            }

            if (src & SearchDataSource::server)
                Ui::GetDispatcher()->post_stats_to_core(core::stats::stats_event_names::chat_search_dialog_server_first_req);
        }

        Q_EMIT hideNoSearchResults();
        Q_EMIT showSearchSpinner();
    }

    void SearchModel::onContactsLocal()
    {
        if (contactSearcher_->getSearchPattern() != searchPattern_)
            return;

        contactLocalRes_ = contactSearcher_->getLocalResults();

        if (searchPattern_.isEmpty())
            for (const auto& temp : std::as_const(temporaryLocalContacts_))
                contactLocalRes_.push_back(temp);

        const auto sortByTime = isSortByTime();
        const auto searchPattern = searchPattern_;
        std::sort(contactLocalRes_.begin(), contactLocalRes_.end(),
                  [sortByTime, searchPattern, favoritesOnTop = favoritesOnTop_](const auto& first, const auto& second)
        {
            return simpleSort(first, second, sortByTime, searchPattern, favoritesOnTop);
        });

        setMessagesSearchFormat();
        composeResults();
    }

    void SearchModel::onContactsServer()
    {
        if (contactSearcher_->getSearchPattern() != searchPattern_)
            return;

        contactServerRes_ = contactSearcher_->getServerResults();

        if (!searchPattern_.isEmpty())
        {
            for (const auto& c : std::as_const(contactServerRes_))
            {
                if (c->highlights_.empty())
                    c->highlights_ = { searchPattern_ };
            }
        }

        setMessagesSearchFormat();
        composeResults();
    }

    void SearchModel::onContactsAll()
    {
        allContactResultsReceived_ = true;
        setMessagesSearchFormat();
        composeResults();
    }

    void SearchModel::onMessagesLocal()
    {
        if (messageSearcher_->getSearchPattern() != searchPattern_)
            return;

        if (messageServerResultsReceived_) // if we already have server results - do nothing
            return;

        msgLocalRes_ = messageSearcher_->getLocalResults();
        localMsgSearchNoMore_ = msgLocalRes_.isEmpty();

        std::sort(msgLocalRes_.begin(), msgLocalRes_.end(), [](const Data::AbstractSearchResultSptr& _first, const Data::AbstractSearchResultSptr& _second)
        {
            if (!_first->isMessage())
                return false;

            if (!_second->isMessage())
                return false;

            const auto msgFirst = std::static_pointer_cast<Data::SearchResultMessage>(_first);
            const auto msgSecond = std::static_pointer_cast<Data::SearchResultMessage>(_second);

            return msgFirst->message_->GetTime() > msgSecond->message_->GetTime();
        });

        if (msgLocalRes_.size() > (int)common::get_limit_search_results())
            msgLocalRes_.resize(common::get_limit_search_results());

        for (const auto& res : msgLocalRes_)
        {
            if (const auto msg = std::static_pointer_cast<Data::SearchResultMessage>(res); msg && msg->parentTopic_)
                Logic::getContactListModel()->markAsThread(msg->message_->AimId_, msg->parentTopic_.get());
        }
        setMessagesSearchFormat();
        composeResults();
    }

    void SearchModel::onMessagesServer()
    {
        if (messageSearcher_->getSearchPattern() != searchPattern_)
            return;

        if (messageSearcher_->isServerTimedOut())
            return;

        endLocalSearch();

        messageServerResultsReceived_ = true;
        allMessageResultsReceived_ = true;

        if (msgServerRes_.isEmpty())
            Ui::GetDispatcher()->post_stats_to_core(core::stats::stats_event_names::chat_search_dialog_server_first_ans);

        msgServerRes_ = messageSearcher_->getServerResults();
        qCDebug(searchModel) << this << "got more results from server, currently have" << msgServerRes_.size() << "items";

        for (const auto& res : msgServerRes_)
        {
            if (const auto msg = std::static_pointer_cast<Data::SearchResultMessage>(res); msg && msg->parentTopic_)
                Logic::getContactListModel()->markAsThread(msg->message_->AimId_, msg->parentTopic_.get());
        }

        setMessagesSearchFormat();
        composeResults();
    }

    void SearchModel::onMessagesAll()
    {
        for (const auto& resV : { msgLocalRes_, msgServerRes_ })
        {
            for (const auto& res : resV)
            {
                if (const auto msg = std::static_pointer_cast<Data::SearchResultMessage>(res); msg && msg->parentTopic_)
                    Logic::getContactListModel()->markAsThread(msg->message_->AimId_, msg->parentTopic_.get());
            }
        }

        allMessageResultsReceived_ = true;
        setMessagesSearchFormat();
        composeResults();
    }

    void SearchModel::onThreadsLocal()
    {
        if (threadSearcher_->getSearchPattern() != searchPattern_)
            return;

        if (threadsServerResultsReceived_) // if we already have server results - do nothing
            return;

        threadsLocalRes_ = threadSearcher_->getLocalResults();
        localThreadSearchNoMore_ = threadsLocalRes_.isEmpty();

        std::sort(threadsLocalRes_.begin(), threadsLocalRes_.end(),
            [](const Data::AbstractSearchResultSptr& _first, const Data::AbstractSearchResultSptr& _second)
            {
                if (!_first->isMessage() || !_second->isMessage())
                    return false;

                const auto msgFirst  = std::static_pointer_cast<Data::SearchResultMessage>(_first);
                const auto msgSecond = std::static_pointer_cast<Data::SearchResultMessage>(_second);

                return msgFirst->message_->GetTime() > msgSecond->message_->GetTime();
            });

        if (threadsLocalRes_.size() > (int)common::get_limit_search_results())
            threadsLocalRes_.resize(common::get_limit_search_results());

        for (const auto& res : threadsLocalRes_)
        {
            if (const auto msg = std::static_pointer_cast<Data::SearchResultMessage>(res))
                Logic::getContactListModel()->markAsThread(msg->message_->AimId_, msg->parentTopic_.get());
        }
        setThreadsSearchFormat();
        composeResults();
    }

    void SearchModel::onThreadsServer()
    {
        if (threadSearcher_->getSearchPattern() != searchPattern_)
            return;

        if (threadSearcher_->isServerTimedOut())
            return;

        endThreadLocalSearch();

        threadsServerResultsReceived_ = true;
        allThreadsResultsReceived_ = true;

        threadsServerRes_ = threadSearcher_->getServerResults();
        qCDebug(searchModel) << this << "got more results from server, currently have" << threadsServerRes_.size() << "items";

        for (const auto& res : threadsServerRes_)
        {
            if (const auto msg = std::static_pointer_cast<Data::SearchResultMessage>(res))
                Logic::getContactListModel()->markAsThread(msg->message_->AimId_, msg->parentTopic_.get());
        }

        setThreadsSearchFormat();
        composeResults();
    }

    void SearchModel::onThreadsAll()
    {
        for (const auto& resV : { threadsLocalRes_, threadsServerRes_ })
        {
            for (const auto& res : resV)
            {
                if (const auto msg = std::static_pointer_cast<Data::SearchResultMessage>(res))
                    Logic::getContactListModel()->markAsThread(msg->message_->AimId_, msg->parentTopic_.get());
            }
        }

        allThreadsResultsReceived_ = true;
        setThreadsSearchFormat();
        composeResults();
    }

    void SearchModel::setSearchPattern(const QString& _newPattern)
    {
        clear();

        const auto prevPattern = std::move(searchPattern_);
        searchPattern_ = _newPattern.trimmed();

        if (isSearchInDialogs() && searchPattern_.isEmpty())
        {
            Q_EMIT hideSearchSpinner();
            Q_EMIT hideNoSearchResults();

            localMsgSearchNoMore_ = false;

            if (isSearchInSingleDialog())
                composeSuggests();
        }
        else
        {
            bool currentPatternContainsPrevious =
                !searchPattern_.isEmpty() &&
                !prevPattern.isEmpty() &&
                searchPattern_.startsWith(prevPattern, Qt::CaseInsensitive) &&
                searchPattern_.length() >= prevPattern.length();

            localMsgSearchNoMore_ = localMsgSearchNoMore_ && currentPatternContainsPrevious;

            if (!searchPattern_.isEmpty())
                timerSearch_->start();
            else
                doSearch();
        }
    }

    int SearchModel::rowCount(const QModelIndex&) const
    {
        return results_.size();
    }

    QVariant SearchModel::data(const QModelIndex& _index, int _role) const
    {
        if (!_index.isValid() || (_role != Qt::DisplayRole && !Testing::isAccessibleRole(_role)))
            return QVariant();

        int cur = _index.row();
        if (cur >= rowCount(_index))
            return QVariant();

        if (Testing::isAccessibleRole(_role))
            return QString(u"AS Search " % results_[cur]->getAccessibleName());

        return QVariant::fromValue(results_[cur]);
    }

    void SearchModel::setFocus()
    {
        clear();
        searchPattern_.clear();
    }

    bool SearchModel::isServiceItem(const QModelIndex& _index) const
    {
        if (_index.isValid())
        {
            const auto i = _index.row();
            if (i < results_.size())
                return results_[i]->isService();
        }

        return false;
    }

    bool SearchModel::isDropableItem(const QModelIndex& _index) const
    {
        const auto item = results_[_index.row()];
        return item->isChat() || item->isContact();
    }

    bool SearchModel::isClickableItem(const QModelIndex& _index) const
    {
        if (_index.isValid())
        {
            const auto i = _index.row();
            if (i < results_.size())
            {
                const auto item = results_[i];
                return !item->isService();
            }
        }

        return false;
    }

    bool SearchModel::isLocalResult(const QModelIndex& _index) const
    {
        if (_index.isValid())
        {
            const auto i = _index.row();
            if (i < results_.size())
                return results_[i]->isLocalResult_;
        }

        return false;
    }

    void SearchModel::clear()
    {
        timerSearch_->stop();
        setMessagesSearchFormat();

        contactSearcher_->clear();
        messageSearcher_->clear();
        if (Features::isThreadsEnabled() && threadSearcher_)
            threadSearcher_->clear();
        allContactResultsReceived_ = !isSearchInContacts();
        allMessageResultsReceived_ = !isSearchInDialogs();
        messageServerResultsReceived_ = false;
        if (Features::isThreadsEnabled())
        {
            allThreadsResultsReceived_ = !isSearchInDialogs();
            threadsServerResultsReceived_ = false;
        }

        for (auto v : { &results_, &messagesResults_, &threadsResults_, &contactLocalRes_, &contactServerRes_, &msgLocalRes_, &msgServerRes_, &threadsLocalRes_, &threadsServerRes_ })
            v->clear();

        Q_EMIT dataChanged(index(0), index(rowCount()));

        qCDebug(searchModel) << this << "cleared";
    }

    void SearchModel::setSearchInDialogs(const bool _enabled)
    {
        isSearchInDialogsEnabled_ = _enabled;
    }

    bool SearchModel::isSearchInDialogs() const
    {
        return isSearchInDialogsEnabled_;
    }

    bool SearchModel::isCategoriesEnabled() const noexcept
    {
        return isCategoriesEnabled_;
    }

    void SearchModel::setCategoriesEnabled(const bool _enabled)
    {
        isCategoriesEnabled_ = _enabled;
    }

    bool SearchModel::isSortByTime() const
    {
        return isSortByTime_ || !searchPattern_.isEmpty();
    }

    void SearchModel::setSortByTime(bool _enabled)
    {
        isSortByTime_ = _enabled;
    }

    void SearchModel::setSearchInSingleDialog(const QString& _contact)
    {
        if (messageSearcher_->getDialogAimId() != _contact
            || (Features::isThreadsEnabled() && (!threadSearcher_ || (threadSearcher_ && threadSearcher_->getDialogAimId() != _contact))))
        {
            messageSearcher_->setDialogAimId(_contact);
            if (Features::isThreadsEnabled())
            {
                initThreadSearcher();
                threadSearcher_->setDialogAimId(_contact);
            }
            setSearchInContacts(!isSearchInSingleDialog());

            clear();
            endLocalSearch();
            endThreadLocalSearch();

            if (!_contact.isEmpty())
            {
                composeSuggests();
                Ui::GetDispatcher()->post_stats_to_core(core::stats::stats_event_names::chat_search_dialog_opened, { { "is_empty", results_.empty() ? "true" : "false" } });
            }
        }
    }

    bool SearchModel::isSearchInSingleDialog() const
    {
        return messageSearcher_->isSearchInSingleDialog();
    }

    void SearchModel::disableSearchInSingleDialog()
    {
        setSearchInSingleDialog(QString());
    }

    QString SearchModel::getSingleDialogAimId() const
    {
        return messageSearcher_->getDialogAimId();
    }

    void SearchModel::setSearchInContacts(const bool _enabled)
    {
        isSearchInContactsEnabled_ = _enabled;
    }

    bool SearchModel::isSearchInContacts() const
    {
        return isSearchInContactsEnabled_;
    }

    void SearchModel::setExcludeChats(SearchDataSource _exclude)
    {
        contactSearcher_->setExcludeChats(_exclude);
    }

    void SearchModel::setHideReadonly(const bool _hide)
    {
        contactSearcher_->setHideReadonly(_hide);
    }

    QModelIndex SearchModel::contactIndex(const QString& _aimId) const
    {
        auto total = messagesResults_ + threadsResults_;
        const auto it = std::find_if(total.begin(), total.end(), [&_aimId](const auto& _res) { return _res->getAimId() == _aimId; });
        if (it != total.end())
        {
            if (_aimId == getThreadsAimId())
                return index(hasOnlyThreadResults() ? 0 : messagesResults_.size() - 1);
            return index(std::distance(total.begin(), it));
        }

        return QModelIndex();
    }

    int SearchModel::getTotalMessagesCount() const
    {
        return !msgServerRes_.isEmpty() ? messageSearcher_->getTotalServerEntries() : msgLocalRes_.size();
    }

    bool Logic::SearchModel::isAllDataReceived() const
    {
        const auto aimId = messageSearcher_->getDialogAimId();
        const auto inThreadOrService = Logic::getContactListModel()->isThread(aimId) || ServiceContacts::isServiceContact(aimId);
        return allContactResultsReceived_ && allMessageResultsReceived_ && (!isSearchInSingleDialog() || inThreadOrService || allThreadsResultsReceived_);
    }

    QString SearchModel::getContactsAndGroupsAimId()
    {
        return qsl("~contgroups~");
    }

    QString SearchModel::getMessagesAimId()
    {
        return qsl("~messages~");
    }

    QString SearchModel::getThreadsAimId()
    {
        return qsl("~threads~");
    }

    QString SearchModel::getSingleThreadAimId()
    {
        return qsl("~thread~");
    }

    const bool SearchModel::simpleSort(const Data::AbstractSearchResultSptr& _first,
                                       const Data::AbstractSearchResultSptr& _second,
                                       bool _sort_by_time,
                                       const QString& _searchPattern,
                                       bool _favorites_on_top)
    {
        const auto fAimId = _first->getAimId();
        const auto sAimId = _second->getAimId();

        if (_favorites_on_top)
        {
            if (Favorites::isFavorites(fAimId))
                return true;

            if (Favorites::isFavorites(sAimId))
                return false;
        }

        if (_sort_by_time)
        {
            if (!Logic::getContactListModel()->contains(fAimId))
                return false;

            if (!Logic::getContactListModel()->contains(sAimId))
                return true;

            const auto fTime = Logic::getRecentsModel()->getTime(fAimId);
            if (fTime == -1)
                return false;

            const auto sTime = Logic::getRecentsModel()->getTime(sAimId);
            if (sTime == -1)
                return true;

            return fTime > sTime;
        }

        const auto firstFriendly = _first->getFriendlyName();
        const auto secondFriendly = _second->getFriendlyName();

        const auto force_cyrillic = _searchPattern.isEmpty() ? Ui::get_gui_settings()->get_value<QString>(settings_language, QString()) == u"ru" : Utils::startsCyrillic(_searchPattern);

        if (!_searchPattern.isEmpty())
        {
            auto fStarts = false;
            for (const auto& h : _first->highlights_)
                fStarts |= firstFriendly.startsWith(h, Qt::CaseInsensitive);

            auto sStarts = false;
            for (const auto& h : _second->highlights_)
                sStarts |= secondFriendly.startsWith(h, Qt::CaseInsensitive);

            if (fStarts != sStarts)
                return fStarts;
        }

        const auto firstNotLetter = Utils::startsNotLetter(firstFriendly);
        const auto secondNotLetter = Utils::startsNotLetter(secondFriendly);
        if (firstNotLetter != secondNotLetter)
            return secondNotLetter;

        if (force_cyrillic)
        {
            const auto firstCyrillic = Utils::startsCyrillic(firstFriendly);
            const auto secondCyrillic = Utils::startsCyrillic(secondFriendly);

            if (firstCyrillic != secondCyrillic)
                return firstCyrillic;
        }

        return firstFriendly.compare(secondFriendly, Qt::CaseInsensitive) < 0;
    }

    void SearchModel::setFavoritesOnTop(bool _enable)
    {
        favoritesOnTop_ = _enable;
    }

    void SearchModel::setForceAddFavorites(bool _enable)
    {
        contactSearcher_->setForceAddFavorites(_enable);
    }

    void SearchModel::addTemporarySearchData(const QString& _data)
    {
        auto temp = std::make_shared<Data::SearchResultContactChatLocal>();
        temp->aimId_ = _data;

        temporaryLocalContacts_.push_back(std::move(temp));
    }

    void SearchModel::removeAllTemporarySearchData()
    {
        temporaryLocalContacts_.clear();
    }

    void SearchModel::setSearchFormat(SearchFormat _format)
    {
        if (format_ == _format)
            return;

        beginResetModel();
        format_ = _format;
        results_.clear();
        results_ = format_ == SearchFormat::ContactsAndMessages ? messagesResults_ : threadsResults_;
        Q_EMIT dataChanged(QModelIndex(), QModelIndex());
        endResetModel();
    }

    bool SearchModel::isSearchInThreads(bool _checkMessages) const
    {
        return (format_ == SearchFormat::Threads) && (!_checkMessages || !hasOnlyThreadResults());
    }

    bool SearchModel::hasOnlyThreadResults() const
    {
        return messagesResults_.empty();
    }

    void SearchModel::refreshComposeResults()
    {
        composeResults();
    }

    bool SearchModel::isServerMessagesNeeded() const
    {
        return isServerSearchEnabled() && searchPattern_.size() >= config::get().number<int64_t>(config::values::server_search_messages_min_symbols);
    }

    bool SearchModel::isServerContactsNeeded() const
    {
        return isServerSearchEnabled() && searchPattern_.size() >= config::get().number<int64_t>(config::values::server_search_contacts_min_symbols);
    }

    void SearchModel::composeResults()
    {
        results_.clear();
        messagesResults_.clear();
        threadsResults_.clear();
        results_.reserve(contactLocalRes_.size() + contactServerRes_.size() + std::max(msgLocalRes_.size(), msgServerRes_.size()) + std::max(threadsLocalRes_.size(), threadsServerRes_.size()));
        messagesResults_.reserve(contactLocalRes_.size() + contactServerRes_.size() + std::max(msgLocalRes_.size(), msgServerRes_.size()));
        threadsResults_.reserve(std::max(threadsLocalRes_.size(), threadsServerRes_.size()));

        if (isCategoriesEnabled())
            composeResultsChatsCategorized();
        else
            composeResultsChatsSimple();

        if (allMessageResultsReceived_)
            composeResultsMessages();

        if (allThreadsResultsReceived_ && Features::isThreadsEnabled())
            composeResultsThreads();

        setThreadsSearchFormat();
        results_ = isSearchInThreads() ? threadsResults_ : messagesResults_;

        modifyResultsBeforeEmit(results_);

        qCDebug(searchModel) << this << "composed" << results_.size() << "items:\n"
            << msgLocalRes_.size() << "local messages" << (!msgServerRes_.isEmpty() ? "(unused)" : "")
            << msgServerRes_.size() << "server messages";

        Q_EMIT dataChanged(index(0), index(rowCount()));

        if (rowCount() > 0)
        {
            Q_EMIT hideSearchSpinner();
            Q_EMIT hideNoSearchResults();

            Q_EMIT results();
            qCDebug(searchModel) << this << "composed Results";
        }
        else if (isAllDataReceived())
        {
            Q_EMIT hideSearchSpinner();
            Q_EMIT showNoSearchResults();

            qCDebug(searchModel) << this << "composed NoSearchResults";
        }
    }

    void SearchModel::composeResultsChatsCategorized()
    {
        if (!contactLocalRes_.isEmpty() || !contactServerRes_.isEmpty())
        {
            const Data::AbstractSearchResultSptr hdr = makeServiceResult(
                getContactsAndGroupsAimId(),
                QT_TRANSLATE_NOOP("search", "Contacts and groups") % u": " % QString::number(contactLocalRes_.size() + contactServerRes_.size()),
                qsl("search_results contactsAndGroups"));

            messagesResults_.append(hdr);

            composeResultsChatsSimple();
        }
    }

    void SearchModel::composeResultsChatsSimple()
    {
        messagesResults_.append(contactLocalRes_);

        for (const auto& res : std::as_const(contactServerRes_))
            if (std::none_of(contactLocalRes_.cbegin(), contactLocalRes_.cend(), [&res](const auto& _localRes) { return _localRes->getAimId() == res->getAimId(); }))
                messagesResults_.append(res);
    }

    void SearchModel::composeResultsMessages()
    {
        const bool haveMessages = !msgServerRes_.isEmpty() || (!msgLocalRes_.isEmpty() && !messageServerResultsReceived_);
        if (haveMessages)
        {
            if (isCategoriesEnabled())
            {
                const auto isInThread = Logic::getContactListModel()->isThread(messageSearcher_->getDialogAimId());
                const Data::AbstractSearchResultSptr msgsHdr = makeServiceResult(
                    isInThread ? getSingleThreadAimId() : getMessagesAimId(),
                    QT_TRANSLATE_NOOP("search", "Messages") % u": " % QString::number(getTotalMessagesCount()),
                    qsl("search_results messages"));

                messagesResults_.append(msgsHdr);
            }

            if (!messageServerResultsReceived_)
                messagesResults_.append(msgLocalRes_);
            else
                messagesResults_.append(msgServerRes_);
        }
    }

    void SearchModel::composeResultsThreads()
    {
        const bool haveMessages = !threadsServerRes_.isEmpty() || (!threadsLocalRes_.isEmpty() && !threadsServerResultsReceived_);
        if (haveMessages)
        {
            Data::SearchResultsV tmpResults;
            if (!threadsServerResultsReceived_)
                tmpResults.append(threadsLocalRes_);
            else
                tmpResults.append(threadsServerRes_);

            Data::SearchResultsV tmpMsgResults;
            if (!messageServerResultsReceived_)
                tmpMsgResults.append(msgLocalRes_);
            else
                tmpMsgResults.append(msgServerRes_);

            for (const auto& msg : tmpMsgResults)
            {
                auto iter = std::remove_if(tmpResults.begin(), tmpResults.end(), [id = msg->getMessageId()](const auto& _res) { return _res->getMessageId() == id; });
                if (iter != tmpResults.end())
                    tmpResults.erase(iter);
            }

            if (!tmpResults.isEmpty())
            {
                if (isCategoriesEnabled())
                {
                    const auto replaceHeaderWithMessages = !threadsServerResultsReceived_ && ServiceContacts::isServiceContact(threadSearcher_->getDialogAimId());
                    const auto searchHeader = replaceHeaderWithMessages ? QT_TRANSLATE_NOOP("search", "Messages") : QT_TRANSLATE_NOOP("search", "Threads");
                    const auto aimId = replaceHeaderWithMessages ? getMessagesAimId() : getThreadsAimId();

                    const Data::AbstractSearchResultSptr msgsHdr = makeServiceResult(
                        aimId,
                        searchHeader % u": " % QString::number(tmpResults.size()),
                        qsl("search_results threads"));

                    threadsResults_.append(msgsHdr);
                }
                threadsResults_.append(tmpResults);
            }
        }
    }

    void SearchModel::composeSuggests()
    {
        if (!isSearchInSingleDialog())
            return;

        const auto dialogAimId = getSingleDialogAimId();
        if (dialogAimId.isEmpty())
            return;

        const auto lastPatterns = Logic::getLastSearchPatterns()->getPatterns(dialogAimId);
        if (lastPatterns.empty())
            return;

        results_.clear();
        messagesResults_.clear();
        threadsResults_.clear();

        if (isCategoriesEnabled())
        {
            static const Data::AbstractSearchResultSptr recentsHdr = makeServiceResult(
                qsl("~recents~"),
                QT_TRANSLATE_NOOP("contact_list", "RECENTS"),
                qsl("search_results recents"));
            results_.append(recentsHdr);
        }

        int i = 0;
        for (const auto& pattern : lastPatterns)
        {
            auto sr = std::make_shared<Data::SearchResultSuggest>();
            sr->suggestAimId_ = dialogAimId % u"_suggest" % QString::number(i++);
            sr->suggestText_ = pattern;
            results_.append(sr);
        }

        qCDebug(searchModel) << this << "composed" << results_.size() - 1 << "suggests for" << dialogAimId;
        Q_EMIT dataChanged(index(0), index(rowCount()));
        Q_EMIT suggests(QPrivateSignal());
    }

    void SearchModel::onDataChanged(const QString& _aimId)
    {
        if (_aimId.isEmpty())
            return;

        int i = 0;
        for (const auto& item : std::as_const(results_))
        {
            if ((item->isChat() || item->isContact() || item->isMessage()) && (item->getAimId() == _aimId || item->getSenderAimId() == _aimId))
            {
                const auto idx = index(i);
                Q_EMIT dataChanged(idx, idx);
            }
            ++i;
        }
    }

    void SearchModel::setMessagesSearchFormat()
    {
        format_ = SearchFormat::ContactsAndMessages;
    }

    void SearchModel::setThreadsSearchFormat()
    {
        if (hasOnlyThreadResults())
            format_ = SearchFormat::Threads;
    }

    void Logic::SearchModel::initThreadSearcher()
    {
        if (threadSearcher_)
            return;

        if (Features::isThreadsEnabled())
        {
            threadSearcher_ = new ThreadSearcher(this);
            connect(threadSearcher_, &MessageSearcher::localResults, this, &SearchModel::onThreadsLocal);
            connect(threadSearcher_, &MessageSearcher::serverResults, this, &SearchModel::onThreadsServer);
            connect(threadSearcher_, &MessageSearcher::allResults, this, &SearchModel::onThreadsAll);
        }
    }
}
