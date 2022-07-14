#include "stdafx.h"

#include "../../../http_request.h"
#include "../../../../common.shared/json_helper.h"
#include "archive/history_message.h"
#include "../log_replace_functor.h"

#include "add_reaction.h"

namespace core::wim
{

add_reaction::add_reaction(wim_packet_params _params, int64_t _msg_id, const std::string& _chat_id, const std::string& _reaction, const std::vector<std::string>& _reactions_list)
    : robusto_packet(std::move(_params)),
      msg_id_(_msg_id),
      chat_id_(_chat_id),
      reaction_(_reaction),
      reactions_list_(_reactions_list)
{

}

const archive::reactions_data& add_reaction::get_result() const
{
    return reactions_;
}

bool add_reaction::is_reactions_for_message_disabled() const
{
    return get_status_code() == robusto_protocol_error::message_reactions_disabled;
}

std::string_view add_reaction::get_method() const
{
    return "reaction/add";
}

int add_reaction::minimal_supported_api_version() const
{
    return core::urls::api_version::instance().minimal_supported();
}

int32_t add_reaction::init_request(const std::shared_ptr<http_request_simple>& _request)
{
    rapidjson::Document doc(rapidjson::Type::kObjectType);
    auto& a = doc.GetAllocator();

    rapidjson::Value node_params(rapidjson::Type::kObjectType);

    node_params.AddMember("msgId", msg_id_, a);
    node_params.AddMember("chatId", chat_id_, a);
    node_params.AddMember("reaction", reaction_, a);

    rapidjson::Value reactions(rapidjson::Type::kArrayType);
    reactions.Reserve(reactions_list_.size(), a);
    for (const auto& reaction : reactions_list_)
        reactions.PushBack(tools::make_string_ref(reaction), a);
    node_params.AddMember("reactions", std::move(reactions), a);

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

int32_t add_reaction::parse_results(const rapidjson::Value& _node_results)
{
    if (!reactions_.unserialize(_node_results))
        return -1;

    return 0;
}

}
