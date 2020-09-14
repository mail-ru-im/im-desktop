#include "stdafx.h"
#include "group_subscribe.h"

#include "../../../http_request.h"
#include "../../../tools/system.h"

#include "../common.shared/json_unserialize_helpers.h"

using namespace core;
using namespace wim;

group_subscribe::group_subscribe(wim_packet_params _params, const std::string& _stamp)
    : robusto_packet(std::move(_params))
    , stamp_(_stamp)
{
}

int32_t group_subscribe::get_resubscribe_in() const
{
    return resubscribe_in_;
}

int32_t group_subscribe::init_request(const std::shared_ptr<core::http_request_simple>& _request)
{
    rapidjson::Document doc(rapidjson::Type::kObjectType);
    auto& a = doc.GetAllocator();

    rapidjson::Value node_params(rapidjson::Type::kObjectType);
    node_params.AddMember("stamp", stamp_, a);

    doc.AddMember("params", std::move(node_params), a);

    setup_common_and_sign(doc, a, _request, "group/subscribe");

    if (!params_.full_log_)
    {
        log_replace_functor f;
        f.add_json_marker("aimsid", aimsid_range_evaluator());
        _request->set_replace_log_function(f);
    }

    return 0;
}

int32_t group_subscribe::parse_results(const rapidjson::Value& _node_results)
{
    if (auto resubscribeIn = common::json::get_value<unsigned int>(_node_results, "resubscribeIn"); resubscribeIn)
        resubscribe_in_ = *resubscribeIn;

    return 0;
}
