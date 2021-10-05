#include "stdafx.h"

#include "oauth2_login.h"
#include "../../urls_cache.h"
#include "../../../http_request.h"
#include "../../../tools/system.h"
#include "../common.shared/json_helper.h"
#include "../common.shared/string_utils.h"
#include "../common.shared/config/config.h"

namespace
{
    enum oauth2_error_code
    {
        invalid_client = 1,
        invalid_request = 2,
        invalid_credentials = 3,
        token_not_found = 6
    };
}

using namespace core;
using namespace wim;

oauth2_login::oauth2_login(wim_packet_params _params, std::string_view _authcode)
    : wim_packet(std::move(_params))
    , authcode_(_authcode)
    , expired_in_(0)
{
}

oauth2_login::oauth2_login(wim_packet_params _params)
    : wim_packet(std::move(_params))
    , expired_in_(0)
{
}

oauth2_login::~oauth2_login() = default;

std::string_view oauth2_login::get_method() const
{
    return "loginWithOAuth2";
}

int32_t oauth2_login::init_request(const std::shared_ptr<core::http_request_simple> & _request)
{
    std::string auth_content_type = "Content-Type: application/x-www-form-urlencoded";

    constexpr std::string_view auth_type_key = "Authorization: Basic ";

    auto auth_token_url = urls::get_url(urls::url_type::oauth2_token);
    if (authcode_.empty())
    {
        std::string client_id(config::get().string(config::values::client_id));

        _request->set_url(auth_token_url);
        _request->push_post_parameter("client_id", std::move(client_id));
        _request->push_post_parameter("grant_type", "refresh_token");
        _request->push_post_parameter("refresh_token", *params_.o2refresh_token_);
    }
    else
    {
        std::string auth_header = su::concat(auth_type_key, config::get().string(config::values::client_b64));

        _request->set_url(auth_token_url);
        _request->push_post_parameter("grant_type", "authorization_code");
        _request->push_post_parameter("code", authcode_);
        _request->push_post_parameter("redirect_uri", "http://localhost");
        _request->set_custom_header_param(std::move(auth_header));
    }
    _request->set_custom_header_param(std::move(auth_content_type));

    if (!params_.full_log_)
    {
        log_replace_functor f;
        f.add_url_marker("client_id=");
        f.add_url_marker("refresh_token=");
        f.add_url_marker("code=");
        f.add_url_marker(auth_type_key);

        f.add_json_marker("refresh_token");
        f.add_json_marker("access_token");

        _request->set_replace_log_function(f);
    }
    return 0;
}

int32_t oauth2_login::on_response_error_code()
{
    return wpie_login_unknown_error;
}

int32_t oauth2_login::parse_response(const std::shared_ptr<core::tools::binary_stream> & response)
{
    if (!response->available())
        return wpie_http_empty_response;

    response->write((char)0);

    auto size = response->available();

    load_response_str((const char*)response->read(size), size);

    response->reset_out();

    try
    {
        const auto json_str = response->read(response->available());

#ifdef DEBUG__OUTPUT_NET_PACKETS
        puts(json_str);
#endif // DEBUG__OUTPUT_NET_PACKETS

        rapidjson::Document doc;
        if (doc.ParseInsitu(json_str).HasParseError())
            return wpie_error_parse_response;

        int error_code = 0;
        if (tools::unserialize_value(doc, "error_code", error_code))
        {
            switch (error_code)
            {
            case invalid_client:
                return wpie_client_http_error;
            case invalid_request:
                return wpie_error_invalid_request;
            case invalid_credentials:
                return wpie_error_access_denied;
            case token_not_found:
                return wpie_login_unknown_error;
            }
        }

        if (!authcode_.empty())
            if (!tools::unserialize_value(doc, "refresh_token", o2refresh_token_))
                return wpie_http_parse_response;

        if (!tools::unserialize_value(doc, "access_token", o2auth_token_))
            return wpie_http_parse_response;

        if (!tools::unserialize_value(doc, "expires_in", expired_in_))
            return wpie_http_parse_response;
    }
    catch (...)
    {
        return wpie_error_parse_response;
    }

    return 0;
}


int32_t oauth2_login::on_http_client_error()
{
    switch (http_code_)
    {
    case 401:
    case 500:
        return wpie_wrong_login;
    }

    return wpie_client_http_error;
}
