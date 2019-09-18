#include "stdafx.h"

#include "SearchModel.h"
#include "ContactListModel.h"
#include "RecentsModel.h"

#include "../../search/ContactSearcher.h"
#include "../../search/MessageSearcher.h"

#include "../../utils/InterConnector.h"
#include "../../core_dispatcher.h"
#include "../../utils/SearchPatternHistory.h"
#include "../../cache/avatars/AvatarStorage.h"
#include "../../gui_settings.h"

Q_LOGGING_CATEGORY(searchModel, "searchModel")

namespace
{
    constexpr auto minSymbolsServerMessages = 2;
    constexpr auto minSymbolsServerContacts = 4;

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
        , isCategoriesEnabled_(true)
        , isSortByTime_(false)
        , isSearchInDialogsEnabled_(true)
        , isSearchInContactsEnabled_(true)
        , localMsgSearchNoMore_(false)
        , timerSearch_(new QTimer(this))
        , allContactResRcvd_(false)
        , allMessageResRcvd_(false)
    {
        timerSearch_->setSingleShot(true);
        timerSearch_->setInterval(typingTimeout.count());
        connect(timerSearch_, &QTimer::timeout, this, &SearchModel::doSearch);

        connect(contactSearcher_, &ContactSearcher::localResults, this, &SearchModel::onContactsLocal);
        connect(contactSearcher_, &ContactSearcher::serverResults, this, &SearchModel::onContactsServer);
        connect(contactSearcher_, &ContactSearcher::allResults, this, &SearchModel::onContactsAll);

        connect(messageSearcher_, &MessageSearcher::localResults, this, &SearchModel::onMessagesLocal);
        connect(messageSearcher_, &MessageSearcher::serverResults, this, &SearchModel::onMessagesServer);
        connect(messageSearcher_, &MessageSearcher::allResults, this, &SearchModel::onMessagesAll);

        connect(&Utils::InterConnector::instance(), &Utils::InterConnector::repeatSearch, this, &SearchModel::repeatSearch);
        connect(&Utils::InterConnector::instance(), &Utils::InterConnector::disableSearchInDialog, this, [this]() { setSearchInSingleDialog(QString()); });

        connect(getContactListModel(), &ContactListModel::contact_removed, this, &SearchModel::contactRemoved);

        connect(GetAvatarStorage(), &Logic::AvatarStorage::avatarChanged, this, &SearchModel::avatarLoaded);

        for (auto v : { &results_, &contactLocalRes_, &contactServerRes_, &msgLocalRes_, &msgServerRes_})
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
                emit dataChanged(idx, idx);
            }
            ++i;
        }
    }

    void SearchModel::requestMore()
    {
        if (messageSearcher_->haveMoreResults() && !messageSearcher_->isDoingServerRequest())
        {
            qCDebug(searchModel) << this << "requested more results from server, currently have" << msgServerRes_.size() << "items";
            messageSearcher_->requestMoreResults();
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
            emit dataChanged(index(0), index(newCount));
            if (newCount == 0)
                emit showNoSearchResults();
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
            assert(!searchPattern_.isEmpty());
            uint32_t src = SearchDataSource::none;

            if (isServerMessagesNeeded())
                src |= SearchDataSource::server;

            if (!localMsgSearchNoMore_)
                src |= SearchDataSource::local;

            messageSearcher_->setSearchSource(static_cast<SearchDataSource>(src));
            messageSearcher_->search(searchPattern_);

            if (src & SearchDataSource::server)
                Ui::GetDispatcher()->post_stats_to_core(core::stats::stats_event_names::chat_search_dialog_server_first_req);
        }

        emit hideNoSearchResults();
        emit showSearchSpinner();
    }

    void SearchModel::onContactsLocal()
    {
        if (contactSearcher_->getSearchPattern() != searchPattern_)
            return;

        contactLocalRes_ = contactSearcher_->getLocalResults();
        const auto sortByTime = isSortByTime();
        const auto searchPattern = searchPattern_;
        std::sort(contactLocalRes_.begin(), contactLocalRes_.end(), [sortByTime, searchPattern](const auto& first, const auto& second) { return simpleSort(first, second, sortByTime, searchPattern); });

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

        composeResults();
    }

    void SearchModel::onContactsAll()
    {
        allContactResRcvd_ = true;
        composeResults();
    }

    void SearchModel::onMessagesLocal()
    {
        if (messageSearcher_->getSearchPattern() != searchPattern_)
            return;

        if (messageResServerRcvd_) // if we already have server results - do nothing
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

        composeResults();
    }

    void SearchModel::onMessagesServer()
    {
        if (messageSearcher_->getSearchPattern() != searchPattern_)
            return;

        if (messageSearcher_->isServerTimedOut())
            return;

        endLocalSearch();

        messageResServerRcvd_ = true;
        allMessageResRcvd_ = true;

        if (msgServerRes_.isEmpty())
            Ui::GetDispatcher()->post_stats_to_core(core::stats::stats_event_names::chat_search_dialog_server_first_ans);

        msgServerRes_ = messageSearcher_->getServerResults();
        qCDebug(searchModel) << this << "got more results from server, currently have" << msgServerRes_.size() << "items";

        composeResults();
    }

    void SearchModel::onMessagesAll()
    {
        allMessageResRcvd_ = true;
        composeResults();
    }

    void SearchModel::setSearchPattern(const QString& _newPattern)
    {
        clear();

        const auto prevPattern = std::move(searchPattern_);
        searchPattern_ = std::move(_newPattern).trimmed();

        if (isSearchInDialogs() && searchPattern_.isEmpty())
        {
            emit hideSearchSpinner();
            emit hideNoSearchResults();

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
            return results_[cur]->getAccessibleName();

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

        contactSearcher_->clear();
        messageSearcher_->clear();

        allContactResRcvd_ = !isSearchInContacts();
        allMessageResRcvd_ = !isSearchInDialogs();
        messageResServerRcvd_ = false;

        for (auto v : { &results_, &contactLocalRes_, &contactServerRes_, &msgLocalRes_, &msgServerRes_ })
            v->clear();

        emit dataChanged(index(0), index(rowCount()));

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
        if (messageSearcher_->getDialogAimId() != _contact)
        {
            messageSearcher_->setDialogAimId(_contact);
            setSearchInContacts(!isSearchInSingleDialog());

            clear();
            endLocalSearch();

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

    void SearchModel::setExcludeChats(const bool _exclude)
    {
        contactSearcher_->setExcludeChats(_exclude);
    }

    void SearchModel::setHideReadonly(const bool _hide)
    {
        contactSearcher_->setHideReadonly(_hide);
    }

    QModelIndex SearchModel::contactIndex(const QString& _aimId) const
    {
        const auto it = std::find_if(results_.begin(), results_.end(), [&_aimId](const auto& _res) { return _res->getAimId() == _aimId; });
        if (it != results_.end())
            return index(std::distance(results_.begin(), it));

        return QModelIndex();
    }

    int SearchModel::getTotalMessagesCount() const
    {
        return !msgServerRes_.isEmpty() ? messageSearcher_->getTotalServerEntries() : msgLocalRes_.size();
    }

    bool Logic::SearchModel::isAllDataReceived() const
    {
        return allContactResRcvd_ && allMessageResRcvd_;
    }

    const QString SearchModel::getContactsAndGroupsAimId()
    {
        return qsl("~contgroups~");
    }

    const QString SearchModel::getMessagesAimId()
    {
        return qsl("~messages~");
    }

    const bool SearchModel::simpleSort(const Data::AbstractSearchResultSptr& _first, const Data::AbstractSearchResultSptr& _second, bool _sort_by_time, const QString& _searchPattern)
    {
        const auto fAimId = _first->getAimId();
        const auto sAimId = _second->getAimId();

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

        const auto force_cyrillic = _searchPattern.isEmpty() ? Ui::get_gui_settings()->get_value<QString>(settings_language, QString()) == qsl("ru") : Utils::startsCyrillic(_searchPattern);

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

    bool SearchModel::isServerMessagesNeeded() const
    {
        return searchPattern_.length() >= minSymbolsServerMessages && isServerSearchEnabled();
    }

    bool SearchModel::isServerContactsNeeded() const
    {
        return searchPattern_.length() >= minSymbolsServerContacts && isServerSearchEnabled();
    }

    void SearchModel::composeResults()
    {
        results_.clear();
        results_.reserve(contactLocalRes_.size() + contactServerRes_.size() + std::max(msgLocalRes_.size(), msgServerRes_.size()));

        if (isCategoriesEnabled())
            composeResultsChatsCategorized();
        else
            composeResultsChatsSimple();

        if (allMessageResRcvd_)
            composeResultsMessages();

        qCDebug(searchModel) << this << "composed" << results_.size() << "items:\n"
            << msgLocalRes_.size() << "local messages" << (!msgServerRes_.isEmpty() ? "(unused)" : "")
            << msgServerRes_.size() << "server messages";

        emit dataChanged(index(0), index(rowCount()));

        if (rowCount() > 0)
        {
            emit hideSearchSpinner();
            emit hideNoSearchResults();

            emit results();
            qCDebug(searchModel) << this << "composed Results";
        }
        else if (isAllDataReceived())
        {
            emit hideSearchSpinner();
            emit showNoSearchResults();

            qCDebug(searchModel) << this << "composed NoSearchResults";
        }
    }

    void SearchModel::composeResultsChatsCategorized()
    {
        if (!contactLocalRes_.isEmpty() || !contactServerRes_.isEmpty())
        {
            const Data::AbstractSearchResultSptr hdr = makeServiceResult(
                getContactsAndGroupsAimId(),
                QT_TRANSLATE_NOOP("search", "CONTACTS AND GROUPS") % ql1s(": ") % QString::number(contactLocalRes_.size() + contactServerRes_.size()),
                qsl("search_results contactsAndGroups"));

            results_.append(hdr);

            composeResultsChatsSimple();
        }
    }

    void SearchModel::composeResultsChatsSimple()
    {
        results_.append(contactLocalRes_);

        for (const auto& res : std::as_const(contactServerRes_))
            if (std::none_of(contactLocalRes_.cbegin(), contactLocalRes_.cend(), [&res](const auto& _localRes) { return _localRes->getAimId() == res->getAimId(); }))
                results_.append(res);
    }

    void SearchModel::composeResultsMessages()
    {
        const bool haveMessages = !msgServerRes_.isEmpty() || (!msgLocalRes_.isEmpty() && !messageResServerRcvd_);
        if (haveMessages)
        {
            if (isCategoriesEnabled())
            {
                const Data::AbstractSearchResultSptr msgsHdr = makeServiceResult(
                    qsl("~messages~"),
                    QT_TRANSLATE_NOOP("search", "MESSAGES") % ql1s(": ") % QString::number(getTotalMessagesCount()),
                    qsl("search_results messages"));

                results_.append(msgsHdr);
            }

            if (!messageResServerRcvd_)
                results_.append(msgLocalRes_);
            else
                results_.append(msgServerRes_);
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
            sr->suggestAimId_ = dialogAimId % ql1s("_suggest") % QString::number(i++);
            sr->suggestText_ = pattern;
            results_.append(sr);
        }

        qCDebug(searchModel) << this << "composed" << results_.size() - 1 << "suggests for" << dialogAimId;
        emit dataChanged(index(0), index(rowCount()));
        emit suggests(QPrivateSignal());
    }
}
