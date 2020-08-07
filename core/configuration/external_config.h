#pragma once

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

            std::string_view get_host(const host_url_type _type) const;
            const std::vector<std::string> &get_vcs_urls() const { return config_.vcs_rooms_; };

            void clear();

            [[nodiscard]] const std::map<host_url_type, std::string>& get_cache() const noexcept { return config_.cache_; }
            [[nodiscard]] bool is_valid() const noexcept { return is_valid_; }

            using config_features = std::map<config::features, bool>;

        private:
            external_url_config();
            bool unserialize(const rapidjson::Value& _node);
            void override_features(const config_features& _values);
            void reset_to_defaults(const config_features& _values);

        private:
            bool is_valid_ = false;

            struct config_p
            {
                std::map<host_url_type, std::string> cache_;
                std::vector<std::string> vcs_rooms_;
                config_features override_values_;

                bool unserialize(const rapidjson::Value& _node);
                void clear();
            };
            config_p config_;

            const config_features default_values_;
        };
    }
}
