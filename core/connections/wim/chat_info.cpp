#include "stdafx.h"
#include "chat_info.h"

#include "../../tools/json_helper.h"

using namespace core;
using namespace wim;

chat_info::chat_info()
    : create_time_(0)
    , admins_count_(0)
    , members_count_(0)
    , friend_count_(0)
    , blocked_count_(0)
    , pending_count_(0)
    , you_blocked_(false)
    , you_pending_(false)
    , you_member_(false)
    , public_(false)
    , live_(false)
    , controlled_(false)
    , joinModeration_(false)
    , persons_(std::make_shared<core::archive::persons_map>())
{
}

int32_t chat_info::unserialize(const rapidjson::Value& _node)
{
    tools::unserialize_value(_node, "sn", aimid_);
    tools::unserialize_value(_node, "name", name_);
    tools::unserialize_value(_node, "location", location_);
    tools::unserialize_value(_node, "stamp", stamp_);
    tools::unserialize_value(_node, "about", about_);
    tools::unserialize_value(_node, "rules", rules_);
    tools::unserialize_value(_node, "createTime", create_time_);
    tools::unserialize_value(_node, "adminsCount", admins_count_);
    tools::unserialize_value(_node, "membersCount", members_count_);
    tools::unserialize_value(_node, "friendsCount", friend_count_);
    tools::unserialize_value(_node, "joinModeration", joinModeration_);

    if (const auto iter_yours = _node.FindMember("you"); iter_yours != _node.MemberEnd())
    {
        tools::unserialize_value(iter_yours->value, "role", your_role_);
        you_member_ = !your_role_.empty();

        tools::unserialize_value(iter_yours->value, "blocked", you_blocked_);
        tools::unserialize_value(iter_yours->value, "pending", you_pending_);
    }

    tools::unserialize_value(_node, "public", public_);
    tools::unserialize_value(_node, "live", live_);
    tools::unserialize_value(_node, "controlled", controlled_);
    tools::unserialize_value(_node, "blockedCount", blocked_count_);
    tools::unserialize_value(_node, "pendingCount", pending_count_);
    tools::unserialize_value(_node, "membersVersion", members_version_);
    tools::unserialize_value(_node, "infoVersion", info_version_);
    tools::unserialize_value(_node, "creator", creator_);
    tools::unserialize_value(_node, "defaultRole", default_role_);

    archive::person p;
    p.friendly_ = name_;
    persons_->emplace(aimid_, std::move(p));

    if (const auto iter_members = _node.FindMember("members"); iter_members != _node.MemberEnd() && iter_members->value.IsArray())
    {
        members_.reserve(iter_members->value.Size());
        for (const auto& member :  iter_members->value.GetArray())
        {
            chat_member_info member_info;

            tools::unserialize_value(member, "sn", member_info.aimid_);
            tools::unserialize_value(member, "role", member_info.role_);
            tools::unserialize_value(member, "friend", member_info.friend_);
            tools::unserialize_value(member, "noAvatar", member_info.no_avatar_);
            tools::unserialize_value(member, "lastseen", member_info.lastseen_);

            if (const auto iter_anketa = member.FindMember("anketa"); iter_anketa != member.MemberEnd())
            {
                tools::unserialize_value(iter_anketa->value, "nick", member_info.nick_);
                tools::unserialize_value(iter_anketa->value, "friendly", member_info.friendly_);

                archive::person p;
                p.friendly_ = member_info.friendly_;
                p.nick_ = member_info.nick_;
                p.official_ = is_official(iter_anketa->value);
                persons_->emplace(member_info.aimid_, std::move(p));
            }
            members_.push_back(std::move(member_info));
        }
    }

    if (const auto iter_owner = _node.FindMember("owner"); iter_owner != _node.MemberEnd())
        tools::unserialize_value(iter_owner->value, "sn", owner_);

    return 0;
}

