#include "stdafx.h"
#include "group_subscribe.h"

#include "../../../http_request.h"
#include "../../../tools/system.h"
#include "../../urls_cache.h"

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

int32_t group_subscribe::init_request(std::shared_ptr<core::http_request_simple> _request)
{
    auto method = std::string("group/subscribe");

    _request->set_url(urls::get_url(urls::url_type::rapi_host));
    _request->set_normalized_url(method);
    _request->set_keep_alive();

    rapidjson::Document doc(rapidjson::Type::kObjectType);
    auto& a = doc.GetAllocator();

    doc.AddMember("method", std::move(method), a);
    doc.AddMember("reqId", get_req_id(), a);

    rapidjson::Value node_params(rapidjson::Type::kObjectType);
    node_params.AddMember("stamp", stamp_, a);

    doc.AddMember("params", std::move(node_params), a);

    sign_packet(doc, a, _request);

    if (!params_.full_log_)
    {
        log_replace_functor f;
        f.add_marker("a");
        f.add_json_marker("aimsid", aimsid_range_evaluator());
        _request->set_replace_log_function(f);
    }

    return 0;
}

int32_t group_subscribe::parse_results(const rapidjson::Value& _node_results)
{
    const auto iter_resubscribe = _node_results.FindMember("resubscribeIn");
    if (iter_resubscribe != _node_results.MemberEnd() && iter_resubscribe->value.IsUint())
        resubscribe_in_ = iter_resubscribe->value.GetUint();

    return 0;
}
