#include "stdafx.h"

#include "../../../log/log.h"
#include "../../../http_request.h"
#include "../log_replace_functor.h"

#include "del_history.h"

CORE_WIM_NS_BEGIN

del_history::del_history(
    wim_packet_params _params,
    const int64_t _up_to_id,
    const std::string &_contact_aimid
)
    : robusto_packet(std::move(_params))
    , up_to_id_(_up_to_id)
    , contact_aimid_(_contact_aimid)
{
    im_assert(up_to_id_ > 0);
    im_assert(!contact_aimid_.empty());
}

int del_history::minimal_supported_api_version() const
{
    return core::urls::api_version::instance().minimal_supported();
}

int32_t del_history::init_request(const std::shared_ptr<core::http_request_simple>& _request)
{
    im_assert(_request);

    rapidjson::Document doc(rapidjson::Type::kObjectType);
    auto& a = doc.GetAllocator();

    rapidjson::Value node_params(rapidjson::Type::kObjectType);
    node_params.AddMember("sn", contact_aimid_, a);
    node_params.AddMember("uptoMsgId", up_to_id_, a);

    doc.AddMember("params", std::move(node_params), a);

    setup_common_and_sign(doc, a, _request, get_method());

    __INFO(
        "delete_history",
        "sending history deletion request\n"
        "    contact_aimid=<%1%>\n"
        "    up_to=<%2%>",
        contact_aimid_ % up_to_id_);

    if (!params_.full_log_)
    {
        log_replace_functor f;
        f.add_json_marker("aimsid", aimsid_range_evaluator());
        _request->set_replace_log_function(f);
    }

    return 0;
}

int32_t del_history::parse_response_data(const rapidjson::Value& _data)
{
    return 0;
}

int32_t del_history::on_response_error_code()
{
    __INFO(
        "delete_history",
        "    status_code=<%1%>",
        status_code_
    );

    return robusto_packet::on_response_error_code();
}

std::string_view del_history::get_method() const
{
    return "delHistory";
}


CORE_WIM_NS_END
