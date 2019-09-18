#include "stdafx.h"
#include "ContactListModel.h"

#include "contact_profile.h"
#include "RecentsModel.h"
#include "UnknownsModel.h"
#include "../friendly/FriendlyContainer.h"
#include "../MainWindow.h"
#include "../history_control/HistoryControlPage.h"
#include "../../core_dispatcher.h"
#include "../../gui_settings.h"
#include "../../my_info.h"
#include "../../utils/gui_coll_helper.h"
#include "../../utils/InterConnector.h"
#include "../../utils/utils.h"
#include "../../cache/avatars/AvatarStorage.h"
#include "Common.h"

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
        connect(Ui::GetDispatcher(), &Ui::core_dispatcher::dialogGalleryState, this, &ContactListModel::dialogGalleryState);

        connect(GetAvatarStorage(), &Logic::AvatarStorage::avatarChanged,   this, &ContactListModel::avatarLoaded);

        connect(&Utils::InterConnector::instance(), &Utils::InterConnector::clSortChanged, this, &ContactListModel::forceSort);

        connect(sortTimer_, &QTimer::timeout, this, Utils::QOverload<>::of(&ContactListModel::sort));
        sortTimer_->start(sort_timer_timeout.count());
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
            return cont->AimId_;

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
                emit dataChanged(index, index);
            }
        }
        else
        {
            if (_contact->isDeleted_)
            {
                deletedContacts_.insert(_contact->AimId_);
                return (int)contacts_.size();
            }

            assert(!contains(_contact->AimId_));
            assert(std::none_of(contacts_.begin(), contacts_.end(), [&a = _contact->AimId_](const auto& _c){ return a == _c.get_aimid(); }));

            contacts_.emplace_back(_contact);

            const auto newNdx = (int)contacts_.size() - 1;
            sorted_index_cl_.emplace_back(newNdx);
            indexes_.insert(_contact->AimId_, newNdx);

            visible_indexes_.emplace_back(newNdx);
            emitChanged(newNdx, newNdx);
            emit contactRename(_contact->AimId_);
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
            assert(std::count_if(contacts_.begin(), contacts_.end(), [&a = contacts_[order_index].get_aimid()](const auto& _c){ return a == _c.get_aimid(); }) == 1);
            assert(!indexes_.contains(contacts_[order_index].get_aimid()));

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
        const int size = (int)contacts_.size();
        beginInsertRows(QModelIndex(), size, _cl->size() + size);

        const bool needSyncSort = contacts_.empty();

        const bool isDeletedType = _type == ql1s("deleted");

        QVector<QString> removedContacts;

        for (auto it = _cl->keyBegin(), end = _cl->keyEnd(); it != end; ++it)
        {
            const auto& iter = *it;
            if (!isDeletedType)
            {
                if (iter->UserType_ != ql1s("sms"))
                {
                    addItem(iter, removedContacts, false);
                    if (!iter->isDeleted_)
                    {
                        if (iter->IsLiveChat_)
                            emit liveChatJoined(iter->AimId_);
                        emit contactChanged(iter->AimId_);
                        Logic::GetFriendlyContainer()->setContactLastSeen(iter->AimId_, iter->LastSeenCore_);
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

        Logic::updatePlaceholders({Logic::Placeholder::Contacts, Logic::Placeholder::Dialog});
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
        auto item = getContactItem(_aimId);
        return item && item->is_channel();
    }

    Data::DialogGalleryState ContactListModel::getGalleryState(const QString& _aimid) const
    {
        if (galleryStates_.contains(_aimid))
            return galleryStates_[_aimid];

        return Data::DialogGalleryState();
    }

    QString ContactListModel::getChatStamp(const QString& _aimid) const
    {
        return chatStamps_[_aimid];
    }

    bool ContactListModel::isReadonly(const QString& _aimId) const // no write access to this chat, for channel check use isChannel
    {
        auto item = getContactItem(_aimId);
        return item && item->is_readonly();
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
            emit liveChatRemoved(_aimId);

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

    void ContactListModel::avatarLoaded(const QString& _aimId)
    {
        auto idx = getOrderIndexByAimid(_aimId);
        if (idx != -1)
        {
            QModelIndex mi(index(idx));
            emit dataChanged(mi, mi);
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
            emit contactChanged(_presence->AimId_);
            Logic::GetFriendlyContainer()->setContactLastSeen(_presence->AimId_, _presence->LastSeenCore_);
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
                emit dataChanged(index, index);
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

    void ContactListModel::setCurrent(const QString& _aimId, qint64 id, bool _sel, std::function<void(Ui::HistoryControlPage*)> _gotPageCallback, qint64 quote_id)
    {
        if (_gotPageCallback)
        {
            auto page = Utils::InterConnector::instance().getMainWindow()->getHistoryPage(_aimId);
            if (page)
            {
                _gotPageCallback(page);
            }
            else
            {
                gotPageCallback_ = _gotPageCallback;
            }
        }

        const auto prev = currentAimId_;
        currentAimId_ = _aimId;
        emit selectedContactChanged(currentAimId_, prev);

        if (!currentAimId_.isEmpty() && _sel)
            emit select(currentAimId_, id, quote_id);
    }

    void ContactListModel::setCurrentCallbackHappened(Ui::HistoryControlPage* _page)
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
        const auto force_cyrillic = Ui::get_gui_settings()->get_value<QString>(settings_language, QString()) == qsl("ru");

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
                    assert(contacts_[_list[i]].get_outgoing_msg_count() <= contacts_[_list[i - 1]].get_outgoing_msg_count());

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

    const QString& ContactListModel::selectedContact() const
    {
        return currentAimId_;
    }

    QString ContactListModel::selectedContactName() const
    {
        return Logic::GetFriendlyContainer()->getFriendly(selectedContact());
    }

    void ContactListModel::add(const QString& _aimId, const QString& _friendly)
    {
        if (deletedContacts_.contains(_aimId))
            return;

        auto addContact = std::make_shared<Data::Contact>();
        addContact->AimId_ = _aimId;
        addContact->State_ = QString();
        addContact->Friendly_ = _friendly;
        addContact->Is_chat_ = Utils::isChat(_aimId);
        QVector<QString> removed;
        addItem(addContact, removed);
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

    QString ContactListModel::getState(const QString& _aimId) const
    {
        auto ci = getContactItem(_aimId);
        if (ci)
            return ci->Get()->GetState();

        return QString();
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

    Ui::Input ContactListModel::getInputText(const QString& _aimId) const
    {
        auto ci = getContactItem(_aimId);
        if (!ci)
            return Ui::Input();

        return Ui::Input(ci->get_input_text(), ci->get_input_pos());
    }

    void ContactListModel::setInputText(const QString& _aimId, const Ui::Input& _input)
    {
        auto ci = getContactItem(_aimId);
        if (!ci)
            return;

        ci->set_input_text(_input.text_);
        ci->set_input_pos(_input.pos_);
    }

    void ContactListModel::getContactProfile(const QString& _aimId, std::function<void(profile_ptr, int32_t error)> _callBack)
    {
        profile_ptr profile;

        if (!_aimId.isEmpty())
        {
            auto ci = getContactItem(_aimId);
            if (ci)
                profile = ci->getContactProfile();
        }

        if (profile)
        {
            emit profile_loaded(profile);

            _callBack(profile, 0);
        }

        if (!profile)
        {
            Ui::gui_coll_helper collection(Ui::GetDispatcher()->create_collection(), true);

            collection.set_value_as_qstring("aimid", _aimId);

            Ui::GetDispatcher()->post_message_to_core("contacts/profile/get", collection.get(), this, [this, _callBack](core::icollection* _coll)
            {
                Ui::gui_coll_helper coll(_coll, false);

                const QString aimid = QString::fromUtf8(coll.get_value_as_string("aimid"));
                int32_t err = coll.get_value_as_int("error");

                if (err == 0)
                {
                    Ui::gui_coll_helper coll_profile(coll.get_value_as_collection("profile"), false);

                    auto profile = std::make_shared<contact_profile>();

                    if (profile->unserialize(coll_profile))
                    {
                        if (!aimid.isEmpty())
                        {
                            ContactItem* ci = getContactItem(aimid);
                            if (ci)
                                ci->set_contact_profile(profile);
                        }

                        emit profile_loaded(profile);

                        _callBack(profile, 0);
                    }
                }
                else
                {
                    _callBack(nullptr, 0);
                }
            });
        }
    }

    void ContactListModel::addContactToCL(const QString& _aimId, std::function<void(bool)> _callBack)
    {
        Ui::gui_coll_helper collection(Ui::GetDispatcher()->create_collection(), true);
        collection.set_value_as_qstring("contact", _aimId);
        collection.set_value_as_qstring("group", qsl("General"));
        collection.set_value_as_qstring("message", QT_TRANSLATE_NOOP("contact_list","Hello. Please add me to your contact list"));

        Ui::GetDispatcher()->post_message_to_core("contacts/add", collection.get(), this, [this, _aimId, _callBack](core::icollection* _coll)
        {
            Ui::gui_coll_helper coll(_coll, false);

            const QString contact = QString::fromUtf8(coll.get_value_as_string("contact"));

            int32_t err = coll.get_value_as_int("error");

            _callBack(err == 0);

            emit contact_added(contact, (err == 0));
            emit needSwitchToRecents();
            emit contactAdd(_aimId);
        });

    }

    void ContactListModel::renameContact(const QString& _aimId, const QString& _friendly, std::function<void(bool)> _callBack)
    {
        Ui::gui_coll_helper collection(Ui::GetDispatcher()->create_collection(), true);

        collection.set_value_as_qstring("contact", _aimId);
        collection.set_value_as_qstring("friendly", _friendly);

        Ui::GetDispatcher()->post_message_to_core("contacts/rename", collection.get(), this, [this, _aimId, _callBack = std::move(_callBack)](core::icollection* _coll){
            Ui::gui_coll_helper coll(_coll, false);
            auto error = coll.get_value_as_int("error", 0);
            if (_callBack)
                _callBack(error == 0);
            emit contactRename(_aimId);
        });
    }

    void ContactListModel::renameChat(const QString& _aimId, const QString& _friendly)
    {
        Ui::gui_coll_helper collection(Ui::GetDispatcher()->create_collection(), true);

        collection.set_value_as_qstring("aimid", _aimId);
        collection.set_value_as_qstring("m_chat_name", _friendly);
        Ui::GetDispatcher()->post_message_to_core("modify_chat", collection.get());
    }

    void ContactListModel::removeContactFromCL(const QString& _aimId)
    {
        Ui::gui_coll_helper collection(Ui::GetDispatcher()->create_collection(), true);
        collection.set_value_as_qstring("contact", _aimId);
        Ui::GetDispatcher()->post_message_to_core("contacts/remove", collection.get());
    }

    void ContactListModel::removeContactsFromModel(const QVector<QString>& _vcontacts)
    {
        if (_vcontacts.isEmpty())
            return;

        for (const auto& _aimid : _vcontacts)
            innerRemoveContact(_aimid);

        rebuildIndex();
        forceSort();

        for (const auto& _aimid : _vcontacts)
            emit contact_removed(_aimid);

        Logic::updatePlaceholders({Logic::Placeholder::Contacts});
    }

    void ContactListModel::ignoreContact(const QString& _aimId, bool _ignore)
    {
        Ui::gui_coll_helper collection(Ui::GetDispatcher()->create_collection(), true);
        collection.set_value_as_qstring("contact", _aimId);
        collection.set_value_as_bool("ignore", _ignore);
        Ui::GetDispatcher()->post_message_to_core("contacts/ignore", collection.get());

        if (_ignore)
        {
            emit ignore_contact(_aimId);
            Ui::GetDispatcher()->getVoipController().setDecline(_aimId.toStdString().c_str(), true);
        }
    }

    bool ContactListModel::ignoreContactWithConfirm(const QString& _aimId)
    {
        auto confirm = Utils::GetConfirmationWithTwoButtons(
            QT_TRANSLATE_NOOP("popup_window", "CANCEL"),
            QT_TRANSLATE_NOOP("popup_window", "YES"),
            QT_TRANSLATE_NOOP("popup_window", "Are you sure you want to move contact to ignore list?"),
            Logic::GetFriendlyContainer()->getFriendly(_aimId),
            nullptr);

        if (confirm)
        {
            ignoreContact(_aimId, true);
        }
        return confirm;
    }

    bool ContactListModel::isYouAdmin(const QString& _aimId) const
    {
        auto cont = getContactItem(_aimId);
        if (cont)
        {
            const auto role = cont->get_chat_role();
            return role == ql1s("admin") || role == ql1s("moder");
        }

        return false;
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
            cont->set_chat_role(_role);
            emit youRoleChanged(_aimId);
        }
    }

    void ContactListModel::getIgnoreList()
    {
        Ui::gui_coll_helper collection(Ui::GetDispatcher()->create_collection(), true);
        Ui::GetDispatcher()->post_message_to_core("contacts/get_ignore", collection.get());
    }

    std::vector<QString> ContactListModel::GetCheckedContacts() const
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

    void ContactListModel::clearChecked()
    {
        for (auto& item : contacts_)
            item.set_checked(false);
    }

    void ContactListModel::setChecked(const QString& _aimid, bool _isChecked)
    {
        auto contact = getContactItem(_aimid);
        if (contact)
            contact->set_checked(_isChecked);
    }

    bool ContactListModel::getIsChecked(const QString& _aimId) const
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
        if (!_info->Stamp_.isEmpty())
            chatStamps_[_info->AimId_] = _info->Stamp_;

        if (auto contact = getContactItem(_info->AimId_))
        {
            QString role = _info->YourRole_;
            if (_info->YouPending_)
                role = qsl("pending");
            else if (!_info->YouMember_ || role.isEmpty())
                role = qsl("notamember");

            contact->set_default_role(_info->DefaultRole_);

            contact->set_stamp(_info->Stamp_);
            if (contact->get_chat_role() != role)
            {
                contact->set_chat_role(role);
                emit youRoleChanged(_info->AimId_);
            }
        }
    }

    void ContactListModel::joinLiveChat(const QString& _stamp, bool _silent)
    {
        if (_silent)
        {
            getContactProfile(Ui::MyInfo()->aimId(), [_stamp](profile_ptr profile, int32_t)
            {
                Ui::gui_coll_helper collection(Ui::GetDispatcher()->create_collection(), true);
                collection.set_value_as_qstring("stamp", _stamp);
                Ui::GetDispatcher()->post_message_to_core("livechat/join", collection.get());
            });
        }
        else
        {
            emit Utils::InterConnector::instance().needJoinLiveChatByStamp(_stamp);
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
        assert(indexes_.size() == int(contacts_.size()));

        if (_aimId.isEmpty())
            return -1;

        if (const auto item = indexes_.find(_aimId); item != indexes_.end())
            return item.value();
        return -1;
    }
}
