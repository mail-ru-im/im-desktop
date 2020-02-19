#ifndef __ASYNC_TASK_H_
#define __ASYNC_TASK_H_

#pragma once

#include "tools/threadpool.h"
#include "core.h"

namespace core
{
    class async_task
    {
    public:
        async_task();
        virtual ~async_task();

        virtual int32_t execute() = 0;
    };

    struct async_task_handlers
    {
        std::function<void(int32_t)>	on_result_;
    };

    template<typename T>
    struct t_async_task_handlers
    {
        std::function<void(T)>	on_result_;
    };

    class auto_callback : private boost::noncopyable
    {
        std::function<void(int32_t)> callback_;

    public:

        auto_callback(std::function<void(int32_t)> _call_back)
            :   callback_(std::move(_call_back))
        {
            assert(callback_);
        }

        ~auto_callback()
        {
            assert(!callback_); // callback must be nullptr
            if (callback_)
                callback_(-1);
        }

        void callback(int32_t _error)
        {
            callback_(_error);
            callback_ = nullptr;
        }
    };

    typedef std::shared_ptr<auto_callback> auto_callback_sptr;

    class async_executer : core::tools::threadpool
    {
    public:
        explicit async_executer(const std::string_view _name, size_t _count = 1, bool _task_trace = false);
        virtual ~async_executer();

        virtual std::shared_ptr<async_task_handlers> run_async_task(std::shared_ptr<async_task> task, std::string_view _task_name = {}, std::function<bool()> _cancel = {});

        virtual std::shared_ptr<async_task_handlers> run_async_function(std::function<int32_t()> func, std::string_view _task_name = {}, std::function<bool()> _cancel = {});

        template<typename T>
        std::shared_ptr<t_async_task_handlers<T>> run_t_async_function(std::function<T()> func, std::string_view _task_name = {}, std::function<bool()> _cancel = {})
        {
            auto handler = std::make_shared<t_async_task_handlers<T>>();

            push_back([f = std::move(func), handler]
            {
                auto res = f();

                g_core->execute_core_context([handler, result = std::move(res)]
                {
                    if (handler->on_result_)
                        handler->on_result_(result);
                });
            }, -1, _task_name, std::move(_cancel));

            return handler;
        }
    };

    using async_executer_uptr = std::unique_ptr<async_executer>;

}


#endif //__ASYNC_TASK_H_
