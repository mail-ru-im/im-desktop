#include "stdafx.h"

#include "thread_update.h"
#include "../common.shared/json_helper.h"
#include "storage.h"
#include "../corelib/collection_helper.h"

namespace
{
    enum data_tlv_fields : uint32_t
    {
        tlv_thread_id = 1,
        tlv_parent_topic_type = 2,
        tlv_parent_topic_chat_id = 3,
        tlv_parent_topic_msg_id = 4,
        tlv_replies_count = 5,
        tlv_unread_msg_count = 6,
        tlv_unread_mention_count = 7,
        tlv_last_read = 8,
        tlv_last_read_mention = 9,
        tlv_subscriber = 10,
        tlv_parent_topic_task_id = 11
    };
}

namespace core::archive
{
    void thread_update::serialize(rapidjson::Value& _node, rapidjson_allocator& _a) const
    {
        _node.AddMember("threadId", id_, _a);
        _node.AddMember("repliesCount", replies_count_, _a);
        _node.AddMember("unreadMessagesCount", unread_message_count_, _a);
        _node.AddMember("unreadMentionsMeCount", unread_mention_me_count_, _a);

        rapidjson::Value yours_node(rapidjson::Type::kObjectType);
        yours_node.AddMember("lastRead", yours_last_read_, _a);
        yours_node.AddMember("lastReadMention", yours_last_read_mention_, _a);
        _node.AddMember("yours", yours_node, _a);

        rapidjson::Value you_node(rapidjson::Type::kObjectType);
        you_node.AddMember("subscriber", subscriber_, _a);
        _node.AddMember("you", you_node, _a);

        rapidjson::Value parent_topic_node(rapidjson::Type::kObjectType);
        parent_topic_.serialize(parent_topic_node, _a);
        _node.AddMember("parentTopic", parent_topic_node, _a);
    }

    int32_t thread_update::unserialize(const rapidjson::Value& _node)
    {
        if (const auto it = _node.FindMember("parentTopic"); it != _node.MemberEnd())
        {
            if (!parent_topic_.unserialize(it->value))
                return 1;
        }

        tools::unserialize_value(_node, "threadId", id_);
        tools::unserialize_value(_node, "repliesCount", replies_count_);

        if (!tools::unserialize_value(_node, "unreadMessagesCount", unread_message_count_)) // fields have different names in threadUpdate event and thread/feed/get
            tools::unserialize_value(_node, "unreadCnt", unread_message_count_);
        if (!tools::unserialize_value(_node, "unreadMentionsMeCount", unread_mention_me_count_))
            tools::unserialize_value(_node, "unreadMentionMeCount", unread_mention_me_count_);

        if (const auto it = _node.FindMember("yours"); it != _node.MemberEnd())
        {
            tools::unserialize_value(it->value, "lastRead", yours_last_read_);
            tools::unserialize_value(it->value, "lastReadMention", yours_last_read_mention_);
        }

        if (const auto it = _node.FindMember("you"); it != _node.MemberEnd())
            tools::unserialize_value(it->value, "subscriber", subscriber_);

        return 0;
    }

    bool thread_update::unserialize(core::tools::binary_stream& _data)
    {
        core::tools::tlvpack pack;
        if (!pack.unserialize(_data))
            return false;

        for (auto field = pack.get_first(); field; field = pack.get_next())
        {
            switch ((data_tlv_fields)field->get_type())
            {
            case tlv_thread_id:
                id_ = field->get_value<std::string>({});
                break;
            case tlv_parent_topic_type:
                parent_topic_.type_ = field->get_value<threads::parent_topic::type>(threads::parent_topic::type::invalid);
                break;
            case tlv_parent_topic_chat_id:
                parent_topic_.chat_id_ = field->get_value<std::string>({});
                break;
            case tlv_parent_topic_msg_id:
                parent_topic_.msg_id_ = field->get_value<int64_t>(-1);
                break;
            case tlv_parent_topic_task_id:
                parent_topic_.task_id_ = field->get_value<std::string>({});
                break;
            case tlv_replies_count:
                replies_count_ = field->get_value<int>(0);
                break;
            case tlv_unread_msg_count:
                unread_message_count_ = field->get_value<int>(0);
                break;
            case tlv_unread_mention_count:
                unread_mention_me_count_ = field->get_value<int>(0);
                break;
            case tlv_last_read:
                yours_last_read_ = field->get_value<int64_t>(-1);
                break;
            case tlv_last_read_mention:
                yours_last_read_mention_ = field->get_value<int64_t>(-1);
                break;
            case tlv_subscriber:
                subscriber_ = field->get_value<bool>(false);
                break;
            default:
                break;
            }
        }

        return parent_topic_.is_valid() && !id_.empty();
    }

    void thread_update::serialize(core::tools::binary_stream& _data) const
    {
        core::tools::tlvpack pack;
        pack.push_child(core::tools::tlv(tlv_thread_id, id_));
        pack.push_child(core::tools::tlv(tlv_parent_topic_type, parent_topic_.type_));
        pack.push_child(core::tools::tlv(tlv_parent_topic_chat_id, parent_topic_.chat_id_));
        pack.push_child(core::tools::tlv(tlv_parent_topic_msg_id, parent_topic_.msg_id_));
        pack.push_child(core::tools::tlv(tlv_parent_topic_task_id, parent_topic_.task_id_));
        pack.push_child(core::tools::tlv(tlv_replies_count, replies_count_));
        pack.push_child(core::tools::tlv(tlv_unread_msg_count, unread_message_count_));
        pack.push_child(core::tools::tlv(tlv_unread_mention_count, unread_mention_me_count_));
        pack.push_child(core::tools::tlv(tlv_last_read, yours_last_read_));
        pack.push_child(core::tools::tlv(tlv_last_read_mention, yours_last_read_mention_));
        pack.push_child(core::tools::tlv(tlv_subscriber, subscriber_));
        pack.serialize(_data);
    }

    void thread_update::serialize(coll_helper& _coll) const
    {
        _coll.set_value_as_string("id", id_);
        _coll.set_value_as_int("repliesCount", replies_count_);
        _coll.set_value_as_int("unreadMsg", unread_message_count_);
        _coll.set_value_as_int("unreadMention", unread_mention_me_count_);
        _coll.set_value_as_int64("lastRead", yours_last_read_);
        _coll.set_value_as_int64("lastReadMention", yours_last_read_mention_);
        _coll.set_value_as_bool("subscriber", subscriber_);

        parent_topic_.serialize(_coll);
    }

    bool thread_update::is_valid() const
    {
        return parent_topic_.is_valid() && !id_.empty();
    }
}