#include "stdafx.h"

#include "id_info.h"
#include "../../tools/json_helper.h"

namespace core::wim
{
    void id_info_response::serialize(core::coll_helper _root_coll) const
    {
        const auto serialize_common = [this](core::coll_helper& _coll)
        {
            _coll.set_value_as_string("sn", sn_);
            _coll.set_value_as_string("name", name_);
            _coll.set_value_as_string("description", description_);
        };

        if (type_ == id_type::user)
        {
            coll_helper coll_user(_root_coll->create_collection(), true);
            serialize_common(coll_user);
            coll_user.set_value_as_string("nick", nick_);

            _root_coll.set_value_as_collection("user", coll_user.get());
        }
        else if (type_ == id_type::chat)
        {
            coll_helper coll_chat(_root_coll->create_collection(), true);
            serialize_common(coll_chat);
            coll_chat.set_value_as_string("stamp", stamp_);
            coll_chat.set_value_as_int64("member_count", member_count_);

            _root_coll.set_value_as_collection("chat", coll_chat.get());
        }
    }

    int32_t id_info_response::unserialize(const rapidjson::Value& _node)
    {
        if (const auto user = _node.FindMember("user"); user != _node.MemberEnd() && user->value.IsObject())
        {
            if (tools::unserialize_value(user->value, "sn", sn_) && !sn_.empty())
            {
                tools::unserialize_value(user->value, "about", description_);
                tools::unserialize_value(user->value, "friendly", name_);
                tools::unserialize_value(user->value, "nick", nick_);

                type_ = id_info_response::id_type::user;
            }
        }
        else if (const auto chat = _node.FindMember("chat"); chat != _node.MemberEnd() && chat->value.IsObject())
        {
            if (tools::unserialize_value(chat->value, "sn", sn_) && !sn_.empty())
            {
                tools::unserialize_value(chat->value, "about", description_);
                tools::unserialize_value(chat->value, "name", name_);
                tools::unserialize_value(chat->value, "memberCount", member_count_);
                tools::unserialize_value(chat->value, "stamp", stamp_);

                type_ = id_info_response::id_type::chat;
            }
        }

        return 0;
    }
}
