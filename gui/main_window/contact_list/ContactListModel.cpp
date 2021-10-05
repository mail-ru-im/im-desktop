#include "stdafx.h"

#include "ContactListModel.h"
#include "ContactListModelSort.h"
#include "ServiceContacts.h"

#include "contact_profile.h"

#include "RecentsModel.h"
#include "UnknownsModel.h"
#include "../containers/FriendlyContainer.h"
#include "../containers/LastseenContainer.h"
#include "../containers/StatusContainer.h"
#include "../common.shared/string_utils.h"
#include "../history_control/HistoryControlPage.h"
#include "../../core_dispatcher.h"
#include "../../gui_settings.h"
#include "../../my_info.h"
#include "../../utils/gui_coll_helper.h"
#include "../../utils/InterConnector.h"
#include "../../utils/utils.h"
#include "../../utils/features.h"
#include "../../utils/log/log.h"
#include "../../cache/avatars/AvatarStorage.h"
#include "Common.h"
#include "types/idinfo.h"

Q_LOGGING_CATEGORY(clModel, "clModel")

namespace
{
    constexpr std::chrono::milliseconds sort_timer_timeout = std::chrono::seconds(30);

    bool isContactVisible(const Logic::ContactItem& _contact, const bool _isGroupsEnabled)
    {
        return
            _contact.is_visible() &&
            (_isGroupsEnabled || !_contact.is_group());
    }
}

namespace Logic
{
    std::unique_ptr<ContactListModel> g_contact_list_model;

    ContactListModel::ContactListModel(QObject* _parent)
        : CustomAbstractListModel(_parent)
        , gotPageCallback_(nullptr)
        , sortNeeded_(false)
        , sortTimer_(new QTimer(this))
        , scrollPosition_(0)
        , minVisibleIndex_(0)
        , maxVisibleIndex_(0)
        , isWithCheckedBox_(false)
        , topSortedCount_(0)
    {
        connect(Ui::GetDispatcher(), &Ui::core_dispatcher::contactList,        this, &ContactListModel::contactList);
        connect(Ui::GetDispatcher(), &Ui::core_dispatcher::presense,           this, &ContactListModel::presence);
        connect(Ui::GetDispatcher(), &Ui::core_dispatcher::outgoingMsgCount,   this, &ContactListModel::outgoingMsgCount);
        connect(Ui::GetDispatcher(), &Ui::core_dispatcher::contactRemoved,     this, &ContactListModel::contactRemoved);
        connect(Ui::GetDispatcher(), &Ui::core_dispatcher::chatInfo,           this, &ContactListModel::chatInfo);
        connect(Ui::GetDispatcher(), &Ui::core_dispatcher::chatInfoFailed,     this, &ContactListModel::chatInfoFailed);
        connect(Ui::GetDispatcher(), &Ui::core_dispatcher::idInfo,             this, &ContactListModel::onIdInfo);
        connect(Ui::GetDispatcher(), &Ui::core_dispatcher::dialogGalleryState, this, &ContactListModel::dialogGalleryState);
        connect(Ui::GetDispatcher(), &Ui::core_dispatcher::threadUpdates,      this, &ContactListModel::onThreadUpdates);
        connect(Ui::GetDispatcher(), &Ui::core_dispatcher::messageBuddies,     this, &ContactListModel::messageBuddies);

        connect(GetAvatarStorage(), &Logic::AvatarStorage::avatarChanged,   this, &ContactListModel::avatarLoaded);
        connect(Logic::GetStatusContainer(), &Logic::StatusContainer::statusChanged, this, [this](const QString& _aimid)
        {
            avatarLoaded(_aimid);
        });

        connect(&Utils::InterConnector::instance(), &Utils::InterConnector::clSortChanged, this, &ContactListModel::forceSort);

        connect(sortTimer_, &QTimer::timeout, this, qOverload<>(&ContactListModel::sort));
        sortTimer_->start(sort_timer_timeout);
    }

    int ContactListModel::rowCount(const QModelIndex &) const
    {
        return visible_indexes_.size();
    }

    int ContactListModel::getAbsIndexByVisibleIndex(int _visibleIndex) const
    {
        if (_visibleIndex >= 0 && _visibleIndex < int(visible_indexes_.size()))
            return visible_indexes_[_visibleIndex];
        return 0;
    }

    bool ContactListModel::isGroupsEnabled() const
    {
        return Ui::get_gui_settings()->get_value<bool>(settings_cl_groups_enabled, false);
    }

    QModelIndex ContactListModel::contactIndex(const QString& _aimId) const
    {
        const auto orderedIndex = getOrderIndexByAimid(_aimId);
        const auto visIter = std::find(visible_indexes_.cbegin(), visible_indexes_.cend(), orderedIndex);
        if (visIter != visible_indexes_.cend())
            return index(*visIter);
        return index(0);
    }

