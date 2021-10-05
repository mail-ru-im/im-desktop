#pragma once

#include "../../common.shared/threads/thread_types.h"

namespace core
{
    class coll_helper;
}

namespace core::archive
{
    struct thread_parent_topic
    {
        using type = core::threads::parent_topic::type;
        type type_ = type::invalid;
        std::string chat_id_;
        int64_t msg_id_ = -1;
        std::string task_id_;

        bool unserialize(const rapidjson::Value& _node);
        void serialize(rapidjson::Value& _node, rapidjson_allocator& _a) const;
        bool unserialize(const coll_helper& _coll);
        void serialize(coll_helper& _coll) const;

        bool is_valid() const noexcept;

        static thread_parent_topic create_message_topic(std::string_view _chat_aimid, int64_t _msg_id);
    };
}