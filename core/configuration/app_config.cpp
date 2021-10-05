#include "stdafx.h"

#include <boost/range/adaptor/map.hpp>

#include "../../corelib/collection_helper.h"
#include "../tools/system.h"
#include "../../common.shared/json_helper.h"
#include "../utils.h"

#include "../core.h"

#include "app_config.h"

#include "../../common.shared/constants.h"
#include "../../common.shared/config/config.h"
#include "../../common.shared/string_utils.h"

#ifndef STRIP_CRASH_HANDLER
#ifdef _WIN32
#include "../../common.shared/crash_report/crash_reporter.h"
#endif //_WIN32
#endif // !STRIP_CRASH_HANDLE


CORE_CONFIGURATION_NS_BEGIN

namespace pt = boost::property_tree;

namespace
{
    std::unique_ptr<app_config> config_;

    const int_set& valid_dpi_values();
    app_config::AppConfigMap parse_ptree_into_map(const pt::ptree& property_tree_);
    const char* option_name(const app_config::AppConfigOption option_);
    bool external_config = false;
}

app_config::app_config()
    : app_config_options_(parse_ptree_into_map(pt::ptree())) /* populate with defaults */
{

}

app_config::app_config(app_config::AppConfigMap _options)
    : app_config_options_(std::move(_options))
{
    im_assert(app_config_options_.find(app_config::AppConfigOption::forced_dpi) != app_config_options_.end());
    im_assert(valid_dpi_values().count(
        boost::any_cast<int32_t>(app_config_options_.at(app_config::AppConfigOption::forced_dpi))) > 0);
}

pt::ptree app_config::as_ptree() const
{
    pt::ptree result;

    for (auto key : boost::adaptors::keys(app_config_options_))
    {
        switch (key)
        {
        case app_config::AppConfigOption::forced_dpi:
            result.add(option_name(key), forced_dpi());
            break;
        case app_config::AppConfigOption::disable_server_history:
            result.add(option_name(key), !is_server_history_enabled());
            break;
        case app_config::AppConfigOption::is_crash_enabled:
            result.add(option_name(key), is_crash_enabled());
            break;
        case app_config::AppConfigOption::is_testing_enabled:
            result.add(option_name(key), is_testing_enabled());
            break;
        case app_config::AppConfigOption::full_log:
            result.add(option_name(key), is_full_log_enabled());
            break;
        case app_config::AppConfigOption::unlock_context_menu_features:
            result.add(option_name(key), unlock_context_menu_features());
            break;
        case app_config::AppConfigOption::show_msg_ids:
            result.add(option_name(key), is_show_msg_ids_enabled());
            break;
        case app_config::AppConfigOption::save_rtp_dumps:
            result.add(option_name(key), is_save_rtp_dumps_enabled());
            break;
        case app_config::AppConfigOption::gdpr_agreement_reported_to_server:
            result.add(option_name(key), gdpr_agreement_reported_to_server());
            break;
        case app_config::AppConfigOption::gdpr_user_has_agreed:
            result.add(option_name(key), gdpr_user_has_agreed());
            break;
        case app_config::AppConfigOption::gdpr_user_has_logged_in_ever:
            result.add(option_name(key), gdpr_user_has_logged_in_ever());
            break;
        case app_config::AppConfigOption::cache_history_pages_secs:
            result.add(option_name(key), cache_history_pages_secs());
            break;
        case app_config::AppConfigOption::cache_history_pages_check_interval_secs:
            result.add(option_name(key), cache_history_pages_check_interval_secs());
            break;
        case app_config::AppConfigOption::is_server_search_enabled:
            result.add(option_name(key), is_server_search_enabled());
            break;
        case app_config::AppConfigOption::is_updateble:
            result.add(option_name(key), is_updateble());
            break;
        case app_config::AppConfigOption::dev_id:
            result.add(option_name(key), device_id());
            break;
        case app_config::AppConfigOption::update_interval:
            result.add(option_name(key), update_interval());
            break;
        case app_config::AppConfigOption::curl_log:
            result.add(option_name(key), is_curl_log_enabled());
            break;
        case app_config::AppConfigOption::task_trace:
            result.add(option_name(key), is_task_trace_enabled());
            break;
        case app_config::AppConfigOption::hide_keyword_pattern:
            result.add(option_name(key), is_hide_keyword_pattern());
            break;
        case app_config::AppConfigOption::show_hidden_themes:
            result.add(option_name(key), is_show_hidden_themes());
            break;
        case app_config::AppConfigOption::sys_crash_handler_enabled:
            result.add(option_name(key), is_sys_crash_handler_enabled());
            break;
        case app_config::AppConfigOption::watch_gui_memory:
            result.add(option_name(key), is_watch_gui_memory_enabled());
            break;
        case app_config::AppConfigOption::app_update_interval_secs:
            result.add(option_name(key), app_update_interval_secs());
            break;
        case app_config::AppConfigOption::net_compression:
            result.add(option_name(key), is_net_compression_enabled());
            break;
        case app_config::AppConfigOption::ssl_verification_enabled:
            result.add(option_name(key), is_ssl_verification_enabled());
            break;
        default:
            im_assert(!"unhandled option for as_ptree");
            continue;
        }
    }

    return result;
}

