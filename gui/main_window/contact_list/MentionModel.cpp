#include "stdafx.h"

#include "MentionModel.h"
#include "ContactListModel.h"
#include "SearchModel.h"

#include "../friendly/FriendlyContainer.h"
#include "../../cache/avatars/AvatarStorage.h"
#include "../../search/ContactSearcher.h"
#include "../../utils/gui_coll_helper.h"
#include "../../core_dispatcher.h"

Q_LOGGING_CATEGORY(mentionsModel, "mentionsModel")

namespace
{
    constexpr std::chrono::milliseconds serverTimeout = std::chrono::seconds(1);
    constexpr std::chrono::milliseconds typingTimeout = std::chrono::milliseconds(100);
    constexpr auto expectedCount = 100;
}

namespace Logic
{
    std::unique_ptr<MentionModel> g_mention_model;

    MentionModel::MentionModel(QObject * _parent)
        : AbstractSearchModel(_parent)
        , localSearcher_(new ContactSearcher(this))
        , sendServerRequests_(true)
        , curChatSendersCount_(0)
        , typingTimer_(new QTimer(this))
        , responseTimer_(new QTimer(this))
        , requestId_(-1)
        , serverFailed_(true)
        , responseTime_(-1)
        , requestsReturnedCount_(0)
        , requestsReturnedNeeded_(0)
    {
        setServerSearchEnabled(true);

        localSearcher_->setExcludeChats(true);
        localSearcher_->setSearchSource(SearchDataSource::local);

        connect(localSearcher_, &ContactSearcher::localResults, this, &MentionModel::onContactListResults);

        connect(getContactListModel(), &ContactListModel::selectedContactChanged, this, &MentionModel::onContactSelected);

        connect(GetAvatarStorage(), &AvatarStorage::avatarChanged, this, &MentionModel::avatarLoaded);
        connect(GetFriendlyContainer(), &FriendlyContainer::friendlyChanged, this, &MentionModel::friendlyChanged);

        connect(Ui::GetDispatcher(), &Ui::core_dispatcher::messageBuddies, this, &MentionModel::messageBuddies);
        connect(Ui::GetDispatcher(), &Ui::core_dispatcher::mentionsSuggestResults, this, &MentionModel::onServerResults);

        responseTimer_->setSingleShot(true);
        responseTimer_->setInterval(serverTimeout.count());
        connect(responseTimer_, &QTimer::timeout, this, &MentionModel::onServerTimeout);

        typingTimer_->setSingleShot(true);
        typingTimer_->setInterval(typingTimeout.count());
        connect(typingTimer_, &QTimer::timeout, this, &MentionModel::search);

        match_.reserve(expectedCount);
        localResults_.reserve(expectedCount);
        serverResults_.reserve(expectedCount);
        clResults_.reserve(expectedCount);
    }

    int MentionModel::rowCount(const QModelIndex &) const
    {
        return match_.size();
    }

    QVariant MentionModel::data(const QModelIndex & _index, int _role) const
    {
        if (!_index.isValid() || (_role != Qt::DisplayRole && !Testing::isAccessibleRole(_role)) || (unsigned)_index.row() >= match_.size())
            return QVariant();

        if (Testing::isAccessibleRole(_role))
            return match_[_index.row()].aimId_;

        return QVariant::fromValue<MentionSuggest>(match_[_index.row()]);
    }

    void MentionModel::filterLocal()
    {
        const auto& curSenders = chatSenders_[dialogAimId_];

        if (searchPattern_.isEmpty())
        {
            localResults_ = curSenders;
        }
        else
        {
            localResults_.clear();

            std::copy_if(curSenders.cbegin(), curSenders.cend(), std::back_inserter(localResults_),
                [this](const auto& item)
            {
                return item.aimId_.contains(searchPattern_, Qt::CaseInsensitive)
                    || item.friendlyName_.contains(searchPattern_, Qt::CaseInsensitive);
            });
        }
        curChatSendersCount_ = localResults_.size();
    }

    void MentionModel::setSearchPattern(const QString& _pattern, const SearchCondition _condition)
    {
        auto trimmedPattern = _pattern.trimmed();
        if (_condition != SearchCondition::Force && searchPattern_ == trimmedPattern)
            return;

        searchPattern_ = std::move(trimmedPattern);

        if (_condition != SearchCondition::Force)
            typingTimer_->start();
        else
            search();
    }

