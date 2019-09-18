#pragma once

#include "../../../corelib/collection_helper.h"

namespace core
{
    namespace wim
    {
        struct user_info
        {
            std::string first_name_;
            std::string last_name_;
            std::string friendly_;
            std::string nick_;
            std::string about_;
            std::string phone_;
            std::string avatar_id_;

            int32_t lastseen_ = -1;
            int32_t common_chats_ = 0;
            bool official_ = false;
            bool mute_ = false;

            int32_t unserialize(const rapidjson::Value& _node);
            void serialize(core::coll_helper _coll);
        };
    }
}
