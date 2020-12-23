#include "stdafx.h"
#include "set_dlg_state.h"

#include "../../../http_request.h"

using namespace core;
using namespace wim;


set_dlg_state::set_dlg_state(wim_packet_params _params, set_dlg_state_params _dlg_params)
    : robusto_packet(std::move(_params))
    , params_(std::move(_dlg_params))
{
}

set_dlg_state::~set_dlg_state() = default;

int32_t set_dlg_state::init_request(const std::shared_ptr<core::http_request_simple>& _request)
{
    rapidjson::Document doc(rapidjson::Type::kObjectType);
    auto& a = doc.GetAllocator();

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

    setup_common_and_sign(doc, a, _request, get_method());

    if (!robusto_packet::params_.full_log_)
    {
        log_replace_functor f;
        f.add_json_marker("aimsid", aimsid_range_evaluator());
        _request->set_replace_log_function(f);
    }

    return 0;
}

priority_t set_dlg_state::get_priority() const
{
    return priority_protocol();
}

std::string_view set_dlg_state::get_method() const
{
    return "setDlgState";
}

bool set_dlg_state::support_self_resending() const
{
    return true;
}

const set_dlg_state_params& set_dlg_state::get_params() const
{
    return params_;
}

void set_dlg_state::set_params(const set_dlg_state_params& _params)
{
    params_ = _params;
}
