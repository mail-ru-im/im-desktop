#pragma once

namespace core
{
    enum class ext_url_config_error;
}

namespace config
{
    namespace hosts
    {
        enum class host_url_type;

        class external_url_config
        {
        public:
            core::ext_url_config_error load(std::string_view _host);
            bool load_from_file();

            std::string_view get_host(const host_url_type _type) const;

            void clear();

            [[nodiscard]] const std::map<host_url_type, std::string>& get_cache() const noexcept { return cache_; }
            [[nodiscard]] bool is_valid() const noexcept { return is_valid_; }

        private:
            bool unserialize(const rapidjson::Value& _node);

        private:
            bool is_valid_ = false;
            std::map<host_url_type, std::string> cache_;
        };
    }
}