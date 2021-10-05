#include "stdafx.h"
#include "get_chat_member_info.h"

#include "../../../http_request.h"
#include "../../../../common.shared/json_helper.h"

using namespace core;
using namespace wim;

get_chat_member_info::get_chat_member_info(wim_packet_params _params, std::string_view _aimid, const std::vector<std::string>& _members)
    : robusto_packet(std::move(_params))
    , aimid_(_aimid)
    , members_(_members)
{
}

get_chat_member_info::~get_chat_member_info() = default;

std::string_view get_chat_member_info::get_method() const
{
    return "getChatMemberInfo";
}

int32_t get_chat_member_info::init_request(const std::shared_ptr<core::http_request_simple>& _request)
{
    rapidjson::Document doc(rapidjson::Type::kObjectType);
    auto& a = doc.GetAllocator();

    rapidjson::Value node_params(rapidjson::Type::kObjectType);

    node_params.AddMember("id", aimid_, a);

    rapidjson::Value node_members(rapidjson::Type::kArrayType);
    node_members.Reserve(members_.size(), a);
    for (const auto& member : members_)
        node_members.PushBack(tools::make_string_ref(member), a);

    rapidjson::Value filter_params(rapidjson::Type::kObjectType);
    filter_params.AddMember("members", std::move(node_members), a);
    node_params.AddMember("filter", std::move(filter_params), a);

    doc.AddMember("params", std::move(node_params), a);

    setup_common_and_sign(doc, a, _request, get_method());

    if (!robusto_packet::params_.full_log_)
    {
        log_replace_functor f;
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
