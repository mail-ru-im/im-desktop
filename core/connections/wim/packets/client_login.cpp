#include "stdafx.h"
#include "client_login.h"

#include "../../../http_request.h"
#include "../../../tools/hmac_sha_base64.h"
#include "../../../tools/json_helper.h"
#include "../../../../common.shared/version_info.h"
#include "../../../utils.h"
#include "../../../tools/system.h"
#include "../../../tools/md5.h"
#include "../../urls_cache.h"

#define WIM_APP_TOKENTYPE		"longterm"

namespace
{
    std::string token_type_str(core::token_type _type)
    {
        switch (_type)
        {
            case core::token_type::otp_via_email:
                return "otp_via_email";
            default:
                return WIM_APP_TOKENTYPE;
        }

        return std::string();
    }
}

using namespace core;
using namespace wim;

client_login::client_login(wim_packet_params params, const std::string& login, const std::string& password, const token_type token_type)
    : wim_packet(std::move(params)),
      login_(login),
      password_(password),
      expired_in_(0),
      host_time_(0),
      time_offset_(0),
      token_type_(token_type),
      need_fill_profile_(false)
{
}


client_login::~client_login()
{
}


void client_login::set_product_guid_8x(const std::string& _guid)
{
    product_guid_8x_ = _guid;
}

int32_t client_login::parse_response_data(const rapidjson::Value& _data)
{
    if (token_type_ == token_type::otp_via_email)
        return 0;

    try
    {
        auto iter_session_secret = _data.FindMember("sessionSecret");
        if (iter_session_secret == _data.MemberEnd() || !iter_session_secret->value.IsString())
            return wpie_http_parse_response;

        session_secret_ = rapidjson_get_string(iter_session_secret->value);

        std::vector<uint8_t> data(session_secret_.size());
        std::vector<uint8_t> password(password_.size());
        memcpy(&data[0], session_secret_.c_str(), session_secret_.size());
        memcpy(&password[0], password_.c_str(), password_.size());
        session_key_ = core::tools::base64::hmac_base64(data, password);

        auto iter_host_time = _data.FindMember("hostTime");
        if (iter_host_time == _data.MemberEnd() || !iter_host_time->value.IsUint())
            return wpie_http_parse_response;

        host_time_ = iter_host_time->value.GetUint();

        auto iter_token = _data.FindMember("token");
        if (iter_token == _data.MemberEnd() || !iter_token->value.IsObject())
            return wpie_http_parse_response;

        auto iter_expired_in = iter_token->value.FindMember("expiresIn");
        auto iter_a = iter_token->value.FindMember("a");
        if (iter_expired_in == iter_token->value.MemberEnd() || iter_a == iter_token->value.MemberEnd() ||
            !iter_expired_in->value.IsUint() || !iter_a->value.IsString())
            return wpie_http_parse_response;

        auto settings = _data.FindMember("settings");
        if (settings != iter_token->value.MemberEnd() && settings->value.IsObject())
        {
            auto iter_need_fill_profile = settings->value.FindMember("needFillProfile");
            if (iter_need_fill_profile != settings->value.MemberEnd() && iter_need_fill_profile->value.IsBool())
                need_fill_profile_ = iter_need_fill_profile->value.GetBool();
        }


        expired_in_ = iter_expired_in->value.GetUint();
        a_token_ = rapidjson_get_string(iter_a->value);

        time_offset_ = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now()) - host_time_;
    }
    catch (const std::exception&)
    {
        return wpie_http_parse_response;
    }

    return 0;
}


int32_t client_login::init_request(const std::shared_ptr<core::http_request_simple>& _request)
{
    if (login_.empty() || password_.empty() && token_type_ == token_type::basic)
        return wpie_invalid_login;

    _request->set_compression_auto();
    _request->set_url(su::concat(urls::get_url(urls::url_type::wim_host), "auth/clientLogin"));
    _request->set_normalized_url("authClientLogin");
    _request->push_post_parameter("f", "json");
    _request->push_post_parameter("devId", params_.dev_id_);
    _request->push_post_parameter("s", escape_symbols(login_));

    if (token_type_ == token_type::otp_via_email)
        password_ = "none";

    std::string password = escape_symbols(password_);

    _request->push_post_parameter("pwd", password);

    _request->push_post_parameter("service", std::string(core::utils::get_client_string()));

    if (!product_guid_8x_.empty())
    {
        _request->push_post_parameter("installationToken", escape_symbols(product_guid_8x_));
    }

    _request->push_post_parameter("clientName", escape_symbols(utils::get_app_name()));
    _request->push_post_parameter("clientVersion", core::tools::version_info().get_version());
    _request->push_post_parameter("tokenType", token_type_str(token_type_));
    _request->push_post_parameter("r", core::tools::system::generate_guid());

    _request->set_keep_alive();

    log_replace_functor f;
    f.add_marker("pwd");

    if (!params_.full_log_)
    {
        f.add_json_marker("a");
        f.add_json_marker("sessionSecret");
    }

    _request->set_replace_log_function(f);

    return 0;
}

int32_t client_login::on_response_error_code()
{
    switch (status_code_)
    {
        case 330:
        {
            if (status_detail_code_ == 3012)
            {
                return wpie_wrong_login_2x_factor;
            }

            return wpie_wrong_login;
        }
        case 408:
        {
            return wpie_request_timeout;
        }
        default:
        {
            return wpie_login_unknown_error;
        }

    }
}


int32_t client_login::execute_request(const std::shared_ptr<core::http_request_simple>& request)
{
    if (!request->post())
        return wpie_network_error;

    http_code_ = (uint32_t)request->get_response_code();

    if (http_code_ != 200)
        return wpie_http_error;

    return 0;
}

int32_t client_login::on_empty_data()
{
    if (token_type_ == token_type::otp_via_email)
        return 0;

    return wim_packet::on_empty_data();
}
