#pragma once

#include "AbstractSearchModel.h"
#include "../../types/message.h"

namespace Logic
{
    class CallsSearchModel : public AbstractSearchModel
    {
        Q_OBJECT
    public:
        CallsSearchModel(QObject* _parent);
        ~CallsSearchModel();

        int rowCount(const QModelIndex& _parent = QModelIndex()) const override;
        QVariant data(const QModelIndex& _index, int _role) const override;

        void setSearchPattern(const QString& _pattern) override;

    private Q_SLOTS:
        void doSearch();

    private:
        Data::CallInfoVec results_;
        QString lastSearchPattern_;
        QTimer* timer_;
    };
}
