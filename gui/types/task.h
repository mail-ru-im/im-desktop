#pragma once

#include "../../common.shared/tasks/task_types.h"
#include "thread.h"

namespace core
{
    struct icollection;
}

namespace Data
{
    struct TaskChange;

    struct TaskParams
    {
        void serialize(core::icollection* _collection) const;
        void unserialize(core::icollection* _collection);

        QString title_;
        QString assignee_;
        QDateTime date_;
        core::tasks::status status_ = core::tasks::status::unknown;
        QString creator_;
        QString threadId_;
        QDateTime createTime_;
        QDateTime lastChange_;
        std::vector<core::tasks::status> allowedStatuses_;
    };

    struct TaskData
    {
        explicit TaskData();

        void serialize(core::icollection* _collection) const;
        void unserialize(core::icollection* _collection);

        bool needToLoadData() const;
        bool hasId() const;
        bool hasIdAndData() const;
        bool hasAllowedStatuses() const;
        bool isEmpty() const;
        bool needToSend() const;
        bool didReceiveFetch() const;

        QString id_;
        TaskParams params_;
        std::shared_ptr<Data::ThreadUpdate> threadUpdate_;

        bool subscribed_ = false;

        static QString statusDescription(core::tasks::status _status);
    };
    using Task = std::optional<TaskData>;

    struct TaskChange
    {
        TaskChange() = default;
        TaskChange(const TaskData& _initialTask, const TaskData& _changedTask);
        TaskChange(const QString& _id, core::tasks::status _newStatus);
        void serialize(core::icollection* _collection) const;
        void unserialize(core::icollection* _collection);

        QString id_;
        std::optional<QString> title_;
        std::optional<QString> assignee_;
        std::optional<QDateTime> date_;
        std::optional<core::tasks::status> status_;
    };

}
