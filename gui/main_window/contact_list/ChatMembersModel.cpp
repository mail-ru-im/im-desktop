#include "stdafx.h"
#include "ChatMembersModel.h"

#include "ContactListModel.h"
#include "../../my_info.h"
#include "../../cache/avatars/AvatarStorage.h"
#include "../../utils/utils.h"
#include "../friendly/FriendlyContainer.h"

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
        , isDoingRequest_(false)
        , mode_(MembersMode::All)
        , timerSearch_(new QTimer(this))
    {
        updateInfo(_info);
        connect(GetAvatarStorage(), &AvatarStorage::avatarChanged, this, &ChatMembersModel::onAvatarLoaded);
        connect(Ui::GetDispatcher(), &Ui::core_dispatcher::chatMembersPage, this, &ChatMembersModel::onChatMembersPage);
        connect(Ui::GetDispatcher(), &Ui::core_dispatcher::searchChatMembersPage, this, &ChatMembersModel::onSearchChatMembersPage);

        timerSearch_->setSingleShot(true);
        timerSearch_->setInterval(typingTimeout.count());
        connect(timerSearch_, &QTimer::timeout, this, &ChatMembersModel::doSearch);
    }

    ChatMembersModel::~ChatMembersModel()
    {
    }

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
                emit dataChanged(idx, idx);
                break;
            }
            ++i;
        }
    }

    void ChatMembersModel::onChatMembersPage(const qint64 _seq, const std::shared_ptr<Data::ChatMembersPage>& _page, const int _pageSize, const bool _resetPages)
    {
        if (!isSeqRelevant(_seq) || _pageSize < 0)
            return;

        if (_resetPages)
            clearResults();

        addResults(_page);
    }

    void ChatMembersModel::onSearchChatMembersPage(const qint64 _seq, const std::shared_ptr<Data::ChatMembersPage>& _page, const int _pageSize, const bool _resetPages)
    {
        if (!isSeqRelevant(_seq) || _pageSize < 0 || searchPattern_.isEmpty())
            return;

        if (_resetPages)
            clearResults();

        addResults(_page);
    }

    void ChatMembersModel::doSearch()
    {
        searchChatMembersPage();
        emit showSearchSpinner();
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
            return results_[curIndex]->getAccessibleName();

        return QVariant::fromValue(results_[curIndex]);
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
        {
            loadChatMembersPage();
        }
        else
        {
            timerSearch_->start();
        }
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

    void ChatMembersModel::loadBlocked()
    {
        mode_ = MembersMode::Blocked;
        clearResults();
        loadChatMembersPage();
    }

    void ChatMembersModel::loadPending()
    {
        mode_ = MembersMode::Pending;
        clearResults();
        loadChatMembersPage();
    }

    void ChatMembersModel::loadAdmins()
    {
        mode_ = MembersMode::Admins;
        clearResults();
        loadChatMembersPage();
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

    void ChatMembersModel::loadChatMembersPage()
    {
        Ui::gui_coll_helper collection(Ui::GetDispatcher()->create_collection(), true);
        collection.set_value_as_qstring("aimid", AimId_);
        collection.set_value_as_uint("page_size", Logic::MembersPageSize);

        if (cursorNextPage_.isEmpty())
            collection.set_value_as_qstring("role", getRoleName(mode_));
        else
            collection.set_value_as_qstring("cursor", cursorNextPage_);

        sequences_[mode_] = Ui::GetDispatcher()->post_message_to_core("chats/members/get", collection.get());
        isDoingRequest_ = true;
    }

    void ChatMembersModel::searchChatMembersPage()
    {
        Ui::gui_coll_helper collection(Ui::GetDispatcher()->create_collection(), true);
        collection.set_value_as_qstring("aimid", AimId_);
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

        sequences_[mode_] = Ui::GetDispatcher()->post_message_to_core("chats/members/search", collection.get());
        isDoingRequest_ = true;
    }

    void ChatMembersModel::loadAllMembers()
    {
        mode_ = MembersMode::All;
        clearResults();
        loadChatMembersPage();
    }

    void ChatMembersModel::updateInfo(const std::shared_ptr<Data::ChatInfo>& _info)
    {
        if (!_info)
            return;

        clearResults();

        setAimId(_info->AimId_);
        YourRole_ = _info->YourRole_;
        membersCount_ = _info->MembersCount_;

        if (mode_ == MembersMode::All)
        {
            for (const auto& member : _info->Members_)
            {
                auto resItem = std::make_shared<Data::SearchResultChatMember>();
                resItem->info_ = member;
                if (!searchPattern_.isEmpty())
                    resItem->highlights_ = { searchPattern_ };

                results_.push_back(std::move(resItem));
            }

            //!! todo: temporarily for voip
            members_.assign(_info->Members_.cbegin(), _info->Members_.cend());

            emit dataChanged(index(0), index(rowCount()));
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
        return YourRole_ == ql1s("admin");
    }

    bool ChatMembersModel::isModer() const
    {
        return YourRole_ == ql1s("moder");
    }

    QString ChatMembersModel::getRoleName(MembersMode _mode) const
    {
        switch (_mode)
        {
        case MembersMode::All:
            return qsl("members");
        case MembersMode::Admins:
            return qsl("admins");
        case MembersMode::Pending:
            return qsl("pending");
        case MembersMode::Blocked:
            return qsl("blocked");
        }
        return QString();
    }

    void ChatMembersModel::addResults(const std::shared_ptr<Data::ChatMembersPage>& _page)
    {
        cursorNextPage_ = _page->Cursor_;

        for (const auto& member : _page->Members_)
        {
            auto resItem = std::make_shared<Data::SearchResultChatMember>();
            resItem->info_ = member;
            if (!searchPattern_.isEmpty())
                resItem->highlights_ = { searchPattern_ };

            results_.push_back(std::move(resItem));
            members_.push_back(member);
        }

        emit dataChanged(index(0), index(rowCount()));

        emit hideSearchSpinner();

        if (!searchPattern_.isEmpty() && results_.empty())
            emit showNoSearchResults();

        isDoingRequest_ = false;
    }

    void ChatMembersModel::clearResults()
    {
        isDoingRequest_ = false;
        timerSearch_->stop();

        results_.clear();
        sequences_.clear();
        cursorNextPage_.clear();
        members_.clear();

        emit dataChanged(index(0), index(rowCount()));
        emit hideNoSearchResults();
        emit hideSearchSpinner();
    }

    void ChatMembersModel::setAimId(const QString& _aimId)
    {
        AimId_ = _aimId;
    }

    const std::vector<Data::ChatMemberInfo>&ChatMembersModel::getMembers() const
    {
        return members_;
    }
}