    QVariant ContactListModel::data(const QModelIndex& _i, int _r) const
    {
        int cur = _i.row();

        if (!_i.isValid() || (unsigned)cur >= contacts_.size())
            return QVariant();

        if (_r == Qt::DisplayRole)
        {
            if (maxVisibleIndex_ == 0 && minVisibleIndex_ == 0)
                maxVisibleIndex_ = minVisibleIndex_ = cur;

            if (cur < minVisibleIndex_)
                minVisibleIndex_ = cur;
            if (cur > maxVisibleIndex_)
                maxVisibleIndex_ = cur;
        }

        const auto ndx = getAbsIndexByVisibleIndex(cur);
        const auto cont = contacts_[ndx].Get();

        if (Testing::isAccessibleRole(_r))
            return QString(u"AS Contacts " % cont->AimId_);

        return QVariant::fromValue(cont);
    }

    QString ContactListModel::contactToTryOnTheme() const
    {
        if (!currentAimId_.isEmpty())
            return currentAimId_;

        QString recentsFirstContact = Logic::getRecentsModel()->firstContact();
        if (!recentsFirstContact.isEmpty())
            return recentsFirstContact;

        if (!contacts_.empty())
            return contacts_[getIndexByOrderedIndex(0)].get_aimid();

        return QString();
    }

    Qt::ItemFlags ContactListModel::flags(const QModelIndex& _i) const
    {
        if (!_i.isValid())
            return Qt::ItemIsEnabled;

        unsigned flags = CustomAbstractListModel::flags(_i) | Qt::ItemIsEnabled;

        const auto ndx = getAbsIndexByVisibleIndex(_i.row());
        const auto& cont = contacts_[ndx];

        if (_i.row() == 0 || cont.is_group())
            flags |= ~Qt::ItemIsSelectable;
        flags |= ~Qt::ItemIsEditable;

        return (Qt::ItemFlags)flags;
    }

    int ContactListModel::addItem(Data::ContactPtr _contact, QVector<QString>& _removed,  const bool _updatePlaceholder)
    {
        im_assert(!isThread(_contact->AimId_));

        auto item = getContactItem(_contact->AimId_);
        if (item != nullptr)
        {
            if (_contact->isDeleted_)
            {
                deletedContacts_.insert(_contact->AimId_);
                _removed.push_back(_contact->AimId_);
            }
            else
            {
                item->Get()->ApplyBuddy(_contact);
                Logic::GetAvatarStorage()->UpdateDefaultAvatarIfNeed(_contact->AimId_);

                const auto index = contactIndex(_contact->AimId_);
                Q_EMIT dataChanged(index, index);
            }
        }
        else
        {
            if (_contact->isDeleted_)
            {
                deletedContacts_.insert(_contact->AimId_);
                return (int)contacts_.size();
            }

            im_assert(!contains(_contact->AimId_));
            im_assert(std::none_of(contacts_.begin(), contacts_.end(), [&a = _contact->AimId_](const auto& _c){ return a == _c.get_aimid(); }));

            contacts_.emplace_back(_contact);

            const auto newNdx = (int)contacts_.size() - 1;
            sorted_index_cl_.emplace_back(newNdx);
            indexes_.insert(_contact->AimId_, newNdx);

            visible_indexes_.emplace_back(newNdx);
            emitChanged(newNdx, newNdx);
            Q_EMIT contactRename(_contact->AimId_);
        }

        sortNeeded_ = true;

        if (_updatePlaceholder)
            Logic::updatePlaceholders({Logic::Placeholder::Contacts});

        return (int)contacts_.size();
    }

    void ContactListModel::rebuildIndex()
    {
        int i = 0;

        indexes_.clear();
        for (const auto order_index : sorted_index_cl_)
        {
            im_assert(std::count_if(contacts_.begin(), contacts_.end(), [&a = contacts_[order_index].get_aimid()](const auto& _c){ return a == _c.get_aimid(); }) == 1);
            im_assert(!indexes_.contains(contacts_[order_index].get_aimid()));

            indexes_.insert(contacts_[order_index].get_aimid(), i++);
        }

        rebuildVisibleIndex();
    }

    void ContactListModel::rebuildVisibleIndex()
    {
        const auto groupsEnabled = isGroupsEnabled();

        visible_indexes_.clear();
        visible_indexes_.reserve(sorted_index_cl_.size());
        for (const auto &order_index : sorted_index_cl_)
        {
            if (isContactVisible(contacts_[order_index], groupsEnabled))
                visible_indexes_.emplace_back(order_index);
        }
    }

