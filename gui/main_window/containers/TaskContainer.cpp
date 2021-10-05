#include "stdafx.h"

#include "TaskContainer.h"
#include "core_dispatcher.h"
#include "utils/InterConnector.h"
#include "utils/features.h"
#include "../../previewer/toast.h"
#include "../contact_list/ContactListModel.h"
#include "ThreadSubContainer.h"

namespace Logic
{
    TaskContainer::TaskContainer(QObject* _parent)
        : QObject(_parent)
    {
        setObjectName(qsl("TaskContainer"));

        connect(Ui::GetDispatcher(), &Ui::core_dispatcher::updateTask, this, &TaskContainer::onTaskUpdate);
        connect(Ui::GetDispatcher(), &Ui::core_dispatcher::tasksLoaded, this, &TaskContainer::onTasksLoaded);

        Ui::GetDispatcher()->requestInitialTasks();
    }

    const Data::TaskData& TaskContainer::getTask(const QString& _taskId)
    {
        im_assert(!_taskId.isEmpty());
        if (_taskId.isEmpty())
        {
            static const auto e = Data::TaskData{};
            return e;
        }

        const auto it = tasks_.find(_taskId);
        if (it != tasks_.end())
        {
            Ui::GetDispatcher()->updateTaskLastUsedTime(_taskId);
            const auto& task = it->second;
            checkIfNeedToSubscribe(task);
            return task;
        }

        Data::TaskData empty;
        empty.id_ = _taskId;
        addTask(empty);
        return tasks_[_taskId];
    }

    bool TaskContainer::contains(const QString& _taskId)
    {
        const auto it = tasks_.find(_taskId);
        return it != tasks_.end();
    }

    void TaskContainer::addTask(const Data::TaskData& _task)
    {
        im_assert(!contains(_task.id_));
        tasks_[_task.id_] = _task;
        subscribeToTask(_task.id_);
    }

    void TaskContainer::subscribeToTask(const QString& _taskId)
    {
        im_assert(!_taskId.isEmpty());
        im_assert(contains(_taskId));
        Ui::GetDispatcher()->subscribeTask(_taskId);
        tasks_[_taskId].subscribed_ = true;
    }

    void TaskContainer::checkIfNeedToSubscribe(const Data::TaskData& _task)
    {
        if (!_task.subscribed_)
            subscribeToTask(_task.id_);
    }

    void TaskContainer::onTaskUpdate(const Data::TaskData& _task)
    {
        const auto& id = _task.id_;
        im_assert(!id.isEmpty());
        if (id.isEmpty())
            return;
        Logic::getContactListModel()->markAsThread(_task.params_.threadId_, {});
        if (!contains(id))
        {
            tasks_[id] = _task; // just add, subscribe only on task show in chat
            return;
        }
        auto& task = tasks_[id];
        const auto subscribed = task.subscribed_;
        if (const auto& newThreadId = _task.params_.threadId_; task.params_.threadId_.isEmpty() && !newThreadId.isEmpty())
            Logic::ThreadSubContainer::instance().subscribe(newThreadId);
        task = _task;
        task.subscribed_ = subscribed;
        Q_EMIT taskChanged(id, QPrivateSignal());
    }

    void TaskContainer::onTasksLoaded(const QVector<Data::TaskData>& _tasks)
    {
        for (const auto& task : _tasks)
            onTaskUpdate(task);
    }

    static std::unique_ptr<TaskContainer> g_TaskContainer;

    TaskContainer* GetTaskContainer()
    {
        Utils::ensureMainThread();
        if (!g_TaskContainer)
            g_TaskContainer = std::make_unique<TaskContainer>(nullptr);

        return g_TaskContainer.get();
    }

    void ResetTaskContainer()
    {
        Utils::ensureMainThread();
        g_TaskContainer.reset();
    }
}