bool app_config::is_crash_enabled() const
{
    auto it = app_config_options_.find(app_config::AppConfigOption::is_crash_enabled);
    return it == app_config_options_.end() ? false
                                           : boost::any_cast<bool>(it->second);
}

bool app_config::is_testing_enabled() const
{
    auto it = app_config_options_.find(app_config::AppConfigOption::is_testing_enabled);
    return it == app_config_options_.end() ? false
                                           : boost::any_cast<bool>(it->second);
}

bool app_config::is_full_log_enabled() const
{
    auto it = app_config_options_.find(app_config::AppConfigOption::full_log);
    return it == app_config_options_.end() ? false
                                           : boost::any_cast<bool>(it->second);
}

bool app_config::is_curl_log_enabled() const
{
    auto it = app_config_options_.find(app_config::AppConfigOption::curl_log);
    return it == app_config_options_.end() ? false
        : boost::any_cast<bool>(it->second);
}

bool app_config::is_show_msg_ids_enabled() const
{
    auto it = app_config_options_.find(app_config::AppConfigOption::show_msg_ids);
    return it == app_config_options_.end() ? false
                                           : boost::any_cast<bool>(it->second);
}

bool app_config::is_server_history_enabled() const
{
    auto it = app_config_options_.find(app_config::AppConfigOption::disable_server_history);
    return it == app_config_options_.end() ? true
                                           : !(boost::any_cast<bool>(it->second)); /* disable -> enable */
}

bool app_config::is_server_search_enabled() const
{
    auto it = app_config_options_.find(app_config::AppConfigOption::is_server_search_enabled);
    return it == app_config_options_.end() ? true
        : boost::any_cast<bool>(it->second);
}

bool app_config::unlock_context_menu_features() const
{
    auto it = app_config_options_.find(app_config::AppConfigOption::unlock_context_menu_features);
    return it == app_config_options_.end() ? false
                                           : boost::any_cast<bool>(it->second);
}

bool app_config::is_task_trace_enabled() const
{
    auto it = app_config_options_.find(app_config::AppConfigOption::task_trace);
    return it == app_config_options_.end() ? false
                                           : boost::any_cast<bool>(it->second);
}

bool app_config::is_hide_keyword_pattern() const
{
    auto it = app_config_options_.find(app_config::AppConfigOption::hide_keyword_pattern);
    return it == app_config_options_.end() ? false
                                           : boost::any_cast<bool>(it->second);
}

bool app_config::is_show_hidden_themes() const
{
    auto it = app_config_options_.find(app_config::AppConfigOption::show_hidden_themes);
    return it == app_config_options_.end() ? false
                                           : boost::any_cast<bool>(it->second);
}

bool app_config::is_sys_crash_handler_enabled() const
{
    auto it = app_config_options_.find(app_config::AppConfigOption::sys_crash_handler_enabled);
    return it == app_config_options_.end() ? false
                                           : boost::any_cast<bool>(it->second);
}