    void MentionModel::setDialogAimId(const QString& _aimid)
    {
        if (dialogAimId_ == _aimid)
            return;

        responseTimer_->stop();
        requestId_ = -1;

        dialogAimId_ = _aimid;
        sendServerRequests_ = Logic::getContactListModel()->isChat(dialogAimId_) && isServerSearchEnabled();
    }

    QString MentionModel::getDialogAimId() const
    {
        return dialogAimId_;
    }

    bool MentionModel::isServiceItem(const QModelIndex & _index) const
    {
        if (!_index.isValid())
            return false;

        const auto row = _index.row();
        if (row < 0 || row >= int(match_.size()))
            return false;

        return match_[row].isServiceItem();
    }

    bool MentionModel::isClickableItem(const QModelIndex& _index) const
    {
        return !isServiceItem(_index);
    }

    bool MentionModel::isFromChatSenders(const QModelIndex & _index) const
    {
        return _index.row() < curChatSendersCount_;
    }

    void MentionModel::requestServer()
    {
        Ui::gui_coll_helper collection(Ui::GetDispatcher()->create_collection(), true);
        collection.set_value_as_qstring("aimid", getDialogAimId());
        collection.set_value_as_qstring("pattern", searchPattern_);

        qCDebug(mentionsModel) << "Requested server for" << searchPattern_;
        requestId_ = Ui::GetDispatcher()->post_message_to_core("mentions/suggests/get", collection.get());

        responseTimer_->start();
    }

    void MentionModel::onRequestReturned()
    {
        requestsReturnedCount_++;

        if (requestsReturnedCount_ >= requestsReturnedNeeded_)
            composeResults();
    }

    void MentionModel::composeResults()
    {
        const bool useLocalResults = (serverFailed_ || !sendServerRequests_);

        qCDebug(mentionsModel) << "Using mentions chat suggests from" << (useLocalResults ? "LOCAL" : "SERVER");

        const auto isChat = Logic::getContactListModel()->isChat(dialogAimId_);

        match_ = std::move(useLocalResults ? localResults_: serverResults_);

        if (!isChat)
            match_.reserve(match_.size() + clResults_.size());

        curChatSendersCount_ = match_.size();

        if (!isChat)
        {
            if (!match_.empty())
                match_.emplace_back(qsl("~notinchat~"), QT_TRANSLATE_NOOP("mentions", "not in this chat"), searchPattern_);

            std::copy_if(clResults_.cbegin(), clResults_.cend(), std::back_inserter(match_),
                [this](const auto& item)
            {
                return curChatSendersCount_ == 0 || std::none_of(
                    match_.cbegin(),
                    match_.cbegin() + curChatSendersCount_,
                    [&item](const auto& mi) { return mi.aimId_ == item.aimId_; });
            });

            if (!match_.empty() && match_.back().isServiceItem())
                match_.pop_back();
        }

        emit dataChanged(index(0), index(match_.size()));
        emit results();

        if (sendServerRequests_)
        {
            Ui::GetDispatcher()->post_stats_to_core(
                core::stats::stats_event_names::mentions_suggest_request_roundtrip,
                {
                    { "Timing", responseTime_ != -1 ? std::to_string(responseTime_) : "Timeout" },
                    { "From", serverFailed_ ? "Local" : "Server" }
                }
            );
        }
    }

    void MentionModel::onContactListResults()
    {
        if (localSearcher_->getSearchPattern() != searchPattern_)
            return;

        auto contacts = localSearcher_->getLocalResults();
        std::sort(contacts.begin(), contacts.end(), [](const auto& first, const auto& second) { return SearchModel::simpleSort(first, second, true, QString()); });

        clResults_.reserve(clResults_.size() + contacts.size());

        for (const auto& c : contacts)
            clResults_.emplace_back(c->getAimId(), c->getFriendlyName(), searchPattern_, c->getNick());

        qCDebug(mentionsModel) << "CL returned" << contacts.size() << "items";
        onRequestReturned();
    }

