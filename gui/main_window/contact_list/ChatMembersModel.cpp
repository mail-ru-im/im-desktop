#include "stdafx.h"
#include "ChatMembersModel.h"

#include "ContactListModel.h"
#include "ContactListUtils.h"
#include "../../my_info.h"
#include "../../cache/avatars/AvatarStorage.h"
#include "../../utils/utils.h"
#include "../containers/StatusContainer.h"
#include "../containers/LastseenContainer.h"
#include "../../search/ContactSearcher.h"

namespace
{
    constexpr std::chrono::milliseconds typingTimeout = std::chrono::milliseconds(300);
}

namespace Logic
{
    ChatMembersModel::ChatMembersModel(QObject* _parent)
        : ChatMembersModel(nullptr, _parent)
    {
    }

    ChatMembersModel::ChatMembersModel(const std::shared_ptr<Data::ChatInfo>& _info, QObject *_parent)
        : AbstractSearchModel(_parent)
        , membersCount_(0)
        , countFirstCheckedMembers_(0)
        , isDoingRequest_(false)
        , isInitialPage_(false)
        , mode_(MembersMode::All)
        , timerSearch_(new QTimer(this))
        , contactSearcher_(new ContactSearcher(this))
        , checkMode_(CheckMode::Force)
    {
        updateInfo(_info);
        connect(GetAvatarStorage(), &AvatarStorage::avatarChanged, this, &ChatMembersModel::onAvatarLoaded);
        connect(Ui::GetDispatcher(), &Ui::core_dispatcher::chatMembersPage, this, &ChatMembersModel::onChatMembersPage);
        connect(Ui::GetDispatcher(), &Ui::core_dispatcher::chatMembersPageCached, this, &ChatMembersModel::onChatMembersPageCached);
        connect(Ui::GetDispatcher(), &Ui::core_dispatcher::searchChatMembersPage, this, &ChatMembersModel::onSearchChatMembersPage);
        connect(Logic::GetStatusContainer(), &Logic::StatusContainer::statusChanged, this, [this](const QString& _aimid)
        {
            onAvatarLoaded(_aimid);
        });
        connect(contactSearcher_, &ContactSearcher::allResults, this, &ChatMembersModel::onContactsServer);

        timerSearch_->setSingleShot(true);
        timerSearch_->setInterval(typingTimeout);
        connect(timerSearch_, &QTimer::timeout, this, &ChatMembersModel::doSearch);

        contactSearcher_->setExcludeChats(SearchDataSource::localAndServer);
        contactSearcher_->setSearchSource(SearchDataSource::localAndServer);
    }

    ChatMembersModel::~ChatMembersModel() = default;

    void ChatMembersModel::onAvatarLoaded(const QString& _aimId)
    {
        if (results_.isEmpty() || _aimId.isEmpty())
            return;

        int i = 0;
        for (const auto& item : std::as_const(results_))
        {
            if (item->getAimId() == _aimId)
            {
                const auto idx = index(i);
                Q_EMIT dataChanged(idx, idx);
                break;
            }
            ++i;
        }
    }

    void ChatMembersModel::onChatMembersPage(const qint64 _seq, const std::shared_ptr<Data::ChatMembersPage>& _page, const int _pageSize, const bool _resetPages)
    {
        if (!isDoingRequest_ || !isSeqRelevant(_seq) || _pageSize < 0)
            return;

        if (_resetPages || isInitialPage_)
            clearResults();

        addResults(_page);
    }

    void ChatMembersModel::onChatMembersPageCached(const std::shared_ptr<Data::ChatMembersPage>& _page, const QString& _role)
    {
        if (!isDoingRequest_ || _role != getRoleName(mode_) || !searchPattern_.isEmpty())
            return;

        addResults(_page, true);
    }

    void ChatMembersModel::onSearchChatMembersPage(const qint64 _seq, const std::shared_ptr<Data::ChatMembersPage>& _page, const int _pageSize, const bool _resetPages)
    {
        if (!isDoingRequest_ || !isSeqRelevant(_seq) || _pageSize < 0 || searchPattern_.isEmpty())
        {
            if (!searchPattern_.isEmpty() && contactSearcher_->getSearchPattern() == searchPattern_)
            {
                Q_EMIT hideSearchSpinner();

                addSearcherResults();
                checkMode_ = CheckMode::Normal;

                if (results_.empty() && results_.empty())
                    Q_EMIT showNoSearchResults();
            }
            return;
        }

        if (_resetPages)
            clearResults();

        checkMode_ = CheckMode::Normal;
        addResults(_page);
    }

    void ChatMembersModel::doSearch()
    {
        if (isServerSearchEnabled_ && !searchPattern_.isEmpty())
            contactSearcher_->search(searchPattern_);
        searchChatMembersPage();
        Q_EMIT showSearchSpinner();
    }