void chat_info::serialize(core::coll_helper _coll) const
{
    _coll.set_value_as_string("aimid", aimid_);
    _coll.set_value_as_string("name", name_);
    _coll.set_value_as_string("location", location_);
    _coll.set_value_as_string("stamp", stamp_);
    _coll.set_value_as_string("about", about_);
    _coll.set_value_as_string("rules", rules_);
    _coll.set_value_as_string("your_role", your_role_);
    _coll.set_value_as_string("owner", owner_);
    _coll.set_value_as_string("members_version", members_version_);
    _coll.set_value_as_string("info_version", info_version_);
    _coll.set_value_as_int("create_time", create_time_);
    _coll.set_value_as_int("admins_count", admins_count_);
    _coll.set_value_as_int("members_count", members_count_);
    _coll.set_value_as_int("friend_count", friend_count_);
    _coll.set_value_as_int("blocked_count", blocked_count_);
    _coll.set_value_as_int("pending_count", pending_count_);
    _coll.set_value_as_bool("you_blocked", you_blocked_);
    _coll.set_value_as_bool("you_pending", you_pending_);
    _coll.set_value_as_bool("you_member", you_member_);
    _coll.set_value_as_bool("public", public_);
    _coll.set_value_as_bool("live", live_);
    _coll.set_value_as_bool("controlled", controlled_);
    _coll.set_value_as_string("stamp", stamp_);
    _coll.set_value_as_bool("joinModeration", joinModeration_);
    _coll.set_value_as_string("creator", creator_);
    _coll.set_value_as_string("default_role", default_role_);

    ifptr<iarray> members_array(_coll->create_array());

    if (!members_.empty())
    {
        members_array->reserve((int32_t)members_.size());
        for (const auto& member : members_)
        {
            coll_helper _coll_message(_coll->create_collection(), true);
            _coll_message.set_value_as_string("aimid", member.aimid_);
            _coll_message.set_value_as_string("role", member.role_);
            _coll_message.set_value_as_string("nick", member.nick_);
            _coll_message.set_value_as_string("friendly", member.friendly_);
            _coll_message.set_value_as_bool("friend", member.friend_);
            _coll_message.set_value_as_bool("no_avatar", member.no_avatar_);
            _coll_message.set_value_as_int("lastseen", member.lastseen_);
            ifptr<ivalue> val(_coll->create_value());
            val->set_as_collection(_coll_message.get());
            members_array->push_back(val.get());
        }
    }

    _coll.set_value_as_array("members", members_array.get());
}

common_chat_info::common_chat_info()
    : members_count_(0)
{
}

int32_t common_chat_info::unserialize(const rapidjson::Value& _node)
{
    tools::unserialize_value(_node, "sn", aimid_);
    tools::unserialize_value(_node, "name", friednly_);
    tools::unserialize_value(_node, "membersCount", members_count_);

    return 0;
}

void common_chat_info::serialize(core::coll_helper _coll) const
{
    _coll.set_value_as_string("aimid", aimid_);
    _coll.set_value_as_string("friendly", friednly_);
    _coll.set_value_as_int("membersCount", members_count_);
}

chat_members::chat_members()
    : persons_(std::make_shared<core::archive::persons_map>())
{
}

int32_t chat_members::unserialize(const rapidjson::Value& _node)
{
    tools::unserialize_value(_node, "sn", aimid_);
    tools::unserialize_value(_node, "cursor", cursor_);

    if (const auto iter_members = _node.FindMember("members"); iter_members != _node.MemberEnd() && iter_members->value.IsArray())
    {
        members_.reserve(iter_members->value.Size());
        for (const auto& member : iter_members->value.GetArray())
        {
            chat_member_short_info member_info;
            tools::unserialize_value(member, "sn", member_info.aimid_);
            tools::unserialize_value(member, "role", member_info.role_);
            tools::unserialize_value(member, "lastseen", member_info.lastseen_);
            members_.push_back(std::move(member_info));
        }
    }

    *persons_ = parse_persons(_node);

    return 0;
}

void chat_members::serialize(core::coll_helper _coll) const
{
    _coll.set_value_as_string("aimid", aimid_);
    _coll.set_value_as_string("cursor", cursor_);

    ifptr<iarray> members_array(_coll->create_array());
    if (!members_.empty())
    {
        members_array->reserve((int32_t)members_.size());
        for (const auto& member : members_)
        {
            coll_helper _coll_message(_coll->create_collection(), true);
            _coll_message.set_value_as_string("aimid", member.aimid_);
            _coll_message.set_value_as_string("role", member.role_);
            _coll_message.set_value_as_int("lastseen", member.lastseen_);
            ifptr<ivalue> val(_coll->create_value());
            val->set_as_collection(_coll_message.get());
            members_array->push_back(val.get());
        }
    }
    _coll.set_value_as_array("members", members_array.get());
}

chat_contacts::chat_contacts()
    : persons_(std::make_shared<core::archive::persons_map>())
{
}

int32_t chat_contacts::unserialize(const rapidjson::Value& _node)
{
    tools::unserialize_value(_node, "cursor", cursor_);

    if (const auto iter_members = _node.FindMember("members"); iter_members != _node.MemberEnd() && iter_members->value.IsArray())
    {
        members_.reserve(iter_members->value.Size());
        for (const auto& member : iter_members->value.GetArray())
        {
            std::string aimid;
            tools::unserialize_value(member, "sn", aimid);
            members_.push_back(std::move(aimid));
        }
    }

    *persons_ = parse_persons(_node);

    return 0;
}

void chat_contacts::serialize(core::coll_helper _coll) const
{
    _coll.set_value_as_string("cursor", cursor_);

    ifptr<iarray> members_array(_coll->create_array());
    if (!members_.empty())
    {
        members_array->reserve((int32_t)members_.size());
        for (const auto& member : members_)
        {
            coll_helper _coll_message(_coll->create_collection(), true);
            _coll_message.set_value_as_string("aimid", member);
            ifptr<ivalue> val(_coll->create_value());
            val->set_as_collection(_coll_message.get());
            members_array->push_back(val.get());
        }
    }
    _coll.set_value_as_array("members", members_array.get());
}
