#include "stdafx.h"

#include "external_config.h"
#include "host_config.h"

#include "../common.shared/string_utils.h"
#include "../corelib/enumerations.h"
#include "../common.shared/json_helper.h"
#include "tools/system.h"
#include "tools/strings.h"
#include "tools/features.h"
#include "http_request.h"
#include "utils.h"
#include "core.h"

using namespace config;
using namespace hosts;
using namespace core;

namespace
{
    constexpr std::string_view json_name() noexcept { return "myteam-config.json"; };
    constexpr std::string_view develop_json_name() noexcept { return "develop-myteam-config.json"; };

    constexpr std::string_view platform_key_name() noexcept
    {
        if constexpr (platform::is_apple())
        {
            return "mac_x64";
        }
        else if constexpr (platform::is_windows())
        {
            return "win_x32";
        }
        else if constexpr (platform::is_linux())
        {
            if constexpr (platform::is_x86_64())
                return "linux_x64";
            else
                return "linux_x32";
        }
        return "unknown";
    };

    std::wstring config_filename()
    {
        return su::wconcat(core::utils::get_product_data_path(), L'/', core::tools::from_utf8(json_name()));
    }

    std::wstring develop_config_filename()
    {
        return su::wconcat(core::utils::get_product_data_path(), L'/', core::tools::from_utf8(develop_json_name()));
    }
}

external_url_config::external_url_config() = default;

external_url_config::~external_url_config() = default;

std::string_view external_url_config::extract_host(std::string_view _host)
{
    if (_host.empty())
        return {};

    if (_host.back() == '/')
        _host.remove_suffix(1);

    if (auto idx = _host.find("://"); idx != std::string_view::npos)
        _host.remove_prefix(idx + 3);

    _host = tools::trim_left_copy(_host, " \n\r\t");
    _host = tools::trim_right_copy(_host, " \n\r\t");

    return _host;
}

std::string external_url_config::make_url(std::string_view _domain, std::string_view _query)
{
    if (!_query.empty())
        return su::concat("https://", _domain, '/', json_name(), '?', _query);
    return su::concat("https://", _domain, '/', json_name());
}

std::string external_url_config::make_url_preset(std::string_view _login_domain, bool _has_domain)
{
    std::string url;

    if (auto suggested_host = get().string(config::values::external_config_preset_url);
        !suggested_host.empty() && !_has_domain)
    {
        url = make_url(suggested_host, su::concat("domain=", _login_domain));
    }
    return url;
}

std::string external_url_config::make_url_auto_preset(std::string_view _login_domain, std::string_view _host, bool _has_domain)
{
    if (auto url_preset = make_url_preset(_login_domain, _has_domain); !url_preset.empty())
        return url_preset;

    if (!_host.empty())
        return make_url(_host, su::concat("domain=", _login_domain));

    return make_url(_login_domain, su::concat("domain=", _login_domain));
}

external_url_config& external_url_config::instance()
{
    static external_url_config c;
    return c;
}

