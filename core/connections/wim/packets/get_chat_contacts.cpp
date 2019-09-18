#include "stdafx.h"
#include "get_chat_contacts.h"

#include "../../../http_request.h"
#include "../../../tools/json_helper.h"
#include "../../urls_cache.h"

using namespace core;
using namespace wim;

get_chat_contacts::get_chat_contacts(wim_packet_params _params, std::string_view _aimid, std::string_view _cursor, uint32_t _page_size)
    : robusto_packet(std::move(_params))
    , aimid_(_aimid)
    , cursor_(_cursor)
    , page_size_(_page_size)
{
}

get_chat_contacts::~get_chat_contacts()
{
}

int32_t get_chat_contacts::init_request(std::shared_ptr<http_request_simple> _request)
{
    constexpr char method[] = "getChatContacts";

    _request->set_gzip(true);
    _request->set_url(urls::get_url(urls::url_type::rapi_host));
    _request->set_normalized_url(method);
    _request->set_keep_alive();

    rapidjson::Document doc(rapidjson::Type::kObjectType);
    auto& a = doc.GetAllocator();

    doc.AddMember("method", method, a);
    doc.AddMember("reqId", get_req_id(), a);

    rapidjson::Value node_params(rapidjson::Type::kObjectType);

    node_params.AddMember("id", aimid_, a);
    node_params.AddMember("pageSize", page_size_, a);
    if (!cursor_.empty())
        node_params.AddMember("cursor", cursor_, a);

    doc.AddMember("params", std::move(node_params), a);

    sign_packet(doc, a, _request);

    if (!robusto_packet::params_.full_log_)
    {
        log_replace_functor f;
        f.add_marker("a");
        f.add_json_marker("aimsid", aimsid_range_evaluator());
        _request->set_replace_log_function(f);
    }

    return 0;
}

int32_t get_chat_contacts::parse_results(const rapidjson::Value& _node_results)
{
    if (result_.unserialize(_node_results) != 0)
        return wpie_http_parse_response;

    return 0;
}

int32_t get_chat_contacts::on_response_error_code()
{
    if (status_code_ == 40001)
        return wpie_error_robusto_you_are_not_chat_member;
    else if (status_code_ == 40002)
        return wpie_error_robusto_you_are_blocked;

    return robusto_packet::on_response_error_code();
}
