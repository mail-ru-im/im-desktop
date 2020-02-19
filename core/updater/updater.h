#pragma once

namespace core
{
    class async_executer;

    namespace update
    {
        class updater
        {
            uint32_t			timer_id_;

            std::atomic<bool>	stop_;

            std::chrono::steady_clock::time_point last_check_time_;

            std::unique_ptr<core::async_executer>	thread_;

        public:

            bool must_stop() const;

            updater();
            virtual ~updater();

            void check_if_need();

            void check(std::optional<int64_t> _seq, std::optional<std::string> _custom_url);
        };

    }
}
