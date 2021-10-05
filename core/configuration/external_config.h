#pragma once

#include "../common.shared/spin_lock.h"
#include "../common.shared/config/config.h"

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

        using host_cache = std::vector<std::pair<host_url_type, std::string>>;

        using load_callback_t = std::function<void(core::ext_url_config_error _error, std::string _host)>;

        class external_url_config
        {
        public:
            static std::string make_url_preset(std::string_view _login_domain);
            static std::string make_url_auto_preset(std::string_view _login_domain, std::string_view _host = {});
            static std::string_view extract_host(std::string_view _host);

            static external_url_config& instance();

            void load_async(std::string_view _host, load_callback_t _callback);
            bool load_from_file();

            [[nodiscard]] std::string get_host(host_url_type _type) const;
            [[nodiscard]] std::vector<std::string> get_vcs_urls() const;

            void clear();

            [[nodiscard]] host_cache get_cache() const;
            [[nodiscard]] bool is_valid() const;

        private:
            static std::string make_url(std::string_view _domain, std::string_view _query = {});
            external_url_config();
            ~external_url_config();
            bool unserialize(const rapidjson::Value& _node);
            void override_config(const features_vector& _features, const values_vector& _values);
            void reset_to_defaults();

        private:
            struct config_p
            {
                host_cache cache_;
                std::vector<std::string> vcs_rooms_;
                features_vector override_features_;
                values_vector override_values_;

                std::string_view get_host(host_url_type _type) const;
                bool unserialize(const rapidjson::Value& _node);
            };

            std::shared_ptr<config_p> config_;
            mutable common::tools::spin_lock spin_lock_;
            bool use_develop_local_config_ = false;

        private:
            std::shared_ptr<config_p> get_config() const;
        };
    }
}
