#include "stdafx.h"
#include "join_chat_alpha.h"

#include "../../../http_request.h"
#include "../../urls_cache.h"

using namespace core;
using namespace wim;


join_chat_alpha::join_chat_alpha(wim_packet_params _params, const std::string& _stamp)
    : robusto_packet(std::move(_params)), stamp_(_stamp)
{
}


join_chat_alpha::~join_chat_alpha()
{

}

int32_t join_chat_alpha::init_request(std::shared_ptr<core::http_request_simple> _request)
{
    constexpr char method[] = "joinChatAlpha";

    _request->set_url(urls::get_url(urls::url_type::rapi_host));
    _request->set_normalized_url(method);
    _request->set_keep_alive();

    rapidjson::Document doc(rapidjson::Type::kObjectType);
    auto& a = doc.GetAllocator();

    doc.AddMember("method", method, a);
    doc.AddMember("reqId", get_req_id(), a);

    rapidjson::Value node_params(rapidjson::Type::kObjectType);
    node_params.AddMember("stamp", stamp_, a);
    doc.AddMember("params", std::move(node_params), a);

    sign_packet(doc, a, _request);

    if (!params_.full_log_)
    {
        log_replace_functor f;
        f.add_marker("a");
        f.add_json_marker("aimsid", aimsid_range_evaluator());
        _request->set_replace_log_function(f);
    }

    return 0;
}

int32_t join_chat_alpha::parse_results(const rapidjson::Value& _node_results)
{
    return 0;
}

int32_t join_chat_alpha::on_response_error_code()
{
    return robusto_packet::on_response_error_code();
}
