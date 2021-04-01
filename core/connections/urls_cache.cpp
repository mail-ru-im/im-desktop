#include "stdafx.h"
#include "urls_cache.h"

#include "../common.shared/omicron_keys.h"
#include "../common.shared/string_utils.h"
#include "../common.shared/config/config.h"

#include "../../libomicron/include/omicron/omicron.h"

#include "configuration/host_config.h"

using namespace core;
using namespace urls;

namespace
{
    struct url_cache_item
    {
        url_type url_type_;
        config::hosts::host_url_type host_type_;
        std::string url_;
    };

    using urls_cache = std::vector<url_cache_item>;

    const urls_cache& get_urls_vector()
    {
        const static urls_cache cache = []()
        {
            const auto api_ver = api_version_prefix();
            urls_cache cache =
            {
                { url_type::wim_host,                   config::hosts::host_url_type::base, su::concat(api_ver, "/wim/") },
                { url_type::rapi_host,                  config::hosts::host_url_type::base, su::concat(api_ver, "/rapi/") },
                { url_type::stickers_store_host,        config::hosts::host_url_type::base, su::concat(api_ver, "/store") },
                { url_type::files_host,                 config::hosts::host_url_type::base, su::concat(api_ver, "/files") },
                { url_type::smsreg_host,                config::hosts::host_url_type::base, su::concat(api_ver, "/smsreg") },
                { url_type::wapi_host,                  config::hosts::host_url_type::base, su::concat(api_ver, "/wapi") },
                { url_type::misc_www_host,              config::hosts::host_url_type::base, su::concat(api_ver, "/misc/www") },
                { url_type::preview_host,               config::hosts::host_url_type::base, su::concat(api_ver, "/misc/preview") },
                { url_type::account_operations_host,    config::hosts::host_url_type::base, su::concat(api_ver, "/misc") },
                { url_type::smsapi_host,                config::hosts::host_url_type::base, su::concat(api_ver, "/smsapi") },
                { url_type::webrtc_host,                config::hosts::host_url_type::base, su::concat(api_ver, "/wim") },
                { url_type::mytracker,                  config::hosts::host_url_type::base, su::concat(api_ver, "/mytracker-s2s/v1") },
                { url_type::url_content,                config::hosts::host_url_type::base_binary, su::concat(api_ver, "/misc/preview/getURLContent") },
                { url_type::files_info,                 config::hosts::host_url_type::base, su::concat(api_ver, "/files/info/") },
                { url_type::files_preview,              config::hosts::host_url_type::base_binary, su::concat(api_ver, "/files/preview") },
                { url_type::avatars,                    config::hosts::host_url_type::base_binary, su::concat(api_ver, "/files/avatar") },
                { url_type::sticker,                    config::hosts::host_url_type::base_binary, su::concat(api_ver, "/files/sticker") },
                { url_type::wallpapers,                 config::hosts::host_url_type::base_binary, "/images/wallpapers/" },
            };
            return cache;
        }();

        return cache;
    }
}

std::string core::urls::get_url(url_type _type, with_https _https)
{
    constexpr std::string_view https = "https://";

    const auto& cache = get_urls_vector();
    if (const auto it = std::find_if(cache.begin(), cache.end(), [_type](const auto& item) { return item.url_type_ == _type; }); it != cache.end())
        return su::concat(_https == with_https::yes ? https : std::string_view(), config::hosts::get_host_url(it->host_type_), it->url_);

    assert(false);
    return {};
}

bool core::urls::is_one_domain_url(std::string_view _url) noexcept
{
    const auto ini_base_url = std::string(config::hosts::get_host_url(config::hosts::host_url_type::base));
    return _url.find(omicronlib::_o(omicron::keys::one_domain_url, ini_base_url)) != _url.npos;
}

std::string core::urls::api_version_prefix()
{
    if (const auto version  = config::get().number<int64_t>(config::values::server_api_version); version)
        return su::concat("/api/v", std::to_string(*version));

    return {};
}
