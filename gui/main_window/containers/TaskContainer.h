#pragma once

#include "../../types/task.h"

namespace Logic
{
    class TaskContainer : public QObject
    {
        Q_OBJECT

    Q_SIGNALS:
        void taskChanged(const QString& _taskId, QPrivateSignal);

    public:
        TaskContainer(QObject* _parent = nullptr);
        ~TaskContainer() = default;

        const Data::TaskData& getTask(const QString& _taskId);
        bool contains(const QString& _taskId);

    private:
        void addTask(const Data::TaskData& _task);
        void subscribeToTask(const QString& _taskId);
        void checkIfNeedToSubscribe(const Data::TaskData& _task);

    private Q_SLOTS:
        void onTaskUpdate(const Data::TaskData& _task);
        void onTasksLoaded(const QVector<Data::TaskData>& _tasks);

    private:
        std::unordered_map<QString, Data::TaskData> tasks_;
    };

    TaskContainer* GetTaskContainer();
    void ResetTaskContainer();
}
