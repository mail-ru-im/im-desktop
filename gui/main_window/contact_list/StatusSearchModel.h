#pragma once

#include "AbstractSearchModel.h"
#include "../../statuses/Status.h"
#include "StatusListModel.h"

namespace Logic
{
    class StatusSearchModel : public AbstractSearchModel
    {
        Q_OBJECT
    public:
        StatusSearchModel(QObject* _parent);
        ~StatusSearchModel();

        int rowCount(const QModelIndex& _parent = QModelIndex()) const override;
        QVariant data(const QModelIndex& _index, int _role) const override;

        void setSearchPattern(const QString& _pattern) override;
        void updateTime(const QString& _status);

    private:
        void doSearch();

    private:
        StatusV results_;
        QTimer* timer_;
    };

    StatusSearchModel* getStatusSearchModel();
}
