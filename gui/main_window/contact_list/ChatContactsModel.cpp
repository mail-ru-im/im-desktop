#include "stdafx.h"
#include "ChatContactsModel.h"

#include "../../core_dispatcher.h"
#include "../../utils/gui_coll_helper.h"

namespace
{
    // page size for method getChatContacts (use it without pagination)
    constexpr auto ContactsPageSize = 1000U;
}

namespace Logic
{
    void ChatContactsModel::onChatContacts(const qint64 _seq, const std::shared_ptr<Data::ChatContacts>& _contacts)
    {
        if (chatId_ != _contacts->AimId_)
            return;

        contacts_ = _contacts;

        emit dataChanged(index(0), index(rowCount()));
    }

    ChatContactsModel::ChatContactsModel(QObject* _parent)
        : CustomAbstractListModel(_parent)
        , contacts_(nullptr)
    {
        connect(Ui::GetDispatcher(), &Ui::core_dispatcher::chatContacts, this, &ChatContactsModel::onChatContacts);
    }

    ChatContactsModel::~ChatContactsModel()
    {
    }

    int ChatContactsModel::itemsCount() const
    {
        return contacts_ ? static_cast<int>(contacts_->Members_.size()) : 0;
    }

    int ChatContactsModel::rowCount(const QModelIndex&) const
    {
        return itemsCount();
    }

    QVariant ChatContactsModel::data(const QModelIndex& _i, int _r) const
    {
        if (!_i.isValid() || (_r != Qt::DisplayRole && !Testing::isAccessibleRole(_r)))
            return QVariant();

        int cur = _i.row();
        if (cur >= rowCount(_i))
            return QVariant();

        return QVariant::fromValue(contacts_->Members_[cur]);
    }

    bool ChatContactsModel::contains(const QString& _aimId) const
    {
        return contacts_ ? std::any_of(contacts_->Members_.cbegin(), contacts_->Members_.cend(), [&_aimId](const auto& _member) { return _member == _aimId; }) : false;
    }

    void ChatContactsModel::loadChatContacts(const QString& _chatId)
    {
        chatId_ = _chatId;

        Ui::gui_coll_helper collection(Ui::GetDispatcher()->create_collection(), true);
        collection.set_value_as_qstring("aimid", chatId_);
        collection.set_value_as_uint("page_size", ContactsPageSize);
        Ui::GetDispatcher()->post_message_to_core("chats/contacts/get", collection.get());
    }
}
