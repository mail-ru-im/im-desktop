#include "stdafx.h"
#include "block_chat_member.h"

#include "../../../http_request.h"
#include "../../../tools/system.h"
#include "../common.shared/json_unserialize_helpers.h"

using namespace core;
using namespace wim;

block_chat_member::block_chat_member(
    wim_packet_params _params,
    const std::string& _aimid,
    const std::string& _contact,
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

    setup_common_and_sign(doc, a, _request, block_ ? "blockChatMembers" : "unblockChatMembers");

    if (!params_.full_log_)
    {
        log_replace_functor f;
        f.add_json_marker("aimsid", aimsid_range_evaluator());
        f.add_json_marker("authToken");
        _request->set_replace_log_function(f);
    }

    return 0;
}

int32_t block_chat_member::parse_response_data(const rapidjson::Value& _data)
{
    return 0;
}
