#include "stdafx.h"

#include "../../../http_request.h"
#include "../../../tools/json_helper.h"

#include "remove_reaction.h"

namespace core::wim
{

remove_reaction::remove_reaction(wim_packet_params _params, int64_t _msg_id, const std::string& _chat_id)
    : robusto_packet(std::move(_params)),
      msg_id_(_msg_id),
      chat_id_(_chat_id)
{

}

int32_t remove_reaction::init_request(const std::shared_ptr<http_request_simple>& _request)
{
    rapidjson::Document doc(rapidjson::Type::kObjectType);
    auto& a = doc.GetAllocator();

    rapidjson::Value node_params(rapidjson::Type::kObjectType);

    node_params.AddMember("msgId", msg_id_, a);
    node_params.AddMember("chatId", chat_id_, a);

    doc.AddMember("params", std::move(node_params), a);

    setup_common_and_sign(doc, a, _request, "reaction/remove");

    if (!robusto_packet::params_.full_log_)
    {
        log_replace_functor f;
        f.add_json_marker("aimsid", aimsid_range_evaluator());
        _request->set_replace_log_function(f);
    }

    return 0;
}

}
