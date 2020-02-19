#include "stdafx.h"
#include "urls_cache.h"

#include "../common.shared/omicron_keys.h"
#include "../common.shared/string_utils.h"

#include "../../libomicron/include/omicron/omicron.h"

#include "configuration/host_config.h"

using namespace core;
using namespace urls;

using urls_cache = std::map<url_type, std::pair<config::hosts::host_url_type, std::string>>;

const urls_cache& get_urls_map()
{
    const static urls_cache cache =
    {
        { url_type::wim_host,                   { config::hosts::host_url_type::base, "/wim/" }},
        { url_type::rapi_host,                  { config::hosts::host_url_type::base, "/rapi/" }},
        { url_type::stickers_store_host,        { config::hosts::host_url_type::base, "/store" }},
        { url_type::files_host,                 { config::hosts::host_url_type::base, "/files" }},
        { url_type::smsreg_host,                { config::hosts::host_url_type::base, "/smsreg" }},
        { url_type::wapi_host,                  { config::hosts::host_url_type::base, "/wapi" }},
        { url_type::misc_www_host,              { config::hosts::host_url_type::base, "/misc/www" }},
        { url_type::preview_host,               { config::hosts::host_url_type::base, "/misc/preview" }},
        { url_type::account_operations_host,    { config::hosts::host_url_type::base, "/misc" }},
        { url_type::smsapi_host,                { config::hosts::host_url_type::base, "/smsapi" }},
        { url_type::webrtc_host,                { config::hosts::host_url_type::base, "/wim" }},
        { url_type::files_info,                 { config::hosts::host_url_type::base, "/files/api/v1.1/info/" }},

        { url_type::files_preview,              { config::hosts::host_url_type::base_binary, "/files/api/v1.1/preview" }},
        { url_type::avatars,                    { config::hosts::host_url_type::base_binary, "/files/api/v1.1/avatar" }},
    };

    return cache;
}

std::string core::urls::get_url(const url_type _type, const with_https _https)
{
    constexpr std::string_view https = "https://";

    const auto& cache = get_urls_map();
    if (const auto it = cache.find(_type); it != cache.end())
        return su::concat(_https == with_https::yes ? https : std::string_view(), config::hosts::get_host_url(it->second.first), it->second.second);

    assert(false);
    return std::string();
}

bool core::urls::is_one_domain_url(std::string_view _url) noexcept
{
    const auto ini_base_url = std::string(config::hosts::get_host_url(config::hosts::host_url_type::base));
    return _url.find(omicronlib::_o(omicron::keys::one_domain_url, ini_base_url)) != _url.npos;
}
