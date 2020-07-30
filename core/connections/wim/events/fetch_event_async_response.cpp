#include "stdafx.h"
#include "fetch_event_async_response.h"
#include "../../wim/wim_packet.h"
#include "../../wim/wim_im.h"

#include "../../../tools/json_helper.h"

using namespace core;
using namespace wim;

fetch_event_async_response::fetch_event_async_response() = default;

fetch_event_async_response::~fetch_event_async_response() = default;

int32_t fetch_event_async_response::parse(const rapidjson::Value& _node_event_data)
{
    tools::unserialize_value(_node_event_data, "reqId", req_id_);
    payload_.unserialize(_node_event_data);

    return 0;
}

void core::wim::fetch_event_async_response::on_im(std::shared_ptr<core::wim::im> _im, std::shared_ptr<auto_callback> _on_complete)
{
    _im->on_event_async_response(this, _on_complete);
}

const std::string& fetch_event_async_response::get_bot_req_id() const noexcept
{
    return req_id_;
}

const bot_payload& fetch_event_async_response::get_payload() const noexcept
{
    return payload_;
}