void external_url_config::load_async(std::string_view _url, load_callback_t _callback, int _retry_count)
{
    auto fire_callback = [_callback = std::move(_callback)](core::ext_url_config_error _error, std::string _url) mutable
    {
        g_core->execute_core_context({ [_callback = std::move(_callback), _error, _url = std::move(_url)] () mutable { _callback(_error, std::move(_url)); } });
    };

    if (_url.empty())
    {
        fire_callback(ext_url_config_error::config_host_invalid, std::string());
        return;
    }

    g_core->write_string_to_network_log(su::concat("host config: downloading from ", _url, "\r\n"));

    auto request = std::make_shared<http_request_simple>(g_core->get_proxy_settings(), utils::get_user_agent(), default_priority());
    request->set_url(_url);
    request->set_send_im_stats(false);
    request->set_normalized_url("ext_url_cfg");
    request->set_use_curl_decompresion(true);

    request->get_async([fire_callback = std::move(fire_callback), request, url = std::string(_url), use_develop_local_config = use_develop_local_config_, _retry_count, this](curl_easy::completion_code _code) mutable
    {
        if (_code != curl_easy::completion_code::success)
        {
            if (_code != curl_easy::completion_code::cancelled && _retry_count > 0 && g_core->try_to_apply_alternative_settings())
                return load_async(std::move(url), std::move(fire_callback), _retry_count - 1);

            fire_callback(ext_url_config_error::config_host_invalid, std::move(url));
            return;
        }

        if (request->get_response_code() != 200)
        {
            fire_callback(ext_url_config_error::invalid_http_code, std::move(url));
            return;
        }

        std::string_view data;
        auto response = std::dynamic_pointer_cast<tools::binary_stream>(request->get_response());
        if (response)
        {
            if (const auto size = response->available())
                data = std::string_view(response->read_available(), size);
        }

        if (data.empty())
        {
            fire_callback(ext_url_config_error::empty_response, std::move(url));
            return;
        }

        rapidjson::Document doc;
        if (doc.Parse(data.data(), data.size()).HasParseError())
        {
            fire_callback(ext_url_config_error::answer_parse_error, std::move(url));
            return;
        }

        if (!use_develop_local_config && !external_url_config::instance().unserialize(doc))
        {
            fire_callback(ext_url_config_error::answer_not_enough_fields, std::move(url));
            return;
        }

        response->reset_out();
        response->save_2_file(config_filename());
        external_config_loaded_ = true;

        fire_callback(ext_url_config_error::ok, std::move(url));
    });
}

bool external_url_config::load_from_file()
{
    core::tools::binary_stream bstream;

    use_develop_local_config_ = environment::is_develop() && bstream.load_from_file(develop_config_filename());

    if (!use_develop_local_config_ && !bstream.load_from_file(config_filename()))
        return false;

    bstream.write<char>('\0');
    bstream.reset_out();

    rapidjson::Document doc;
    if (doc.ParseInsitu((char*)bstream.read(bstream.available())).HasParseError())
        return false;

    if (!unserialize(doc))
        return false;

    return true;
}

std::string external_url_config::get_host(host_url_type _type) const
{
    if (const auto c = get_config())
        return std::string(c->get_host(_type));

    return {};
}

std::vector<std::string> config::hosts::external_url_config::get_vcs_urls() const
{
    if (const auto c = get_config(); c)
        return c->vcs_rooms_;
    return {};
}

void external_url_config::clear()
{
    std::scoped_lock lock(spin_lock_);
    config_ = {};
    reset_to_defaults();

    tools::system::delete_file(config_filename());
}

host_cache config::hosts::external_url_config::get_cache() const
{
    if (const auto c = get_config())
        return c->cache_;
    return {};
}

bool config::hosts::external_url_config::is_valid() const
{
    return static_cast<bool>(get_config());
}

bool external_url_config::unserialize(const rapidjson::Value& _node)
{
    config_p new_config;
    if (!new_config.unserialize(_node))
        return false;

    auto c = std::make_shared<config_p>(std::move(new_config));

    std::scoped_lock lock(spin_lock_);
    config_ = std::move(c);
    override_config(config_->override_features_, config_->override_values_);

    return true;
}

void external_url_config::override_config(const features_vector& _features, const values_vector& _values)
{
    if (_values.empty() && _features.empty())
    {
        reset_to_defaults();
    }
    else
    {
        external_configuration conf;
        conf.features = _features;
        conf.values = _values;
        config::set_external(std::make_shared<external_configuration>(std::move(conf)), use_develop_local_config_);
    }
}

void external_url_config::reset_to_defaults()
{
    config::reset_external();
}

std::shared_ptr<external_url_config::config_p> external_url_config::get_config() const
{
    std::scoped_lock lock(spin_lock_);
    return config_;
}

std::string_view external_url_config::config_p::get_host(host_url_type _type) const
{
    const auto it = std::find_if(cache_.begin(), cache_.end(), [_type](const auto& x) { return x.first == _type; });
    if (it != cache_.end())
        return it->second;
    return {};
}

