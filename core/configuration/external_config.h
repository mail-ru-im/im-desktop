#pragma once

#include "../common.shared/spin_lock.h"

namespace core
{
    enum class ext_url_config_error;
}

namespace config
{
    enum class features;

    namespace hosts
    {
        enum class host_url_type;

        using load_callback_t = std::function<void(core::ext_url_config_error _error, std::string _host)>;

        class external_url_config
        {
        public:
            static std::string make_url(std::string_view _domain, std::string_view _query = std::string_view());
            static std::string make_url_preset(std::string_view _login_domain);
            static std::string_view extract_host(std::string_view _host);

            static external_url_config& instance();

            void load_async(std::string_view _host, load_callback_t _callback);
            bool load_from_file();

            [[nodiscard]] std::string get_host(host_url_type _type) const;
            [[nodiscard]] std::vector<std::string> get_vcs_urls() const;

            void clear();

            [[nodiscard]] std::map<host_url_type, std::string> get_cache() const;
            [[nodiscard]] bool is_valid() const;

            using config_features = std::map<config::features, bool>;

        private:
            external_url_config();
            bool unserialize(const rapidjson::Value& _node);
            void override_features(const config_features& _values);
            void reset_to_defaults();

        private:
            struct config_p
            {
                std::map<host_url_type, std::string> cache_;
                std::vector<std::string> vcs_rooms_;
                config_features override_values_;

                bool unserialize(const rapidjson::Value& _node);
            };

            std::shared_ptr<config_p> config_;
            mutable common::tools::spin_lock spin_lock_;

        private:
            std::shared_ptr<config_p> get_config() const;
        };
    }
}
