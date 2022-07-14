#include "stdafx.h"
#include "async_task.h"
#include "core.h"
#include "../common.shared/string_utils.h"

using namespace core;

//////////////////////////////////////////////////////////////////////////
// async_task
//////////////////////////////////////////////////////////////////////////
async_task::async_task() = default;

async_task::~async_task() = default;




//////////////////////////////////////////////////////////////////////////
// async_executer
//////////////////////////////////////////////////////////////////////////
async_executer::async_executer(std::string_view _name, size_t _count, bool _task_trace)
    : threadpool(su::concat("ae_", _name), _count, []()
{
    g_core->on_thread_finish();
}, _task_trace)
{

}

async_executer::~async_executer() = default;

std::shared_ptr<async_task_handlers> async_executer::run_async_task(std::shared_ptr<async_task> task, std::string_view _task_name, std::function<bool()> _cancel)
{
    return run_async_function([t = std::move(task)]() { return t->execute(); }, _task_name, std::move(_cancel));
}

std::shared_ptr<async_task_handlers> core::async_executer::run_async_function(std::function<int32_t()> func, std::string_view _task_name, std::function<bool()> _cancel)
{
    auto handler = std::make_shared<async_task_handlers>();

    push_back({ [f = std::move(func), handler]
    {
        g_core->execute_core_context({ [handler, result = f()]
        {
            if (handler->on_result_)
                handler->on_result_(result);
        }});
    } }, -1, _task_name, std::move(_cancel));

    return handler;
}
