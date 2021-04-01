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
            decltype(timed_tasks_) tmp_list;
            {
                std::unique_lock<std::mutex> lock(mutex_);
                const auto res = condition_.wait_for(lock, tick_timeout);
                if (is_stop_)
                    return;

                if (res == std::cv_status::timeout)
                {
                    const auto current_time = std::chrono::steady_clock::now();

                    for (auto timer_task = timed_tasks_.begin(); timer_task != timed_tasks_.end();)
                    {
                        if (current_time - timer_task->last_execute_time_ >= timer_task->timeout_)
                        {
                            timer_task->last_execute_time_ = current_time;
                            g_core->execute_core_context(timer_task->function_);
                            if (timer_task->single_shot_)
                            {
                                auto splice_it = timer_task++;
                                tmp_list.splice(tmp_list.begin(), timed_tasks_, splice_it);
                                continue;
                            }
                        }
                        ++timer_task;
                    }
                }
            }
        }
    });
}


scheduler::~scheduler()
{
    {
        std::scoped_lock lock(mutex_);
        is_stop_ = true;
    }
    condition_.notify_all();
    thread_->join();
}

static uint32_t get_id()
{
    static std::atomic<uint32_t> id(0);
    return ++id;
}

uint32_t core::scheduler::push_timer(stacked_task _function, std::chrono::milliseconds _timeout, bool _single_shot)
{
    const auto currentId = get_id();

    scheduler_timer_task timer_task;
    timer_task.function_ = std::move(_function);
    timer_task.timeout_ = _timeout;
    timer_task.id_ = currentId;
    timer_task.last_execute_time_ = std::chrono::steady_clock::now();
    timer_task.single_shot_ = _single_shot;

    decltype(timed_tasks_) tmp_list;
    tmp_list.push_back(std::move(timer_task));

    {
        std::scoped_lock lock(mutex_);
        timed_tasks_.splice(timed_tasks_.end(), tmp_list, tmp_list.begin());
    }

    return currentId;
}

uint32_t core::scheduler::push_timer(stacked_task _function, std::chrono::milliseconds _timeout)
{
    return push_timer(_function, _timeout, false);
}

uint32_t core::scheduler::push_single_shot_timer(stacked_task _function, std::chrono::milliseconds _timeout)
{
    return push_timer(_function, _timeout, true);
}

void core::scheduler::stop_timer(uint32_t _id)
{
    decltype(timed_tasks_) tmp_list;
    {
        std::scoped_lock lock(mutex_);
        const auto end = timed_tasks_.end();
        if (const auto it = std::find_if(timed_tasks_.begin(), end, [_id](const auto& x) { return x.id_ == _id; }); it != end)
            tmp_list.splice(tmp_list.begin(), timed_tasks_, it);
    }
}