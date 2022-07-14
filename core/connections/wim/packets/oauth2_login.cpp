#include "stdafx.h"

#include "oauth2_login.h"
#include "../../urls_cache.h"
#include "../../../http_request.h"
#include "../../../tools/system.h"
#include "../common.shared/json_helper.h"
#include "../common.shared/string_utils.h"
#include "../common.shared/config/config.h"
#include "../log_replace_functor.h"

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

oauth2_login::oauth2_login(wim_packet_params _params, std::string_view _authcode, oauth2_type _type)
    : wim_packet(std::move(_params))
    , authcode_(_authcode)
    , expired_in_(0)
    , auth_type_(_type)
{
}

oauth2_login::oauth2_login(wim_packet_params _params)
    : wim_packet(std::move(_params))
    , expired_in_(0)
{
}

oauth2_login::~oauth2_login() = default;

oauth2_login::oauth2_type oauth2_login::get_auth_type(std::string_view _type)
{
    constexpr std::pair<std::string_view, oauth2_type> auth_types[] =
    {
        { "oauth2_mail", oauth2_type::oauth2 },
        { "keycloak", oauth2_type::keycloak }
    };

    auto it = std::find_if(std::begin(auth_types), std::end(auth_types), [_type](const auto& _val)
    {
        return _val.first == _type;
    });
    return (it != std::end(auth_types) ? it->second : oauth2_type::oauth2);
}

std::string_view oauth2_login::get_method() const
{
    return "loginWithOAuth2";
}

int core::wim::oauth2_login::minimal_supported_api_version() const
{
    return core::urls::api_version::instance().minimal_supported();
}

int32_t oauth2_login::init_request(const std::shared_ptr<core::http_request_simple> & _request)
{
    switch (auth_type_)
    {
    case oauth2_type::oauth2:
        init_oauth2_request(_request);
        break;
    case oauth2_type::keycloak:
        init_keycloak_request(_request);
        break;
    default:
        return 1;
    }

    return 0;
}

int32_t oauth2_login::init_oauth2_request(const std::shared_ptr<core::http_request_simple>& _request)
{
    std::string auth_content_type = "Content-Type: application/x-www-form-urlencoded";

    constexpr std::string_view auth_type_key = "Authorization: Basic ";

    auto auth_token_url = core::urls::get_url(urls::url_type::oauth2_token);
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

    mask_request(_request, { "client_id=", "refresh_token=", "code=", auth_type_key, "refresh_token", "access_token" });

    return 0;
}

int32_t oauth2_login::init_keycloak_request(const std::shared_ptr<core::http_request_simple>& _request)
{
    std::string auth_content_type = "Content-Type: application/x-www-form-urlencoded";

    auto auth_token_url = core::urls::get_url(urls::url_type::oauth2_token);
    auto auth_redirect_url = core::urls::get_url(urls::url_type::oauth2_redirect);
    if (authcode_.empty())
    {
        _request->set_url(auth_token_url);
        _request->push_post_parameter("client_id", "nomailcli");
        _request->push_post_parameter("grant_type", "refresh_token");
        _request->push_post_parameter("refresh_token", *params_.o2refresh_token_);
    }
    else
    {
        _request->set_url(auth_token_url);
        _request->push_post_parameter("client_id", "nomailcli");
        _request->push_post_parameter("grant_type", "authorization_code");
        _request->push_post_parameter("code", authcode_);
        _request->push_post_parameter("redirect_uri", std::move(auth_redirect_url));
        _request->push_post_parameter("client_secret", "use_secrets_luke");
    }
    _request->set_custom_header_param(std::move(auth_content_type));

    mask_request(_request, { "client_id=", "refresh_token=", "code=", "client_secret=", "refresh_token", "access_token" });

    return 0;
}

void oauth2_login::mask_request(const std::shared_ptr<core::http_request_simple>& _request, const std::vector<std::string_view>& _keys)
{
    if (!params_.full_log_)
    {
        log_replace_functor f;
        for (auto key : _keys)
            f.add_url_marker(key);

        _request->set_replace_log_function(f);
    }
}

int32_t oauth2_login::on_response_error_code()
{
    return wpie_login_unknown_error;
}

int32_t oauth2_login::parse_response(const std::shared_ptr<core::tools::binary_stream>& _response)
{
    if (!_response->available())
        return wpie_http_empty_response;

    _response->write((char)0);

    auto size = _response->available();

    load_response_str((const char*)_response->read(size), size);

    _response->reset_out();

    try
    {
        const auto json_str = _response->read(_response->available());

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
