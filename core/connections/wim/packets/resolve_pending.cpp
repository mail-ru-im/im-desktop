#include "stdafx.h"
#include "resolve_pending.h"

#include "../../../http_request.h"
#include "../../../tools/system.h"

using namespace core;
using namespace wim;

resolve_pending::resolve_pending(
    wim_packet_params _params,
    const std::string& _aimid,
    const std::vector<std::string>& _contacts,
    bool _approve)
    : robusto_packet(std::move(_params))
    , aimid_(_aimid)
    , contacts_(_contacts)
    , approve_(_approve)
{
}

resolve_pending::~resolve_pending()
{
}

std::string_view resolve_pending::get_method() const
{
    return "chatResolvePending";
}

int32_t resolve_pending::init_request(const std::shared_ptr<core::http_request_simple>& _request)
{
    rapidjson::Document doc(rapidjson::Type::kObjectType);
    auto& a = doc.GetAllocator();

    rapidjson::Value node_params(rapidjson::Type::kObjectType);
    node_params.AddMember("sn", aimid_, a);

    rapidjson::Value node_members(rapidjson::Type::kArrayType);
    node_members.Reserve(contacts_.size(), a);
    for (const auto& iter : contacts_)
    {
        rapidjson::Value node_member(rapidjson::Type::kObjectType);
        node_member.AddMember("sn", iter, a);
        node_member.AddMember("approve", approve_, a);
        node_members.PushBack(std::move(node_member), a);
    }

    node_params.AddMember("members", std::move(node_members), a);
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

int32_t resolve_pending::parse_response_data(const rapidjson::Value& _data)
{
    return 0;
}
