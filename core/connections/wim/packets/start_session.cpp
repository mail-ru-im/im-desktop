#include "stdafx.h"
#include "start_session.h"
#include "client_login.h"

#include "../../../http_request.h"
#include "../../../tools/hmac_sha_base64.h"
#include "../../../core.h"
#include "../../../../common.shared/version_info.h"
#include "../../../utils.h"
#include "../../../tools/system.h"
#include "../../../tools/json_helper.h"
#include "../../urls_cache.h"
#include "subscriptions/subscr_types.h"


std::string get_start_session_host()
{
    return su::concat(core::urls::get_url(core::urls::url_type::wim_host), "aim/startSession");
}

#define WIM_EVENTS "myInfo,presence,buddylist,typing,dataIM,userAddedToBuddyList,webrtcMsg,mchat,hist,hiddenChat,diff,permitDeny,imState,notification,apps"
#define WIM_PRESENCEFIELDS "aimId,iconId,bigIconId,largeIconId,displayId,friendly,offlineMsg,state,statusMsg,userType,phoneNumber,cellNumber,smsNumber,workNumber,otherNumber,capabilities,ssl,abPhoneNumber,moodIcon,lastName,abPhones,abContactName,lastseen,mute,livechat,official,public,autoAddition,readonly,nick,bot"
#define WIM_INTERESTCAPS "8eec67ce70d041009409a7c1602a5c84," WIM_CAP_VOIP_VOICE "," WIM_CAP_VOIP_VIDEO "," WIM_CAP_VOIP_RINGING "," WIM_CAP_FOCUS_GROUP_CALLS
#define WIM_ASSERTCAPS WIM_CAP_VOIP_VOICE "," WIM_CAP_VOIP_VIDEO "," WIM_CAP_VOIP_RINGING "," WIM_CAP_FOCUS_GROUP_CALLS "," WIM_CAP_UNIQ_REQ_ID "," WIM_CAP_EMOJI "," WIM_CAP_MAIL_NOTIFICATIONS "," WIM_CAP_MENTIONS "," WIM_CAP_INTRO_DLG_STATE "," WIM_CAP_CHAT_HEADS "," WIM_CAP_GALLERY_NOTIFY "," WIM_CAP_GROUP_SUBSCRIPTION "," WIM_CAP_RECENT_CALLS "," WIM_CAP_REACTIONS
#define WIM_INVISIBLE "false"


using namespace core;
using namespace wim;

#define SAAB__CODE_SAAB_OPENAUTH_REQUEST_NOT_FRESH 1015

start_session::start_session(
    wim_packet_params params,
    const bool _is_ping,
    const std::string& _uniq_device_id,
    const std::string& _locale,
    std::chrono::milliseconds _timeout,
    std::function<bool(std::chrono::milliseconds)> _wait_function)
    : wim_packet(std::move(params))
    , uniq_device_id_(_uniq_device_id)
    , locale_(_locale)
    , is_ping_(_is_ping)
    , need_relogin_(false)
    , correct_ts_(false)
    , timeout_(_timeout)
    , wait_function_(_wait_function)
    , ts_(0)
{
}


start_session::~start_session()
{
}

int32_t start_session::init_request_full_start_session(const std::shared_ptr<core::http_request_simple>& request)
{
    request->set_compression_auto();
    request->set_url(get_start_session_host());
    request->set_normalized_url("startSession");
    request->set_keep_alive();

    request->push_post_parameter("f", "json");
    request->push_post_parameter("k", escape_symbols(params_.dev_id_));

    request->push_post_parameter("a", escape_symbols(params_.a_token_));
    request->push_post_parameter("clientName", escape_symbols(utils::get_app_name()));

    request->push_post_parameter("imf", "plain");

    //push valid version info
    request->push_post_parameter("buildNumber", core::tools::version_info().get_build_version());
    request->push_post_parameter("majorVersion", core::tools::version_info().get_major_version() + core::tools::version_info().get_minor_version());
    request->push_post_parameter("minorVersion", core::tools::version_info().get_minor_version());
    request->push_post_parameter("pointVersion", "0");
    request->push_post_parameter("clientVersion", WIM_APP_VER);

    request->push_post_parameter("events", escape_symbols(WIM_EVENTS));
    request->push_post_parameter("includePresenceFields", escape_symbols(WIM_PRESENCEFIELDS));

    if (std::string_view assert_caps = WIM_ASSERTCAPS; !assert_caps.empty())
        request->push_post_parameter("assertCaps", escape_symbols(assert_caps));

    request->push_post_parameter("interestCaps", escape_symbols(WIM_INTERESTCAPS));
    request->push_post_parameter("invisible", WIM_INVISIBLE);
    request->push_post_parameter("language", escape_symbols(locale_.empty() ? "en-us" : locale_));
    request->push_post_parameter("mobile", "0");
    request->push_post_parameter("rawMsg", "0");
    request->push_post_parameter("deviceId", escape_symbols(uniq_device_id_));

    constexpr std::chrono::seconds sessionTimeout = std::chrono::hours(24 * (build::is_debug() ? 3 : 90));
    request->push_post_parameter("sessionTimeout", std::to_string(sessionTimeout.count()));

    //move instance into na state after activeTimeout time elapsed since last activity
    //provided by calling fetchEvent or validateSid Api methods
    request->push_post_parameter("inactiveView", "offline");

    constexpr std::chrono::seconds activeTimeout = std::chrono::minutes(3);
    request->push_post_parameter("activeTimeout", std::to_string(activeTimeout.count()));

    time_t ts = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now()) - params_.time_offset_;
    request->push_post_parameter("ts", std::to_string(ts));
    request->push_post_parameter("view", "online");
    request->push_post_parameter("nonce", core::tools::system::generate_guid());

    if (const auto& types = subscriptions::types_for_start_session_subscribe(); !types.empty())
    {
        rapidjson::Document doc(rapidjson::Type::kArrayType);
        auto& a = doc.GetAllocator();
        doc.Reserve(types.size(), a);

        for (const auto t : types)
        {
            rapidjson::Value obj(rapidjson::Type::kObjectType);
            obj.AddMember("type", tools::make_string_ref(subscriptions::type_2_string(t)), a);
            doc.PushBack(std::move(obj), a);
        }

        rapidjson::StringBuffer buffer;
        rapidjson::Writer<rapidjson::StringBuffer> writer(buffer);
        doc.Accept(writer);

        request->push_post_parameter("subscriptions", escape_symbols(rapidjson_get_string_view(buffer)));
    }

    return 0;
}

