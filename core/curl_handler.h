#pragma once

#include "tools/threadpool.h"

#include <curl.h>

namespace core
{
    namespace curl_easy
    {
        enum class completion_code
        {
            success = 0,
            cancelled,
            failed
        };

        using completion_function = std::function<void(completion_code _code)>;
        typedef std::future<CURLcode> future_t;
        typedef std::promise<CURLcode> promise_t;
    }

    struct curl_context;
    class curl_task
    {
    public:
        curl_task(std::weak_ptr<curl_context> _context, curl_easy::completion_function _completion_func);
        curl_task(std::weak_ptr<curl_context> _context, curl_easy::promise_t& _promise);

        void execute(CURL* _curl);
        void cancel();

        CURL* init_handler();
        bool execute_multi(CURLM* _multi, CURL* _curl);
        void finish_multi(CURLM* _multi, CURL* _curl, CURLcode _res);

        priority_t get_priority() const;
        int64_t get_id() const;

        static curl_easy::completion_code get_completion_code(CURLcode _err);

        bool is_stopped() const;

    private:
        std::weak_ptr<curl_context> context_;
        curl_easy::completion_function completion_func_;
        curl_easy::promise_t promise_;

        bool async_;
    };

    class curl_base_handler
    {
    public:
        virtual ~curl_base_handler() {}

        virtual curl_easy::future_t perform(std::shared_ptr<curl_context> _context) = 0;
        virtual void perform_async(std::shared_ptr<curl_context> _context, curl_easy::completion_function _completion_func) = 0;
        virtual void raise_task(int64_t _id) = 0;

        virtual void init() = 0;
        virtual void cleanup() = 0;
        virtual void reset() {}
        virtual void process_stopped_tasks() {}

        virtual bool is_stopped() const = 0;
    };

    class curl_easy_handler;
    class curl_multi_handler;

    class curl_handler : public curl_base_handler
    {
    public:
        static curl_handler& instance();

        virtual curl_easy::future_t perform(std::shared_ptr<curl_context> _context) override;
        virtual void perform_async(std::shared_ptr<curl_context> _context, curl_easy::completion_function _completion_func) override;
        virtual void raise_task(int64_t _id) override;

        virtual void init() override;
        virtual void cleanup() override;
        virtual void reset() override;

        virtual void process_stopped_tasks() override;

        virtual bool is_stopped() const override;

    private:
        curl_handler();

    private:
        std::unique_ptr<curl_easy_handler> easy_handler_;
        std::unique_ptr<curl_multi_handler> multi_handler_;
    };
}
