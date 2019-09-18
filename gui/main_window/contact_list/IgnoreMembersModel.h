#pragma once

#include "AbstractSearchModel.h"
#include "../../types/search_result.h"
#include "../../types/chat.h"

namespace Logic
{
    class IgnoreMembersModel : public AbstractSearchModel
    {
        Q_OBJECT

    private Q_SLOTS:
        void avatarLoaded(const QString& _aimId);

    public:
        IgnoreMembersModel(QObject* _parent);

        int rowCount(const QModelIndex& _parent = QModelIndex()) const override;
        QVariant data(const QModelIndex& _index, int _role) const override;
        bool contains(const QString& _aimId) const override;

        void setFocus() override;
        void setSearchPattern(const QString& _pattern) override;

        void updateMembers(const QVector<QString>& _ignoredList);
        int getMembersCount() const;

    private:
        mutable Data::SearchResultsV results_;

        std::vector<Data::ChatMemberInfo> members_;
    };

    void updateIgnoredModel(const QVector<QString>& _ignoredList);
    IgnoreMembersModel* getIgnoreModel();
}
