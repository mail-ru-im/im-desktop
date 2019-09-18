#include "stdafx.h"

#include <boost/range/adaptor/map.hpp>

#include "../../corelib/collection_helper.h"
#include "../tools/system.h"

#include "app_config.h"

#include "../../common.shared/constants.h"

CORE_CONFIGURATION_NS_BEGIN

namespace pt = boost::property_tree;

namespace
{
    std::unique_ptr<app_config> config_;

    const int_set& valid_dpi_values();
    app_config::AppConfigMap parse_ptree_into_map(const pt::ptree& property_tree_);
    const char* option_name(const app_config::AppConfigOption option_);

    enum class app_url_type
    {
        base = 0,
        files = 1,
        files_get = 2,
        profile_agent = 3,
        auth_mail_ru = 4,
        r_mail_ru = 5,
        win_mail_ru = 6,
        read_msg = 7,
        cicq_org = 8,
        cicq_com = 9,

        // add type before this place
        url_size
    };

    using urls_array = std::array<std::pair<app_url_type, std::string_view>, static_cast<size_t>(app_url_type::url_size)>;

    // use std::all_of since C++20
    template<class InputIt, class UnaryPredicate>
    static constexpr bool std_all_of(InputIt first, InputIt last, UnaryPredicate p)
    {
        for (; first != last; ++first)
            if (!p(*first))
                return false;

        return true;
    }

    template<class InputIt>
    static constexpr bool is_correct_order(InputIt first, InputIt last)  noexcept
    {
        for (int i = 0; first != last; ++first, ++i)
            if (i != static_cast<int>(first->first))
                return false;

        return true;
    }

    constexpr urls_array dit_default_urls() noexcept
    {
        constexpr urls_array default_app_urls = {
            std::pair(app_url_type::base, std::string_view("u.icxq.com")),
            std::pair(app_url_type::files, std::string_view("files.icxq.com")),
            std::pair(app_url_type::files_get, std::string_view("files-api.icxq.com")),
            std::pair(app_url_type::profile_agent, std::string_view("at.www.devmail.ru")),
            std::pair(app_url_type::auth_mail_ru, std::string_view("auth-mr.devel.email/cgi-bin/auth")),
            std::pair(app_url_type::r_mail_ru, std::string_view("r.devel.email")),
            std::pair(app_url_type::win_mail_ru, std::string_view("win.devel.email/cgi-bin/auth")),
            std::pair(app_url_type::read_msg, std::string_view("mra-mail.devel.email/cgi-bin/readmsg")),
            std::pair(app_url_type::cicq_org, std::string_view("cicq.org")),
            std::pair(app_url_type::cicq_com, std::string_view("c.icq.com"))
        };
        static_assert(default_app_urls.size() == static_cast<size_t>(app_url_type::url_size));
        static_assert(std_all_of(default_app_urls.cbegin(), default_app_urls.cend(), [](auto x) { return !x.second.empty(); }));
        static_assert(is_correct_order(default_app_urls.cbegin(), default_app_urls.cend()));

        return default_app_urls;
    }

    constexpr urls_array icq_default_urls() noexcept
    {
        constexpr urls_array default_app_urls = {
            std::pair(app_url_type::base, std::string_view("u.icq.net")),
            std::pair(app_url_type::files, std::string_view("files.icq.com")),
            std::pair(app_url_type::files_get, std::string_view("files.icq.net")),
            std::pair(app_url_type::profile_agent, std::string_view("agent.mail.ru/profile")),
            std::pair(app_url_type::auth_mail_ru, std::string_view("auth.mail.ru/cgi-bin/auth")),
            std::pair(app_url_type::r_mail_ru, std::string_view("r.mail.ru")),
            std::pair(app_url_type::win_mail_ru, std::string_view("win.mail.ru/cgi-bin/auth")),
            std::pair(app_url_type::read_msg, std::string_view("mra-mail.mail.ru/cgi-bin/readmsg")),
            std::pair(app_url_type::cicq_org, std::string_view("cicq.org")),
            std::pair(app_url_type::cicq_com, std::string_view("c.icq.com")),
        };
        static_assert(default_app_urls.size() == static_cast<size_t>(app_url_type::url_size));
        static_assert(std_all_of(default_app_urls.cbegin(), default_app_urls.cend(), [](auto x) { return !x.second.empty(); }));
        static_assert(is_correct_order(default_app_urls.cbegin(), default_app_urls.cend()));

        return default_app_urls;
    }

