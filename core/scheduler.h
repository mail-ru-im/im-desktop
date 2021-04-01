#ifndef __SCHEDULER_H_
#define __SCHEDULER_H_

#pragma once

#include "core.h"
namespace core
{
    class async_executer;

    class scheduler
    {
        struct scheduler_timer_task
        {
            std::chrono::steady_clock::time_point last_execute_time_;
            uint32_t id_ = 0;
            std::chrono::milliseconds timeout_ = std::chrono::milliseconds(0);
            stacked_task function_;
            bool single_shot_ = false;
        };

        std::unique_ptr<std::thread> thread_;
        std::list<scheduler_timer_task> timed_tasks_;

        std::mutex mutex_;
        std::condition_variable condition_;
        bool is_stop_;

        uint32_t push_timer(stacked_task _function, std::chrono::milliseconds _timeout, bool _single_shot);

    public:

        uint32_t push_timer(stacked_task _function, std::chrono::milliseconds _timeout);
        uint32_t push_single_shot_timer(stacked_task _function, std::chrono::milliseconds _timeout);

        void stop_timer(uint32_t _id);

        scheduler();
        virtual ~scheduler();
    };

}

#endif //__SCHEDULER_H_