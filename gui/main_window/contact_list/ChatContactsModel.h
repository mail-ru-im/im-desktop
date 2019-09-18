#pragma once

#include "CustomAbstractListModel.h"

namespace Data
{
    class ChatContacts;
}

namespace Logic
{
    class ChatContactsModel : public CustomAbstractListModel
    {
        Q_OBJECT

    private Q_SLOTS:
        void onChatContacts(const qint64 _seq, const std::shared_ptr<Data::ChatContacts>& _contacts);

    public:
        explicit ChatContactsModel(QObject* _parent = nullptr);
        ~ChatContactsModel();

        int itemsCount() const;

        QVariant data(const QModelIndex& _index, int _role) const override;
        int rowCount(const QModelIndex& _parent = QModelIndex()) const override;

        bool contains(const QString& _name) const override;

        void loadChatContacts(const QString& _chatId);

    private:
        QString chatId_;
        std::shared_ptr<Data::ChatContacts> contacts_;
    };
}