bool app_config::is_watch_gui_memory_enabled() const
{
    auto it = app_config_options_.find(app_config::AppConfigOption::watch_gui_memory);
    return it == app_config_options_.end() ? false
                                           : boost::any_cast<bool>(it->second);
}

bool app_config::is_net_compression_enabled() const
{
    auto it = app_config_options_.find(app_config::AppConfigOption::net_compression);
    return it == app_config_options_.end() ? true
                                           : boost::any_cast<bool>(it->second);
}

bool app_config::gdpr_user_has_agreed() const
{
    auto it = app_config_options_.find(app_config::AppConfigOption::gdpr_user_has_agreed);
    return it == app_config_options_.end() ? false
                                           : boost::any_cast<bool>(it->second);
}

int32_t app_config::gdpr_agreement_reported_to_server() const
{
    auto it = app_config_options_.find(app_config::AppConfigOption::gdpr_agreement_reported_to_server);
    return it == app_config_options_.end() ? static_cast<int32_t>(gdpr_report_to_server_state::not_sent)
                                           : boost::any_cast<int32_t>(it->second);
}

bool app_config::gdpr_user_has_logged_in_ever() const
{
    auto it = app_config_options_.find(app_config::AppConfigOption::gdpr_user_has_logged_in_ever);
    return it == app_config_options_.end() ? false
                                           : boost::any_cast<bool>(it->second);
}

bool app_config::is_updateble() const
{
    auto it = app_config_options_.find(app_config::AppConfigOption::is_updateble);
    return it == app_config_options_.end() ? false
        : boost::any_cast<bool>(it->second);
}

int32_t app_config::forced_dpi() const
{
    auto it = app_config_options_.find(app_config::AppConfigOption::forced_dpi);
    return it == app_config_options_.end() ? 0
                                           : boost::any_cast<int32_t>(it->second);
}

bool app_config::is_save_rtp_dumps_enabled() const
{
    auto it = app_config_options_.find(app_config::AppConfigOption::save_rtp_dumps);
    return it == app_config_options_.end() ? false
        : boost::any_cast<bool>(it->second);
}

bool app_config::is_ssl_verification_enabled() const
{
    auto it = app_config_options_.find(app_config::AppConfigOption::ssl_verification_enabled);
    return it == app_config_options_.end() ? true
        : boost::any_cast<bool>(it->second);
}

int app_config::cache_history_pages_secs() const
{
    auto it = app_config_options_.find(app_config::AppConfigOption::cache_history_pages_secs);
    return it == app_config_options_.end() ? 0
                                           : boost::any_cast<int>(it->second);
}

int app_config::cache_history_pages_check_interval_secs() const
{
    auto it = app_config_options_.find(app_config::AppConfigOption::cache_history_pages_check_interval_secs);
    return it == app_config_options_.end() ? 0
                                           : boost::any_cast<int>(it->second);
}

uint32_t app_config::app_update_interval_secs() const
{
    auto it = app_config_options_.find(app_config::AppConfigOption::app_update_interval_secs);
    return it == app_config_options_.end() ? 0
                                           : boost::any_cast<uint32_t>(it->second);
}

std::string app_config::device_id() const
{
    auto it = app_config_options_.find(app_config::AppConfigOption::dev_id);
    return it == app_config_options_.end() ? std::string()
                                           : boost::any_cast<std::string>(it->second);
}

std::string_view app_config::get_update_win_alpha_url() const
{
    return config::get().url(config::urls::update_win_alpha);
}

std::string_view app_config::get_update_win_beta_url() const
{
    return config::get().url(config::urls::update_win_beta);
}

std::string_view app_config::get_update_win_release_url() const
{
    return config::get().url(config::urls::update_win_release);
}

std::string_view app_config::get_update_mac_alpha_url() const
{
    return config::get().url(config::urls::update_mac_alpha);
}

std::string_view app_config::get_update_mac_beta_url() const
{
    return config::get().url(config::urls::update_mac_beta);
}

