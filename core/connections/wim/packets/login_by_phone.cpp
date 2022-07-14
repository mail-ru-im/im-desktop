#include "stdafx.h"

#include "login_by_phone.h"
#include "../../../http_request.h"
#include "../../login_info.h"
#include "../../../tools/system.h"
#include "../../urls_cache.h"
#include "../../../../common.shared/json_helper.h"
#include "../log_replace_functor.h"

using namespace core;
using namespace wim;

phone_login::phone_login(wim_packet_params params, phone_info _info)
    : wim_packet(std::move(params))
    , info_(std::make_shared<phone_info>(std::move(_info)))
    , expired_in_(0)
    , host_time_(0)
    , time_offset_(0)
    , need_fill_profile_(false)
{
}

phone_login::~phone_login() = default;

std::string_view phone_login::get_method() const
{
    return "loginWithPhoneNumber";
}

int core::wim::phone_login::minimal_supported_api_version() const
{
    return core::urls::api_version::instance().minimal_supported();
}

int32_t phone_login::init_request(const std::shared_ptr<core::http_request_simple>& _request)
{
    std::stringstream ss_url;
    ss_url << urls::get_url(urls::url_type::smsreg_host) << std::string_view("/loginWithPhoneNumber.php?") <<
        "&msisdn=" << info_->get_phone() <<
        "&trans_id=" << info_->get_trans_id() <<
        "&k=" << params_.dev_id_ <<
        "&r=" << core::tools::system::generate_guid() <<
        "&f=json" <<
        "&sms_code=" << info_->get_sms_code() <<
        "&create_account=1";

    _request->set_url(ss_url.str());
    _request->set_normalized_url(get_method());
    _request->set_keep_alive();

    if (!params_.full_log_)
    {
        log_replace_functor f;
        f.add_json_marker("a");
        f.add_json_marker("sessionKey");
        _request->set_replace_log_function(f);
    }

    return 0;
}

int32_t core::wim::phone_login::on_response_error_code()
{
    return wpie_phone_not_verified;
}

int32_t core::wim::phone_login::parse_response_data(const rapidjson::Value& _data)
{
    if (bool verified = false; !tools::unserialize_value(_data, "phone_verified", verified))
        return wpie_http_parse_response;
    else if (!verified)
        return wpie_phone_not_verified;

    if (!tools::unserialize_value(_data, "hostTime", host_time_))
        return wpie_http_parse_response;

    const auto iter_token = _data.FindMember("token");
    if (iter_token == _data.MemberEnd() || !iter_token->value.IsObject())
        return wpie_http_parse_response;

    if (!tools::unserialize_value(iter_token->value, "expiresIn", expired_in_))
        return wpie_http_parse_response;

    if (!tools::unserialize_value(iter_token->value, "a", a_token_))
        return wpie_http_parse_response;

    const auto settings = _data.FindMember("settings");
    if (settings != _data.MemberEnd() && settings->value.IsObject())
        tools::unserialize_value(settings->value, "needFillProfile", need_fill_profile_);

    time_offset_ = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now()) - host_time_;

    return 0;
}

int32_t core::wim::phone_login::on_http_client_error()
{
    switch (http_code_)
    {
    case 401:
    case 463:
    case 466:
    case 468:
    case 469:
        return wpie_wrong_login;
    }

    return wpie_client_http_error;
}