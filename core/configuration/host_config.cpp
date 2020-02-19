#include "stdafx.h"

#include "core.h"
#include "../../corelib/collection_helper.h"
#include "host_config.h"
#include "external_config.h"
#include "../../common.shared/config/config.h"

using namespace config;
using namespace hosts;

using host_cache = std::map<host_url_type, std::string>;

const host_cache& get_bundled_cache()
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

external_url_config& get_external_config()
{
    static external_url_config c;
    return c;
}

void send_config_to_gui(const host_cache& _cache)
{
    using namespace core;

    const auto get_value = [&_cache](const host_url_type _type)
    {
        if (const auto it = _cache.find(_type); it != _cache.end())
            return it->second;

        return std::string();
    };

    coll_helper coll(g_core->create_collection(), true);
    coll.set_value_as_string("filesParse", get_value(host_url_type::files_parse));
    coll.set_value_as_string("stickerShare", get_value(host_url_type::stickerpack_share));
    coll.set_value_as_string("profile", get_value(host_url_type::profile));

    coll.set_value_as_string("mailAuth", get_value(host_url_type::mail_auth));
    coll.set_value_as_string("mailRedirect", get_value(host_url_type::mail_redirect));
    coll.set_value_as_string("mailWin", get_value(host_url_type::mail_win));
    coll.set_value_as_string("mailRead", get_value(host_url_type::mail_read));

    g_core->post_message_to_gui("external_url_config/hosts", 0, coll.get());
}

void send_config_to_gui()
{
    if (config::get().is_on(config::features::external_url_config))
        send_config_to_gui(get_external_config().get_cache());
    else
        send_config_to_gui(get_bundled_cache());
}

std::string_view config::hosts::get_host_url(const host_url_type _type)
{
    if (config::get().is_on(config::features::external_url_config))
        return get_external_config().get_host(_type);

    const auto& cache = get_bundled_cache();
    if (const auto it = cache.find(_type); it != cache.end())
        return it->second;

    assert(false);
    return std::string_view();
}

bool config::hosts::is_valid()
{
    if (config::get().is_on(config::features::external_url_config))
        return get_external_config().is_valid();

    return true;
}

bool config::hosts::load_config()
{
    auto res = true;
    if (config::get().is_on(config::features::external_url_config))
        res = get_external_config().load_from_file();

    send_config_to_gui();
    return res;
}

core::ext_url_config_error config::hosts::load_external_config_from_url(std::string_view _url)
{
    const auto res = get_external_config().load(_url);
    send_config_to_gui();
    return res;
}

void config::hosts::clear_external_config()
{
    get_external_config().clear();
    send_config_to_gui();
}