std::string_view app_config::get_update_mac_release_url() const
{
    return config::get().url(config::urls::update_mac_release);
}

std::string_view app_config::get_update_linux_alpha_32_url() const
{
    return config::get().url(config::urls::update_linux_alpha_32);
}

std::string_view app_config::get_update_linux_alpha_64_url() const
{
    return config::get().url(config::urls::update_linux_alpha_64);
}

std::string_view app_config::get_update_linux_beta_32_url() const
{
    return config::get().url(config::urls::update_linux_beta_32);
}

std::string_view app_config::get_update_linux_beta_64_url() const
{
    return config::get().url(config::urls::update_linux_beta_64);
}

std::string_view app_config::get_update_linux_release_32_url() const
{
    return config::get().url(config::urls::update_linux_release_32);
}

std::string_view app_config::get_update_linux_release_64_url() const
{
    return config::get().url(config::urls::update_linux_release_64);
}

std::string_view app_config::get_stat_base_url() const
{
    return config::get().url(config::urls::stat_base);
}

std::string_view app_config::get_url_omicron_data() const
{
    return config::get().url(config::urls::omicron);
}

std::string_view app_config::get_url_attach_phone() const
{
    return config::get().url(config::urls::attach_phone);
}

std::string app_config::get_update_url(std::string_view _updateble_build_version) const
{
    const auto make_url = [version = _updateble_build_version](std::string_view url)
    {
        return su::concat("https://", url, version);
    };

    if constexpr (platform::is_windows())
    {
        if constexpr (environment::is_alpha())
            return make_url(get_update_win_alpha_url());

        if (g_core && g_core->get_install_beta_updates())
            return make_url(get_update_win_beta_url());

        return make_url(get_update_win_release_url());
    }
    else if constexpr (platform::is_x86_64())
    {
        if (environment::is_alpha())
            return make_url(get_update_linux_alpha_64_url());

        if (g_core && g_core->get_install_beta_updates())
            return make_url(get_update_linux_beta_64_url());

        return make_url(get_update_linux_release_64_url());
    }
    else
    {
        if constexpr (environment::is_alpha())
            return make_url(get_update_linux_alpha_32_url());

        if (g_core && g_core->get_install_beta_updates())
            return make_url(get_update_linux_beta_32_url());

        return make_url(get_update_linux_release_32_url());
    }
}

uint32_t app_config::update_interval() const
{
    auto it = app_config_options_.find(app_config::AppConfigOption::update_interval);
    return it == app_config_options_.end() ? 0
        : boost::any_cast<uint32_t>(it->second);
}

template <typename ValueType>
void app_config::set_config_option(app_config::AppConfigOption option, ValueType&& value)
{
    app_config_options_[option] = std::forward<ValueType>(value);
}

