#include "stdafx.h"

#include "external_config.h"
#include "host_config.h"

#include "../common.shared/string_utils.h"
#include "../common.shared/config/config.h"
#include "../corelib/enumerations.h"
#include "tools/json_helper.h"
#include "tools/system.h"
#include "tools/strings.h"
#include "http_request.h"
#include "utils.h"
#include "core.h"

using namespace config;
using namespace hosts;
using namespace core;

namespace
{
    constexpr std::string_view json_name = "myteam-config.json";

    std::wstring config_filename()
    {
        return su::wconcat(core::utils::get_product_data_path(), L'/', core::tools::from_utf8(json_name));
    }

    std::vector<config::features> get_features()
    {
        return {
            config::features::avatar_change_allowed,
            config::features::changeable_name,
            config::features::info_change_allowed,
            config::features::snippet_in_chat,
            config::features::vcs_call_by_link_enabled,
            config::features::phone_allowed,
            config::features::vcs_webinar_enabled
        };
    }

    external_url_config::config_features make_defaults()
    {
        external_url_config::config_features res;
        for (auto f : get_features())
            res[f] = config::get().is_on(f);
        return res;
    }
}

external_url_config::external_url_config()
    : default_values_(make_defaults())
{
}

std::string_view external_url_config::extract_host(std::string_view _host)
{
    if (_host.empty())
        return std::string_view();

    if (_host.back() == '/')
        _host.remove_suffix(1);

    if (auto idx = _host.find("://"); idx != std::string_view::npos)
        _host.remove_prefix(idx + 3);

    tools::trim_left(_host, " \n\r\t");
    tools::trim_right(_host, " \n\r\t");

    return _host;
}

std::string external_url_config::make_url(std::string_view _domain, std::string_view _query)
{
    std::string query;
    if (!_query.empty())
        query = su::concat('?', _query);
    return su::concat("https://", _domain, '/', json_name, query);
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

external_url_config& external_url_config::instance()
{
    static external_url_config c;
    return c;
}

void external_url_config::load_async(std::string_view _url, load_callback_t _callback)
{
    auto fire_callback = [_callback = std::move(_callback)](core::ext_url_config_error _error, std::string _url) mutable
    {
        g_core->execute_core_context([_callback = std::move(_callback), _error, _url = std::move(_url)](){ _callback(_error, _url); });
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

    request->get_async([fire_callback, request, url = std::string(_url)](curl_easy::completion_code _code) mutable
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

    is_valid_ = true;
    return true;
}

std::string_view external_url_config::get_host(const host_url_type _type) const
{
    if (is_valid())
    {
        if (const auto iter_u = config_.cache_.find(_type); iter_u != config_.cache_.end())
            return iter_u->second;
    }

    assert(false);
    return std::string_view();
}

void external_url_config::clear()
{
    is_valid_ = false;
    config_.clear();
    reset_to_defaults(default_values_);

    tools::system::delete_file(config_filename());
}

bool external_url_config::unserialize(const rapidjson::Value& _node)
{
    config_p new_config;
    if (!new_config.unserialize(_node))
        return false;

    config_ = std::move(new_config);
    reset_to_defaults(default_values_);
    override_features(config_.override_values_);

    is_valid_ = true;
    return true;
}

void external_url_config::override_features(const config_features& _values)
{
    for (const auto& [feature, enabled] : _values)
    {
        assert(default_values_.find(feature) != default_values_.end());
        config::override_feature(feature, enabled);
    }
}

void external_url_config::reset_to_defaults(const config_features& _values)
{
    for (const auto& [feature, _] : _values)
    {
        assert(default_values_.find(feature) != default_values_.end());
        config::reset_feature_to_default(feature);
    }
}

bool external_url_config::config_p::unserialize(const rapidjson::Value& _node)
{
    const auto get_url = [](const auto& _url_node, std::string_view _node_name, std::string& _out)
    {
        if (!tools::unserialize_value(_url_node, _node_name, _out))
            return false;

        _out = std::string(extract_host(_out));
        return !_out.empty();
    };

    {
        const auto iter_api = _node.FindMember("api-urls");
        if (iter_api == _node.MemberEnd() || !iter_api->value.IsObject())
            return false;
        if (!get_url(iter_api->value, "main-api", cache_[host_url_type::base]))
            return false;
        if (!get_url(iter_api->value, "main-binary-api", cache_[host_url_type::base_binary]))
            return false;
    }
    {
        const auto iter_templ = _node.FindMember("templates-urls");
        if (iter_templ == _node.MemberEnd() || !iter_templ->value.IsObject())
            return false;
        if (!get_url(iter_templ->value, "files-parsing", cache_[host_url_type::files_parse]))
            return false;
        if (!get_url(iter_templ->value, "stickerpack-sharing", cache_[host_url_type::stickerpack_share]))
            return false;
        if (!get_url(iter_templ->value, "profile", cache_[host_url_type::profile]))
            return false;

        vcs_rooms_.clear();
        const auto iter_vcs = iter_templ->value.FindMember("vcs-room");
        if (iter_vcs != iter_templ->value.MemberEnd() && iter_vcs->value.IsString())
        {
            std::istringstream iss(rapidjson_get_string(iter_vcs->value));
            std::string url;
            while (std::getline(iss, url, ';'))
                vcs_rooms_.push_back(url);
        }
    }
    {
        const auto iter_mail = _node.FindMember("mail-interop");
        if (iter_mail != _node.MemberEnd() && iter_mail->value.IsObject())
        {
            if (!get_url(iter_mail->value, "mail-auth", cache_[host_url_type::mail_auth]))
                return false;
            if (!get_url(iter_mail->value, "desktop-mail-redirect", cache_[host_url_type::mail_redirect]))
                return false;
            if (!get_url(iter_mail->value, "desktop-mail", cache_[host_url_type::mail_win]))
                return false;
            if (!get_url(iter_mail->value, "desktop-single-mail", cache_[host_url_type::mail_read]))
                return false;
        }
    }

    const auto unserialize_feature = [this, &_node](auto feature, auto name)
    {
        if (bool enabled = false; tools::unserialize_value(_node, name, enabled))
            override_values_[feature] = enabled;
    };
    unserialize_feature(config::features::avatar_change_allowed, "allow-self-avatar-change");
    unserialize_feature(config::features::changeable_name, "allow-self-name-change");
    unserialize_feature(config::features::info_change_allowed, "allow-self-info-change");
    unserialize_feature(config::features::snippet_in_chat, "snippets-enabled");
    unserialize_feature(config::features::vcs_call_by_link_enabled, "allow-vcs-call-creation");
    unserialize_feature(config::features::vcs_webinar_enabled, "allow-vcs-webinar-creation");
    unserialize_feature(config::features::phone_allowed, "attach-phone-enabled");

    return true;
}

void external_url_config::config_p::clear()
{
    cache_.clear();
    vcs_rooms_.clear();
    override_values_.clear();
}