    void ContactListModel::contactList(std::shared_ptr<Data::ContactList> _cl, const QString& _type)
    {
        const int size = rowCount({});
        beginInsertRows(QModelIndex(), size, _cl->size() + size);

        const bool needSyncSort = contacts_.empty();

        const bool isDeletedType = _type == u"deleted";

        QVector<QString> removedContacts;

        for (auto it = _cl->keyBegin(), end = _cl->keyEnd(); it != end; ++it)
        {
            const auto& iter = *it;
            if (!isDeletedType)
            {
                if (iter->UserType_ != u"sms")
                {
                    addItem(iter, removedContacts, false);
                    if (!iter->isDeleted_)
                    {
                        if (iter->IsLiveChat_)
                            Q_EMIT liveChatJoined(iter->AimId_);
                        emitContactChanged(iter->AimId_);
                        Logic::GetLastseenContainer()->setContactLastSeen(iter->AimId_, iter->LastSeen_);
                    }
                }
            }
            else
            {
                removedContacts.push_back(iter->AimId_);
            }
        }

        std::vector<int> groupIds;
        for (const auto& iter : *_cl)
        {
            if (std::find(groupIds.begin(), groupIds.end(), iter->Id_) != groupIds.end())
                continue;

            if (!isDeletedType)
            {
                groupIds.push_back(iter->Id_);
                auto group = std::make_shared<Data::Group>();
                group->ApplyBuddy(iter);
                addItem(group, removedContacts, false);
            }
            else if (iter->Removed_)
            {
                for (const auto& contact: contacts_)
                {
                    if (contact.Get()->GroupId_ == iter->Id_)
                        removedContacts.push_back(contact.get_aimid());
                }
            }
        }
        endInsertRows();

        rebuildIndex();

        removeContactsFromModel(removedContacts);

        Logic::updatePlaceholders({Logic::Placeholder::Contacts});
        sortNeeded_ = true;

        if (needSyncSort)
            sort();
    }

    int ContactListModel::getTopSortedCount() const
    {
        return topSortedCount_;
    }

    int ContactListModel::contactsCount() const
    {
        return std::count_if(contacts_.begin(), contacts_.end(), [](const auto & item) { return !item.is_group();});
    }

    bool ContactListModel::isChannel(const QString& _aimId) const
    {
        if (!_aimId.isEmpty())
        {
            if (const auto item = getContactItem(_aimId))
                return item->is_channel();
            else if (auto it = chatsCache_.find(_aimId); it != chatsCache_.end())
                return it->channel_;
        }
        return false;
    }

    const Data::DialogGalleryState& ContactListModel::getGalleryState(const QString& _aimid, RequestIfEmpty _request) const
    {
        const auto& res = [this, _aimid]() -> const Data::DialogGalleryState&
        {
            if (const auto it = galleryStates_.find(_aimid); it != galleryStates_.end())
                return it->second;

            static const Data::DialogGalleryState empty;
            return empty;
        }();

        if (_request == RequestIfEmpty::Yes && res.isEmpty())
            Ui::GetDispatcher()->requestDialogGalleryState(_aimid);
        return res;
    }

    QString ContactListModel::getChatStamp(const QString& _aimid) const
    {
        if (!_aimid.isEmpty())
        {
            if (const auto item = getContactItem(_aimid))
                return item->get_stamp();
            else if (auto it = chatsCache_.find(_aimid); it != chatsCache_.end())
                return it->stamp_;
        }
        return QString();
    }

    QString ContactListModel::getChatName(const QString& _aimid) const
    {
        return chatsCache_[_aimid].name_;
    }

    QString ContactListModel::getChatDescription(const QString& _aimid) const
    {
        return chatsCache_[_aimid].description_;
    }

    QString ContactListModel::getChatRules(const QString& _aimid) const
    {
        return chatsCache_[_aimid].rules_;
    }

    const QString& ContactListModel::getAimidByStamp(const QString& _stamp) const
    {
        const auto it = std::find_if(chatsCache_.begin(), chatsCache_.end(), [&_stamp](const auto& data){ return data.stamp_ == _stamp; });
        if (it != chatsCache_.end())
            return it.key();

        static const QString empty;
        return empty;
    }

    void ContactListModel::updateChatInfo(const Data::ChatInfo& _info)
    {
        CachedChatData data;
        data.stamp_ = _info.Stamp_;
        data.name_ = _info.Name_;
        data.description_ = _info.About_;
        data.rules_ = _info.Rules_;
        data.joinModeration_ = _info.ApprovedJoin_;
        data.public_ = _info.Public_;
        data.channel_ = _info.isChannel();
        data.trustRequired_ = _info.trustRequired_;
        chatsCache_[_info.AimId_] = std::move(data);

        if (!_info.YouMember_ || _info.YourRole_.isEmpty())
            notAMemberChats_.push_back(_info.AimId_);
        else if (auto iter = std::find(notAMemberChats_.begin(), notAMemberChats_.end(), _info.AimId_); iter != notAMemberChats_.end())
            notAMemberChats_.erase(iter);

        if (auto iter = std::find(youBlockedChats_.begin(), youBlockedChats_.end(), _info.AimId_); iter != youBlockedChats_.end())
            youBlockedChats_.erase(iter);

        if (auto contact = getContactItem(_info.AimId_))
        {
            QString role = _info.YourRole_;
            if (_info.YouPending_)
                role = qsl("pending");
            else if (!_info.YouMember_ || role.isEmpty())
                role = qsl("notamember");

            contact->set_default_role(_info.DefaultRole_);

            contact->set_stamp(_info.Stamp_);
            if (contact->get_chat_role() != role)
            {
                contact->set_chat_role(role);
                Q_EMIT yourRoleChanged(_info.AimId_);
            }

            contact->Get()->IsPublic_ = _info.Public_;
            contact->Get()->isChannel_ = _info.isChannel();
        }
    }

