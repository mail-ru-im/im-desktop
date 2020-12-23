#include "stdafx.h"
#include "DisallowedInvitersContactsModel.h"

#include "../../core_dispatcher.h"

namespace
{
    constexpr auto pageSize = 1000U;
}

namespace Logic
{
    DisallowedInvitersContactsModel::DisallowedInvitersContactsModel(QObject* _parent)
        : CustomAbstractListModel(_parent)
    {
        connect(Ui::GetDispatcher(), &Ui::core_dispatcher::inviterBlacklistGetResults, this, &DisallowedInvitersContactsModel::onContacts);
        doRequest();
    }

    DisallowedInvitersContactsModel::~DisallowedInvitersContactsModel() = default;

    int DisallowedInvitersContactsModel::rowCount(const QModelIndex&) const
    {
        return contacts_.size();
    }

    QVariant DisallowedInvitersContactsModel::data(const QModelIndex& _i, int _r) const
    {
        if (!_i.isValid() || (_r != Qt::DisplayRole && !Testing::isAccessibleRole(_r)))
            return QVariant();

        int cur = _i.row();
        if (cur >= rowCount(_i))
            return QVariant();

        return QVariant::fromValue(contacts_[cur]);
    }

    bool DisallowedInvitersContactsModel::contains(const QString& _aimId) const
    {
        return std::any_of(contacts_.cbegin(), contacts_.cend(), [&_aimId](const auto& _member) { return _member == _aimId; });
    }

    void DisallowedInvitersContactsModel::onContacts(const std::vector<QString>& _contacts, const QString& _cursor, int _error)
    {
        if (_error != 0)
        {
            Q_EMIT requestFailed(QPrivateSignal());
            return;
        }

        if (cursor_.isEmpty())
            contacts_ = _contacts;
        else
            contacts_.insert(contacts_.end(), _contacts.begin(), _contacts.end());
        cursor_ = _cursor;

        Q_EMIT dataChanged(index(0), index(rowCount()));

        if (!cursor_.isEmpty())
            doRequest();
    }

    void DisallowedInvitersContactsModel::doRequest()
    {
        Ui::gui_coll_helper collection(Ui::GetDispatcher()->create_collection(), true);
        collection.set_value_as_qstring("cursor", cursor_);
        collection.set_value_as_uint("page_size", pageSize);
        Ui::GetDispatcher()->post_message_to_core("group/invitebl/cl", collection.get());
    }
}
