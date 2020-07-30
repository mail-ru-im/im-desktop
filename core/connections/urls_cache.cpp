#include "stdafx.h"
#include "urls_cache.h"

#include "../common.shared/omicron_keys.h"
#include "../common.shared/string_utils.h"
#include "../common.shared/config/config.h"

#include "../../libomicron/include/omicron/omicron.h"

#include "configuration/host_config.h"

using namespace core;
using namespace urls;

using urls_cache = std::map<url_type, std::pair<config::hosts::host_url_type, std::string>>;

const urls_cache& get_urls_map()
{
    const static urls_cache cache = []()
    {
        const auto api_ver = api_version_prefix();
        urls_cache cache =
        {
            { url_type::wim_host,                   { config::hosts::host_url_type::base, su::concat(api_ver, "/wim/") }},
            { url_type::rapi_host,                  { config::hosts::host_url_type::base, su::concat(api_ver, "/rapi/") }},
            { url_type::stickers_store_host,        { config::hosts::host_url_type::base, su::concat(api_ver, "/store") }},
            { url_type::files_host,                 { config::hosts::host_url_type::base, su::concat(api_ver, "/files") }},
            { url_type::smsreg_host,                { config::hosts::host_url_type::base, su::concat(api_ver, "/smsreg") }},
            { url_type::wapi_host,                  { config::hosts::host_url_type::base, su::concat(api_ver, "/wapi") }},
            { url_type::misc_www_host,              { config::hosts::host_url_type::base, su::concat(api_ver, "/misc/www") }},
            { url_type::preview_host,               { config::hosts::host_url_type::base, su::concat(api_ver, "/misc/preview") }},
            { url_type::account_operations_host,    { config::hosts::host_url_type::base, su::concat(api_ver, "/misc") }},
            { url_type::smsapi_host,                { config::hosts::host_url_type::base, su::concat(api_ver, "/smsapi") }},
            { url_type::webrtc_host,                { config::hosts::host_url_type::base, su::concat(api_ver, "/wim") }},
            { url_type::url_content,                { config::hosts::host_url_type::base_binary, su::concat(api_ver, "/misc/preview/getURLContent") }},
        };

        if (get_api_version() > 1)
        {
            cache.insert
            ({
                { url_type::files_info,             { config::hosts::host_url_type::base, su::concat(api_ver, "/files/info/") }},
                { url_type::files_preview,          { config::hosts::host_url_type::base_binary, su::concat(api_ver, "/files/preview") }},
                { url_type::avatars,                { config::hosts::host_url_type::base_binary, su::concat(api_ver, "/files/avatar") }},
            });
        }
        else
        {
            cache.insert
            ({
                 { url_type::files_info,             { config::hosts::host_url_type::base, "/files/api/v1.1/info/" }},
                 { url_type::files_preview,          { config::hosts::host_url_type::base_binary, "/files/api/v1.1/preview" }},
                 { url_type::avatars,                { config::hosts::host_url_type::base_binary, "/files/api/v1.1/avatar" }},
             });
        }
        return cache;
    }();

    return cache;
}

std::string core::urls::get_url(url_type _type, with_https _https)
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

uint32_t core::urls::get_api_version()
{
    return std::max(int64_t(0), config::get().number<int64_t>(config::values::server_api_version).value_or(1));
}

std::string core::urls::api_version_prefix()
{
    if (const auto version  = get_api_version(); version > 1)
        return su::concat("/api/v", std::to_string(version));

    return std::string();
}