    void ContactListModel::emitContactChanged(const QString& _aimId) const
    {
        Q_EMIT contactChanged(_aimId, QPrivateSignal());
    }

    bool ContactListModel::isThread(const QString& _aimId) const
    {
        return !_aimId.isEmpty() && threads_.find(_aimId) != threads_.end();
    }

    void ContactListModel::markAsThread(const QString& _threadId, const QString& _parentChatId)
    {
        // parent chat can be empty - it's ok
        if (!_threadId.isEmpty())
        {
            auto& parentId = threads_[_threadId]; // store thread id
            if (!_parentChatId.isEmpty())
                parentId = _parentChatId;
        }
    }

    std::optional<QString> ContactListModel::getThreadParent(const QString& _threadId) const
    {
        if (const auto it = threads_.find(_threadId); it != threads_.end() && !it->second.isEmpty())
            return it->second;
        return {};
    }

    bool ContactListModel::isReadonly(const QString& _aimId) const // no write access to this chat, for channel check use isChannel
    {
        auto item = getContactItem(_aimId);
        return item && item->is_readonly();
    }

    bool ContactListModel::isJoinModeration(const QString& _aimId) const
    {
        return chatsCache_[_aimId].joinModeration_;
    }

    bool ContactListModel::isTrustRequired(const QString& _aimId) const
    {
        if (!_aimId.isEmpty() && Features::isRestrictedFilesEnabled())
        {
            auto id = std::cref(_aimId);
            if (const auto it = threads_.find(_aimId); it != threads_.end() && !it->second.isEmpty())
                id = it->second;

            if (const auto it = chatsCache_.find(id); it != chatsCache_.end())
                return it->trustRequired_;
        }

        return false;
    }

    int ContactListModel::innerRemoveContact(const QString& _aimId)
    {
        const auto it = std::find_if(contacts_.cbegin(), contacts_.cend(), [&_aimId](const auto& _c) { return _c.get_aimid() == _aimId; });
        if (it == contacts_.cend())
            return contacts_.size();

        const int idx = std::distance(contacts_.cbegin(), it);
        const auto isLiveChat = it->is_live_chat();

        contacts_.erase(it);
        indexes_.remove(_aimId);
        updateIndexesListAfterRemoveContact(sorted_index_cl_, idx);
        updateIndexesListAfterRemoveContact(visible_indexes_, idx);

        if (isLiveChat)
            Q_EMIT liveChatRemoved(_aimId);

        return idx;
    }

    void ContactListModel::updateIndexesListAfterRemoveContact(std::vector<int>& _list, int _index)
    {
        _list.erase(std::remove(_list.begin(), _list.end(), _index), _list.end());
        for (auto& idx : _list)
        {
            if (idx > _index)
                idx -= 1;
        }
    }

    void ContactListModel::contactRemoved(const QString& _contact)
    {
        removeContactsFromModel({ _contact });
    }

    void ContactListModel::outgoingMsgCount(const QString & _aimid, const int _count)
    {
        auto contact = getContactItem(_aimid);
        if (contact && contact->get_outgoing_msg_count() != _count)
        {
            contact->set_outgoing_msg_count(_count);
            sortNeeded_ = true;
        }
    }

    void ContactListModel::dialogGalleryState(const QString& _aimId, const Data::DialogGalleryState& _state)
    {
        galleryStates_[_aimId] = _state;
    }

    void ContactListModel::onThreadUpdates(const Data::ThreadUpdates& _updates)
    {
        for (const auto& [_, upd] : _updates.messages_)
            markAsThread(upd->threadId_, Data::MessageParentTopic::getChat(upd->parent_));
    }

    void ContactListModel::messageBuddies(const Data::MessageBuddies& _buddies)
    {
        for (const auto& buddy : _buddies)
            markAsThread(buddy->threadData_.id_, Data::MessageParentTopic::getChat(buddy->threadData_.parentTopic_));
    }

    void ContactListModel::avatarLoaded(const QString& _aimId)
    {
        auto idx = getOrderIndexByAimid(_aimId);
        if (idx != -1)
        {
            QModelIndex mi(index(idx));
            Q_EMIT dataChanged(mi, mi);
        }
    }

    void ContactListModel::presence(std::shared_ptr<Data::Buddy> _presence)
    {
        auto contact = getContactItem(_presence->AimId_);
        if (contact)
        {
            contact->Get()->ApplyBuddy(_presence);
            sortNeeded_ = true;

            pushChange(getOrderIndexByAimid(_presence->AimId_));
            emitContactChanged(_presence->AimId_);
            Logic::GetLastseenContainer()->setContactLastSeen(_presence->AimId_, _presence->LastSeen_);
        }
    }

