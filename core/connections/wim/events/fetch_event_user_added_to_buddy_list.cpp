#include "stdafx.h"

#include "../wim_im.h"
#include "../wim_packet.h"

#include "../../../log/log.h"

#include "fetch_event_user_added_to_buddy_list.h"

CORE_WIM_NS_BEGIN

int32_t fetch_event_user_added_to_buddy_list::parse(const rapidjson::Value& _node_event_data)
{
    const auto requester_iter = _node_event_data.FindMember("requester");
    if (requester_iter == _node_event_data.MemberEnd())
    {
        return -1;
    }

    const auto display_aimid_iter = _node_event_data.FindMember("displayAIMid");
    if (display_aimid_iter == _node_event_data.MemberEnd())
    {
        return -1;
    }

    requester_uid_ = rapidjson_get_string(requester_iter->value);
    assert(!requester_uid_.empty());

    display_aimid_ = rapidjson_get_string(display_aimid_iter->value);
    assert(!display_aimid_.empty());

    if (requester_uid_.empty() || display_aimid_.empty())
    {
        return -1;
    }

    return 0;
}

void fetch_event_user_added_to_buddy_list::on_im(std::shared_ptr<core::wim::im> _im, std::shared_ptr<auto_callback> _on_complete)
{
    assert(_im);

    _im->on_event_user_added_to_buddy_list(this, _on_complete);

}

const std::string& fetch_event_user_added_to_buddy_list::get_display_aimid() const
{
    assert(!display_aimid_.empty());
    return display_aimid_;
}

const std::string& fetch_event_user_added_to_buddy_list::get_requester_aimid() const
{
    assert(!requester_uid_.empty());
    return requester_uid_;
}

CORE_WIM_NS_END