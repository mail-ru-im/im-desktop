#include "stdafx.h"

#include "core.h"
#include "../utils.h"
#include "../curl_handler.h"
#include "../tools/features.h"
#include "../archive/storage.h"
#include "../../corelib/collection_helper.h"
#include "../../corelib/enumerations.h"
#include "host_config.h"
#include "external_config.h"
#include "../../common.shared/config/config.h"
#include "../../common.shared/string_utils.h"
#include "../../common.shared/spin_lock.h"

using namespace core;
using namespace config;
using namespace hosts;

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
        { host_url_type::mail_oauth2,       std::string(config::get().url(config::urls::oauth2_mail_ru)) },
        { host_url_type::mail_redirect,     std::string(config::get().url(config::urls::r_mail_ru)) },
        { host_url_type::mail_win,          std::string(config::get().url(config::urls::win_mail_ru)) },
        { host_url_type::mail_read,         std::string(config::get().url(config::urls::read_msg)) },
    };

    return cache;
}

constexpr int32_t dns_cache_field = 1;

struct dns_entry
{
    bool ip_mode_ = true;
    std::string ip_;
};

typedef std::unordered_map<std::string, dns_entry> dns_cache;
static dns_cache dns_cache_;

static common::tools::spin_lock dns_cache_lock_;
static bool dns_workaround_was_enabled = false;

static void modify_dns_cache(std::string_view _url, std::string_view _ip, std::optional<bool> _ip_mode = std::nullopt)
{
    auto url = std::string(_url);
    std::scoped_lock lb(dns_cache_lock_);
    auto entry = dns_cache_.find(url);
    if (entry == dns_cache_.end())
    {
        auto& entry = dns_cache_[std::move(url)];
        entry.ip_ = _ip;
        entry.ip_mode_ = true;
        return;
    }

    if (!_ip.empty())
        entry->second.ip_ = _ip;

    if (_ip_mode)
        entry->second.ip_mode_ = *_ip_mode;
}

static dns_cache get_dns_cache()
{
    std::scoped_lock lb(dns_cache_lock_);

    return dns_cache_;
}

static void load_dns_cahe_from_string(std::string_view _str)
{
    if (_str.empty())
        return;

    auto parse_entry = [](std::string_view _entry)
    {
        if (const auto pos = _entry.find(':'); pos != _entry.npos)
        {
            modify_dns_cache(_entry.substr(0, pos), _entry.substr(pos + 1, _entry.size() - pos - 1), true);
        }
        else
        {
            im_assert(false);
        }
    };

    auto pos = _str.find(';');
    while (pos != _str.npos)
    {
        parse_entry(_str.substr(0, pos));
        _str = _str.substr(pos + 1, _str.size() - pos - 1);
        pos = _str.find(';');
    }

    if (!_str.empty())
        parse_entry(_str);
}

static std::string save_dns_cache_to_string()
{
    std::string result;
    for (const auto& [url, entry] : get_dns_cache())
    {
        result += url;
        result += ':';
        result += entry.ip_;
        result += ';';
    }

    if (!result.empty())
        result.pop_back();

    return result;
}

static std::wstring get_dns_cache_path()
{
    return su::wconcat(utils::get_product_data_path(),  L"/cache/dns_.cache");
}

static void send_stat(core::stats::stats_event_names _event, std::string_view _url, int _error)
{
    core::stats::event_props_type props;
    props.emplace_back("error", su::concat("Error(Domain=", _url, ", Code=", std::to_string(_error), ')'));
    g_core->insert_event(_event, std::move(props));
}

static void send_stat(core::stats::im_stat_event_names _event, std::string_view _url, int /*_error*/)
{
    auto url = std::string(_url);
    std::replace(url.begin(), url.end(), '.', '_');
    core::stats::event_props_type props;
    props.emplace_back("endpoint", url);
    g_core->insert_im_stats_event(_event, std::move(props));
}

static bool check_dns_workaround()
{
    auto result = ::features::is_dns_workaround_enabled();
    dns_workaround_was_enabled |= result;
    return result;
}

static void save_dns_cache()
{
    g_core->run_async([]()
    {
        if (!check_dns_workaround())
            return 0;

        archive::storage_mode mode;
        mode.flags_.write_ = true;
        mode.flags_.truncate_ = true;

        auto storage = std::make_unique<archive::storage>(get_dns_cache_path());
        if (storage->open(mode))
        {
            core::tools::tlvpack pack;
            pack.push_child(core::tools::tlv(dns_cache_field, save_dns_cache_to_string()));

            core::tools::binary_stream stream;
            pack.serialize(stream);

            int64_t offset = 0;
            storage->write_data_block(stream, offset);
            storage->close();

            g_core->write_string_to_network_log("DNS: cache was saved on disk\r\n");
        }
        else
        {
            g_core->write_string_to_network_log("DNS: failed to save cache on disk\r\n");
        }

        return 0;
    });
}

