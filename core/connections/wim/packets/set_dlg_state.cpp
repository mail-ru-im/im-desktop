#include "stdafx.h"
#include "set_dlg_state.h"

#include "../../../http_request.h"
#include "../../urls_cache.h"

using namespace core;
using namespace wim;


set_dlg_state::set_dlg_state(wim_packet_params _params, set_dlg_state_params _dlg_params)
    :    robusto_packet(std::move(_params)),
    params_(std::move(_dlg_params))
{
}


set_dlg_state::~set_dlg_state()
{

}

int32_t set_dlg_state::init_request(std::shared_ptr<core::http_request_simple> _request)
{
    constexpr char method[] = "setDlgState";

    _request->set_gzip(true);
    _request->set_url(urls::get_url(urls::url_type::rapi_host));
    _request->set_normalized_url(method);
    _request->set_keep_alive();
    _request->set_priority(priority_protocol());

    rapidjson::Document doc(rapidjson::Type::kObjectType);

    auto& a = doc.GetAllocator();

    doc.AddMember("method", method, a);
    doc.AddMember("reqId", get_req_id(), a);

    rapidjson::Value node_params(rapidjson::Type::kObjectType);

    node_params.AddMember("sn", params_.aimid_, a);
    if (params_.last_read_ > 0)
        node_params.AddMember("lastRead", params_.last_read_, a);
    if (params_.last_delivered_ > 0)
        node_params.AddMember("lastDelivered", params_.last_delivered_, a);
    if (params_.stranger_)
        node_params.AddMember("stranger", *params_.stranger_, a);

    if (params_.has_exclude())
    {
        rapidjson::Value exclude(rapidjson::Type::kArrayType);
        if (params_.test_exclude(set_dlg_state_params::exclude::call))
            exclude.PushBack("call", a);
        if (params_.test_exclude(set_dlg_state_params::exclude::text))
            exclude.PushBack("text", a);
        if (params_.test_exclude(set_dlg_state_params::exclude::mention))
            exclude.PushBack("mention", a);
        if (params_.test_exclude(set_dlg_state_params::exclude::ptt))
            exclude.PushBack("pttListen", a);
        assert(!exclude.Empty());
        node_params.AddMember("exclude", std::move(exclude), a);
    }

    doc.AddMember("params", std::move(node_params), a);

    sign_packet(doc, a, _request);

    if (!robusto_packet::params_.full_log_)
    {
        log_replace_functor f;
        f.add_json_marker("aimsid", aimsid_range_evaluator());
        f.add_marker("a");
        _request->set_replace_log_function(f);
    }

    return 0;
}
