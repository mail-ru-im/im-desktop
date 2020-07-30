#include "stdafx.h"

#include "../../../http_request.h"
#include "../../../tools/json_helper.h"
#include "archive/history_message.h"

#include "get_reaction.h"

namespace core::wim
{

get_reaction::get_reaction(wim_packet_params _params, const std::string& _chat_id, const msgids_list& _msg_ids)
    : robusto_packet(std::move(_params)),
      chat_id_(_chat_id),
      msg_ids_(_msg_ids)
{

}

reactions_vector_sptr get_reaction::get_result() const
{
    return reactions_;
}

int32_t get_reaction::init_request(const std::shared_ptr<http_request_simple>& _request)
{
    rapidjson::Document doc(rapidjson::Type::kObjectType);
    auto& a = doc.GetAllocator();

    rapidjson::Value node_params(rapidjson::Type::kObjectType);

    node_params.AddMember("chatId", chat_id_, a);

    rapidjson::Value node_ids(rapidjson::Type::kArrayType);
    node_ids.Reserve(msg_ids_.size(), a);
    for (const auto& id : msg_ids_)
    {
        rapidjson::Value node_id(rapidjson::Type::kStringType);
        node_id.SetInt64(id);
        node_ids.PushBack(std::move(node_id), a);
    }
    node_params.AddMember("msgIds", std::move(node_ids), a);

    doc.AddMember("params", std::move(node_params), a);

    setup_common_and_sign(doc, a, _request, "reaction/get");

    if (!robusto_packet::params_.full_log_)
    {
        log_replace_functor f;
        f.add_json_marker("aimsid", aimsid_range_evaluator());
        _request->set_replace_log_function(f);
    }

    return 0;
}

int32_t get_reaction::parse_results(const rapidjson::Value& _node_results)
{
    reactions_ = std::make_shared<reactions_vector>();

    auto iter_reactions = _node_results.FindMember("reactions");
    if (iter_reactions == _node_results.MemberEnd() || !iter_reactions->value.IsArray())
        return -1;

    for (const auto& reactions_item : iter_reactions->value.GetArray())
    {
        archive::reactions_data reactions;
        if (reactions.unserialize(reactions_item))
            reactions_->push_back(reactions);
    }

    return 0;
}

}