    void ContactListModel::groupClicked(int _groupId)
    {
        for (auto& contact: contacts_)
        {
            if (contact.Get()->GroupId_ == _groupId && !contact.is_group())
            {
                contact.set_visible(!contact.is_visible());

                const auto index = contactIndex(contact.get_aimid());
                Q_EMIT dataChanged(index, index);
            }
        }
        rebuildVisibleIndex();
    }

    void ContactListModel::scrolled(int _value)
    {
        minVisibleIndex_ = 0;
        maxVisibleIndex_ = 0;
        scrollPosition_ = _value;
    }

    void ContactListModel::pushChange(int _i)
    {
        updatedItems_.push_back(_i);
        int scrollPos = scrollPosition_;
        QTimer::singleShot(2000, this, [this, scrollPos](){ if (scrollPosition_ == scrollPos) processChanges(); });
    }

    void ContactListModel::processChanges()
    {
        if (updatedItems_.empty())
            return;

        for (auto iter : updatedItems_)
        {
            if (iter >= minVisibleIndex_ && iter <= maxVisibleIndex_)
                emitChanged(iter, iter);
        }

        updatedItems_.clear();
    }

    void ContactListModel::setCurrent(const QString& _aimId, qint64 _id, bool _sel, std::function<void(Ui::PageBase*)> _gotPageCallback, bool _ignoreScroll)
    {
        if (_gotPageCallback)
        {
            if (auto page = Utils::InterConnector::instance().getHistoryPage(_aimId))
                _gotPageCallback(page);
            else
                gotPageCallback_ = std::move(_gotPageCallback);
        }

        const auto prev = currentAimId_;
        currentAimId_ = _aimId;
        Q_EMIT selectedContactChanged(currentAimId_, prev);

        if (!currentAimId_.isEmpty())
        {
            if (_sel)
                Q_EMIT select(currentAimId_, _id, Logic::UpdateChatSelection::No, _ignoreScroll);

            if (!ServiceContacts::isServiceContact(currentAimId_))
                Ui::GetDispatcher()->contactSwitched(currentAimId_);
        }
    }

    void ContactListModel::setCurrentCallbackHappened(Ui::PageBase* _page)
    {
        if (gotPageCallback_)
        {
            gotPageCallback_(_page);
            gotPageCallback_ = nullptr;
        }
    }

    void ContactListModel::sort()
    {
        if (!sortNeeded_)
            return;

        forceSort();
    }

    void ContactListModel::forceSort()
    {
        sortNeeded_ = false;

        updateSortedIndexesList(sorted_index_cl_, getLessFuncCL(QDateTime::currentDateTime()));
        rebuildIndex();
        emitChanged(minVisibleIndex_, maxVisibleIndex_);
    }

    ContactListSorting::contact_sort_pred ContactListModel::getLessFuncCL(const QDateTime& current) const
    {
        const auto force_cyrillic = Ui::get_gui_settings()->get_value<QString>(settings_language, QString()) == u"ru";

        ContactListSorting::contact_sort_pred less = ContactListSorting::ItemLessThanDisplayName(force_cyrillic);

        if (Ui::get_gui_settings()->get_value<bool>(settings_cl_groups_enabled, false))
            less = ContactListSorting::ItemLessThan();

        return less;
    }

    void ContactListModel::updateSortedIndexesList(std::vector<int>& _list, ContactListSorting::contact_sort_pred _less)
    {
        _list.resize(contacts_.size());
        std::iota(_list.begin(), _list.end(), 0);

        auto it = _list.begin();
        topSortedCount_ = 0;

        const auto showPopular = Ui::get_gui_settings()->get_value<bool>(settings_show_popular_contacts, true);
        const auto groupsEnabled = isGroupsEnabled();

        if (showPopular)
        {
            std::sort(_list.begin(), _list.end(), [this, groupsEnabled](const int& a, const int& b)
            {
                const auto& fc = contacts_[a];
                const auto& sc = contacts_[b];

                if (!isContactVisible(fc, groupsEnabled))
                    return false;

                if (!isContactVisible(sc, groupsEnabled))
                    return true;

                const auto fOutgoingCount = fc.get_outgoing_msg_count();
                const auto sOutgoingCount = sc.get_outgoing_msg_count();

                if (fOutgoingCount > 0 && fOutgoingCount == sOutgoingCount)
                {
                    const auto fTime = Logic::getRecentsModel()->getTime(fc.get_aimid());
                    if (fTime == -1)
                        return false;

                    const auto sTime = Logic::getRecentsModel()->getTime(fc.get_aimid());
                    if (sTime == -1)
                        return true;

                    return fTime > sTime;
                }
                return fOutgoingCount > sOutgoingCount;
            });

            while (it != _list.end() && (it - _list.begin() < ContactListSorting::maxTopContactsByOutgoing))
            {
                if (!isContactVisible(contacts_[*it], groupsEnabled) || contacts_[*it].get_outgoing_msg_count() <= 0)
                    break;
                ++it;
            }
            topSortedCount_ = it - _list.begin();

            if constexpr (build::is_debug())
            {
                for (int i = 1; i < topSortedCount_; ++i)
                    im_assert(contacts_[_list[i]].get_outgoing_msg_count() <= contacts_[_list[i - 1]].get_outgoing_msg_count());

                qCDebug(clModel) << topSortedCount_ << "top contacts:";
                for (int i = 0; i < topSortedCount_; ++i)
                    qCDebug(clModel) << i + 1 << contacts_[_list[i]].Get()->Friendly_ << contacts_[_list[i]].get_outgoing_msg_count();
            }
        }

        std::sort(it, _list.end(), [_less, this] (const int& a, const int& b)
        {
            return _less(contacts_[a], contacts_[b]);
        });
    }