    constexpr urls_array biz_default_urls() noexcept
    {
        constexpr urls_array default_app_urls = {
            std::pair(app_url_type::base, std::string_view("u.myteam.vmailru.net")),
            std::pair(app_url_type::files, std::string_view("files.icq.com")),
            std::pair(app_url_type::files_get, std::string_view("files.icq.net")),
            std::pair(app_url_type::profile_agent, std::string_view("agent.mail.ru/profile")),
            std::pair(app_url_type::auth_mail_ru, std::string_view("auth.mail.ru/cgi-bin/auth")),
            std::pair(app_url_type::r_mail_ru, std::string_view("r.mail.ru")),
            std::pair(app_url_type::win_mail_ru, std::string_view("win.mail.ru/cgi-bin/auth")),
            std::pair(app_url_type::read_msg, std::string_view("mra-mail.mail.ru/cgi-bin/readmsg")),
            std::pair(app_url_type::cicq_org, std::string_view("myteam.mail.ru")),
            std::pair(app_url_type::cicq_com, std::string_view("c.icq.com")),
        };
        static_assert(default_app_urls.size() == static_cast<size_t>(app_url_type::url_size));
        static_assert(std_all_of(default_app_urls.cbegin(), default_app_urls.cend(), [](auto x) { return !x.second.empty(); }));
        static_assert(is_correct_order(default_app_urls.cbegin(), default_app_urls.cend()));

        return default_app_urls;
    }

    constexpr std::string_view get_default_app_url(app_url_type _type) noexcept
    {
        constexpr auto urls = build::is_dit() ? dit_default_urls() : (build::is_biz() ? biz_default_urls() : icq_default_urls());

        return urls[static_cast<size_t>(_type)].second;
    }
}

app_config::app_config()
    : app_config_options_(parse_ptree_into_map(pt::ptree())) /* populate with defaults */
{

}

