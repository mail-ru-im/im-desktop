#pragma once

#include "../core.h"
namespace core
{
    namespace tools
    {
        class task
        {
        public:
            task();

            task(stacked_task _action, const int64_t _id, std::string_view _name, std::chrono::steady_clock::time_point _time_stamp, std::function<bool()> _cancel);

            task(task&&) = default;
            task& operator=(task&&) = default;

            void execute();

            int64_t get_id() const noexcept;
            std::string_view get_name() const noexcept;
            std::chrono::steady_clock::time_point get_time_stamp() const noexcept;

            operator bool() const noexcept;

            stack_vec get_stack_trace() const;

        private:

            stacked_task action_;
            std::function<bool()> cancel_;

            int64_t id_;

            std::string_view name_;
            std::chrono::steady_clock::time_point time_stamp_;
        };

        using finish_action = std::function<void(std::chrono::milliseconds, const stack_vec&, std::string_view _name)>;

        class threadpool : boost::noncopyable
        {
            std::thread::id creator_thread_id_;

            finish_action on_task_finish_;

        public:

            explicit threadpool(
                const std::string_view _name,
                const size_t _count,
                std::function<void()> _on_thread_exit = std::function<void()>(),
                bool _task_trace = false);

            virtual ~threadpool();

            bool push_back(stacked_task _task, int64_t _id = -1, std::string_view _name = {}, std::function<bool()> _cancel = []() { return false; });
            bool push_front(stacked_task _task, int64_t _id = -1, std::string_view _name = {}, std::function<bool()> _cancel = []() { return false; });

            void raise_task(int64_t _id);

            const std::vector<std::thread::id>& get_threads_ids() const;

        protected:

            std::vector<std::thread> threads_;
            std::vector<std::thread::id> threads_ids_;
            std::mutex queue_mutex_;
            std::condition_variable condition_;
            std::deque<task> tasks_;

            std::atomic<bool> stop_;

            bool task_trace_;

            bool run_task_impl();
            bool run_task();

            void set_task_finish_callback(finish_action _on_task_finish);
        };
    }

}
