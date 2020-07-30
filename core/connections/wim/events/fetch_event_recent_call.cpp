#include "stdafx.h"

#include "fetch_event_recent_call.h"
#include "../wim_im.h"
#include "../wim_packet.h"

using namespace core;
using namespace wim;

fetch_event_recent_call::fetch_event_recent_call()
{
}


fetch_event_recent_call::~fetch_event_recent_call() = default;

int32_t fetch_event_recent_call::parse(const rapidjson::Value& _node_event_data)
{
    persons_ = parse_persons(_node_event_data);
    call_.unserialize(_node_event_data, persons_);
    return 0;
}

void fetch_event_recent_call::on_im(std::shared_ptr<core::wim::im> _im, std::shared_ptr<auto_callback> _on_complete)
{
    _im->on_event_recent_call(this, _on_complete);
}

const core::archive::call_info fetch_event_recent_call::get_call() const
{
    return call_;
}

const core::archive::persons_map& fetch_event_recent_call::get_persons() const
{
    return persons_;
}