    int ChatMembersModel::rowCount(const QModelIndex& _parent) const
    {
        return static_cast<int>(getVisibleRowsCount());
    }

    QVariant ChatMembersModel::data(const QModelIndex& _index, int _role) const
    {
        const auto curIndex = _index.row();
        if (!_index.isValid() || (_role != Qt::DisplayRole && !Testing::isAccessibleRole(_role)) || curIndex >= static_cast<int>(getVisibleRowsCount()))
            return QVariant();

        if (Testing::isAccessibleRole(_role))
            return QString(u"AS Sidebar GroupMember " % results_[curIndex]->getAccessibleName());

        return QVariant::fromValue(results_[curIndex]);
    }

    void ChatMembersModel::clearCheckedItems()
    {
        checkedMembers_.clear();
    }

    int ChatMembersModel::getCheckedItemsCount() const
    {
        return checkedMembers_.size();
    }

    void ChatMembersModel::setCheckedItem(const QString& _name, bool _isChecked)
    {
        if (std::none_of(results_.cbegin(), results_.cend(), [&_name](const auto& _el) {return _el->getAimId() == _name; })
            && std::none_of(checkedMembers_.cbegin(), checkedMembers_.cend(), [&_name](const auto& _el) {return _el == _name; }))
            return;

        if (_isChecked)
            checkedMembers_.insert(_name);
        else
            checkedMembers_.erase(_name);
    }

    bool ChatMembersModel::isCheckedItem(const QString& _name) const
    {
        return checkedMembers_.find(_name) != checkedMembers_.end();
    }

    std::vector<QString> ChatMembersModel::getCheckedMembers() const
    {
        std::vector<QString> members;
        members.reserve(getCheckedItemsCount());

        std::copy(checkedMembers_.cbegin(), checkedMembers_.cend(), std::back_inserter(members));

        return members;
    }

    void ChatMembersModel::setCheckedFirstMembers(int _count)
    {
        clearCheckedItems();

        countFirstCheckedMembers_ = std::max(_count, 0);

        if (!results_.isEmpty() && countFirstCheckedMembers_)
        {
            auto it = results_.cbegin();
            const size_t limit = std::min(results_.size(), countFirstCheckedMembers_);
            for (size_t i = 0; i < limit; ++i, ++it)
                checkedMembers_.insert((*it)->getAimId());

            countFirstCheckedMembers_ = 0;
        }

        Q_EMIT dataChanged(index(0), index(rowCount()));
    }

    void ChatMembersModel::setFocus()
    {
        results_.clear();
    }

    void ChatMembersModel::setSearchPattern(const QString& _pattern)
    {
        if (AimId_.isEmpty())
            return;

        clearResults();

        searchPattern_ = _pattern.trimmed();

        if (searchPattern_.isEmpty())
            loadChatMembersPage();
        else
            timerSearch_->start();
    }

    unsigned int ChatMembersModel::getVisibleRowsCount() const
    {
        return results_.size();
    }

    int ChatMembersModel::get_limit(int _limit)
    {
        return std::max(_limit, 0);
    }

    Data::ChatMemberSearchResultSptr ChatMembersModel::getMemberItem(const QString& _aimId) const
    {
        for (const auto& item : std::as_const(results_))
            if (item->getAimId() == _aimId)
                return std::static_pointer_cast<Data::SearchResultChatMember>(item);

        return nullptr;
    }

    int ChatMembersModel::getMembersCount() const
    {
        return membersCount_;
    }

    void ChatMembersModel::loadAllMembers()
    {
        loadImpl(MembersMode::All);
    }

    void ChatMembersModel::loadThreadSubscribers()
    {
        loadImpl(MembersMode::ThreadSubscribers);
    }

    void ChatMembersModel::loadBlocked()
    {
        loadImpl(MembersMode::Blocked);
    }

    void ChatMembersModel::loadPending()
    {
        loadImpl(MembersMode::Pending);
    }

    void ChatMembersModel::loadAdmins()
    {
        loadImpl(MembersMode::Admins);
    }

    void ChatMembersModel::loadYourInvites()
    {
        loadImpl(MembersMode::YourInvites);
    }

    void ChatMembersModel::requestNextPage()
    {
        if (!isFullMembersListLoaded() && !isDoingRequest_)
        {
            if (searchPattern_.isEmpty())
                loadChatMembersPage();
            else
                searchChatMembersPage();
        }
    }

    bool ChatMembersModel::isFullMembersListLoaded() const
    {
        return cursorNextPage_.isEmpty();
    }

    void ChatMembersModel::addIgnoredMember(const QString& _aimId)
    {
        ignoredMembers_.insert(_aimId);
    }

    bool ChatMembersModel::isIgnoredMember(const QString& _aimId)
    {
        return ignoredMembers_.find(_aimId) != ignoredMembers_.end();
    }