int32_t start_session::init_request_short_start_session(const std::shared_ptr<core::http_request_simple>& _request)
{
    std::stringstream ss_url;
    ss_url << urls::get_url(urls::url_type::wim_host) << "aim/pingSession" <<
        "?f=json" <<
        "&aimsid=" << escape_symbols(get_params().aimsid_) <<
        "&k=" << escape_symbols(params_.dev_id_);

    _request->set_url(ss_url.str());
    _request->set_normalized_url("pingSession");
    _request->set_keep_alive();

    return 0;
}


int32_t start_session::init_request(const std::shared_ptr<core::http_request_simple>& _request)
{
    if (!params_.full_log_)
    {
        log_replace_functor f;
        f.add_marker("a");
        f.add_marker("aimsid", aimsid_range_evaluator());
        f.add_json_marker("aimsid", aimsid_range_evaluator());
        _request->set_replace_log_function(f);
    }

    return (is_ping_ ? init_request_short_start_session(_request) : init_request_full_start_session(_request));
}

int32_t start_session::on_response_error_code()
{
    need_relogin_ = true;

    switch (get_status_code())
    {
    case 401:
        {
            return wpie_start_session_wrong_credentials;
        }
    case 400:
        {
            if (get_status_detail_code() == SAAB__CODE_SAAB_OPENAUTH_REQUEST_NOT_FRESH)
            {
                correct_ts_ = true;
                need_relogin_ = false;
            }

            return wpie_start_session_invalid_request;
        }
    case 408:
        {
            need_relogin_ = false;
            return wpie_start_session_request_timeout;
        }

    case 430:
    case 607:
        {
            need_relogin_ = false;
            return wpie_error_too_fast_sending;
        }
    case 440:
        return wpie_start_session_wrong_devid;
    default:
        return wpie_start_session_unknown_error;
    }
}

int32_t start_session::execute_request(const std::shared_ptr<core::http_request_simple>& _request)
{
    wait_function_(timeout_);

    url_ = _request->get_url();

    if (auto error_code = get_error(is_ping_ ? _request->get() : _request->post()))
        return *error_code;

    http_code_ = (uint32_t)_request->get_response_code();

    if (http_code_ != 200)
        return wpie_http_error;

    return 0;
}


int32_t start_session::parse_response_data(const rapidjson::Value& _data)
{
    auto iter_ts = _data.FindMember("ts");
    if (iter_ts == _data.MemberEnd() || !iter_ts ->value.IsUint())
        return wpie_error_parse_response;

    ts_ = iter_ts->value.GetUint();

    auto iter_aimsid = _data.FindMember("aimsid");
    if (iter_aimsid == _data.MemberEnd() || !iter_aimsid->value.IsString())
        return wpie_error_parse_response;

    aimsid_ = rapidjson_get_string(iter_aimsid->value);

    auto iter_fetch_url = _data.FindMember("fetchBaseURL");
    if (iter_fetch_url == _data.MemberEnd() || !iter_fetch_url->value.IsString())
        return wpie_error_parse_response;

    fetch_url_ = rapidjson_get_string(iter_fetch_url->value);

    auto iter_my_info = _data.FindMember("myInfo");
    if (iter_my_info != _data.MemberEnd() && iter_my_info->value.IsObject())
    {
        auto iter_aimid = iter_my_info->value.FindMember("aimId");
        if (iter_aimid != iter_my_info->value.MemberEnd() || !iter_aimid->value.IsString())
            aimid_ = rapidjson_get_string(iter_aimid->value);
    }

    return 0;
}

void start_session::parse_response_data_on_error(const rapidjson::Value& _data)
{
    auto iter_ts = _data.FindMember("ts");
    if (iter_ts == _data.MemberEnd() || !iter_ts ->value.IsUint())
        return;

    ts_ = iter_ts->value.GetUint();
}

priority_t start_session::get_priority() const
{
    return priority_protocol();
}
