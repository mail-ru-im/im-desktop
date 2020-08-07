#pragma once

#if defined _WIN32 || defined __linux__ || BUILD_FOR_STORE

namespace core
{
    class async_executer;
    struct proxy_settings;

    namespace dump
    {
        const std::string& get_hockeyapp_url();

        class report_sender : public std::enable_shared_from_this<report_sender>
        {
        public:
            report_sender(std::string _login, bool _full_log);
            ~report_sender();

            void send_report();

        private:
            void clear_report_folder();
            bool is_report_existed() const;
            bool send(std::string_view _login, const proxy_settings& _proxy);
			void insert_imstat_event();
            std::unique_ptr<async_executer> send_thread_;
            const std::string login_;
            const bool full_log_;
        };
    }
}

#endif // defined _WIN32 || defined __linux__
