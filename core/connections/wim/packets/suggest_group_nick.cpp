#include "stdafx.h"
#include "suggest_group_nick.h"

#include "../../../http_request.h"
#include "../../../tools/json_helper.h"

using namespace core;
using namespace wim;

suggest_group_nick::suggest_group_nick(
    wim_packet_params _params,
    const std::string& _aimid,
    bool _public)
    : robusto_packet(std::move(_params))
    , aimid_(_aimid)
    , public_(_public)
{
}

suggest_group_nick::~suggest_group_nick() = default;

int32_t suggest_group_nick::init_request(const std::shared_ptr<core::http_request_simple>& _request)
{
    rapidjson::Document doc(rapidjson::Type::kObjectType);
    auto& a = doc.GetAllocator();

    rapidjson::Value node_params(rapidjson::Type::kObjectType);
    node_params.AddMember("sn", aimid_, a);

    doc.AddMember("params", std::move(node_params), a);

    setup_common_and_sign(doc, a, _request, public_ ? "suggestPublicGroupStamp" : "suggestPrivateGroupStamp");

    if (!params_.full_log_)
    {
        log_replace_functor f;
        f.add_json_marker("aimsid", aimsid_range_evaluator());
        _request->set_replace_log_function(f);
    }

    return 0;
}

int32_t suggest_group_nick::parse_results(const rapidjson::Value& _node_results)
{
    tools::unserialize_value(_node_results, "stamp", nick_);
    return 0;
}

int32_t suggest_group_nick::parse_response_data(const rapidjson::Value& _data)
{
    return 0;
}
