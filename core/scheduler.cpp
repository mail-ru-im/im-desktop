#include "stdafx.h"
#include "scheduler.h"
#include "async_task.h"
#include "core.h"
#include "utils.h"

using namespace core;


scheduler::scheduler()
    : is_stop_(false)
{
    thread_ = std::make_unique<std::thread>([this]
    {
        utils::set_this_thread_name("scheduler");

        constexpr auto tick_timeout = std::chrono::milliseconds(100);

        for(;;)
        {
            std::unique_lock<std::mutex> lock(mutex_);
            const auto res = condition_.wait_for(lock, tick_timeout);
            if (is_stop_)
                return;

            if (res == std::cv_status::timeout)
            {
                const auto current_time = std::chrono::system_clock::now();

                for (const auto& timer_task : timed_tasks_)
                {
                    if (current_time - timer_task->last_execute_time_ >= timer_task->timeout_)
                    {
                        timer_task->last_execute_time_ = current_time;
                        g_core->execute_core_context(timer_task->function_);
                    }
                }
            }
        }
    });
}


scheduler::~scheduler()
{
    is_stop_ = true;
    condition_.notify_all();
    thread_->join();
}

static uint32_t get_id()
{
    static std::atomic<uint32_t> id(0);
    return ++id;
}

uint32_t core::scheduler::push_timer(std::function<void()> _function, std::chrono::milliseconds _timeout)
{
    const auto currentId = get_id();

    auto timer_task = std::make_shared<scheduler_timer_task>();
    timer_task->function_ = std::move(_function);
    timer_task->timeout_ = _timeout;
    timer_task->id_ = currentId;

    decltype(timed_tasks_) tmp_list;
    tmp_list.push_back(std::move(timer_task));

    {
        std::lock_guard<std::mutex> lock(mutex_);
        tmp_list.front()->last_execute_time_ = std::chrono::system_clock::now();
        timed_tasks_.splice(timed_tasks_.end(), tmp_list, tmp_list.begin());
    }

    return currentId;
}

void core::scheduler::stop_timer(uint32_t _id)
{
    decltype(timed_tasks_) tmp_list;
    {
        std::lock_guard<std::mutex> lock(mutex_);
        const auto end = timed_tasks_.end();
        const auto it = std::find_if(timed_tasks_.begin(), end, [_id](const auto& x) { return x->id_ == _id; });
        if (it != end)
            tmp_list.splice(tmp_list.begin(), timed_tasks_, it);
    }
}