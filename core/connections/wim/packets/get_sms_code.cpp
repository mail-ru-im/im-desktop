#include "stdafx.h"

#include <sstream>
#include <time.h>

#include "get_sms_code.h"
#include "../../../http_request.h"
#include "../../../core.h"
#include "../../../../corelib/enumerations.h"
#include "../../../tools/system.h"
#include "../../../tools/json_helper.h"
#include "../../urls_cache.h"

using namespace core;
using namespace wim;

validate_phone::validate_phone(
    wim_packet_params params,
    const std::string& phone,
    const std::string& locale)
    :
wim_packet(std::move(params)),
    phone_(phone),
    locale_(locale),
    code_length_(0),
    existing_(false)
{
}


validate_phone::~validate_phone()
{
}

std::string_view validate_phone::get_method() const
{
    return "phoneValidation";
}

int32_t validate_phone::init_request(const std::shared_ptr<core::http_request_simple>& _request)
{
    std::stringstream ss_url;
    ss_url << urls::get_url(urls::url_type::smsreg_host) << std::string_view("/requestPhoneValidation.php?") <<
        "locale=" << locale_ <<
        "&msisdn=" << phone_ <<
        "&smsFormatType=human" <<
        "&k=" << params_.dev_id_ <<
        "&r=" <<  core::tools::system::generate_guid();


    _request->set_url(ss_url.str());
    _request->set_normalized_url("phoneValidation");
    _request->set_keep_alive();
    return 0;
}

int32_t core::wim::validate_phone::on_response_error_code()
{
    switch (status_code_)
    {
    case 465:
        return wpie_error_rate_limit;
    case 471:
        return wpie_error_attach_busy_phone;
    default:
        return wpie_get_sms_code_unknown_error;
    }
}

int32_t core::wim::validate_phone::parse_response_data(const rapidjson::Value& _data)
{
    if (!tools::unserialize_value(_data, "trans_id", trans_id_))
        return wpie_http_parse_response;

    tools::unserialize_value(_data, "existing", existing_);
    tools::unserialize_value(_data, "code_length", code_length_);
    tools::unserialize_value(_data, "ivr_url", ivr_url_);
    tools::unserialize_value(_data, "checks", checks_);

    boost::replace_all(ivr_url_, "&amp;", "&");

    g_core->insert_event(core::stats::stats_event_names::reg_sms_send);
    return 0;
}

get_code_by_phone_call::get_code_by_phone_call(wim_packet_params params, const std::string& _ivr_url)
    : wim_packet(std::move(params))
    , ivr_url_(_ivr_url)
{
}

get_code_by_phone_call::~get_code_by_phone_call()
{
}

int32_t get_code_by_phone_call::init_request(const std::shared_ptr<core::http_request_simple>& _request)
{
    _request->set_url(ivr_url_);
    _request->set_keep_alive();
    return 0;
}

std::string_view get_code_by_phone_call::get_method() const
{
    return "get_code_by_phone_call";
}