void app_config::serialize(Out core::coll_helper& _collection) const
{
    _collection.set<bool>("history.is_server_history_enabled", is_server_history_enabled());
    _collection.set<bool>(option_name(app_config::AppConfigOption::unlock_context_menu_features), unlock_context_menu_features());
    _collection.set<bool>(option_name(app_config::AppConfigOption::is_crash_enabled), is_crash_enabled());
    _collection.set<bool>(option_name(app_config::AppConfigOption::is_testing_enabled), is_testing_enabled());
    _collection.set<bool>(option_name(app_config::AppConfigOption::full_log), is_full_log_enabled());
    _collection.set<bool>(option_name(app_config::AppConfigOption::show_msg_ids), is_show_msg_ids_enabled());
    _collection.set<int>(option_name(app_config::AppConfigOption::cache_history_pages_secs), cache_history_pages_secs());
    _collection.set<int>(option_name(app_config::AppConfigOption::cache_history_pages_check_interval_secs), cache_history_pages_check_interval_secs());
    _collection.set<bool>(option_name(app_config::AppConfigOption::save_rtp_dumps), is_save_rtp_dumps_enabled());
    _collection.set<bool>(option_name(app_config::AppConfigOption::is_server_search_enabled), is_server_search_enabled());
    _collection.set<bool>(option_name(app_config::AppConfigOption::gdpr_user_has_agreed), gdpr_user_has_agreed());
    _collection.set<int32_t>(option_name(app_config::AppConfigOption::gdpr_agreement_reported_to_server),
                             static_cast<int32_t>(gdpr_agreement_reported_to_server()));
    _collection.set<bool>(option_name(app_config::AppConfigOption::gdpr_user_has_logged_in_ever), gdpr_user_has_logged_in_ever());
    _collection.set<bool>(option_name(app_config::AppConfigOption::is_updateble), is_updateble());
    _collection.set<std::string>(option_name(app_config::AppConfigOption::dev_id), device_id());
    _collection.set<uint32_t>(option_name(app_config::AppConfigOption::update_interval), update_interval());
    _collection.set<bool>(option_name(app_config::AppConfigOption::curl_log), is_curl_log_enabled());
    _collection.set<bool>(option_name(app_config::AppConfigOption::task_trace), is_task_trace_enabled());
    _collection.set<bool>(option_name(app_config::AppConfigOption::hide_keyword_pattern), is_hide_keyword_pattern());
    _collection.set<bool>(option_name(app_config::AppConfigOption::show_hidden_themes), is_show_hidden_themes());
    _collection.set<bool>(option_name(app_config::AppConfigOption::sys_crash_handler_enabled), is_sys_crash_handler_enabled());
    _collection.set<bool>(option_name(app_config::AppConfigOption::watch_gui_memory), is_watch_gui_memory_enabled());
    _collection.set<uint32_t>(option_name(app_config::AppConfigOption::app_update_interval_secs), app_update_interval_secs());
    _collection.set<bool>(option_name(app_config::AppConfigOption::net_compression), is_net_compression_enabled());
    _collection.set<bool>(option_name(app_config::AppConfigOption::ssl_verification_enabled), is_ssl_verification_enabled());

    // urls
    _collection.set<std::string_view>("urls.url_update_mac_alpha", get_update_mac_alpha_url());
    _collection.set<std::string_view>("urls.url_update_mac_beta", get_update_mac_beta_url());
    _collection.set<std::string_view>("urls.url_update_mac_release", get_update_mac_release_url());
    _collection.set<std::string_view>("urls.url_omicron_data", get_url_omicron_data());
    _collection.set<std::string_view>("urls.url_attach_phone", get_url_attach_phone());

    auto _forced_dpi = forced_dpi();
    if (_forced_dpi != 0)
    {
        im_assert(valid_dpi_values().count(_forced_dpi) > 0);
        _collection.set_value_as_int("gui.forced_dpi", _forced_dpi);
    }
}

const app_config& get_app_config()
{
    im_assert(config_);

    return *config_;
}

void load_app_config(const boost::filesystem::wpath& _path)
{
    im_assert(!config_);

    config_ = std::make_unique<app_config>();

    if (!core::tools::system::is_exist(_path))
        return;

    pt::ptree options;

    auto file = tools::system::open_file_for_read(_path.wstring());

    try
    {
        pt::ini_parser::read_ini(file, Out options);

        config_ = std::make_unique<app_config>(parse_ptree_into_map(options));
    }
    catch (const std::exception&)
    {

    }
#ifndef STRIP_CRASH_HANDLER
#ifdef _WIN32
    crash_system::reporter::instance().set_sys_handler_enabled(config_->is_sys_crash_handler_enabled());
#endif //_WIN32
#endif // !STRIP_CRASH_HANDLER
}

void dump_app_config_to_disk(const boost::filesystem::wpath& _path)
{
    im_assert(config_);
    if (!config_)
        return;

    if (!tools::system::is_exist(_path))
    {
        if (!tools::system::create_empty_file(_path.wstring()))
            return;
    }

    pt::ptree options = (*config_).as_ptree();

    try
    {
        pt::write_ini(_path.string(), options);
    }
    catch (const std::exception&)
    {
    }
}

template <typename ValueType>
void set_config_option(app_config::AppConfigOption option, ValueType&& value)
{
    config_->set_config_option(option, std::forward<ValueType>(value));
}

