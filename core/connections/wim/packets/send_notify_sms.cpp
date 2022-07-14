#include "stdafx.h"

#include "../../../http_request.h"
#include "../../../../common.shared/json_helper.h"
#include "../log_replace_functor.h"

#include "send_notify_sms.h"


namespace core::wim
{

send_notify_sms::send_notify_sms(wim_packet_params _params, std::string_view _sms_notify_context)
    : robusto_packet(std::move(_params))
    , sms_notify_context_(_sms_notify_context)
{
}

std::string_view send_notify_sms::get_method() const
{
    return "sendNotifySms";
}

int send_notify_sms::minimal_supported_api_version() const
{
    return core::urls::api_version::instance().minimal_supported();
}

int32_t send_notify_sms::init_request(const std::shared_ptr<http_request_simple>& _request)
{
    rapidjson::Document doc(rapidjson::Type::kObjectType);
    auto& a = doc.GetAllocator();

    rapidjson::Value node_params(rapidjson::Type::kObjectType);
    node_params.AddMember("smsNotifyContext", sms_notify_context_, a);

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

}
