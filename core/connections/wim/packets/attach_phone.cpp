#include "stdafx.h"
#include "attach_phone.h"

#include "../../../http_request.h"
#include "../../login_info.h"

using namespace core;
using namespace wim;

attach_phone::attach_phone(wim_packet_params _params, const phone_info& _phone_info)
    : robusto_packet(std::move(_params))
    , phone_info_(_phone_info)
{
}

attach_phone::~attach_phone()
{
}

int32_t attach_phone::init_request(const std::shared_ptr<core::http_request_simple>& _request)
{
    if (phone_info_.get_phone().empty())
        return wpie_invalid_login;

    rapidjson::Document doc(rapidjson::Type::kObjectType);
    auto& a = doc.GetAllocator();

    rapidjson::Value node_params(rapidjson::Type::kObjectType);
    node_params.AddMember("sn", params_.aimid_, a);
    node_params.AddMember("sessionId", phone_info_.get_trans_id(), a);
    node_params.AddMember("smsCode", phone_info_.get_sms_code(), a);
    node_params.AddMember("phone", phone_info_.get_phone(), a);

    doc.AddMember("params", std::move(node_params), a);

    setup_common_and_sign(doc, a, _request, get_method());

    if (!params_.full_log_)
    {
        log_replace_functor f;
        f.add_json_marker("aimsid", aimsid_range_evaluator());
        _request->set_replace_log_function(f);
    }
    return 0;
}

int32_t attach_phone::get_response_error_code()
{
    switch (status_code_)
    {
    case 20000:
        return 0;
    case 40010: // invalid verification number
        return wpie_invalid_phone_number;
    case 40020: // phone attached to another account
        return wpie_error_attach_busy_phone;
    case 40500: // rate limit overflow
        return wpie_error_robuso_too_fast_sending;
    case 50000: // internal server error
        return wpie_error_message_unknown;
    }

    return robusto_packet::on_response_error_code();
}

std::string_view attach_phone::get_method() const
{
    return "auth/attachPhone";
}
