#pragma once

#include "SearchModel.h"

namespace Logic
{
    class TaskAssigneeModel : public SearchModel
    {
        Q_OBJECT
    public:
        TaskAssigneeModel(QObject* _parent);
        ~TaskAssigneeModel();

        void setSelectedContact(const QString& _aimId);
        static QString noAssigneeAimId();

    protected:
        void modifyResultsBeforeEmit(Data::SearchResultsV& _results) override;

    private:
        QString selectedContact_;
    };
}