    bool ContactListModel::contains(const QString& _aimId) const
    {
        return indexes_.contains(_aimId);
    }

    bool ContactListModel::hasContact(const QString& _aimId) const
    {
        auto item = getContactItem(_aimId);
        if (item)
        {
            auto buddy = item->Get();
            if (buddy)
                return !buddy->isAutoAdded_;
        }

        return false;
    }

    const QString& ContactListModel::selectedContact() const
    {
        return currentAimId_;
    }

    void ContactListModel::add(const QString& _aimId, const QString& _friendly, bool _isTemporary)
    {
        if (deletedContacts_.contains(_aimId))
            return;

        auto addContact = std::make_shared<Data::Contact>();
        addContact->AimId_ = _aimId;
        addContact->Friendly_ = _friendly;
        addContact->Is_chat_ = Utils::isChat(_aimId);
        addContact->isAutoAdded_ = true;
        addContact->isTemporary_ = _isTemporary;

        if (auto it = chatsCache_.find(_aimId); it != chatsCache_.end())
        {
            addContact->isChannel_ = it->channel_;
            addContact->IsPublic_ = it->public_;
        }

        QVector<QString> removed;
        addItem(addContact, removed);
        setContactVisible(_aimId, false);
    }

    void ContactListModel::setContactVisible(const QString& _aimId, bool _visible)
    {
        auto item = getContactItem(_aimId);
        if (item && item->is_visible() != _visible)
        {
            item->set_visible(_visible);
            rebuildVisibleIndex();
            emitChanged(0, indexes_.size());
        }
    }

    ContactItem* ContactListModel::getContactItem(const QString& _aimId)
    {
        return const_cast<ContactItem*>(std::as_const(*this).getContactItem(_aimId));
    }

    QString ContactListModel::getAimidByABName(const QString &_name)
    {
        if (_name.isEmpty())
            return QString();

        std::vector<ContactItem> items;
        for (auto &contact : contacts_)
        {
            if (contact.Get() && _name == contact.Get()->GetDisplayName())
                items.push_back(contact);
        }

        if (items.size() == 1)
            return items.front().get_aimid();

        return QString();
    }

    const ContactItem* ContactListModel::getContactItem(const QString& _aimId) const
    {
        const auto idx = getOrderIndexByAimid(_aimId);
        if (idx == -1)
            return nullptr;

        if (idx >= (int) contacts_.size())
            return nullptr;

        return &contacts_[getIndexByOrderedIndex(idx)];
    }

    bool ContactListModel::isChat(const QString& _aimId) const
    {
        auto ci = getContactItem(_aimId);
        if (ci)
            return ci->is_chat();

        return false;
    }

    bool ContactListModel::isMuted(const QString& _aimId) const
    {
        auto ci = getContactItem(_aimId);
        if (ci)
            return ci->is_muted();

        return false;
    }

    bool ContactListModel::isLiveChat(const QString& _aimId) const
    {
        auto ci = getContactItem(_aimId);
        if (ci)
            return ci->is_live_chat();

        return false;
    }

    bool ContactListModel::isOfficial(const QString& _aimId) const
    {
        auto ci = getContactItem(_aimId);
        if (ci)
            return ci->is_official();

        return false;
    }

    bool ContactListModel::isPublic(const QString & _aimId) const
    {
        if (const auto ci = getContactItem(_aimId))
            return ci->is_public();
        else if (auto it = chatsCache_.find(_aimId); it != chatsCache_.end())
            return it->public_;

        return false;
    }

    int ContactListModel::getOutgoingCount(const QString& _aimId) const
    {
        auto ci = getContactItem(_aimId);
        if (!ci)
            return -2;

        return ci->get_outgoing_msg_count();
    }

    ContactListModel* getContactListModel()
    {
        if (!g_contact_list_model)
            g_contact_list_model = std::make_unique<Logic::ContactListModel>(nullptr);

        return g_contact_list_model.get();
    }

    void ResetContactListModel()
    {
        if (g_contact_list_model)
            g_contact_list_model.reset();
    }

