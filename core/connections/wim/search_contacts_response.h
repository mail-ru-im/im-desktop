#pragma once

#include "../../../corelib/collection_helper.h"
#include "chat_info.h"
#include "persons.h"

namespace core::wim
{
    class search_contacts_response
    {
        bool finish_ = false;
        std::string next_tag_;

        struct contact_chunk
        {
            std::string aimid_;
            std::string stamp_;
            std::string type_;
            int32_t score_ = -1;

            std::string first_name_;
            std::string last_name_;
            std::string nick_name_;
            std::string friendly_;
            std::string city_;
            std::string state_;
            std::string country_;
            std::string gender_;
            struct birthdate
            {
                int32_t year_ = -1;
                int32_t month_ = -1;
                int32_t day_ = -1;
            }
            birthdate_;
            std::string about_;
            int32_t mutual_friend_count_ = 0;

            std::vector<std::string> highlights_;
        };
        std::vector<contact_chunk> contacts_data_;
        std::vector<chat_info> chats_data_;

        std::shared_ptr<core::archive::persons_map> persons_;

        bool unserialize_contacts(const rapidjson::Value& _node);
        bool unserialize_chats(const rapidjson::Value& _node);

        void serialize_contacts(core::coll_helper _root_coll) const;
        void serialize_chats(core::coll_helper _root_coll) const;

    public:
        bool unserialize(const rapidjson::Value& _node);
        void serialize(core::coll_helper _root_coll) const;

        const auto& get_persons() const { return persons_; }
    };
}