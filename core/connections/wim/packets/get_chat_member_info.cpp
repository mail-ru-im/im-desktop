#include "stdafx.h"
#include "get_chat_member_info.h"

#include "../../urls_cache.h"
#include "../../../http_request.h"

using namespace core;
using namespace wim;

get_chat_member_info::get_chat_member_info(wim_packet_params _params, std::string_view _aimid, const std::vector<std::string>& _members)
    : robusto_packet(std::move(_params))
    , aimid_(_aimid)
    , members_(_members)
{
}

get_chat_member_info::~get_chat_member_info()
{
}

int32_t get_chat_member_info::init_request(std::shared_ptr<core::http_request_simple> _request)
{
    constexpr char method[] = "getChatMemberInfo";

    _request->set_gzip(true);
    _request->set_url(urls::get_url(urls::url_type::rapi_host));
    _request->set_normalized_url(method);
    _request->set_keep_alive();

    rapidjson::Document doc(rapidjson::Type::kObjectType);
    auto& a = doc.GetAllocator();

    doc.AddMember("method", method, a);
    doc.AddMember("reqId", get_req_id(), a);

    rapidjson::Value node_params(rapidjson::Type::kObjectType);

    node_params.AddMember("id", aimid_, a);

    rapidjson::Value node_members(rapidjson::Type::kArrayType);
    node_members.Reserve(members_.size(), a);
    for (const auto& member : members_)
    {
        rapidjson::Value node_member(rapidjson::Type::kStringType);
        node_member.SetString(member.c_str(), member.size());
        node_members.PushBack(std::move(node_member), a);
    }

    rapidjson::Value filter_params(rapidjson::Type::kObjectType);
    filter_params.AddMember("members", std::move(node_members), a);
    node_params.AddMember("filter", std::move(filter_params), a);

    doc.AddMember("params", std::move(node_params), a);

    sign_packet(doc, a, _request);

    if (!robusto_packet::params_.full_log_)
    {
        log_replace_functor f;
        f.add_marker("a");
        f.add_json_marker("aimsid", aimsid_range_evaluator());
        _request->set_replace_log_function(f);
    }

    return 0;
}

int32_t get_chat_member_info::parse_results(const rapidjson::Value& _node_results)
{
     if (result_.unserialize(_node_results) != 0)
         return wpie_http_parse_response;

    return 0;
}

int32_t get_chat_member_info::on_response_error_code()
{
    if (status_code_ == 40001)
        return wpie_error_robusto_you_are_not_chat_member;
    else if (status_code_ == 40002)
        return wpie_error_robusto_you_are_blocked;

    return robusto_packet::on_response_error_code();
}
