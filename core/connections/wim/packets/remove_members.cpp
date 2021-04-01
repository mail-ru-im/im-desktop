#include "stdafx.h"

#include "remove_members.h"
#include "../../../http_request.h"
#include "tools/json_helper.h"

using namespace core;
using namespace wim;

remove_members::remove_members(wim_packet_params _params, std::string _aimid, std::vector<std::string> _members_to_delete)
    : robusto_packet(std::move(_params))
    , aimid_(std::move(_aimid))
    , members_to_delete_(std::move(_members_to_delete))
{
    assert(!aimid_.empty());
    assert(!members_to_delete_.empty());
}

std::string_view remove_members::get_method() const
{
    return "group/members/delete";
}

int32_t remove_members::init_request(const std::shared_ptr<core::http_request_simple>& _request)
{
    rapidjson::Document doc(rapidjson::Type::kObjectType);
    auto& a = doc.GetAllocator();

    rapidjson::Value node_params(rapidjson::Type::kObjectType);
    node_params.AddMember("sn", aimid_, a);

    rapidjson::Value members(rapidjson::Type::kArrayType);
    members.Reserve(members_to_delete_.size(), a);
    for (const auto& c : members_to_delete_)
        members.PushBack(tools::make_string_ref(c), a);
    node_params.AddMember("members", std::move(members), a);

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

int32_t remove_members::parse_results(const rapidjson::Value& _node_results)
{
    if (get_status_code() == 20070)
    {
        const auto failures = _node_results.FindMember("failures");
        if (failures != _node_results.MemberEnd() && failures->value.IsArray())
        {
            for (const auto& f : failures->value.GetArray())
            {
                if (std::string_view err; tools::unserialize_value(f, "error", err) && !err.empty())
                    if (const auto failure = string_to_failure(err); failure != chat_member_failure::invalid)
                        if (std::string_view id; tools::unserialize_value(f, "id", id) && !id.empty())
                            failures_[failure].emplace_back(std::move(id));
            }
        }
    }

    return 0;
}

int32_t remove_members::on_response_error_code()
{
    if (status_code_ == 40001)
        return wpie_error_permission_denied;
    else if (status_code_ == 40002)
        return wpie_error_user_blocked;
    else if (status_code_ == 40401)
        return wpie_error_group_not_found;

    return robusto_packet::on_response_error_code();
}

bool remove_members::is_status_code_ok() const
{
    return get_status_code() == 20070 || get_status_code() == 20100 || robusto_packet::is_status_code_ok();
}
