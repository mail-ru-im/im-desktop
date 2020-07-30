#include "stdafx.h"
#include "fetch_event_buddy_list.h"

#include "../wim_im.h"
#include "../wim_contactlist_cache.h"
#include "../wim_packet.h"

using namespace core;
using namespace wim;

fetch_event_buddy_list::fetch_event_buddy_list() = default;

fetch_event_buddy_list::~fetch_event_buddy_list() = default;

int32_t fetch_event_buddy_list::parse(const rapidjson::Value& _node_event_data)
{
    cl_ = std::make_shared<core::wim::contactlist>();

    return cl_->unserialize(_node_event_data);
}

void fetch_event_buddy_list::on_im(std::shared_ptr<core::wim::im> _im, std::shared_ptr<auto_callback> _on_complete)
{
    _im->on_event_buddies_list(this, _on_complete);
}

std::shared_ptr<wim::contactlist> fetch_event_buddy_list::get_contactlist() const
{
    return cl_;
}