#include "stdafx.h"
#include "group_pending_cancel.h"
#include "../../../http_request.h"
#include "../../../tools/json_helper.h"

using namespace core;
using namespace wim;

group_pending_cancel::group_pending_cancel(wim_packet_params _params, std::string _sn)
    : robusto_packet(std::move(_params))
    , sn_(std::move(_sn))
{
    assert(!sn_.empty());
}

int32_t group_pending_cancel::init_request(const std::shared_ptr<core::http_request_simple>& _request)
{
    rapidjson::Document doc(rapidjson::Type::kObjectType);
    auto& a = doc.GetAllocator();

    rapidjson::Value node_params(rapidjson::Type::kObjectType);
    node_params.AddMember("sn", sn_, a);
    doc.AddMember("params", std::move(node_params), a);

    setup_common_and_sign(doc, a, _request, "group/pending/cancel");

    if (!params_.full_log_)
    {
        log_replace_functor f;
        f.add_marker("a");
        f.add_json_marker("aimsid", aimsid_range_evaluator());
        _request->set_replace_log_function(f);
    }

    return 0;
}