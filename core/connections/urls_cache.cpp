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

    using urls_cache_vector = std::vector<url_cache_item>;

    urls_cache_vector generate_urls_vector()
    {
        const auto api_ver = api_version::instance().prefix();
        urls_cache_vector cache =
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
            { url_type::tasks_host,                 config::hosts::host_url_type::base, su::concat(api_ver, "/tasks/")  },
            { url_type::oauth2_login,               config::hosts::host_url_type::oauth_url, "" },
            { url_type::oauth2_token,               config::hosts::host_url_type::token_url, ""},
            { url_type::oauth2_redirect,            config::hosts::host_url_type::redirect_uri, ""}
        };
        return cache;
    }

    struct urls_cache
    {
        static urls_cache& instance()
        {
            static urls_cache cache;
            return cache;
        }

        const auto& urls_vector() const
        {
            return urls_;
        }

        void update()
        {
            urls_ = generate_urls_vector();
        }

    private:
        urls_cache() = default;

    private:
        urls_cache_vector urls_ = generate_urls_vector();
    };
} // namespace

std::string core::urls::get_url(url_type _type, with_https _https)
{
    constexpr std::string_view https = "https://";

    const auto& cache = urls_cache::instance().urls_vector();
    if (const auto it = std::find_if(cache.begin(), cache.end(), [_type](const auto& item) { return item.url_type_ == _type; }); it != cache.end())
        return su::concat(_https == with_https::yes ? https : std::string_view(), config::hosts::get_host_url(it->host_type_), it->url_);

    im_assert(false);
    return {};
}

bool core::urls::is_one_domain_url(std::string_view _url) noexcept
{
    const auto ini_base_url = std::string(config::hosts::get_host_url(config::hosts::host_url_type::base));
    return _url.find(omicronlib::_o(omicron::keys::one_domain_url, ini_base_url)) != _url.npos;
}

api_version& core::urls::api_version::instance()
{
    static api_version version;
    return version;
}

int core::urls::api_version::server() const noexcept
{
    return server_;
}

int core::urls::api_version::client() const noexcept
{
    return client_;
}

bool core::urls::api_version::update()
{
    server_ = evaluate_server_version();
    const auto new_prefered = evaluate_preferred_version();
    const bool changed = new_prefered != preferred_;
    preferred_ = new_prefered;
    if (changed)
        urls_cache::instance().update();
    return changed;
}

int core::urls::api_version::preferred() const noexcept
{
    return preferred_;
}

std::string core::urls::api_version::prefix() const
{
    return su::concat("/api/v", std::to_string(preferred()));
}

core::urls::api_version::api_version()
    : client_{ evaluate_client_version() }
    , server_{ evaluate_server_version() }
    , preferred_{ evaluate_preferred_version() }
{
}

int core::urls::api_version::evaluate_client_version()
{
    return config::get().number<int64_t>(config::values::client_api_version).value_or(minimal_supported());
}

int core::urls::api_version::evaluate_server_version() const
{
    return config::get().number<int64_t>(config::values::server_api_version).value_or(client_);
}

int core::urls::api_version::evaluate_preferred_version() const
{
    return std::max(minimal_supported(), std::min(server_, client_));
}
