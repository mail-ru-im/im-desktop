#include "stdafx.h"

#include "fetch_event_my_info.h"
#include "../wim_im.h"
#include "../wim_packet.h"
#include "../wim_contactlist_cache.h"

using namespace core;
using namespace wim;

fetch_event_my_info::fetch_event_my_info()
    :    info_(std::make_shared<my_info>())
{
}


fetch_event_my_info::~fetch_event_my_info() = default;

int32_t fetch_event_my_info::parse(const rapidjson::Value& _node_event_data)
{
    return info_->unserialize(_node_event_data);
}

void fetch_event_my_info::on_im(std::shared_ptr<core::wim::im> _im, std::shared_ptr<auto_callback> _on_complete)
{
    _im->on_event_my_info(this, _on_complete);
}