    void MentionModel::messageBuddies(const Data::MessageBuddies& _buddies, const QString & _aimId, Ui::MessagesBuddiesOpt /*_option*/, bool /*_havePending*/, qint64 /*_seq*/, int64_t /*_last_msgid*/)
    {
        if (_aimId.isEmpty())
            return;

        auto &senders = chatSenders_[_aimId];
        for (const auto& msg : _buddies)
        {
            if (msg && !msg->IsChatEvent() && !msg->IsOutgoing())
            {
                const auto sender = hist::normalizeAimId(msg->Chat_ ? msg->GetChatSender() : msg->AimId_);
                const auto end = senders.end();
                auto it = std::find_if(senders.begin(), end, [&sender](const auto &x) { return x.aimId_ == sender; });
                if (it != end)
                    it->friendlyName_ = Logic::GetFriendlyContainer()->getFriendly(sender);
                else
                    senders.emplace_back(sender, Logic::GetFriendlyContainer()->getFriendly(sender), searchPattern_);
            }
        }

        if (senders.empty())
        {
            if (const auto contact = Logic::getContactListModel()->getContactItem(_aimId))
            {
                if (!contact->is_chat())
                    senders.emplace_back(_aimId, Logic::GetFriendlyContainer()->getFriendly(_aimId), searchPattern_);
            }
        }
    }

    void MentionModel::onContactSelected(const QString& _aimid)
    {
        if (!_aimid.isEmpty())
            setDialogAimId(_aimid);
    }

    void MentionModel::avatarLoaded(const QString & _aimid)
    {
        if (_aimid.isEmpty())
            return;

        int i = 0;
        for (const auto& s : match_)
        {
            if (s.aimId_ == _aimid)
            {
                const auto ndx = index(i);
                emit dataChanged(ndx, ndx);
                break;
            }
            ++i;
        }
    }

    void MentionModel::friendlyChanged(const QString& _aimid, const QString& _friendlyName)
    {
        if (_aimid.isEmpty())
            return;

        int i = 0;
        for (auto& s : match_)
        {
            if (s.aimId_ == _aimid)
            {
                s.friendlyName_ = _friendlyName;
                const auto ndx = index(i);
                emit dataChanged(ndx, ndx);
                break;
            }
            ++i;
        }

        for (auto& [_, senders] : chatSenders_)
        {
            for (auto& s : senders)
            {
                if (s.aimId_ == _aimid)
                {
                    s.friendlyName_ = _friendlyName;
                    break;
                }
            }
        }
    }

    void MentionModel::onServerTimeout()
    {
        requestId_ = -1;

        qCDebug(mentionsModel) << "Server timed out";
        onRequestReturned();
    }

    void MentionModel::onServerResults(const int64_t _seq, const MentionsSuggests& _suggests)
    {
        if (requestId_ != _seq)
            return;

        responseTime_ = responseTimer_->interval() - responseTimer_->remainingTime();
        requestId_ = -1;
        serverFailed_ = false;
        responseTimer_->stop();

        serverResults_ = _suggests;
        for (auto& s : serverResults_)
            s.highlight_ = searchPattern_;

        qCDebug(mentionsModel) << "Server returned" << _suggests.size() << "items in" << responseTime_ << "ms";
        onRequestReturned();
    }

    void MentionModel::search()
    {
        if (searchPattern_.isEmpty())
            match_.clear();

        localResults_.clear();
        serverResults_.clear();
        clResults_.clear();
        curChatSendersCount_ = 0;
        requestId_ = -1;
        serverFailed_ = true;
        responseTime_ = -1;
        requestsReturnedNeeded_ = sendServerRequests_ ? 2 : 1;
        requestsReturnedCount_ = 0;

        qCDebug(mentionsModel) << "Searching for" << searchPattern_;

        if (sendServerRequests_)
            requestServer();

        localSearcher_->search(searchPattern_);
        filterLocal();
    }

    MentionModel* GetMentionModel()
    {
        if (!g_mention_model)
            g_mention_model = std::make_unique<MentionModel>(nullptr);

        return g_mention_model.get();
    }

    void ResetMentionModel()
    {
        if (g_mention_model)
            g_mention_model.reset();
    }
}
