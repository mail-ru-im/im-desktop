#pragma once

#include "thread_parent_topic.h"

namespace core
{
    class coll_helper;
}

namespace core::tools
{
    class binary_stream;
}

namespace core::archive
{
    struct thread_update
    {
        std::string id_;
        archive::thread_parent_topic parent_topic_;
        int64_t yours_last_read_ = -1;
        int64_t yours_last_read_mention_ = -1;
        int replies_count_ = 0;
        int unread_message_count_ = 0;
        int unread_mention_me_count_ = 0;
        bool subscriber_ = false;

        void serialize(rapidjson::Value& _node, rapidjson_allocator& _a) const;
        int32_t unserialize(const rapidjson::Value& _node);
        bool unserialize(core::tools::binary_stream& _data);

        void serialize(core::tools::binary_stream& _data) const;
        void serialize(coll_helper& _coll) const;

        bool is_valid() const;

        int64_t get_msgid() const noexcept { return parent_topic_.msg_id_; }
        const std::string& get_parent_chat() const noexcept { return parent_topic_.chat_id_; }
    };
}