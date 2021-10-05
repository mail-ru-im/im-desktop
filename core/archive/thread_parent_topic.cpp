#include "thread_parent_topic.h"
#include "thread_parent_topic.h"
#include "stdafx.h"

#include "thread_parent_topic.h"
#include "../common.shared/json_helper.h"
#include "../corelib/collection_helper.h"

namespace core::archive
{
    bool thread_parent_topic::unserialize(const rapidjson::Value& _node)
    {
        if (std::string_view type; tools::unserialize_value(_node, "type", type))
            type_ = core::threads::parent_topic::string_to_type(type);

        tools::unserialize_value(_node, "chatId", chat_id_);
        tools::unserialize_value(_node, "messageId", msg_id_);
        tools::unserialize_value(_node, "taskId", task_id_);

        return is_valid();
    }

    void thread_parent_topic::serialize(rapidjson::Value& _node, rapidjson_allocator& _a) const
    {
        _node.AddMember("type", tools::make_string_ref(core::threads::parent_topic::type_to_string(type_)), _a);
        _node.AddMember("chatId", chat_id_, _a);
        _node.AddMember("messageId", msg_id_, _a);
        if (core::threads::parent_topic::type::task == type_)
            _node.AddMember("taskId", task_id_, _a);
    }

    bool thread_parent_topic::unserialize(const coll_helper& _coll)
    {
        coll_helper c(_coll.get_value_as_collection("parentTopic"), false);

        type_ = c.get_value_as_enum<core::threads::parent_topic::type>("type");
        chat_id_ = c.get_value_as_string("chat");
        msg_id_ = c.get_value_as_int64("msgId");
        return is_valid();
    }

    void thread_parent_topic::serialize(coll_helper& _coll) const
    {
        coll_helper c(_coll->create_collection(), true);
        c.set_value_as_string("chat", chat_id_);
        c.set_value_as_int64("msgId", msg_id_);
        c.set_value_as_string("taskId", task_id_);
        c.set_value_as_enum("type", type_);
        _coll.set_value_as_collection("parentTopic", c.get());
    }

    bool thread_parent_topic::is_valid() const noexcept
    {
        using core::threads::parent_topic::type;
        return (type_ == type::message && !chat_id_.empty()) || (type_ == type::task && !task_id_.empty());
    }

    thread_parent_topic thread_parent_topic::create_message_topic(std::string_view _chat_aimid, int64_t _msg_id)
    {
        thread_parent_topic topic;
        topic.chat_id_ = _chat_aimid;
        topic.msg_id_ = _msg_id;
        topic.type_ = core::threads::parent_topic::type::message;
        return topic;
    }
}
