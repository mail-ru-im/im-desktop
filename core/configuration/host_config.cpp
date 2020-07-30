#include "stdafx.h"

#include "core.h"
#include "../../corelib/collection_helper.h"
#include "../../corelib/enumerations.h"
#include "host_config.h"
#include "external_config.h"
#include "../../common.shared/config/config.h"

using namespace core;
using namespace config;
using namespace hosts;

using host_cache = std::map<host_url_type, std::string>;

static const host_cache& get_bundled_cache()
{
    const static host_cache cache
    {
        { host_url_type::base,              std::string(config::get().url(config::urls::base)) },
        { host_url_type::base_binary,       std::string(config::get().url(config::urls::base_binary)) },

        { host_url_type::files_parse,       std::string(config::get().url(config::urls::files_parser_url)) },
        { host_url_type::stickerpack_share, std::string(config::get().url(config::urls::stickerpack_share)) },
        { host_url_type::profile,           std::string(config::get().url(config::urls::profile)) },

        { host_url_type::mail_auth,         std::string(config::get().url(config::urls::auth_mail_ru)) },
        { host_url_type::mail_redirect,     std::string(config::get().url(config::urls::r_mail_ru)) },
        { host_url_type::mail_win,          std::string(config::get().url(config::urls::win_mail_ru)) },
        { host_url_type::mail_read,         std::string(config::get().url(config::urls::read_msg)) },
    };

    return cache;
}

void config::hosts::send_config_to_gui()
{
    coll_helper coll(g_core->create_collection(), true);
    const auto serialize_hosts = [&coll](const host_cache& _cache, const std::vector<std::string>& _vcs_urls)
    {
        const auto get_value = [&_cache](const host_url_type _type)
        {
            if (const auto it = _cache.find(_type); it != _cache.end())
                return it->second;

            return std::string();
        };

        coll.set_value_as_string("filesParse", get_value(host_url_type::files_parse));
        coll.set_value_as_string("stickerShare", get_value(host_url_type::stickerpack_share));
        coll.set_value_as_string("profile", get_value(host_url_type::profile));

        coll.set_value_as_string("mailAuth", get_value(host_url_type::mail_auth));
        coll.set_value_as_string("mailRedirect", get_value(host_url_type::mail_redirect));
        coll.set_value_as_string("mailWin", get_value(host_url_type::mail_win));
        coll.set_value_as_string("mailRead", get_value(host_url_type::mail_read));

        core::ifptr<core::iarray> arr(coll->create_array());
        for (const auto& url: _vcs_urls)
        {
            core::ifptr<core::ivalue> val(coll->create_value());
            val->set_as_string(url.c_str(), url.length());
            arr->push_back(val.get());
        }
        coll.set_value_as_array("vcs_urls", arr.get());
    };

    if (config::get().is_on(config::features::external_url_config))
        serialize_hosts(external_url_config::instance().get_cache(), external_url_config::instance().get_vcs_urls());
    else
        serialize_hosts(get_bundled_cache(), {});

    g_core->post_message_to_gui("external_url_config/hosts", 0, coll.get());
}

std::string_view config::hosts::get_host_url(const host_url_type _type)
{
    if (config::get().is_on(config::features::external_url_config))
        return external_url_config::instance().get_host(_type);

    const auto& cache = get_bundled_cache();
    if (const auto it = cache.find(_type); it != cache.end())
        return it->second;

    assert(false);
    return std::string_view();
}

bool config::hosts::is_valid()
{
    if (config::get().is_on(config::features::external_url_config))
        return external_url_config::instance().is_valid();

    return true;
}

bool config::hosts::load_config()
{
    auto res = true;
    if (config::get().is_on(config::features::external_url_config))
        res = external_url_config::instance().load_from_file();

    send_config_to_gui();
    return res;
}

void config::hosts::load_external_config_from_url(std::string_view _url, load_callback_t _callback)
{
    external_url_config::instance().load_async(_url, [_callback = std::move(_callback)](core::ext_url_config_error _error, std::string _url)
    {
        if (_error == core::ext_url_config_error::ok)
            send_config_to_gui();
        _callback(_error, std::move(_url));
    });
}

void config::hosts::load_external_url_config(std::vector<std::string>&& _hosts, load_callback_t&& _callback)
{
    if (_hosts.empty())
    {
        _callback(core::ext_url_config_error::config_host_invalid, std::string());
        return;
    }

    auto host = std::move(_hosts.back());
    _hosts.pop_back();
    load_external_config_from_url(host, [_hosts = std::move(_hosts), _callback = std::move(_callback)](core::ext_url_config_error _error, std::string _host) mutable
    {
        if (_error == ext_url_config_error::ok || _hosts.empty())
            _callback(_error, std::move(_host));
        else
            load_external_url_config(std::move(_hosts), std::move(_callback));
    });
}

void config::hosts::clear_external_config()
{
    g_core->set_external_config_host({});
    external_url_config::instance().clear();
    send_config_to_gui();
}