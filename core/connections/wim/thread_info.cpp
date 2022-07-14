#include "stdafx.h"
#include "thread_info.h"

#include "../../../common.shared/json_helper.h"

using namespace core;
using namespace wim;

thread_info::thread_info()
    : persons_(std::make_shared<core::archive::persons_map>())
{
}

int32_t thread_info::unserialize(const rapidjson::Value& _node)
{
    tools::unserialize_value(_node, "subscribersCount", subscribers_count_);
    tools::unserialize_value(_node, "threadId", aimid_);

    if (const auto iter_yours = _node.FindMember("you"); iter_yours != _node.MemberEnd())
        tools::unserialize_value(iter_yours->value, "subscriber", you_subscriber_);

    if (const auto iter_members = _node.FindMember("topSubscribers"); iter_members != _node.MemberEnd() && iter_members->value.IsArray())
    {
        subscribers_.reserve(iter_members->value.Size());
        for (const auto& member : iter_members->value.GetArray())
        {
            chat_member_short_info member_info;
            tools::unserialize_value(member, "sn", member_info.aimid_);
            member_info.lastseen_.unserialize(member);
            subscribers_.push_back(std::move(member_info));
        }
    }

    return 0;
}

void thread_info::serialize(core::coll_helper _coll) const
{
    _coll.set_value_as_string("thread_id", aimid_);
    _coll.set_value_as_int("subscribers_count", subscribers_count_);
    _coll.set_value_as_bool("you_subscriber", you_subscriber_);

    ifptr<iarray> members_array(_coll->create_array());
    if (!subscribers_.empty())
    {
        members_array->reserve(static_cast<int32_t>(subscribers_.size()));
        for (const auto& member : subscribers_)
        {
            coll_helper _coll_message(_coll->create_collection(), true);
            _coll_message.set_value_as_string("sn", member.aimid_);
            member.lastseen_.serialize(_coll_message);

            ifptr<ivalue> val(_coll->create_value());
            val->set_as_collection(_coll_message.get());
            members_array->push_back(val.get());
        }
    }
    _coll.set_value_as_array("top_subscribers", members_array.get());
}

thread_subscribers::thread_subscribers()
    : persons_(std::make_shared<core::archive::persons_map>())
{
}

int32_t thread_subscribers::unserialize(const rapidjson::Value& _node)
{
    tools::unserialize_value(_node, "cursor", cursor_);

    if (const auto iter_subscribers = _node.FindMember("subscribers"); iter_subscribers != _node.MemberEnd() && iter_subscribers->value.IsArray())
    {
        subscribers_.reserve(iter_subscribers->value.Size());
        for (const auto& subscriber : iter_subscribers->value.GetArray())
        {
            chat_member_short_info subscriber_info;
            tools::unserialize_value(subscriber, "sn", subscriber_info.aimid_);
            subscriber_info.lastseen_.unserialize(subscriber);

            subscribers_.push_back(std::move(subscriber_info));
        }
    }

    *persons_ = parse_persons(_node);

    return 0;
}

void thread_subscribers::serialize(core::coll_helper _coll) const
{
    _coll.set_value_as_string("aimid", thread_id_);
    _coll.set_value_as_string("cursor", cursor_);

    ifptr<iarray> subscribers_array(_coll->create_array());
    if (!subscribers_.empty())
    {
        subscribers_array->reserve((int32_t)subscribers_.size());
        for (const auto& subscriber : subscribers_)
        {
            coll_helper _coll_message(_coll->create_collection(), true);
            _coll_message.set_value_as_string("aimid", subscriber.aimid_);
            subscriber.lastseen_.serialize(_coll_message);

            ifptr<ivalue> val(_coll->create_value());
            val->set_as_collection(_coll_message.get());
            subscribers_array->push_back(val.get());
        }
    }
    _coll.set_value_as_array("subscribers", subscribers_array.get());
}

void thread_subscribers::set_id(const std::string& _id)
{
    thread_id_ = _id;
}

