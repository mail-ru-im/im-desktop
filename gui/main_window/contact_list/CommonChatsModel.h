#pragma once

#include "../contact_list/CustomAbstractListModel.h"
#include "../contact_list/AbstractSearchModel.h"
#include "../../types/chat.h"
#include "../../types/search_result.h"

namespace Logic
{
    class CommonChatsModel : public CustomAbstractListModel
    {
        Q_OBJECT
    public:
        CommonChatsModel(QWidget* _parent);

        int rowCount(const QModelIndex &parent = QModelIndex()) const override;
        QVariant data(const QModelIndex &index, int role) const override;

        void load(const QString& _sn);
        const std::vector<Data::CommonChatInfo>& getChats() const;

    private Q_SLOTS:
        void onCommonChatsGet(const qint64 _seq, const std::vector<Data::CommonChatInfo>& _chats);
        void avatarChanged(const QString& _aimid);

    private:
        std::vector<Data::CommonChatInfo> chats_;
        qint64 seq_;
    };

    class CommonChatsSearchModel : public AbstractSearchModel
    {
        Q_OBJECT
    public:
        CommonChatsSearchModel(QWidget* _parent, CommonChatsModel* _model);

        void setSearchPattern(const QString&) override;
        int rowCount(const QModelIndex& _parent = QModelIndex()) const override;
        QVariant data(const QModelIndex& _index, int _role) const override;

    private:
        Data::SearchResultsV results_;
        CommonChatsModel* model_;
        QString currentPattern_;
    };
}
