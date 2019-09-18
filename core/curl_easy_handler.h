#pragma once

#include "curl_handler.h"
#include "tools/threadpool.h"
#include <curl.h>

namespace core
{
    class curl_easy_handler : public curl_base_handler
    {
    public:
        using task = std::function<void()>;

        curl_easy_handler();

        virtual curl_easy::future_t perform(std::shared_ptr<curl_context> _context) override;
        virtual void perform_async(std::shared_ptr<curl_context> _context, curl_easy::completion_function _completion_func) override;
        virtual void raise_task(int64_t _id) override;

        virtual void init() override;
        virtual void cleanup() override;

        virtual bool is_stopped() const override;

    private:
        void add_task(std::shared_ptr<curl_task> _task, priority_t _priorioty, int64_t _id);

        void run_task(const task& _task);

    private:
        std::unique_ptr<tools::threadpool> top_priority_tasks_;
        std::mutex top_priority_tasks_mutex_;

        std::unique_ptr<tools::threadpool> medium_priority_tasks_;
        std::mutex medium_priority_tasks_mutex_;

        std::unique_ptr<tools::threadpool> low_priority_tasks_;
        std::mutex low_priority_tasks_mutext_;

        std::thread fetch_thread_;
        std::mutex fetch_tasks_mutex_;
        std::deque<task> fetch_tasks_;
        std::condition_variable condition_fetch_;

        std::thread protocol_thread_;
        std::mutex protocol_tasks_mutex_;
        std::deque<task> protocol_tasks_;
        std::condition_variable condition_protocol_;

        std::atomic_bool stop_;
    };
}