app_config::app_config(app_config::AppConfigMap _options)
    : app_config_options_(std::move(_options))
{
    assert(app_config_options_.find(app_config::AppConfigOption::forced_dpi) != app_config_options_.end());
    assert(valid_dpi_values().count(
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
        case app_config::AppConfigOption::url_base:
            result.add(option_name(key), get_url_base());
            break;
        case app_config::AppConfigOption::url_files:
            result.add(option_name(key), get_url_files());
            break;
        case app_config::AppConfigOption::url_files_get:
            result.add(option_name(key), get_url_files_get());
            break;
        case app_config::AppConfigOption::url_profile_agent:
            result.add(option_name(key), get_url_profile_agent());
            break;
        case app_config::AppConfigOption::url_auth_mail_ru:
            result.add(option_name(key), get_url_auth_mail_ru());
            break;
        case app_config::AppConfigOption::url_r_mail_ru:
            result.add(option_name(key), get_url_r_mail_ru());
            break;
        case app_config::AppConfigOption::url_win_mail_ru:
            result.add(option_name(key), get_url_win_mail_ru());
            break;
        case app_config::AppConfigOption::url_read_msg:
            result.add(option_name(key), get_url_read_msg());
            break;
        case app_config::AppConfigOption::url_cicq_org:
            result.add(option_name(key), get_url_cicq_org());
            break;
        case app_config::AppConfigOption::url_cicq_com:
            result.add(option_name(key), get_url_cicq_com());
            break;
        default:
            assert(!"unhandled option for as_ptree");
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

int app_config::cache_history_pages_secs() const
{
    auto it = app_config_options_.find(app_config::AppConfigOption::cache_history_pages_secs);
    return it == app_config_options_.end() ? 0
                                           : boost::any_cast<int>(it->second);
}

std::string app_config::device_id() const
{
    auto it = app_config_options_.find(app_config::AppConfigOption::dev_id);
    return it == app_config_options_.end() ? std::string()
        : boost::any_cast<std::string>(it->second);
}

std::string app_config::get_url_base() const
{
    auto it = app_config_options_.find(app_config::AppConfigOption::url_base);
    return it == app_config_options_.end() ? std::string(get_default_app_url(app_url_type::base))
        : boost::any_cast<std::string>(it->second);
}

std::string app_config::get_url_files() const
{
    auto it = app_config_options_.find(app_config::AppConfigOption::url_files);
    return it == app_config_options_.end() ? std::string(get_default_app_url(app_url_type::files))
        : boost::any_cast<std::string>(it->second);
}

std::string app_config::get_url_files_get() const
{
    auto it = app_config_options_.find(app_config::AppConfigOption::url_files_get);
    return it == app_config_options_.end() ? std::string(get_default_app_url(app_url_type::files_get))
        : boost::any_cast<std::string>(it->second);
}

std::string app_config::get_url_profile_agent() const
{
    auto it = app_config_options_.find(app_config::AppConfigOption::url_profile_agent);
    return it == app_config_options_.end() ? std::string(get_default_app_url(app_url_type::profile_agent))
        : boost::any_cast<std::string>(it->second);
}

std::string app_config::get_url_auth_mail_ru() const
{
    auto it = app_config_options_.find(app_config::AppConfigOption::url_auth_mail_ru);
    return it == app_config_options_.end() ? std::string(get_default_app_url(app_url_type::auth_mail_ru))
        : boost::any_cast<std::string>(it->second);
}

std::string app_config::get_url_r_mail_ru() const
{
    auto it = app_config_options_.find(app_config::AppConfigOption::url_r_mail_ru);
    return it == app_config_options_.end() ? std::string(get_default_app_url(app_url_type::r_mail_ru))
        : boost::any_cast<std::string>(it->second);
}

std::string app_config::get_url_win_mail_ru() const
{
    auto it = app_config_options_.find(app_config::AppConfigOption::url_win_mail_ru);
    return it == app_config_options_.end() ? std::string(get_default_app_url(app_url_type::win_mail_ru))
        : boost::any_cast<std::string>(it->second);
}

std::string app_config::get_url_read_msg() const
{
    auto it = app_config_options_.find(app_config::AppConfigOption::url_read_msg);
    return it == app_config_options_.end() ? std::string(get_default_app_url(app_url_type::read_msg))
        : boost::any_cast<std::string>(it->second);
}

std::string app_config::get_url_cicq_org() const
{
    auto it = app_config_options_.find(app_config::AppConfigOption::url_cicq_org);
    return it == app_config_options_.end() ? std::string(get_default_app_url(app_url_type::cicq_org))
        : boost::any_cast<std::string>(it->second);
}

std::string app_config::get_url_cicq_com() const
{
    auto it = app_config_options_.find(app_config::AppConfigOption::url_cicq_com);
    return it == app_config_options_.end() ? std::string(get_default_app_url(app_url_type::cicq_com))
        : boost::any_cast<std::string>(it->second);
}

std::string app_config::get_url_files_if_not_default() const
{
    auto url_files = get_url_files();
    if (url_files != get_default_app_url(app_url_type::files))
        return url_files;

    return std::string();
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

void app_config::serialize(Out core::coll_helper &_collection) const
{
    _collection.set<bool>("history.is_server_history_enabled", is_server_history_enabled());
    _collection.set<bool>("dev.unlock_context_menu_features", unlock_context_menu_features());
    _collection.set<bool>("enable_crash", is_crash_enabled());
    _collection.set<bool>("enable_testing", is_testing_enabled());
    _collection.set<bool>("fulllog", is_full_log_enabled());
    _collection.set<bool>("dev.show_message_ids", is_show_msg_ids_enabled());
    _collection.set<int>("dev.cache_history_pages_secs", cache_history_pages_secs());
    _collection.set<bool>("dev.save_rtp_dumps", is_save_rtp_dumps_enabled());
    _collection.set<bool>("dev.server_search", is_server_search_enabled());
    _collection.set<bool>(option_name(app_config::AppConfigOption::gdpr_user_has_agreed), gdpr_user_has_agreed());
    _collection.set<int32_t>(option_name(app_config::AppConfigOption::gdpr_agreement_reported_to_server),
                             static_cast<int32_t>(gdpr_agreement_reported_to_server()));
    _collection.set<bool>(option_name(app_config::AppConfigOption::gdpr_user_has_logged_in_ever), gdpr_user_has_logged_in_ever());
    _collection.set<bool>(option_name(app_config::AppConfigOption::is_updateble), is_updateble());
    _collection.set<std::string>(option_name(app_config::AppConfigOption::dev_id), device_id());
    _collection.set<uint32_t>(option_name(app_config::AppConfigOption::update_interval), update_interval());
    _collection.set<bool>(option_name(app_config::AppConfigOption::curl_log), is_curl_log_enabled());

    // urls
    _collection.set<std::string>(option_name(app_config::AppConfigOption::url_base), get_url_base());
    _collection.set<std::string>(option_name(app_config::AppConfigOption::url_files), get_url_files());
    _collection.set<std::string>(option_name(app_config::AppConfigOption::url_files_get), get_url_files_get());
    _collection.set<std::string>(option_name(app_config::AppConfigOption::url_profile_agent), get_url_profile_agent());
    _collection.set<std::string>(option_name(app_config::AppConfigOption::url_auth_mail_ru), get_url_auth_mail_ru());
    _collection.set<std::string>(option_name(app_config::AppConfigOption::url_r_mail_ru), get_url_r_mail_ru());
    _collection.set<std::string>(option_name(app_config::AppConfigOption::url_win_mail_ru), get_url_win_mail_ru());
    _collection.set<std::string>(option_name(app_config::AppConfigOption::url_read_msg), get_url_read_msg());
    _collection.set<std::string>(option_name(app_config::AppConfigOption::url_cicq_org), get_url_cicq_org());
    _collection.set<std::string>(option_name(app_config::AppConfigOption::url_cicq_com), get_url_cicq_com());

    auto _forced_dpi = forced_dpi();
    if (_forced_dpi != 0)
    {
        assert(valid_dpi_values().count(_forced_dpi) > 0);
        _collection.set_value_as_int("gui.forced_dpi", _forced_dpi);
    }
}

const app_config& get_app_config()
{
    assert(config_);

    return *config_;
}

void load_app_config(const boost::filesystem::wpath &_path)
{
    assert(!config_);

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
}

void dump_app_config_to_disk(const boost::filesystem::wpath &_path)
{
    assert(config_);
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
                app_config::AppConfigOption::url_base,
                property_tree_.get<std::string>(option_name(app_config::AppConfigOption::url_base), std::string(get_default_app_url(app_url_type::base)))
            },
            {
                app_config::AppConfigOption::url_files,
                property_tree_.get<std::string>(option_name(app_config::AppConfigOption::url_files), std::string(get_default_app_url(app_url_type::files)))
            },
            {
                app_config::AppConfigOption::url_files_get,
                property_tree_.get<std::string>(option_name(app_config::AppConfigOption::url_files_get), std::string(get_default_app_url(app_url_type::files_get)))
            },
            {
                app_config::AppConfigOption::url_profile_agent,
                property_tree_.get<std::string>(option_name(app_config::AppConfigOption::url_profile_agent), std::string(get_default_app_url(app_url_type::profile_agent)))
            },
            {
                app_config::AppConfigOption::url_auth_mail_ru,
                property_tree_.get<std::string>(option_name(app_config::AppConfigOption::url_auth_mail_ru), std::string(get_default_app_url(app_url_type::auth_mail_ru)))
            },
            {
                app_config::AppConfigOption::url_r_mail_ru,
                property_tree_.get<std::string>(option_name(app_config::AppConfigOption::url_r_mail_ru), std::string(get_default_app_url(app_url_type::r_mail_ru)))
            },
            {
                app_config::AppConfigOption::url_win_mail_ru,
                property_tree_.get<std::string>(option_name(app_config::AppConfigOption::url_win_mail_ru), std::string(get_default_app_url(app_url_type::win_mail_ru)))
            },
            {
                app_config::AppConfigOption::url_read_msg,
                property_tree_.get<std::string>(option_name(app_config::AppConfigOption::url_read_msg), std::string(get_default_app_url(app_url_type::read_msg)))
            },
            {
                app_config::AppConfigOption::url_cicq_org,
                property_tree_.get<std::string>(option_name(app_config::AppConfigOption::url_cicq_org), std::string(get_default_app_url(app_url_type::cicq_org)))
            },
            {
                app_config::AppConfigOption::url_cicq_com,
                property_tree_.get<std::string>(option_name(app_config::AppConfigOption::url_cicq_com), std::string(get_default_app_url(app_url_type::cicq_com)))
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
        case app_config::AppConfigOption::url_base:
            return "urls.url_base";
        case app_config::AppConfigOption::url_files:
            return "urls.url_files";
        case app_config::AppConfigOption::url_files_get:
            return "urls.url_files_get";
        case app_config::AppConfigOption::url_profile_agent:
            return "urls.url_profile_agent";
        case app_config::AppConfigOption::url_auth_mail_ru:
            return "urls.url_auth_mail_ru";
        case app_config::AppConfigOption::url_r_mail_ru:
            return "urls.url_r_mail_ru";
        case app_config::AppConfigOption::url_win_mail_ru:
            return "urls.url_win_mail_ru";
        case app_config::AppConfigOption::url_read_msg:
            return "urls.url_read_msg";
        case app_config::AppConfigOption::url_cicq_org:
            return "urls.url_cicq_org";
        case app_config::AppConfigOption::url_cicq_com:
            return "urls.url_cicq_com";
        default:
            assert(!"unhandled option for option_name");
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