#include "stdafx.h"

#include "event_subscribe.h"

#include "../../../http_request.h"
#include "../../../tools/json_helper.h"
#include "subscriptions/subscr_types.h"
#include "subscriptions/subscription.h"

using namespace core;
using namespace wim;

event_subscribe::event_subscribe(wim_packet_params _params, subscriptions::subscr_ptr_v _subscriptions)
    : robusto_packet(std::move(_params))
    , subscriptions_(std::move(_subscriptions))
{
    assert(!subscriptions_.empty());
}

int32_t event_subscribe::init_request(const std::shared_ptr<core::http_request_simple>& _request)
{
    rapidjson::Document doc(rapidjson::Type::kObjectType);
    auto& a = doc.GetAllocator();

    rapidjson::Value node_params(rapidjson::Type::kObjectType);

    rapidjson::Value subs(rapidjson::Type::kArrayType);
    subs.Reserve(subscriptions_.size(), a);
    for (const auto& s : subscriptions_)
    {
        rapidjson::Value s_node(rapidjson::Type::kObjectType);
        s_node.AddMember("type", tools::make_string_ref(subscriptions::type_2_string(s->get_type())), a);

        rapidjson::Value data_node(rapidjson::Type::kObjectType);
        s->serialize(data_node, a);
        s_node.AddMember("data", std::move(data_node), a);

        subs.PushBack(std::move(s_node), a);
    }
    node_params.AddMember("subscriptions", std::move(subs), a);

    doc.AddMember("params", std::move(node_params), a);

    setup_common_and_sign(doc, a, _request, "eventSubscribe");

    if (!params_.full_log_)
    {
        log_replace_functor f;
        f.add_json_marker("aimsid", aimsid_range_evaluator());

        for (const auto& s : subscriptions_)
            for (const auto& marker : s->get_log_markers())
                f.add_json_marker(marker);

        _request->set_replace_log_function(f);
    }

    return 0;
}