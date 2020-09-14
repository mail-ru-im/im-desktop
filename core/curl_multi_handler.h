#pragma once
#include "curl_handler.h"
#include "async_task.h"
namespace core
{
    class curl_multi_handler : public curl_base_handler
    {
    public:
        curl_multi_handler();

        curl_easy::future_t perform(const std::shared_ptr<curl_context>& _context) override;
        void perform_async(const std::shared_ptr<curl_context>& _context, curl_easy::completion_function _completion_func) override;
        void raise_task(int64_t _id) override;

        void init() override;
        void cleanup() override;
        void reset() override;

        void process_stopped_tasks() override;
        bool is_stopped() const override;

        void resolve_host(std::string_view, std::function<void(std::string _result, int _error)> _callback);

    private:
        void add_task();
        void add_task_to_queue(std::shared_ptr<core::curl_task> _task);

        void cancel_tasks();
        void process_stopped_tasks_internal();

        void notify();

    private:
        std::thread service_thread_;
        std::unique_ptr<async_executer> resolve_hosts_thread_;
    };
}
