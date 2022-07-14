#pragma once

#include "../corelib/enumerations.h"

namespace core
{
    class coll_helper;
    class core_settings;

    namespace proxy_detail
    {
        class ignore_list
        {
            std::vector<std::string> host_list_; // example.com
            std::vector<std::string> host_mask_list_; // [*.example.com] without '*'
            std::vector<std::string> ip_list_; // 10.215.0.1
            std::vector<std::string> ip_mask_list_; // 10.215

        public:
            void parse_hosts(std::string_view _hosts);
            bool need_ignore(std::string_view _url, std::string_view _ip_address);

            std::string to_string() const;

        private:
            void add_host(std::string_view _host);
        };
    }

    struct proxy_settings
    {
        static const int32_t default_proxy_port;

        std::string proxy_server_;
        std::string login_;
        std::string password_;
        proxy_detail::ignore_list ignore_list_;
        int32_t proxy_port_ = default_proxy_port;
        int32_t proxy_type_ = (int32_t)core::proxy_type::auto_proxy;
        core::proxy_auth auth_type_ = core::proxy_auth::basic;
        bool use_proxy_ = false;
        bool need_auth_ = false;
        bool is_system_ = false;

        void serialize(tools::binary_stream& _bs) const;
        bool unserialize(tools::binary_stream& _bs);

        void serialize(core::coll_helper _collection) const;

        static proxy_settings auto_proxy()
        {
            return proxy_settings();
        }

        bool need_use_proxy(std::string_view _url, std::string_view _ip_address);
        bool parse_servers_list(std::string_view _server);

        std::string to_string() const;

    private:
        bool parse_server(std::string_view _server);
    };

    class proxy_settings_manager
    {
        core_settings& settings_storage_;

#ifdef _WIN32
        std::array<proxy_settings, 3> settings_;
#else
        std::array<proxy_settings, 2> settings_;
#endif

        size_t current_settings_;

        mutable std::mutex mutex_;

    public:
        explicit proxy_settings_manager(core_settings& _settings_storage);

        proxy_settings get_current_settings() const;

        void apply(const proxy_settings& _settings);

        bool try_to_apply_alternative_settings();

    private:
#ifdef _WIN32
        proxy_settings load_from_registry();
#endif
    };
}