bool external_url_config::config_p::unserialize(const rapidjson::Value& _node)
{
    cache_.clear();
    auto get_url = [&cache_ = cache_](const auto& _url_node, auto _node_name, auto _type)
    {
        if (auto res = common::json::get_value<std::string_view>(_url_node, _node_name))
        {
            if (auto url = extract_host(*res); !url.empty())
            {
                cache_.emplace_back(_type , url);
                return true;
            }
        }
        return false;
    };

    {
        const auto iter_api = _node.FindMember("api-urls");
        if (iter_api == _node.MemberEnd() || !iter_api->value.IsObject())
            return false;
        if (!get_url(iter_api->value, "main-api", host_url_type::base))
            return false;
        if (!get_url(iter_api->value, "main-binary-api", host_url_type::base_binary))
            return false;
    }

    {
        const auto iter_templ = _node.FindMember("templates-urls");
        if (iter_templ == _node.MemberEnd() || !iter_templ->value.IsObject())
            return false;
        if (!get_url(iter_templ->value, "files-parsing", host_url_type::files_parse))
            return false;
        if (!get_url(iter_templ->value, "stickerpack-sharing", host_url_type::stickerpack_share))
            return false;
        get_url(iter_templ->value, "miniapp-sharing", host_url_type::miniapp_sharing);
        if (!get_url(iter_templ->value, "profile", host_url_type::profile))
            return false;

        vcs_rooms_.clear();
        const auto iter_vcs = iter_templ->value.FindMember("vcs-room");
        if (iter_vcs != iter_templ->value.MemberEnd() && iter_vcs->value.IsString())
        {
            std::istringstream iss(rapidjson_get_string(iter_vcs->value));
            std::string url;
            while (std::getline(iss, url, ';'))
                vcs_rooms_.emplace_back(std::exchange(url, {}));
        }

        // TODO: remove when deprecated
        get_url(iter_templ->value, "di", host_url_type::di);
        get_url(iter_templ->value, "di-dark", host_url_type::di_dark);
        get_url(iter_templ->value, "tasks", host_url_type::tasks);
        get_url(iter_templ->value, "calendar", host_url_type::calendar);
        get_url(iter_templ->value, "mail", host_url_type::mail);

        get_url(iter_templ->value, "tarm-mail", host_url_type::tarm_mail);
        get_url(iter_templ->value, "tarm-cloud", host_url_type::tarm_cloud);
        get_url(iter_templ->value, "tarm-calls", host_url_type::tarm_calls);
    }
    {
        const auto iter_mail = _node.FindMember("mail-interop");
        if (iter_mail != _node.MemberEnd() && iter_mail->value.IsObject())
        {
            if (!get_url(iter_mail->value, "mail-auth", host_url_type::mail_auth))
                return false;
            if (!get_url(iter_mail->value, "desktop-mail-redirect", host_url_type::mail_redirect))
                return false;
            if (!get_url(iter_mail->value, "desktop-mail", host_url_type::mail_win))
                return false;
            if (!get_url(iter_mail->value, "desktop-single-mail", host_url_type::mail_read))
                return false;
        }
    }

    if (::features::is_update_from_backend_enabled())
    {
        const auto iter_apps = _node.FindMember("apps");
        if (iter_apps != _node.MemberEnd() && iter_apps->value.IsObject())
        {
            const auto iter_platform = iter_apps->value.FindMember(tools::make_string_ref(platform_key_name()));
            if (iter_platform != iter_apps->value.MemberEnd() && iter_platform->value.IsObject())
                get_url(iter_platform->value, "url", host_url_type::app_update);
        }
    }

    override_values_.clear();

    const auto unserialize_feature_from_node = [this](const rapidjson::Value& _node, auto feature, auto name)
    {
        if (auto enabled = common::json::get_value<bool>(_node, name); enabled)
            override_features_.emplace_back(feature, *enabled);
    };

    const auto unserialize_feature = [&_node, &unserialize_feature_from_node](auto feature, auto name)
    {
        unserialize_feature_from_node(_node, feature, name);
    };

    const auto unserialize_value = [this, &_node](auto value, auto name)
    {
        if (auto val = common::json::get_value<std::string>(_node, name); val)
            override_values_.emplace_back(value, *val);
    };

    const auto unserialize_number = [this, &_node](auto value, auto name)
    {
        if (auto val = common::json::get_value<int64_t>(_node, name))
            override_values_.emplace_back(value, *val);
    };

    const auto node_to_config = [this](rapidjson::GenericStringRef<char> _name, auto _node, auto _value)
    {
        rapidjson::Document doc;
        doc.SetObject();
        auto& a = doc.GetAllocator();
        rapidjson::Value newNode(_node->value, a);
        doc.SetObject().AddMember(_name, newNode, a);
        rapidjson::StringBuffer sb;
        rapidjson::Writer writer(sb);
        doc.Accept(writer);
        override_values_.emplace_back(_value, sb.GetString());
    };

    unserialize_feature(config::features::avatar_change_allowed, "allow-self-avatar-change");
    unserialize_feature(config::features::changeable_name, "allow-self-name-change");
    unserialize_feature(config::features::allow_contacts_rename, "allow-contacts-rename");
    unserialize_feature(config::features::info_change_allowed, "allow-self-info-change");
    unserialize_feature(config::features::snippet_in_chat, "snippets-enabled");
    unserialize_feature(config::features::call_link_v2_enabled, "call-link-v2-enabled");
    unserialize_feature(config::features::vcs_call_by_link_enabled, "allow-vcs-call-creation");
    unserialize_feature(config::features::vcs_webinar_enabled, "allow-vcs-webinar-creation");
    unserialize_feature(config::features::phone_allowed, "attach-phone-enabled");
    unserialize_feature(config::features::silent_message_delete, "silent-message-delete");
    unserialize_feature(config::features::support_shared_federation_stickerpacks, "support-shared-federation-stickerpacks");
    unserialize_feature(config::features::smartreply_suggests_stickers, "smart-reply-stickers-enabled");
    unserialize_feature(config::features::smartreply_suggests_text, "smart-reply-text-enabled");
    unserialize_feature(config::features::restricted_files_enabled, "restricted-files-enabled");
    unserialize_feature(config::features::antivirus_check_enabled, "antivirus-check-enabled");
    unserialize_feature(config::features::threads_enabled, "threads-enabled");
    unserialize_feature(config::features::omicron_enabled, "omicron-enabled");
    unserialize_feature(config::features::statistics, "external-statistics-enabled");
    unserialize_feature(config::features::statistics_mytracker, "external-statistics-enabled");
    unserialize_feature(config::features::task_creation_in_chat_enabled, "task-creation-in-chat-enabled");
    unserialize_feature(config::features::hiding_message_info_enabled, "force-hide-notification-all-data");
    unserialize_feature(config::features::hiding_message_text_enabled, "force-hide-notification-text");
    unserialize_feature(config::features::delete_account_enabled, "delete-account-enabled");
    unserialize_value(config::values::status_banner_emoji_csv, "status-banner-emoji");
    unserialize_value(config::values::bots_commands_disabled, "bots-commands-disabled");
    unserialize_value(config::values::additional_theme, "additional-theme");
    unserialize_number(config::values::wim_parallel_packets_count, "wim-parallel-packets-count");
    unserialize_number(config::values::server_api_version, "api-version");
    unserialize_value(config::values::digital_assistant_bot_aimid, "digital-assistant-bot-aimid");
    unserialize_feature(config::features::digital_assistant_search_positioning, "digital-assistant-search-positioning");
    unserialize_feature(config::features::leading_last_name, "leading-last-name");
    unserialize_feature(config::features::report_messages_enabled, "report-messages-enabled");
    unserialize_feature(config::features::ptt_recognition, "ptt-recognition-enabled");

    bool service_apps_overridden = false;
    const auto iter_services = _node.FindMember("services");
    if (iter_services != _node.MemberEnd() && iter_services->value.IsObject())
    {
        const auto disposition = iter_services->value.FindMember("disposition");
        if (disposition != iter_services->value.MemberEnd() && disposition->value.IsObject())
        {
            const auto desktop = disposition->value.FindMember("desktop");
            if (desktop != disposition->value.MemberEnd() && desktop->value.IsObject())
            {
                const auto leftbar = desktop->value.FindMember("leftbar");
                if (leftbar != desktop->value.MemberEnd() && leftbar->value.IsArray())
                {
                    node_to_config("apps-order", leftbar, config::values::service_apps_order);
                    service_apps_overridden = true;
                }
            }
        }

        auto service_apps_config = iter_services->value.FindMember("config");
        if (service_apps_config != iter_services->value.MemberEnd() && service_apps_config->value.IsObject())
            node_to_config("apps-config", service_apps_config, config::values::service_apps_config);
    }

    const auto iter_custom_apps = _node.FindMember("custom-miniapps");
    {
        if (iter_custom_apps != _node.MemberEnd() && iter_custom_apps->value.IsArray())
            node_to_config("custom-miniapps", iter_custom_apps, config::values::custom_miniapps);
    }

    unserialize_feature(config::features::calendar_self_auth, "calendar-self-auth");
    unserialize_feature(config::features::mail_self_auth, "mail-self-auth");
    unserialize_feature(config::features::cloud_self_auth, "cloud-self-auth");

    // TODO: remove node parsing when deprecated
    if (!service_apps_overridden)
    {
        const auto iter_apps = _node.FindMember("mini-apps");
        if (iter_apps != _node.MemberEnd() && iter_apps->value.IsObject())
        {
            unserialize_feature_from_node(iter_apps->value, config::features::tasks_enabled, "tasks-enabled");
            unserialize_feature_from_node(iter_apps->value, config::features::organization_structure_enabled, "organization-structure-enabled");
            unserialize_feature_from_node(iter_apps->value, config::features::calendar_enabled, "calendar-enabled");
            unserialize_feature_from_node(iter_apps->value, config::features::mail_enabled, "mail-enabled");
            unserialize_feature_from_node(iter_apps->value, config::features::tarm_mail, "tarm-mail-enabled");
            unserialize_feature_from_node(iter_apps->value, config::features::tarm_cloud, "tarm-cloud-enabled");
            unserialize_feature_from_node(iter_apps->value, config::features::tarm_calls, "tarm-calls-enabled");
        }
    }

    // user agreement
    const auto user_agreement = _node.FindMember("user-agreement");
    if (user_agreement != _node.MemberEnd() && user_agreement->value.IsObject())
    {
        unserialize_feature_from_node(user_agreement->value, config::features::external_user_agreement, "enabled");
        const auto urls = user_agreement->value.FindMember("config");
        if (urls != _node.MemberEnd() && urls->value.IsObject())
        {
            get_url(urls->value, "privacy-policy-url", host_url_type::privacy_policy_url);
            get_url(urls->value, "terms-of-use-url", host_url_type::terms_of_use_url);
        }
    }

    // authorization
    const auto authentication = _node.FindMember("oauth-authorization");
    if (authentication != _node.MemberEnd() && authentication->value.IsObject())
    {
        unserialize_feature_from_node(authentication->value, config::features::login_by_oauth2_allowed, "enabled");

        const auto config_iter = authentication->value.FindMember("config");
        if (config_iter != _node.MemberEnd() && config_iter->value.IsObject())
        {
            get_url(config_iter->value, "oauth-url", host_url_type::oauth_url);
            get_url(config_iter->value, "token-url", host_url_type::token_url);
            get_url(config_iter->value, "redirect-uri", host_url_type::redirect_uri);
            unserialize_value(config::values::oauth_scope, "scope");
            unserialize_value(config::values::client_id, "client-id");
        }
    }

    // enable smartreply suggests for quotes if enabled for stickers and text
    const auto is_smartreply_suggests_stickers = std::any_of(override_features_.cbegin(), override_features_.cend(), [](const auto& x) { return x == std::pair(config::features::smartreply_suggests_stickers, true); });
    const auto is_smartreply_suggests_text = std::any_of(override_features_.cbegin(), override_features_.cend(), [](const auto& x) { return x == std::pair(config::features::smartreply_suggests_text, true); });
    if (is_smartreply_suggests_stickers && is_smartreply_suggests_text)
        override_features_.emplace_back(config::features::smartreply_suggests_for_quotes, true);

    return true;
}

