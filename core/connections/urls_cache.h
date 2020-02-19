#pragma once

namespace core
{
    namespace urls
    {
        enum class url_type
        {
            wim_host,
            rapi_host,
            stickers_store_host,
            files_host,
            files_preview,
            smsreg_host,
            wapi_host,
            misc_www_host,
            preview_host,
            account_operations_host,
            smsapi_host,
            webrtc_host,
            files_info,
            avatars,
        };

        enum class with_https
        {
            no,
            yes
        };

        std::string get_url(const url_type _type, const with_https _https = with_https::yes);

        bool is_one_domain_url(std::string_view _url) noexcept;
    }
}
