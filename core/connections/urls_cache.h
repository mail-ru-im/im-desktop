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
            mytracker,
            files_info,
            avatars,
            url_content,
            sticker,
            wallpapers
        };

        enum class with_https
        {
            no,
            yes
        };

        std::string get_url(url_type _type, with_https _https = with_https::yes);

        bool is_one_domain_url(std::string_view _url) noexcept;

        std::string api_version_prefix();
    }
}
