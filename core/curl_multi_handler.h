#pragma once
#include "curl_handler.h"
namespace core
{
    class curl_multi_handler : public curl_base_handler
    {
    public:
        virtual curl_easy::future_t perform(std::shared_ptr<curl_context> _context) override;
        virtual void perform_async(std::shared_ptr<curl_context> _context, curl_easy::completion_function _completion_func) override;
        virtual void raise_task(int64_t _id) override;

        virtual void init() override;
        virtual void cleanup() override;
        virtual void reset() override;

        virtual void process_stopped_tasks() override;

        virtual bool is_stopped() const override;

    private:
        void add_task_to_queue(std::shared_ptr<core::curl_task> _task);

    private:
        std::thread service_thread_;
    };
}
