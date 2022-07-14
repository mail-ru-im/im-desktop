#include "stdafx.h"

#include "group_invitations_cancel.h"
#include "../../../http_request.h"
#include "../../../../common.shared/json_helper.h"
#include "../log_replace_functor.h"

using namespace core;
using namespace wim;

group_invitations_cancel::group_invitations_cancel(wim_packet_params _params, std::string _contact, std::string _chat)
    : robusto_packet(std::move(_params))
    , contact_(std::move(_contact))
    , chat_(std::move(_chat))
{
    im_assert(!contact_.empty() && !chat_.empty());
}

std::string_view group_invitations_cancel::get_method() const
{
    return "group/invitations/cancel";
}

int core::wim::group_invitations_cancel::minimal_supported_api_version() const
{
    return core::urls::api_version::instance().minimal_supported();
}

int32_t group_invitations_cancel::init_request(const std::shared_ptr<core::http_request_simple>& _request)
{
    rapidjson::Document doc(rapidjson::Type::kObjectType);
    auto& a = doc.GetAllocator();

    rapidjson::Value node_params(rapidjson::Type::kObjectType);
    node_params.AddMember("sn", contact_, a);
    node_params.AddMember("chatId", chat_, a);
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