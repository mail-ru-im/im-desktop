#pragma once

#include "semaphore.h"

namespace core
{
    namespace tools
    {
        typedef std::unique_ptr<boost::stacktrace::stacktrace> core_stacktrace;

        class task
        {
        public:
            task();

            task(std::function<void()> _action, const int64_t _id);

            task(task&&) = default;
            task& operator=(task&&) = default;

            task(const task&) = default;
            task& operator=(const task&) = default;

            void execute();

            int64_t get_id() const noexcept;

            operator bool() const noexcept;

            const core_stacktrace& get_stack_trace() const;

        private:

            std::function<void()> action_;

            int64_t id_;

            core_stacktrace st_;
        };

        typedef std::function<void(const std::chrono::milliseconds, const core_stacktrace&)> finish_action;

        class threadpool : boost::noncopyable
        {
            std::thread::id creator_thread_id_;

            finish_action on_task_finish_;

        public:

            typedef std::function<void()> task_action;

            explicit threadpool(
                const std::string_view _name,
                const size_t _count,
                std::function<void()> _on_thread_exit = std::function<void()>());

            virtual ~threadpool();

            bool push_back(task_action _task, int64_t _id = -1);
            bool push_front(task_action _task, int64_t _id = -1);

            void raise_task(int64_t _id);

            const std::vector<std::thread::id>& get_threads_ids() const;

        protected:

            std::vector<std::thread> threads_;
            std::vector<std::thread::id> threads_ids_;
            std::mutex queue_mutex_;
            std::condition_variable condition_;
            std::deque<task> tasks_;

            std::atomic<bool> stop_;

            bool run_task_impl();
            bool run_task();

            void set_task_finish_callback(finish_action _on_task_finish);
        };
    }

}
