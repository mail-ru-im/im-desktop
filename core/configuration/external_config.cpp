#include "stdafx.h"

#include "external_config.h"
#include "host_config.h"

#include "../common.shared/string_utils.h"
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
    std::string_view extract_host(std::string_view _host)
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

    constexpr std::string_view json_name = "myteam-config.json";
    std::wstring config_filename()
    {
        return su::wconcat(core::utils::get_product_data_path(), L'/', core::tools::from_utf8(json_name));
    }
}


ext_url_config_error external_url_config::load(std::string_view _host)
{
    clear();

    _host = extract_host(_host);
    if (_host.empty())
        return ext_url_config_error::config_host_invalid;

    const auto url = su::concat("https://", _host, '/', json_name);
    g_core->write_string_to_network_log(su::concat("host config: downloading from ", url, "\r\n"));

    http_request_simple request(g_core->get_proxy_settings(), utils::get_user_agent());
    request.set_url(url);
    request.set_send_im_stats(false);
    request.set_normalized_url("ext_url_cfg");
    request.set_use_curl_decompresion(true);

    if (!request.get() || request.get_response_code() != 200)
        return ext_url_config_error::config_host_invalid;

    char* data = nullptr;
    auto response = std::dynamic_pointer_cast<tools::binary_stream>(request.get_response());
    if (response && response->available())
    {
        response->write(char(0));
        data = response->read_available();
    }

    if (!data)
        return ext_url_config_error::answer_parse_error;

    rapidjson::Document doc;
    if (doc.Parse(data).HasParseError()) // dont use ParseInsitu because we need unmodified data later
        return ext_url_config_error::answer_parse_error;

    if (!unserialize(doc))
    {
        clear();
        return ext_url_config_error::answer_not_enough_fields;
    }

    response->reset_out();
    response->save_2_file(config_filename());
    is_valid_ = true;

    return ext_url_config_error::ok;
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
    {
        clear();
        return false;
    }

    is_valid_ = true;
    return true;
}

std::string_view external_url_config::get_host(const host_url_type _type) const
{
    if (is_valid())
    {
        if (const auto iter_u = cache_.find(_type); iter_u != cache_.end())
            return iter_u->second;
    }

    assert(false);
    return std::string_view();
}

void external_url_config::clear()
{
    is_valid_ = false;
    cache_.clear();

    tools::system::delete_file(config_filename());
}

bool external_url_config::unserialize(const rapidjson::Value& _node)
{
    const auto get_url = [](const auto& _url_node, std::string_view _node_name, std::string& _out)
    {
        if (!tools::unserialize_value(_url_node, _node_name, _out))
            return false;

        _out = std::string(extract_host(_out));
        if (_out.empty())
            return false;

        return true;
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

    return true;
}