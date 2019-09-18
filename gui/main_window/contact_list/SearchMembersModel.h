#pragma once

#include "ContactItem.h"
#include "ContactList.h"
#include "AbstractSearchModel.h"
#include "../../types/contact.h"
#include "../../types/search_result.h"

namespace Logic
{
    //!! todo: temporarily for voip
    class SearchMembersModel : public AbstractSearchModel
    {
        Q_OBJECT

    private Q_SLOTS:
        void avatarLoaded(const QString& _aimId);

    public:
        SearchMembersModel(QObject* _parent);

        void setSearchPattern(const QString&) override;
        int rowCount(const QModelIndex& _parent = QModelIndex()) const override;
        QVariant data(const QModelIndex& _index, int _role) const override;
        void setFocus() override;
        void setChatMembersModel(CustomAbstractListModel* _membersModel);

    private:
        mutable Data::SearchResultsV results_;
        bool searchRequested_;
        CustomAbstractListModel* chatMembersModel_;
    };
}