    void ContactListModel::addContactToCL(const QString& _aimId, std::function<void(bool)> _callBack)
    {
        Ui::gui_coll_helper collection(Ui::GetDispatcher()->create_collection(), true);
        collection.set_value_as_qstring("contact", _aimId);
        collection.set_value_as_qstring("group", qsl("General"));
        collection.set_value_as_qstring("message", QT_TRANSLATE_NOOP("contact_list","Hello. Please add me to your contact list"));

        Ui::GetDispatcher()->post_message_to_core("contacts/add", collection.get(), this, [this, _aimId, _callBack = std::move(_callBack)](core::icollection* _coll)
        {
            Ui::gui_coll_helper coll(_coll, false);

            const QString contact = QString::fromUtf8(coll.get_value_as_string("contact"));

            int32_t err = coll.get_value_as_int("error");

            if (_callBack)
                _callBack(err == 0);

            Q_EMIT contact_added(contact, (err == 0));
            Q_EMIT needSwitchToRecents();
            Q_EMIT contactAdd(_aimId);
        });
    }

    void ContactListModel::renameContact(const QString& _aimId, const QString& _friendly, std::function<void(bool)> _callBack)
    {
        Ui::gui_coll_helper collection(Ui::GetDispatcher()->create_collection(), true);

        collection.set_value_as_qstring("contact", _aimId);
        collection.set_value_as_qstring("friendly", _friendly);

        Ui::GetDispatcher()->post_message_to_core("contacts/rename", collection.get(), this, [this, _aimId, _callBack = std::move(_callBack)](core::icollection* _coll)
        {
            Ui::gui_coll_helper coll(_coll, false);
            auto error = coll.get_value_as_int("error", 0);
            if (_callBack)
                _callBack(error == 0);
            Q_EMIT contactRename(_aimId);
        });
    }

    void ContactListModel::removeContactFromCL(const QString& _aimId)
    {
        if (!Utils::isChat(_aimId) && !Features::clRemoveContactsAllowed())
            return;

        Ui::gui_coll_helper collection(Ui::GetDispatcher()->create_collection(), true);
        collection.set_value_as_qstring("contact", _aimId);
        Ui::GetDispatcher()->post_message_to_core("contacts/remove", collection.get());
    }

    void ContactListModel::removeContactsFromModel(const QVector<QString>& _vcontacts, bool _emit)
    {
        if (_vcontacts.isEmpty())
            return;

        for (const auto& _aimid : _vcontacts)
            innerRemoveContact(_aimid);

        rebuildIndex();
        forceSort();

        if (_emit)
        {
            for (const auto& _aimid : _vcontacts)
                Q_EMIT contact_removed(_aimid);
        }

        Logic::updatePlaceholders({Logic::Placeholder::Contacts});
    }

    void ContactListModel::removeTemporaryContactsFromModel()
    {
        QVector<QString> autoContacts;

        for (const auto& contact : contacts_)
            if (const auto& item = contact.Get(); item && item->isTemporary_)
                autoContacts.push_back(item->AimId_);

        if (!autoContacts.isEmpty())
            removeContactsFromModel(autoContacts);
    }

    void ContactListModel::ignoreContact(const QString& _aimId, bool _ignore)
    {
#ifndef STRIP_VOIP
        if (_ignore && Ui::GetDispatcher()->getVoipController().hasEstablishCall())
            Ui::GetDispatcher()->getVoipController().setDecline("", _aimId.toStdString().c_str(), true);
#endif

        Ui::gui_coll_helper collection(Ui::GetDispatcher()->create_collection(), true);
        collection.set_value_as_qstring("contact", _aimId);
        collection.set_value_as_bool("ignore", _ignore);
        Ui::GetDispatcher()->post_message_to_core("contacts/ignore", collection.get());

        if (_ignore)
            Q_EMIT ignore_contact(_aimId);
    }

    bool ContactListModel::ignoreContactWithConfirm(const QString& _aimId)
    {
        auto confirm = Utils::GetConfirmationWithTwoButtons(
            QT_TRANSLATE_NOOP("popup_window", "Cancel"),
            QT_TRANSLATE_NOOP("popup_window", "Yes"),
            QT_TRANSLATE_NOOP("popup_window", "Are you sure you want to move contact to ignore list?"),
            Logic::GetFriendlyContainer()->getFriendly(_aimId),
            nullptr);

        if (confirm)
            ignoreContact(_aimId, true);

        return confirm;
    }

    bool ContactListModel::areYouAdmin(const QString& _aimId) const
    {
        const auto role = getYourRole(_aimId);
        return role == u"admin" || role == u"moder";
    }

    QString ContactListModel::getYourRole(const QString& _aimId) const
    {
        auto cont = getContactItem(_aimId);
        if (cont)
            return cont->get_chat_role();

        return QString();
    }

    void ContactListModel::setYourRole(const QString& _aimId, const QString& _role)
    {
        if (auto cont = getContactItem(_aimId); cont && cont->get_chat_role() != _role)
        {
            Log::write_network_log(su::concat("contact-list-debug",
                " setYourRole before, _aimId=", _aimId.toStdString(), ", role=", _role.toStdString(),
                "\r\n"));

            cont->set_chat_role(_role);
            Q_EMIT yourRoleChanged(_aimId);
        }
    }

    void ContactListModel::getIgnoreList()
    {
        Ui::gui_coll_helper collection(Ui::GetDispatcher()->create_collection(), true);
        Ui::GetDispatcher()->post_message_to_core("contacts/get_ignore", collection.get());
    }

