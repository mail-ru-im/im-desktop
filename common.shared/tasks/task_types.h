#pragma once

namespace core
{
namespace tasks
{
    enum class status
    {
        unknown,
        new_task,
        in_progress,
        ready,
        rejected,
        closed
    };

    status string_to_status(std::string_view _status);
    std::string_view status_to_string(status _status);

    enum class task_user
    {
        reporter,
        assignee,
        unknown
    };

    std::vector<status> offline_available_statuses(status _status, task_user _task_user);
}
}
