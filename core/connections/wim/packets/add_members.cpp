#include "stdafx.h"

#include "add_members.h"
#include "../../../http_request.h"
#include "../common.shared/json_helper.h"
#include "../corelib/enumerations.h"
#include "../log_replace_functor.h"

using namespace core;
using namespace wim;

add_members::add_members(wim_packet_params _params, std::string _aimid, std::vector<std::string> _members_to_add, bool _unblock)
    : robusto_packet(std::move(_params))
    , aimid_(std::move(_aimid))
    , members_to_add_(std::move(_members_to_add))
    , unblock_(_unblock)
{
    im_assert(!aimid_.empty());
    im_assert(!members_to_add_.empty());
}

int32_t add_members::init_request(const std::shared_ptr<core::http_request_simple>& _request)
{
    rapidjson::Document doc(rapidjson::Type::kObjectType);
    auto& a = doc.GetAllocator();

    rapidjson::Value node_params(rapidjson::Type::kObjectType);
    node_params.AddMember("sn", aimid_, a);
    if (unblock_ || members_to_add_.size() > 1)
        node_params.AddMember("confirmUnblock", true, a);

    rapidjson::Value members(rapidjson::Type::kArrayType);
    members.Reserve(members_to_add_.size(), a);
    for (const auto& c : members_to_add_)
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

int32_t add_members::parse_results(const rapidjson::Value& _node_results)
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

int32_t add_members::on_response_error_code()
{
    if (status_code_ == 40001)
        return wpie_error_permission_denied;
    else if (status_code_ == 40401)
        return wpie_error_group_not_found;

    return robusto_packet::on_response_error_code();
}

bool add_members::is_status_code_ok() const
{
    return get_status_code() == 20070 || robusto_packet::is_status_code_ok();
}

std::string_view add_members::get_method() const
{
    return "group/members/add";
}

int core::wim::add_members::minimal_supported_api_version() const
{
    return core::urls::api_version::instance().minimal_supported();
}
