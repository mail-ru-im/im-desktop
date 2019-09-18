#pragma once

namespace core
{
    namespace urls
    {
        enum class url_type
        {
            wim_host = 1,
            rapi_host = 2,
            stickers_store_host = 3,
            files_host = 4,
            files_gateway_host = 5,
            smsreg_host = 6,
            wapi_host = 7,
            misc_www_host = 8,
            preview_host = 9,
            account_operations_host = 10,
            smsapi_host = 11,
            webrtc_host = 12,
            client_stat_url = 13,
            imstat_host = 14,
            client_login = 15
        };

        std::string_view get_url(const url_type _type);

        bool is_one_domain_url(std::string_view _url) noexcept;
    }
}