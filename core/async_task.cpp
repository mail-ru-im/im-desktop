#include "stdafx.h"
#include "async_task.h"
#include "core.h"

using namespace core;

//////////////////////////////////////////////////////////////////////////
// async_task
//////////////////////////////////////////////////////////////////////////
async_task::async_task()
{
}


async_task::~async_task()
{
}




//////////////////////////////////////////////////////////////////////////
// async_executer
//////////////////////////////////////////////////////////////////////////
async_executer::async_executer(const std::string_view _name, size_t _count)
    : threadpool("ae_" + std::string(_name), _count, []()
{
    g_core->on_thread_finish();
})
{

}

async_executer::~async_executer()
{

}

std::shared_ptr<async_task_handlers> async_executer::run_async_task(std::shared_ptr<async_task> task)
{
    return run_async_function([t = std::move(task)]() { return t->execute(); });
}

std::shared_ptr<async_task_handlers> core::async_executer::run_async_function(std::function<int32_t()> func)
{
    auto handler = std::make_shared<async_task_handlers>();

    push_back([f = std::move(func), handler]
    {
        auto result = f();

        g_core->execute_core_context([handler, result]
        {
            if (handler->on_result_)
                handler->on_result_(result);
        });
    });

    return handler;
}
