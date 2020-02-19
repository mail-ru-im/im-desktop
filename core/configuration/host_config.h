#pragma once

namespace core
{
    enum class ext_url_config_error;
}

namespace config
{
    namespace hosts
    {
        enum class host_url_type
        {
            base,
            base_binary,

            files_parse,
            stickerpack_share,
            profile,

            mail_auth,
            mail_redirect,
            mail_win,
            mail_read
        };

        [[nodiscard]] std::string_view get_host_url(const host_url_type _type);

        bool is_valid();
        bool load_config();
        core::ext_url_config_error load_external_config_from_url(std::string_view _url);
        void clear_external_config();
    }
}