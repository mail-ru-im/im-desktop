#include "stdafx.h"

#include "../../../http_request.h"
#include "../../../tools/json_helper.h"
#include "../../urls_cache.h"
#include "archive/history_message.h"

#include "get_poll.h"



namespace core::wim
{

get_poll::get_poll(wim_packet_params _params, const std::string& _poll_id)
    : robusto_packet(std::move(_params)),
      poll_id_(_poll_id)
{

}

const archive::poll_data& get_poll::get_result() const
{
    return poll_;
}

int32_t get_poll::init_request(std::shared_ptr<http_request_simple> _request)
{
    constexpr char method[] = "poll/get";

    _request->set_url(urls::get_url(urls::url_type::rapi_host));
    _request->set_normalized_url(method);
    _request->set_keep_alive();

    rapidjson::Document doc(rapidjson::Type::kObjectType);
    auto& a = doc.GetAllocator();

    doc.AddMember("method", method, a);
    doc.AddMember("reqId", get_req_id(), a);

    rapidjson::Value node_params(rapidjson::Type::kObjectType);

    node_params.AddMember("id", poll_id_, a);
    node_params.AddMember("subscribe", true, a);

    doc.AddMember("params", std::move(node_params), a);

    sign_packet(doc, a, _request);

    if (!robusto_packet::params_.full_log_)
    {
        log_replace_functor f;
        f.add_json_marker("aimsid", aimsid_range_evaluator());
        _request->set_replace_log_function(f);
    }

    return 0;
}

int32_t get_poll::parse_results(const rapidjson::Value& _node_results)
{
    poll_.unserialize(_node_results);
    poll_.id_ = poll_id_;

    return 0;
}

int32_t get_poll::on_response_error_code()
{
    return robusto_packet::on_response_error_code();
}

}
