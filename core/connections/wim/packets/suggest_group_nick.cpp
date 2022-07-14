#include "stdafx.h"
#include "suggest_group_nick.h"

#include "../../../http_request.h"
#include "../../../../common.shared/json_helper.h"
#include "../log_replace_functor.h"

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

std::string_view suggest_group_nick::get_method() const
{
    return public_ ? "suggestPublicGroupStamp" : "suggestPrivateGroupStamp";
}

int core::wim::suggest_group_nick::minimal_supported_api_version() const
{
    return core::urls::api_version::instance().minimal_supported();
}

int32_t suggest_group_nick::init_request(const std::shared_ptr<core::http_request_simple>& _request)
{
    rapidjson::Document doc(rapidjson::Type::kObjectType);
    auto& a = doc.GetAllocator();

    rapidjson::Value node_params(rapidjson::Type::kObjectType);
    node_params.AddMember("sn", aimid_, a);

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

int32_t suggest_group_nick::parse_results(const rapidjson::Value& _node_results)
{
    tools::unserialize_value(_node_results, "stamp", nick_);
    return 0;
}

int32_t suggest_group_nick::parse_response_data(const rapidjson::Value& _data)
{
    return 0;
}
