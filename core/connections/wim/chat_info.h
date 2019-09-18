#pragma once

#include "../../../corelib/collection_helper.h"
#include "persons.h"

namespace core
{
    namespace wim
    {
        struct chat_member_info
        {
            std::string aimid_;
            std::string role_;
            std::string nick_;
            std::string friendly_;
            int32_t lastseen_ = -1;

            bool friend_ = false;
            bool no_avatar_ = false;
        };

        struct chat_member_short_info
        {
            std::string aimid_;
            std::string role_;
            int32_t lastseen_ = -1;
        };

        class chat_info
        {
            std::string aimid_;
            std::string name_;
            std::string about_;
            std::string rules_;
            std::string your_role_;
            std::string owner_;
            std::string members_version_;
            std::string info_version_;
            std::string location_;
            std::string stamp_;
            std::string creator_;
            std::string default_role_;

            uint32_t create_time_;
            uint32_t admins_count_;
            uint32_t members_count_;
            uint32_t friend_count_;
            uint32_t blocked_count_;
            uint32_t pending_count_;

            bool you_blocked_;
            bool you_pending_;
            bool you_member_;
            bool public_;
            bool live_;
            bool controlled_;
            bool joinModeration_;

            std::vector<chat_member_info> members_;
            std::shared_ptr<core::archive::persons_map> persons_;

        public:
            chat_info();

            int32_t unserialize(const rapidjson::Value& _node);
            void serialize(core::coll_helper _coll) const;

            const auto& get_persons() const { return persons_; }
        };

        class common_chat_info
        {
            std::string aimid_;
            std::string friednly_;
            int32_t members_count_;

        public:
            common_chat_info();

            int32_t unserialize(const rapidjson::Value& _node);
            void serialize(core::coll_helper _coll) const;
        };

        class chat_members
        {
            std::string aimid_;
            std::string cursor_;

            std::vector<chat_member_short_info> members_;
            std::shared_ptr<core::archive::persons_map> persons_;

        public:
            chat_members();

            int32_t unserialize(const rapidjson::Value& _node);
            void serialize(core::coll_helper _coll) const;

            const auto& get_persons() const { return persons_; }
        };

        class chat_contacts
        {
            std::string cursor_;

            std::vector<std::string> members_;
            std::shared_ptr<core::archive::persons_map> persons_;

        public:
            chat_contacts();

            int32_t unserialize(const rapidjson::Value& _node);
            void serialize(core::coll_helper _coll) const;

            const auto& get_persons() const { return persons_; }
        };
    }
}
