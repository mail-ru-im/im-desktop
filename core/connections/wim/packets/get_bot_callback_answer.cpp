#include "stdafx.h"
#include "get_bot_callback_answer.h"

#include "../../../http_request.h"
#include "../../../../common.shared/json_helper.h"

using namespace core;
using namespace wim;


get_bot_callback_answer::get_bot_callback_answer(wim_packet_params _params, const std::string_view _chat_id, const std::string_view _callback_data, int64_t _msg_id)
    :   robusto_packet(std::move(_params))
    ,   chat_id_(_chat_id)
    ,   callback_data_(_callback_data)
    ,   msg_id_(_msg_id)
{
}

int32_t get_bot_callback_answer::init_request(const std::shared_ptr<core::http_request_simple>& _request)
{
    rapidjson::Document doc(rapidjson::Type::kObjectType);
    auto& a = doc.GetAllocator();

    rapidjson::Value node_params(rapidjson::Type::kObjectType);
    node_params.AddMember("chatId", chat_id_, a);
    node_params.AddMember("msgId", msg_id_, a);
    node_params.AddMember("callbackData", callback_data_, a);
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

int32_t get_bot_callback_answer::parse_results(const rapidjson::Value& _node_results)
{
    tools::unserialize_value(_node_results, "reqId", req_id_);
    payload_.unserialize(_node_results);

    return 0;
}

bool get_bot_callback_answer::is_status_code_ok() const
{
    return get_status_code() == 20080 || robusto_packet::is_status_code_ok();
}

const std::string& get_bot_callback_answer::get_bot_req_id() const noexcept
{
    return req_id_;
}

const bot_payload& get_bot_callback_answer::get_payload() const noexcept
{
    return payload_;
}

std::string_view get_bot_callback_answer::get_method() const
{
    return "getBotCallbackAnswer";
}
