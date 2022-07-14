#include "stdafx.h"

#include "event_subscribe.h"

#include "../../../http_request.h"
#include "../../../../common.shared/json_helper.h"
#include "subscriptions/subscr_types.h"
#include "subscriptions/subscription.h"
#include "../log_replace_functor.h"

using namespace core;
using namespace wim;

event_subscribe::event_subscribe(wim_packet_params _params, subscriptions::subscr_ptr_v _subscriptions)
    : robusto_packet(std::move(_params))
{
    im_assert(!_subscriptions.empty());
    for (const auto& s : _subscriptions)
        subscriptions_[s->get_type()].push_back(s);
}

std::string_view event_subscribe::get_method() const
{
    return "eventSubscribe";
}

int core::wim::event_subscribe::minimal_supported_api_version() const
{
    return core::urls::api_version::instance().minimal_supported();
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
        s_node.AddMember("type", tools::make_string_ref(subscriptions::type_2_string(s.first)), a);
        rapidjson::Value data_node(rapidjson::Type::kObjectType);
        auto iter_sub = s.second.begin();
        (*iter_sub)->serialize(data_node, a);
        while (++iter_sub != s.second.end())
            (*iter_sub)->serialize_args(data_node, a);

        s_node.AddMember("data", std::move(data_node), a);
        subs.PushBack(std::move(s_node), a);
    }

    node_params.AddMember("subscriptions", std::move(subs), a);

    doc.AddMember("params", std::move(node_params), a);

    setup_common_and_sign(doc, a, _request, get_method());

    if (!params_.full_log_)
    {
        log_replace_functor f;
        f.add_json_marker("aimsid", aimsid_range_evaluator());

        for (const auto& s : subscriptions_)
            for (const auto& sub : s.second)
                for (const auto& marker : sub->get_log_markers())
                    f.add_json_array_marker(marker);

        _request->set_replace_log_function(f);
    }

    return 0;
}