template void set_config_option<bool>(app_config::AppConfigOption, bool&&);
template void set_config_option<int32_t>(app_config::AppConfigOption, int32_t&&);
template void set_config_option<std::string>(app_config::AppConfigOption, std::string&&);

namespace
{
    app_config::AppConfigMap parse_ptree_into_map(const pt::ptree& property_tree_)
    {
        auto forced_dpi = property_tree_.get<int32_t>("gui.force_dpi", 0);
        if (valid_dpi_values().count(forced_dpi) == 0)
        {
            forced_dpi = 0;
        }

        return {
            {
                app_config::AppConfigOption::forced_dpi,
                forced_dpi
            },
            {
                app_config::AppConfigOption::disable_server_history,
                property_tree_.get<bool>(option_name(app_config::AppConfigOption::disable_server_history), false)
            },
            {
                app_config::AppConfigOption::is_crash_enabled,
                property_tree_.get<bool>(option_name(app_config::AppConfigOption::is_crash_enabled), false)
            },
            {
                app_config::AppConfigOption::is_testing_enabled,
                property_tree_.get<bool>(option_name(app_config::AppConfigOption::is_testing_enabled), false)
            },
            {
                app_config::AppConfigOption::full_log,
                property_tree_.get<bool>(option_name(app_config::AppConfigOption::full_log), false)
            },
            {
                app_config::AppConfigOption::unlock_context_menu_features,
                property_tree_.get<bool>(option_name(app_config::AppConfigOption::unlock_context_menu_features), ::build::is_debug())
            },
            {
                app_config::AppConfigOption::show_msg_ids,
                property_tree_.get<bool>(option_name(app_config::AppConfigOption::show_msg_ids), false)
            },
            {
                app_config::AppConfigOption::cache_history_pages_secs,
                property_tree_.get<int>(option_name(app_config::AppConfigOption::cache_history_pages_secs), 300)
            },
            {
                app_config::AppConfigOption::cache_history_pages_check_interval_secs,
                property_tree_.get<int>(option_name(app_config::AppConfigOption::cache_history_pages_check_interval_secs), 60)
            },
            {
                app_config::AppConfigOption::save_rtp_dumps,
                property_tree_.get<bool>(option_name(app_config::AppConfigOption::save_rtp_dumps), false)
            },
            {
                app_config::AppConfigOption::gdpr_user_has_agreed,
                property_tree_.get<bool>(option_name(app_config::AppConfigOption::gdpr_user_has_agreed), false)
            },
            {
                app_config::AppConfigOption::gdpr_agreement_reported_to_server,
                property_tree_.get<int32_t>(option_name(app_config::AppConfigOption::gdpr_agreement_reported_to_server),
                                            static_cast<int32_t>(app_config::gdpr_report_to_server_state::not_sent))
            },
            {
                app_config::AppConfigOption::gdpr_user_has_logged_in_ever,
                property_tree_.get<bool>(option_name(app_config::AppConfigOption::gdpr_user_has_logged_in_ever),
                                         false)
            },
            {
                app_config::AppConfigOption::is_server_search_enabled,
                property_tree_.get<bool>(option_name(app_config::AppConfigOption::is_server_search_enabled),
                                         true)
            },
            {
                app_config::AppConfigOption::is_updateble,
                property_tree_.get<bool>(option_name(app_config::AppConfigOption::is_updateble), true)
            },
            {
                app_config::AppConfigOption::dev_id,
                property_tree_.get<std::string>(option_name(app_config::AppConfigOption::dev_id), std::string())
            },
            {
                app_config::AppConfigOption::update_interval,
                property_tree_.get<uint32_t>(option_name(app_config::AppConfigOption::update_interval), 0)
            },
            {
                app_config::AppConfigOption::curl_log,
                property_tree_.get<bool>(option_name(app_config::AppConfigOption::curl_log), false)
            },
            {
                app_config::AppConfigOption::task_trace,
                property_tree_.get<bool>(option_name(app_config::AppConfigOption::task_trace), false)
            },
            {
                app_config::AppConfigOption::hide_keyword_pattern,
                property_tree_.get<bool>(option_name(app_config::AppConfigOption::hide_keyword_pattern), false)
            },
            {
                app_config::AppConfigOption::show_hidden_themes,
                property_tree_.get<bool>(option_name(app_config::AppConfigOption::show_hidden_themes), false)
            },
            {
                app_config::AppConfigOption::sys_crash_handler_enabled,
                property_tree_.get<bool>(option_name(app_config::AppConfigOption::sys_crash_handler_enabled), false)
            },
            {
                app_config::AppConfigOption::watch_gui_memory,
                property_tree_.get<bool>(option_name(app_config::AppConfigOption::watch_gui_memory), false)
            },
            {
                app_config::AppConfigOption::app_update_interval_secs,
                property_tree_.get<uint32_t>(option_name(app_config::AppConfigOption::app_update_interval_secs), 86400)
            },
            {
                app_config::AppConfigOption::net_compression,
                property_tree_.get<bool>(option_name(app_config::AppConfigOption::net_compression), true)
            }
            ,
            {
                app_config::AppConfigOption::ssl_verification_enabled,
                property_tree_.get<bool>(option_name(app_config::AppConfigOption::ssl_verification_enabled), true)
            }
        };
    }

