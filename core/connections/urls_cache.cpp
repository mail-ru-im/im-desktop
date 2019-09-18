#include "stdafx.h"
#include "urls_cache.h"
#include "../../libomicron/include/omicron/omicron.h"

#include "../../corelib/collection_helper.h"
#include "../configuration/app_config.h"



using namespace core;
using namespace urls;

struct protocol_url
{
    const std::string url_;
    const std::string_view url_deprecated_;
};

using urls_cache = std::map<url_type, const protocol_url>;

const urls_cache& get_ursl_cache()
{
    const static std::string base_url = std::string("https://") + core::configuration::get_app_config().get_url_base();

    const static auto statistics_base_url = build::is_dit() ? std::string("https://u.icq.net") : base_url;

    const static urls_cache cache =
    {
        { url_type::client_login, { base_url + std::string("/wim/auth/clientLogin"), std::string_view("https://api.login.icq.net/auth/clientLogin")} },
        { url_type::wim_host, { base_url + std::string("/wim/"), std::string_view("https://api.icq.net/")} },
        { url_type::rapi_host, { base_url + std::string("/rapi/"), std::string_view("https://rapi.icq.net/")} },
        { url_type::stickers_store_host, { base_url + std::string("/store"), std::string_view("https://store.icq.com") } },
        { url_type::files_host, { base_url + std::string("/files"), std::string_view("https://files.icq.com") } },
        { url_type::files_gateway_host, { base_url + std::string("/files"), std::string_view("https://files.icq.com/files") } },
        { url_type::smsreg_host, { base_url + std::string("/smsreg"), std::string_view("https://icq.com/smsreg") } },
        { url_type::wapi_host, { base_url + std::string("/wapi"), std::string_view("https://wapi.icq.com") } },
        { url_type::misc_www_host, { base_url + std::string("/misc/www"), std::string_view("https://icq.com") } },
        { url_type::preview_host, { base_url + std::string("/misc/preview"), std::string_view("https://api.icq.net/preview") } },
        { url_type::account_operations_host, { base_url + std::string("/misc"), std::string_view("https://icq.com") } },
        { url_type::smsapi_host, { base_url + std::string("/smsapi"), std::string_view("https://clientapi.icq.net") } },
        { url_type::webrtc_host, { base_url + std::string("/wim"), std::string_view("https://api.icq.net") } },
        { url_type::client_stat_url, { statistics_base_url + std::string("/srp"), std::string_view("https://srp.icq.com") } },
        { url_type::imstat_host, { statistics_base_url + std::string("/stat"), std::string_view("https://stat.icq.in") } }
    };

    return cache;
}


std::string_view core::urls::get_url(const url_type _type)
{
    const auto& cache = get_ursl_cache();

    if (const auto iter_u = cache.find(_type); iter_u != cache.end())
    {
        if (omicronlib::_o("one_domain_feature", feature::default_one_domain_feature()))
            return iter_u->second.url_;
        else
            return iter_u->second.url_deprecated_;
    }

    assert(false);
    return std::string_view();
}

bool core::urls::is_one_domain_url(std::string_view _url) noexcept
{
    static const auto ini_base_url = core::configuration::get_app_config().get_url_base();
    return _url.find(omicronlib::_o("one_domain_url", ini_base_url)) != _url.npos;
}
