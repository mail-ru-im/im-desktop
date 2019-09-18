#include "stdafx.h"
#include "user_info.h"
#include "../../tools/json_helper.h"

using namespace core;
using namespace wim;

int32_t user_info::unserialize(const rapidjson::Value& _node)
{
    tools::unserialize_value(_node, "firstName", first_name_);
    tools::unserialize_value(_node, "lastName", last_name_);
    tools::unserialize_value(_node, "friendly", friendly_);
    tools::unserialize_value(_node, "nick", nick_);
    tools::unserialize_value(_node, "about", about_);
    tools::unserialize_value(_node, "phone", phone_);
    tools::unserialize_value(_node, "avatarId", avatar_id_);
    tools::unserialize_value(_node, "lastseen", lastseen_);
    tools::unserialize_value(_node, "commonChats", common_chats_);
    tools::unserialize_value(_node, "mute", mute_);
    tools::unserialize_value(_node, "official", official_);

    return 0;
}

void user_info::serialize(core::coll_helper _coll)
{
    _coll.set_value_as_string("firstName", first_name_);
    _coll.set_value_as_string("lastName", last_name_);
    _coll.set_value_as_string("friendly", friendly_);
    _coll.set_value_as_string("nick", nick_);
    _coll.set_value_as_string("about", about_);
    _coll.set_value_as_string("phone", phone_);
    _coll.set_value_as_int("lastseen", lastseen_);
    _coll.set_value_as_int("commonChats", common_chats_);
    _coll.set_value_as_bool("mute", mute_);
    _coll.set_value_as_bool("official", official_);
}
