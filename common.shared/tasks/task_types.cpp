#include "stdafx.h"
#include "task_types.h"

namespace
{
    using core::tasks::status;

    std::vector<status> reporter_statuses(status _status)
    {
        std::vector<status> result = { status::new_task, status::in_progress, status::ready, status::rejected, status::closed };
        result.erase(std::remove(result.begin(), result.end(), _status), result.end());
        return result;
    }

    std::vector<status> assignee_statuses(status _status)
    {
        switch (_status)
        {
        case status::new_task:
            return { status::in_progress, status::rejected };
        case status::in_progress:
            return { status::new_task, status::ready, status::rejected };
        case status::ready:
        case status::rejected:
            return { status::new_task, status::in_progress };
        case status::closed:
            return { status::new_task };
        default:
            break;
        }
        return {};
    }
}

namespace core::tasks
{
    std::string_view status_to_string(status _status)
    {
        switch (_status)
        {
        case status::new_task:
            return "new";
        case status::in_progress:
            return "in_progress";
        case status::ready:
            return "ready";
        case status::rejected:
            return "rejected";
        case status::closed:
            return "closed";
        default:
            break;
        }
        im_assert(!"Unknown task status");
        return "new";
    }

    status string_to_status(std::string_view _status)
    {
        if (_status == "new")
            return status::new_task;
        if (_status == "in_progress")
            return status::in_progress;
        if (_status == "ready")
            return status::ready;
        if (_status == "rejected")
            return status::rejected;
        if (_status == "closed")
            return status::closed;
        im_assert(!"Unknown task status string");
        return status::new_task;
    }

    std::vector<status> offline_available_statuses(status _status, task_user _task_user)
    {
        switch (_task_user)
        {
        case task_user::reporter:
            return reporter_statuses(_status);
        case task_user::assignee:
            return assignee_statuses(_status);
        default:
            break;
        }
        return {};
    }
}
