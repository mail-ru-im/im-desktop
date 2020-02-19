#include "stdafx.h"
#include "main_thread.h"
#include "core.h"
#include "network_log.h"

using namespace core;



main_thread::main_thread()
    : threadpool("core main", 1 )
{
    set_task_finish_callback([](const std::chrono::milliseconds _task_execute_time, const tools::core_stacktrace& _st)
    {
        if (_task_execute_time > std::chrono::milliseconds(200) && _st)
        {
            std::stringstream ss;

            ss << "ATTENTION! Core locked, " << _task_execute_time.count() << " milliseconds\r\n\r\n";
            ss << *_st;

            g_core->write_string_to_network_log(ss.str());

 //           assert(false);
        }
    });
}


main_thread::~main_thread()
{
}

void main_thread::execute_core_context(std::function<void()> task)
{
    push_back(std::move(task));
}

std::thread::id main_thread::get_core_thread_id() const
{
    assert(get_threads_ids().size() == 1);

    if (const auto& thread_ids = get_threads_ids(); thread_ids.size() == 1)
        return thread_ids[0];

    return std::thread::id(); // nobody
}
