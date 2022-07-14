#pragma once

#include "../../../corelib/collection_helper.h"
#include "persons.h"
#include "lastseen.h"
#include "chat_member.h"

namespace core
{
    namespace wim
    {
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
            std::string inviter_;

            uint32_t create_time_ = 0;
            uint32_t admins_count_ = 0;
            uint32_t members_count_ = 0;
            uint32_t friend_count_ = 0;
            uint32_t blocked_count_ = 0;
            uint32_t pending_count_ = 0;
            uint32_t your_invites_count_ = 0;

            bool you_blocked_ = false;
            bool you_pending_ = false;
            bool you_member_ = false;
            bool public_ = false;
            bool live_ = false;
            bool controlled_ = false;
            bool joinModeration_ = false;
            bool trust_required_ = false;
            bool threads_enabled_ = false;
            bool threads_auto_subscribe_ = false;

            std::vector<chat_member_info> members_;
            std::shared_ptr<core::archive::persons_map> persons_;

        public:
            chat_info();

            int32_t unserialize(const rapidjson::Value& _node);
            void serialize(core::coll_helper _coll) const;

            const auto& get_persons() const noexcept { return persons_; }
            const auto& get_aimid() const noexcept { return aimid_; }
            bool are_you_member() const noexcept { return you_member_; }
            bool are_you_blocked() const noexcept { return you_blocked_; }
        };

        class common_chat_info
        {
            std::string aimid_;
            std::string friendly_;
            int32_t members_count_ = 0;

        public:
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
            const auto& get_members() const { return members_; }
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
