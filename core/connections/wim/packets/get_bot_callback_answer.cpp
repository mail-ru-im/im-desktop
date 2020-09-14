#include "stdafx.h"
#include "get_bot_callback_answer.h"

#include "../../../http_request.h"
#include "../../../tools/json_helper.h"

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

    setup_common_and_sign(doc, a, _request, "getBotCallbackAnswer");

    if (!params_.full_log_)
    {
        log_replace_functor f;
        f.add_json_marker("aimsid", aimsid_range_evaluator());
        _request->set_replace_log_function(f);
    }

    return 0;
}

int32_t get_bot_callback_answer::parse_response(const std::shared_ptr<core::tools::binary_stream>& _response)
{
    if (!_response->available())
        return wpie_http_empty_response;

    auto size = _response->available();
    load_response_str((const char*)_response->read(size), size);
    try
    {
        rapidjson::Document doc;
        if (doc.Parse(response_str().c_str()).HasParseError())
            return wpie_error_parse_response;

        auto iter_status = doc.FindMember("status");
        if (iter_status == doc.MemberEnd())
            return wpie_error_parse_response;

        if (!tools::unserialize_value(iter_status->value, "code", status_code_))
            return wpie_error_parse_response;

        if (status_code_ == 20000 || status_code_ == 20080)
        {
            if (const auto iter_result = doc.FindMember("results"); iter_result != doc.MemberEnd())
                return parse_results(iter_result->value);
        }
        else
        {
            return on_response_error_code();
        }
    }
    catch (...)
    {
        return 0;
    }

    return 0;
}

int32_t get_bot_callback_answer::parse_results(const rapidjson::Value& _node_results)
{
    tools::unserialize_value(_node_results, "reqId", req_id_);
    payload_.unserialize(_node_results);

    return 0;
}

const std::string& get_bot_callback_answer::get_bot_req_id() const noexcept
{
    return req_id_;
}

const bot_payload& get_bot_callback_answer::get_payload() const noexcept
{
    return payload_;
}
