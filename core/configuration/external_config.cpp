#include "stdafx.h"

#include "external_config.h"
#include "host_config.h"

#include "../common.shared/string_utils.h"
#include "../corelib/enumerations.h"
#include "tools/json_helper.h"
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

std::string external_url_config::make_url_preset(std::string_view _login_domain)
{
    std::string url;
    if (auto suggested_host = get().string(config::values::external_config_preset_url);
        !suggested_host.empty() && config::get().is_on(config::features::external_config_use_preset_url))
    {
        url = make_url(suggested_host, su::concat("domain=", _login_domain));
    }
    return url;
}

std::string external_url_config::make_url_auto_preset(std::string_view _login_domain, std::string_view _host)
{
    if (auto url_preset = make_url_preset(_login_domain); !url_preset.empty())
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

void external_url_config::load_async(std::string_view _url, load_callback_t _callback)
{
    auto fire_callback = [_callback = std::move(_callback)](core::ext_url_config_error _error, std::string _url) mutable
    {
        g_core->execute_core_context([_callback = std::move(_callback), _error, _url = std::move(_url)]() mutable { _callback(_error, std::move(_url)); });
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

    request->get_async([fire_callback = std::move(fire_callback), request, url = std::string(_url)](curl_easy::completion_code _code) mutable
    {
        if (_code != curl_easy::completion_code::success)
        {
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

        if (!external_url_config::instance().unserialize(doc))
        {
            fire_callback(ext_url_config_error::answer_not_enough_fields, std::move(url));
            return;
        }

        response->reset_out();
        response->save_2_file(config_filename());

        fire_callback(ext_url_config_error::ok, std::move(url));
    });
}

bool external_url_config::load_from_file()
{
    core::tools::binary_stream bstream;
    if (!bstream.load_from_file(config_filename()))
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
    if (const auto c = get_config(); c)
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
    if (const auto c = get_config(); c)
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
    override_features(config_->override_values_);

    return true;
}

void external_url_config::override_features(const features_vector& _values)
{
    if (_values.empty())
    {
        reset_to_defaults();
    }
    else
    {
        external_configuration conf;
        conf.features = _values;
        config::set_external(std::make_shared<external_configuration>(std::move(conf)));
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
        if (auto res = common::json::get_value<std::string_view>(_url_node, _node_name); res)
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

    const auto unserialize_feature = [this, &_node](auto feature, auto name)
    {
        if (auto enabled = common::json::get_value<bool>(_node, name); enabled)
            override_values_.emplace_back(feature, *enabled);
    };
    unserialize_feature(config::features::avatar_change_allowed, "allow-self-avatar-change");
    unserialize_feature(config::features::changeable_name, "allow-self-name-change");
    unserialize_feature(config::features::info_change_allowed, "allow-self-info-change");
    unserialize_feature(config::features::snippet_in_chat, "snippets-enabled");
    unserialize_feature(config::features::vcs_call_by_link_enabled, "allow-vcs-call-creation");
    unserialize_feature(config::features::vcs_webinar_enabled, "allow-vcs-webinar-creation");
    unserialize_feature(config::features::phone_allowed, "attach-phone-enabled");
    unserialize_feature(config::features::silent_message_delete, "silent-message-delete");

    return true;
}

