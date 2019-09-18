#include "stdafx.h"

#include "../../../log/log.h"
#include "../../../http_request.h"

#include "del_message.h"

#include "../../urls_cache.h"

CORE_WIM_NS_BEGIN

del_message::del_message(
    wim_packet_params _params,
    const int64_t _message_id,
    const std::string &_contact_aimid,
    const bool _for_all
)
    : robusto_packet(std::move(_params))
    , message_id_(_message_id)
    , contact_aimid_(_contact_aimid)
    , for_all_(_for_all)
{
    assert(message_id_ > 0);
    assert(!contact_aimid_.empty());
}

int32_t del_message::init_request(std::shared_ptr<core::http_request_simple> _request)
{
    assert(_request);

    constexpr char method[] = "delMsg";

    _request->set_gzip(true);
    _request->set_url(urls::get_url(urls::url_type::rapi_host));
    _request->set_normalized_url(method);
    _request->set_keep_alive();

    rapidjson::Document doc(rapidjson::Type::kObjectType);
    auto& a = doc.GetAllocator();

    doc.AddMember("method", method, a);
    doc.AddMember("reqId", get_req_id(), a);

    rapidjson::Value node_params(rapidjson::Type::kObjectType);
    node_params.AddMember("sn", contact_aimid_, a);
    node_params.AddMember("msgId", message_id_, a);
    node_params.AddMember("shared", for_all_, a);

    doc.AddMember("params", std::move(node_params), a);

    sign_packet(doc, a, _request);

    __INFO(
        "delete_history",
        "sending message deletion request\n"
        "    contact_aimid=<%1%>\n"
        "    message_id=<%2%>\n"
        "    for_all=<%3%>",
        contact_aimid_ % message_id_ % logutils::yn(for_all_)
    );

    if (!params_.full_log_)
    {
        log_replace_functor f;
        f.add_marker("a");
        f.add_json_marker("aimsid", aimsid_range_evaluator());
        _request->set_replace_log_function(f);
    }

    return 0;
}

int32_t del_message::parse_response_data(const rapidjson::Value& _data)
{
    return 0;
}

int32_t del_message::on_response_error_code()
{
    __INFO(
        "delete_history",
        "    status_code=<%1%>",
        status_code_
    );

    return robusto_packet::on_response_error_code();
}

CORE_WIM_NS_END
