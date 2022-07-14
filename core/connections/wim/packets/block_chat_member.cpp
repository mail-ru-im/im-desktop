#include "stdafx.h"
#include "block_chat_member.h"

#include "../../../http_request.h"
#include "../common.shared/json_helper.h"
#include "../log_replace_functor.h"

using namespace core;
using namespace wim;

block_chat_member::block_chat_member(
    wim_packet_params _params,
    std::string_view _aimid,
    std::string_view _contact,
    bool _block,
    bool _remove_messages)
    : robusto_packet(std::move(_params))
    , aimid_(_aimid)
    , contact_(_contact)
    , block_(_block)
    , remove_messages_(_remove_messages)
{
}

block_chat_member::~block_chat_member() = default;

std::string_view block_chat_member::get_method() const
{
    return block_ ? "blockChatMembers" : "unblockChatMembers";
}

int core::wim::block_chat_member::minimal_supported_api_version() const
{
    return core::urls::api_version::instance().minimal_supported();
}

int32_t block_chat_member::init_request(const std::shared_ptr<core::http_request_simple>& _request)
{
    rapidjson::Document doc(rapidjson::Type::kObjectType);
    auto& a = doc.GetAllocator();

    rapidjson::Value node_params(rapidjson::Type::kObjectType);
    node_params.AddMember("sn", aimid_, a);
    if (block_)
        node_params.AddMember("deleteLastMessages", remove_messages_, a);

    rapidjson::Value node_member(rapidjson::Type::kObjectType);
    node_member.AddMember("sn", contact_, a);

    rapidjson::Value node_members(rapidjson::Type::kArrayType);
    node_members.Reserve(1, a);
    node_members.PushBack(std::move(node_member), a);

    node_params.AddMember("members", std::move(node_members), a);
    doc.AddMember("params", std::move(node_params), a);

    setup_common_and_sign(doc, a, _request, get_method());

    if (!params_.full_log_)
    {
        log_replace_functor f;
        f.add_json_marker("aimsid", aimsid_range_evaluator());
        f.add_json_marker("authToken");
        _request->set_replace_log_function(f);
    }

    return 0;
}

int32_t block_chat_member::parse_results(const rapidjson::Value& _node_results)
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

int32_t block_chat_member::on_response_error_code()
{
    if (status_code_ == 40001)
        return wpie_error_access_denied;
    else if (status_code_ == 40002)
        return wpie_error_user_blocked;
    else if (block_ && status_code_ == 40003)
        return wpie_error_group_max_members;
    else if (status_code_ == 40101)
        return wpie_error_user_captched;
    else if (status_code_ == 40501)
        return wpie_error_rate_limit;

    return robusto_packet::on_response_error_code();
}

bool block_chat_member::is_status_code_ok() const
{
    return get_status_code() == 20070 || robusto_packet::is_status_code_ok();
}
