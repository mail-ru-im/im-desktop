#include "stdafx.h"
#include "fetch_event_mchat.h"
#include "../../wim/wim_packet.h"
#include "../../wim/wim_im.h"

#include "../../../tools/json_helper.h"

using namespace core;
using namespace wim;

fetch_event_mchat::fetch_event_mchat() = default;

fetch_event_mchat::~fetch_event_mchat() = default;

int32_t fetch_event_mchat::parse(const rapidjson::Value& _node_event_data)
{
    if (const auto info = _node_event_data.FindMember("mchats"); info != _node_event_data.MemberEnd() && info->value.IsArray())
    {
        for (const auto& x : info->value.GetArray())
        {
            if (tools::unserialize_value(x, "sip_code", sip_code_))
                break;
        }
    }
    return wpie_http_parse_response;
}

void core::wim::fetch_event_mchat::on_im(std::shared_ptr<core::wim::im> _im, std::shared_ptr<auto_callback> _on_complete)
{
    _im->on_event_mchat(this, _on_complete);
}
