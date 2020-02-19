#include "stdafx.h"
#include "suggest_group_nick.h"

#include "../../../http_request.h"
#include "../../../tools/json_helper.h"
#include "../../urls_cache.h"

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

suggest_group_nick::~suggest_group_nick()
{
}

std::string suggest_group_nick::get_nick() const
{
    return nick_;
}

int32_t suggest_group_nick::init_request(std::shared_ptr<core::http_request_simple> _request)
{
    std::string public_method = "suggestPublicGroupStamp";
    std::string private_method = "suggestPrivateGroupStamp";

    _request->set_url(urls::get_url(urls::url_type::rapi_host));
    _request->set_normalized_url(public_ ? public_method : private_method);
    _request->set_keep_alive();

    rapidjson::Document doc(rapidjson::Type::kObjectType);
    auto& a = doc.GetAllocator();

    doc.AddMember("method", (public_ ? public_method : private_method), a);
    doc.AddMember("reqId", get_req_id(), a);

    rapidjson::Value node_params(rapidjson::Type::kObjectType);
    node_params.AddMember("sn", aimid_, a);

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

int32_t suggest_group_nick::parse_results(const rapidjson::Value& _node_results)
{
    tools::unserialize_value(_node_results, "stamp", nick_);
    return 0;
}

int32_t suggest_group_nick::parse_response_data(const rapidjson::Value& _data)
{
    return 0;
}