    void ChatMembersModel::clearIgnoredMember()
    {
        ignoredMembers_.clear();
    }

    void ChatMembersModel::loadChatMembersPage()
    {
        Ui::gui_coll_helper collection(Ui::GetDispatcher()->create_collection(), true);
        const auto isThread = mode_ == MembersMode::ThreadSubscribers;
        collection.set_value_as_qstring(isThread ? "thread_id" : "aimid", AimId_);
        collection.set_value_as_uint("page_size", std::max(Logic::MembersPageSize, static_cast<unsigned int>(countFirstCheckedMembers_)));

        if (cursorNextPage_.isEmpty())
        {
            collection.set_value_as_qstring("role", getRoleName(mode_));
            isInitialPage_ = true;
        }
        else
        {
            collection.set_value_as_qstring("cursor", cursorNextPage_);
        }

        const auto message = isThread ? "threads/subscribers/get" : "chats/members/get";
        sequences_[mode_] = Ui::GetDispatcher()->post_message_to_core(message, collection.get());
        isDoingRequest_ = true;
    }

    void ChatMembersModel::searchChatMembersPage()
    {
        Ui::gui_coll_helper collection(Ui::GetDispatcher()->create_collection(), true);
        const auto isThread = mode_ == MembersMode::ThreadSubscribers;
        collection.set_value_as_qstring(isThread ? "thread_id" : "aimid", AimId_);
        collection.set_value_as_uint("page_size", Logic::MembersPageSize);

        if (cursorNextPage_.isEmpty())
        {
            collection.set_value_as_qstring("role", getRoleName(mode_));
            collection.set_value_as_qstring("keyword", searchPattern_);
        }
        else
        {
            collection.set_value_as_qstring("cursor", cursorNextPage_);
        }

        const auto message = isThread ? "threads/subscribers/search" : "chats/members/search";
        sequences_[mode_] = Ui::GetDispatcher()->post_message_to_core(message, collection.get());
        isDoingRequest_ = true;
    }

    void ChatMembersModel::updateInfo(const std::shared_ptr<Data::ChatInfo>& _info)
    {
        if (!_info)
            return;

        clearResults();

        setAimId(_info->AimId_);
        YourRole_ = _info->YourRole_;
        membersCount_ = _info->MembersCount_;

        std::vector<QString> firstCheckedInOrder;
        firstCheckedInOrder.reserve(countFirstCheckedMembers_);

        if (mode_ == MembersMode::All)
        {
            for (const auto& member : _info->Members_)
            {
                if (isIgnoredMember(member.AimId_))
                    continue;

                auto resItem = std::make_shared<Data::SearchResultChatMember>();
                resItem->info_ = member;
                if (!searchPattern_.isEmpty())
                    resItem->highlights_ = { searchPattern_ };

                if (countFirstCheckedMembers_ > 0)
                {
                    countFirstCheckedMembers_--;
                    firstCheckedInOrder.push_back(member.AimId_);
                    checkedMembers_.insert(member.AimId_);
                }

                results_.push_back(std::move(resItem));
            }

            //!! todo: temporarily for voip
            members_.assign(_info->Members_.cbegin(), _info->Members_.cend());

            Q_EMIT dataChanged(index(0), index(rowCount()));

            if (!firstCheckedInOrder.empty())
                Q_EMIT containsPreCheckedItems(firstCheckedInOrder);
        }
    }

    void ChatMembersModel::updateInfo(const Data::ThreadInfo* _info, bool _invalidateCanRemoveTill)
    {
        setAimId(_info->threadId_);
        membersCount_ = _info->subscribersCount_;

        clearResults();
        if (mode_ == MembersMode::All)
        {
            for (const auto& member : _info->subscribers_)
            {
                if (isIgnoredMember(member.AimId_))
                    continue;

                auto resItem = std::make_shared<Data::SearchResultChatMember>();
                resItem->info_ = member;
                if (_invalidateCanRemoveTill)
                    resItem->info_.canRemoveTill_ = {};
                results_.push_back(std::move(resItem));
            }

            Q_EMIT dataChanged(index(0), index(rowCount()));
        }
    }

    bool ChatMembersModel::isSeqRelevant(qint64 _answerSeq) const
    {
        auto it = sequences_.find(mode_);
        if (it == sequences_.end())
            return false;

        return _answerSeq >= it->second;
    }

    bool ChatMembersModel::contains(const QString& _aimId) const
    {
        // TODO : use hash-table here
        return std::any_of(results_.cbegin(), results_.cend(), [&_aimId](const auto& x) { return x->getAimId() == _aimId; });
    }

    QString ChatMembersModel::getChatAimId() const
    {
        return AimId_;
    }

    bool ChatMembersModel::isAdmin() const
    {
        return YourRole_ == u"admin";
    }