    bool ContactListModel::areYouNotAMember(const QString& _aimid) const
    {
        return std::find(notAMemberChats_.begin(), notAMemberChats_.end(), _aimid) != notAMemberChats_.end();
    }

    bool ContactListModel::areYouBlocked(const QString& _aimid) const
    {
        return std::find(youBlockedChats_.begin(), youBlockedChats_.end(), _aimid) != youBlockedChats_.end();
    }

    bool ContactListModel::areYouAllowedToWriteInThreads(const QString& _aimId) const
    {
        const auto isReadOnly = isReadonly(_aimId);
        const auto isThisChannel = isChannel(_aimId);
        const auto areAMember = !areYouNotAMember(_aimId);
        const auto areBlocked = areYouBlocked(_aimId);
        return areAMember && !areBlocked && (!isReadOnly || isThisChannel);
    }

    std::vector<QString> ContactListModel::getCheckedContacts() const
    {
        std::vector<QString> res;
        res.reserve(contacts_.size());

        for (const auto& ci : contacts_)
        {
            if (ci.is_checked())
                res.push_back(ci.get_aimid());
        }
        return res;
    }

    void ContactListModel::clearCheckedItems()
    {
        for (auto& item : contacts_)
            item.set_checked(false);
    }

    int ContactListModel::getCheckedItemsCount() const
    {
        return getCheckedContacts().size();
    }

    void ContactListModel::setCheckedItem(const QString& _aimid, bool _isChecked)
    {
        auto contact = getContactItem(_aimid);
        if (contact)
            contact->set_checked(_isChecked);
    }

    bool ContactListModel::isCheckedItem(const QString& _aimId) const
    {
        auto contact = getContactItem(_aimId);
        if (contact)
            return contact->is_checked();
        return false;
    }

    bool ContactListModel::isWithCheckedBox() const
    {
        return isWithCheckedBox_;
    }

    void ContactListModel::setIsWithCheckedBox(bool _isWithCheckedBox)
    {
        isWithCheckedBox_ = _isWithCheckedBox;
    }

    void ContactListModel::chatInfo(qint64, const std::shared_ptr<Data::ChatInfo>& _info, const int _requestMembersLimit)
    {
        updateChatInfo(*(_info.get()));
    }

    void ContactListModel::chatInfoFailed(qint64, core::group_chat_info_errors _errorCode, const QString& _aimId)
    {
        if (_errorCode == core::group_chat_info_errors::blocked)
        {
            if (std::none_of(youBlockedChats_.begin(), youBlockedChats_.end(), [&_aimId](const auto& x){ return x == _aimId; }))
                youBlockedChats_.push_back(_aimId);
        }
    }

    void ContactListModel::onIdInfo(qint64, const Data::IdInfo& _info)
    {
        if (_info.isChatInfo())
            chatsCache_[_info.sn_].stamp_ = _info.stamp_;
    }

    void ContactListModel::joinLiveChat(const QString& _stamp, bool _silent)
    {
        Log::write_network_log(su::concat("contact-list-debug",
            " joinLiveChat stamp=", _stamp.toStdString(), ", silent=", _silent ? "true" : "false",
            "\r\n"));

        if (_silent)
        {
            Ui::gui_coll_helper collection(Ui::GetDispatcher()->create_collection(), true);
            collection.set_value_as_qstring("stamp", _stamp);
            Ui::GetDispatcher()->post_message_to_core("livechat/join", collection.get());
        }
        else
        {
            Utils::openDialogOrProfile(_stamp, Utils::OpenDOPParam::stamp);
        }
    }

    void ContactListModel::next()
    {
        int current = 0;
        if (!currentAimId_.isEmpty())
        {
            auto idx = getOrderIndexByAimid(currentAimId_);
            if (idx != -1)
                current = idx;
        }

        ++current;

        for (auto iter = indexes_.cbegin(), end = indexes_.cend(); iter != end; ++iter)
        {
            if (iter.value() == current)
            {
                setCurrent(iter.key(), -1, true);
                break;
            }
        }
    }

    void ContactListModel::prev()
    {
        int current = 0;
        if (!currentAimId_.isEmpty())
        {
            auto idx = getOrderIndexByAimid(currentAimId_);
            if (idx != -1)
                current = idx;
        }

        --current;

        for (auto iter = indexes_.cbegin(), end = indexes_.cend(); iter != end; ++iter)
        {
            if (iter.value() == current)
            {
                setCurrent(iter.key(), -1, true);
                break;
            }
        }
    }

    int ContactListModel::getIndexByOrderedIndex(int _index) const
    {
        return sorted_index_cl_[_index];
    }

    int ContactListModel::getOrderIndexByAimid(const QString& _aimId) const
    {
        im_assert(indexes_.size() == int(contacts_.size()));

        if (_aimId.isEmpty())
            return -1;

        if (const auto item = indexes_.find(_aimId); item != indexes_.end())
            return item.value();
        return -1;
    }
}