static void load_dns_cache()
{
    g_core->run_async([]()
    {
        if (!get_dns_cache().empty())
        {
            config::hosts::resolve_hosts(true);
            return 0;
        }

        auto load_from_config = []()
        {
            g_core->write_string_to_network_log("DNS: failed to load cache from disk\r\n");
            auto str = config::get().url(config::urls::dns_cache);
            load_dns_cahe_from_string(str);

            save_dns_cache();
        };

        if (check_dns_workaround())
        {
            archive::storage_mode mode;
            mode.flags_.read_ = true;

            auto storage = std::make_unique<archive::storage>(get_dns_cache_path());
            if (storage->open(mode))
            {
                core::tools::binary_stream stream;
                while (storage->read_data_block(-1, stream))
                {
                    while (stream.available())
                    {
                        core::tools::tlvpack pack;

                        if (pack.unserialize(stream))
                        {
                            for (auto tlv_field = pack.get_first(); tlv_field; tlv_field = pack.get_next())
                            {
                                switch (tlv_field->get_type())
                                {
                                case dns_cache_field:
                                    load_dns_cahe_from_string(tlv_field->get_value<std::string>());
                                    break;
                                default:
                                    break;
                                }
                            }
                        }
                    }
                }
                stream.reset();
                storage->close();

                if (!get_dns_cache().empty())
                {
                    g_core->write_string_to_network_log("DNS: cache was loaded from disk\r\n");
                    config::hosts::resolve_hosts(true);
                    return 0;
                }
            }
        }

        load_from_config();
        config::hosts::resolve_hosts(true);
        return 0;
    });
}

