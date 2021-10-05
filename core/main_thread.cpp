#include "stdafx.h"
#include "main_thread.h"
#include "core.h"
#include "network_log.h"

using namespace core;



main_thread::main_thread()
    : threadpool("core main", 1 )
{
    set_task_finish_callback([](std::chrono::milliseconds _task_execute_time, const stack_vec& _st, std::string_view _name)
    {
        if (_task_execute_time > std::chrono::milliseconds(200) && !_st.empty())
        {
            std::stringstream ss;
            ss << "ATTENTION! Core locked, ";
            if (!_name.empty())
                ss << "task name: " << _name << ", ";
            ss << _task_execute_time.count() << " milliseconds\r\n\r\n";

            for (const auto& s : _st)
            {
                ss << *s;
                ss << "\r\n - - - - - \r\n";
            }

            g_core->write_string_to_network_log(ss.str());
        }
    });
}


main_thread::~main_thread() = default;

void main_thread::execute_core_context(stacked_task task)
{
    push_back(std::move(task));
}

std::thread::id main_thread::get_core_thread_id() const
{
    if (const auto& thread_ids = get_threads_ids(); thread_ids.size() == 1)
    {
        return thread_ids[0];
    }
    else
    {
        im_assert(thread_ids.size() == 1);
        return std::thread::id(); // nobody
    }
}
