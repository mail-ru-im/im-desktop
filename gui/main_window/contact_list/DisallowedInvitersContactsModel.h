#pragma once

#include "CustomAbstractListModel.h"

namespace Logic
{
    class DisallowedInvitersContactsModel : public CustomAbstractListModel
    {
        Q_OBJECT

    Q_SIGNALS:
        void requestFailed(QPrivateSignal) const;

    public:
        explicit DisallowedInvitersContactsModel(QObject* _parent = nullptr);
        ~DisallowedInvitersContactsModel();

        QVariant data(const QModelIndex& _index, int _role) const override;
        int rowCount(const QModelIndex& _parent = QModelIndex()) const override;

        bool contains(const QString& _name) const override;

    private:
        void onContacts(const std::vector<QString>& _contacts, const QString& _cursor, int _error);
        void doRequest();

    private:
        std::vector<QString> contacts_;
        QString cursor_;
    };
}