void config::hosts::send_config_to_gui()
{
    coll_helper coll(g_core->create_collection(), true);
    const auto serialize_hosts = [&coll](const host_cache& _cache, const std::vector<std::string>& _vcs_urls)
    {
        const auto get_value = [&_cache](host_url_type _type) -> std::string_view
        {
            if (const auto it = std::find_if(_cache.begin(), _cache.end(), [_type](const auto& x) { return x.first == _type; }); it != _cache.end())
                return it->second;
            return {};
        };

        coll.set_value_as_string("base", get_value(host_url_type::base));
        coll.set_value_as_string("baseBinary", get_value(host_url_type::base_binary));
        coll.set_value_as_string("filesParse", get_value(host_url_type::files_parse));
        coll.set_value_as_string("stickerShare", get_value(host_url_type::stickerpack_share));
        coll.set_value_as_string("profile", get_value(host_url_type::profile));

        coll.set_value_as_string("mailAuth", get_value(host_url_type::mail_auth));
        coll.set_value_as_string("mailRedirect", get_value(host_url_type::mail_redirect));
        coll.set_value_as_string("mailWin", get_value(host_url_type::mail_win));
        coll.set_value_as_string("mailRead", get_value(host_url_type::mail_read));

        if (::features::is_update_from_backend_enabled())
            coll.set_value_as_string("appUpdate", get_value(host_url_type::app_update));

        coll.set_value_as_string("di", get_value(host_url_type::di));
        coll.set_value_as_string("di_dark", get_value(host_url_type::di_dark));
        coll.set_value_as_string("tasks", get_value(host_url_type::tasks));
        coll.set_value_as_string("calendar", get_value(host_url_type::calendar));
        coll.set_value_as_string("config-host", g_core->get_external_config_url());

        core::ifptr<core::iarray> arr(coll->create_array());
        arr->reserve(_vcs_urls.size());
        for (const auto& url: _vcs_urls)
        {
            core::ifptr<core::ivalue> val(coll->create_value());
            val->set_as_string(url.data(), url.size());
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

std::string config::hosts::get_host_url(host_url_type _type)
{
    if (config::get().is_on(config::features::external_url_config))
        return external_url_config::instance().get_host(_type);

    const auto& cache = get_bundled_cache();
    if (const auto it = std::find_if(cache.begin(), cache.end(), [_type](const auto& x) { return x.first == _type; }); it != cache.end())
        return it->second;

    im_assert(false);
    return {};
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

void config::hosts::load_dns_cache()
{
    ::load_dns_cache();
}

void config::hosts::load_external_config_from_url(std::string_view _url, load_callback_t _callback)
{
    external_url_config::instance().load_async(_url, [_callback = std::move(_callback)](core::ext_url_config_error _error, std::string _url)
    {
        if (_error == core::ext_url_config_error::ok)
        {
            load_dns_cache();
            send_config_to_gui();
            if (g_core)
            {
                g_core->execute_core_context({ []()
                {
                    g_core->try_send_crash_report(); // base url is received, try to send
                } });
            }
        }
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
    g_core->set_external_config_url({});
    external_url_config::instance().clear();
    send_config_to_gui();
    load_dns_cache();
}

std::string config::hosts::format_resolve_host_str(std::string_view _host, unsigned port)
{
    std::string result;
    if (!check_dns_workaround() && !dns_workaround_was_enabled)
        return result;

    if (get_dns_cache().empty())
        load_dns_cache();

    auto format = [_host, port](std::string_view _cached, std::string_view _resolved, bool _ip_mode)
    {
        if (_host.find(_cached) != _host.npos)
        {
            if (_ip_mode)
            {
                if (_resolved.empty())
                    return su::concat('-', _cached, ':', std::to_string(port));

                if (check_dns_workaround())
                    return su::concat(_cached, ':', std::to_string(port), ':', _resolved);
                else if (dns_workaround_was_enabled)
                    return su::concat('-', _cached, ':', std::to_string(port));
                else
                    im_assert(false);

                return std::string();
            }
            else
            {
                return su::concat('-', _cached, ':', std::to_string(port));
            }
        }

        return std::string();
    };

    for (const auto& [url, entry] : get_dns_cache())
    {
        result = format(url, entry.ip_, entry.ip_mode_);
        if (!result.empty())
            break;
    }

     return result;
}

void config::hosts::resolve_hosts(bool _load)
{
    if (!check_dns_workaround())
        return;

    auto resolve = [_load](std::string_view url)
    {
        core::curl_handler::instance().resolve_host(url, [url = std::string(url), _load](std::string _result, int _error)
        {
            if (!_result.empty())
            {
                g_core->write_string_to_network_log(su::concat("DNS: ", url, " has been manually resolved as ", _result, "\r\n"));
                modify_dns_cache(url, _result);

                save_dns_cache();
            }
            else
            {
                auto cache = get_dns_cache();
                auto entry = cache.find(url);
                if (entry == cache.end())
                {
                    im_assert(false);
                    return;
                }

                g_core->write_string_to_network_log(su::concat("DNS: failed to manually resolve ", url, "\r\n"));
                send_stat((entry->second.ip_mode_ && !_load) ? core::stats::stats_event_names::network_dns_resolve_err_during_workaround : core::stats::stats_event_names::network_dns_resolve_err_during_manual_resolving, url, _error);
                send_stat((entry->second.ip_mode_ && !_load) ? core::stats::im_stat_event_names::network_dns_resolve_err_during_workaround : core::stats::im_stat_event_names::network_dns_resolve_err_during_manual_resolving, url, _error);
            }
        });
    };

    for (const auto& [url, _] : get_dns_cache())
        resolve(url);
}

void config::hosts::switch_to_dns_mode(std::string_view _url, int _error)
{
    if (!check_dns_workaround())
        return;

    for (auto& [cached, entry] : get_dns_cache())
    {
        if (_url.find(cached) != _url.npos)
        {
            if (entry.ip_mode_)
            {
                send_stat(core::stats::stats_event_names::network_fallback_to_auto_dns_resolve, cached, _error);
                send_stat(core::stats::im_stat_event_names::network_fallback_to_auto_dns_resolve, cached, _error);

                g_core->write_string_to_network_log(su::concat("DNS: switched to dns mode for ", cached, "\r\n"));
                modify_dns_cache(cached, std::string_view(), false);
            }
        }
    }
}

void config::hosts::switch_to_ip_mode(std::string_view _url, int _error)
{
    if (!check_dns_workaround())
        return;

    for (const auto& [cached, entry] : get_dns_cache())
    {
        if (_url.find(cached) != _url.npos)
        {
            if (!entry.ip_mode_)
            {
                send_stat(core::stats::stats_event_names::network_dns_resolve_err_during_auto_resolving, cached, _error);
                send_stat(core::stats::im_stat_event_names::network_dns_resolve_err_during_auto_resolving, cached, _error);

                if (::features::is_fallback_to_ip_mode_enabled())
                {
                    g_core->write_string_to_network_log(su::concat("DNS: switched to ip mode for ", cached, "\r\n"));
                    modify_dns_cache(cached, std::string_view(), true);
                }
                else
                {
                    g_core->write_string_to_network_log(su::concat("DNS: failed to automatically resolve ", cached, "\r\n"));
                }
            }
        }
    };
}