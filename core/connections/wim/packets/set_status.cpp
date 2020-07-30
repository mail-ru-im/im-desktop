#include "stdafx.h"

#include "set_status.h"

#include "../../../http_request.h"
#include "../../../tools/system.h"
#include "../../../tools/json_helper.h"

using namespace core;
using namespace wim;

wim::set_status::set_status(wim_packet_params _params, std::string_view _status, std::chrono::seconds _duration)
    : robusto_packet(std::move(_params))
    , status_(std::move(_status))
    , duration_(_duration)
{
}

int32_t set_status::init_request(const std::shared_ptr<core::http_request_simple>& _request)
{
    rapidjson::Document doc(rapidjson::Type::kObjectType);
    auto& a = doc.GetAllocator();

    rapidjson::Value node_params(rapidjson::Type::kObjectType);
    node_params.AddMember("status", status_, a);

    if (duration_.count() > 0)
        node_params.AddMember("duration", duration_.count(), a);

    doc.AddMember("params", std::move(node_params), a);

    setup_common_and_sign(doc, a, _request, "setStatus");

    if (!params_.full_log_)
    {
        log_replace_functor f;
        f.add_json_marker("aimsid", aimsid_range_evaluator());
        _request->set_replace_log_function(f);
    }

    return 0;
}