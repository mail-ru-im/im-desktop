#ifndef __SCHEDULER_H_
#define __SCHEDULER_H_

#pragma once

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
            std::function<void()> function_;
        };

        std::unique_ptr<std::thread> thread_;
        std::list<scheduler_timer_task> timed_tasks_;

        std::mutex mutex_;
        std::condition_variable condition_;
        bool is_stop_;

    public:

        uint32_t push_timer(std::function<void()> _function, std::chrono::milliseconds _timeout);
        void stop_timer(uint32_t _id);

        scheduler();
        virtual ~scheduler();
    };

}

#endif //__SCHEDULER_H_