    const char* option_name(const app_config::AppConfigOption option_)
    {
        switch (option_)
        {
        case app_config::AppConfigOption::forced_dpi:
            return "gui.force_dpi";
        case app_config::AppConfigOption::disable_server_history:
            return "disable_server_history";
        case app_config::AppConfigOption::is_crash_enabled:
            return "enable_crash";
        case app_config::AppConfigOption::is_testing_enabled:
            return "enable_testing";
        case app_config::AppConfigOption::full_log:
            return "fulllog";
        case app_config::AppConfigOption::unlock_context_menu_features:
            return "dev.unlock_context_menu_features";
        case app_config::AppConfigOption::show_msg_ids:
            return "dev.show_message_ids";
        case app_config::AppConfigOption::cache_history_pages_secs:
            return "dev.cache_history_pages_secs";
        case app_config::AppConfigOption::cache_history_pages_check_interval_secs:
            return "dev.cache_history_pages_check_interval_secs";
        case app_config::AppConfigOption::save_rtp_dumps:
            return "dev.save_rtp_dumps";
        case app_config::AppConfigOption::is_server_search_enabled:
            return "dev.server_search";
        case app_config::AppConfigOption::gdpr_user_has_agreed:
            return "gdpr.user_has_agreed";
        case app_config::AppConfigOption::gdpr_agreement_reported_to_server:
            return "gdpr.agreement_reported_to_server";
        case app_config::AppConfigOption::gdpr_user_has_logged_in_ever:
            return "gdpr.user_has_logged_in_ever";
        case app_config::AppConfigOption::is_updateble:
            return "updateble";
        case app_config::AppConfigOption::dev_id:
            return "dev_id";
        case app_config::AppConfigOption::update_interval:
            return "update_interval";
        case app_config::AppConfigOption::curl_log:
            return "curl_log";
        case app_config::AppConfigOption::task_trace:
            return "task_trace";
        case app_config::AppConfigOption::hide_keyword_pattern:
            return "dev.hide_keyword_pattern";
        case app_config::AppConfigOption::show_hidden_themes:
            return "show_hidden_themes";
        case app_config::AppConfigOption::sys_crash_handler_enabled:
            return "sys_crash_handler_enabled";
        case app_config::AppConfigOption::watch_gui_memory:
            return "dev.watch_gui_memory";
        case app_config::AppConfigOption::app_update_interval_secs:
            return "dev.app_update_interval_secs";
        case app_config::AppConfigOption::net_compression:
            return "dev.net_compression";
        case app_config::AppConfigOption::ssl_verification_enabled:
            return "dev.webview_ssl_check";
        default:
            im_assert(!"unhandled option for option_name");
            return "";
        }
    }

    const int_set& valid_dpi_values()
    {
        static int_set values = { 0, 100, 125, 150, 200 };
        return values;
    }
}

CORE_CONFIGURATION_NS_END
