#include "stdafx.h"

#include "../wim_im.h"
#include "tools/json_helper.h"
#include "fetch_event_call_room_info.h"

namespace core::wim
{

void fetch_event_call_room_info::call_room_info::serialize(icollection* _collection) const
{
    coll_helper coll(_collection, false);
    coll.set_value_as_string("room_id", room_id_);
    coll.set_value_as_int64("members_count", members_count_);
    coll.set_value_as_bool("failed", failed_);
}

bool fetch_event_call_room_info::call_room_info::unserialize(const rapidjson::Value& _node)
{
    auto result = tools::unserialize_value(_node, "roomId", room_id_);
    result &= tools::unserialize_value(_node, "membersCount", members_count_);
    failed_ = _node.FindMember("error") != _node.MemberEnd();

    return result;
}

int32_t fetch_event_call_room_info::parse(const rapidjson::Value& _node_event_data)
{
    return info_.unserialize(_node_event_data);
}

void fetch_event_call_room_info::on_im(std::shared_ptr<im> _im, std::shared_ptr<auto_callback> _on_complete)
{
    _im->on_event_call_room_info(this, std::move(_on_complete));
}

const fetch_event_call_room_info::call_room_info& fetch_event_call_room_info::get_info() const
{
    return info_;
}


}