    bool ChatMembersModel::isModer() const
    {
        return YourRole_ == u"moder";
    }

    void ChatMembersModel::loadImpl(MembersMode _mode)
    {
        mode_ = _mode;
        clearResults();
        clearCheckedItems();
        loadChatMembersPage();
    }

    QStringView ChatMembersModel::getRoleName(MembersMode _mode) const
    {
        switch (_mode)
        {
        case MembersMode::All:
            return u"members";
        case MembersMode::Admins:
            return u"admins";
        case MembersMode::Pending:
            return u"pending";
        case MembersMode::Blocked:
            return u"blocked";
        case MembersMode::YourInvites:
            return u"invitations";
        case MembersMode::ThreadSubscribers:
            return u"thread_subscribers";
        }
        return {};
    }

    void ChatMembersModel::addResults(const std::shared_ptr<Data::ChatMembersPage>& _page, bool _fromCache)
    {
        if (!_fromCache)
            cursorNextPage_ = _page->Cursor_;

        std::vector<QString> firstCheckedInOrder;
        firstCheckedInOrder.reserve(countFirstCheckedMembers_);

        for (const auto& member : _page->Members_)
        {
            if (isIgnoredMember(member.AimId_))
                continue;

            auto resItem = std::make_shared<Data::SearchResultChatMember>();
            resItem->info_ = member;
            if (!searchPattern_.isEmpty())
                resItem->highlights_ = { searchPattern_ };

            if (auto it = std::find_if(results_.begin(), results_.end(), [&member](const auto& x) { return x->getAimId() == member.AimId_; }); it != results_.end())
            {
                *it = std::move(resItem);
                continue;
            }

            if (countFirstCheckedMembers_ > 0 && checkMode_ == CheckMode::Force)
            {
                countFirstCheckedMembers_--;
                firstCheckedInOrder.push_back(member.AimId_);
                checkedMembers_.insert(member.AimId_);
            }

            results_.push_back(std::move(resItem));
            members_.push_back(member);
        }

        if (contactSearcher_->getSearchPattern() == searchPattern_)
            addSearcherResults();

        Q_EMIT dataChanged(index(0), index(rowCount()));

        if (!firstCheckedInOrder.empty())
            Q_EMIT containsPreCheckedItems(firstCheckedInOrder);

        Q_EMIT hideSearchSpinner();

        if (!searchPattern_.isEmpty() && results_.empty())
            Q_EMIT showNoSearchResults();

        if (!_fromCache)
            isDoingRequest_ = false;
    }

    void ChatMembersModel::clearResults()
    {
        isDoingRequest_ = false;
        isInitialPage_ = false;
        timerSearch_->stop();

        results_.clear();
        sequences_.clear();
        cursorNextPage_.clear();
        members_.clear();
        contactSearcher_->clear();

        Q_EMIT dataChanged(index(0), index(rowCount()));
        Q_EMIT hideNoSearchResults();
        Q_EMIT hideSearchSpinner();
    }

    void ChatMembersModel::setAimId(const QString& _aimId)
    {
        AimId_ = _aimId;
    }

    const std::vector<Data::ChatMemberInfo>&ChatMembersModel::getMembers() const
    {
        return members_;
    }

    bool ChatMembersModel::isClickableItem(const QModelIndex& _index) const
    {
        const auto aimId = Logic::aimIdFromIndex(_index);
        return aimId.isEmpty() || aimId != Ui::MyInfo()->aimId();
    }

    void ChatMembersModel::add(const QString& _aimId)
    {
        if (_aimId.isEmpty())
            return;

        if (contains(_aimId))
            return;

        Data::ChatMemberInfo member;
        member.AimId_ = _aimId;
        member.Lastseen_ = Logic::GetLastseenContainer()->getLastSeen(_aimId);

        auto resItem = std::make_shared<Data::SearchResultChatMember>();
        resItem->info_ = std::move(member);
        if (!searchPattern_.isEmpty())
            resItem->highlights_ = { searchPattern_ };

        results_.push_back(std::move(resItem));
    }

    void ChatMembersModel::onContactsServer()
    {
        if (contactSearcher_->getSearchPattern() != searchPattern_)
            return;

        Q_EMIT hideNoSearchResults();

        addSearcherResults();

        Q_EMIT dataChanged(index(0), index(rowCount()));

        Q_EMIT hideSearchSpinner();

        if (!searchPattern_.isEmpty() && results_.empty())
            Q_EMIT showNoSearchResults();
    }

    void ChatMembersModel::addSearcherResults()
    {
        auto contactSearcherResults = contactSearcher_->getLocalResults();
        contactSearcherResults.append(contactSearcher_->getServerResults());
        for (const auto &res : as_const(contactSearcherResults))
            add(res->getAimId());
    }
}
