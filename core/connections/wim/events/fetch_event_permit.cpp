#include "stdafx.h"

#include "fetch_event_permit.h"
#include "../wim_im.h"
#include "../permit_info.h"

using namespace core;
using namespace wim;

fetch_event_permit::fetch_event_permit()
    : permit_info_(std::make_unique<permit_info>())
{
}

fetch_event_permit::~fetch_event_permit() = default;

const ignorelist_cache& fetch_event_permit::ignore_list() const
{
    return permit_info_->get_ignore_list();
}

int32_t fetch_event_permit::parse(const rapidjson::Value& _node_event_data)
{
    return permit_info_->parse_response_data(_node_event_data);
}

void fetch_event_permit::on_im(std::shared_ptr<core::wim::im> _im, std::shared_ptr<auto_callback> _on_complete)
{
    _im->on_event_permit(this, _on_complete);
}
