#include "stdafx.h"

#include "fetch_event_recent_call_log.h"
#include "../wim_im.h"
#include "../wim_packet.h"

using namespace core;
using namespace wim;

fetch_event_recent_call_log::fetch_event_recent_call_log()
{
}


fetch_event_recent_call_log::~fetch_event_recent_call_log() = default;

int32_t fetch_event_recent_call_log::parse(const rapidjson::Value& _node_event_data)
{
    persons_ = parse_persons(_node_event_data);
    log_.unserialize(_node_event_data, persons_);
    return 0;
}

void fetch_event_recent_call_log::on_im(std::shared_ptr<core::wim::im> _im, std::shared_ptr<auto_callback> _on_complete)
{
    _im->on_event_recent_call_log(this, _on_complete);
}

const core::archive::call_log_cache& fetch_event_recent_call_log::get_log() const
{
    return log_;
}

const core::archive::persons_map& fetch_event_recent_call_log::get_persons() const
{
    return persons_;
}
