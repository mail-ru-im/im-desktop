#ifndef __SCOPE_H_
#define __SCOPE_H_

#pragma once

namespace core
{
    namespace tools
    {
        enum class auto_scope_thread
        {
            unspec,
            main
        };

        void run_on_core_thread(std::function<void()> _f);

        template<auto_scope_thread T>
        class auto_scope_impl
        {
            std::function<void()> end_lambda_;

        public:

            auto_scope_impl(std::function<void()> _lambda)
                : end_lambda_(std::move(_lambda))
            {
                assert(end_lambda_);
            }

            ~auto_scope_impl()
            {
                if constexpr (T == auto_scope_thread::unspec)
                    end_lambda_();
                else
                    run_on_core_thread(std::move(end_lambda_));
            }
        };

        using auto_scope = auto_scope_impl<auto_scope_thread::unspec>;
        using auto_scope_main = auto_scope_impl<auto_scope_thread::main>;
        using auto_scope_sptr = std::shared_ptr<auto_scope>;

        template<auto_scope_thread T>
        class auto_scope_bool_impl
        {
            std::function<void(const bool)> end_lambda_;

            bool success_ = false;

        public:

            void set_success()
            {
                success_ = true;
            }

            auto_scope_bool_impl(std::function<void(const bool)> _lambda)
                : end_lambda_(std::move(_lambda))
            {
                assert(end_lambda_);
            }

            ~auto_scope_bool_impl()
            {
                if constexpr (T == auto_scope_thread::unspec)
                {
                    end_lambda_(success_);
                }
                else
                {
                    run_on_core_thread([success_ = success_, end_lambda_ = std::move(end_lambda_)]()
                    {
                        end_lambda_(success_);
                    });
                }
            }
        };

        using auto_scope_bool = auto_scope_bool_impl<auto_scope_thread::unspec>;
        using auto_scope_main_bool = auto_scope_bool_impl<auto_scope_thread::main>;
    }
}

#endif //__SCOPE_H_