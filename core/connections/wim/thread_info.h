#pragma once

#include "../../../corelib/collection_helper.h"
#include "persons.h"
#include "chat_member.h"

namespace core
{
    namespace wim
    {
        class thread_info
        {
            std::string aimid_;
            bool you_subscriber_;
            int subscribers_count_;

            std::vector<chat_member_short_info> subscribers_;
            std::shared_ptr<core::archive::persons_map> persons_;

        public:
            thread_info();

            int32_t unserialize(const rapidjson::Value& _node);
            void serialize(core::coll_helper _coll) const;

            const auto& get_persons() const noexcept { return persons_; }
            const auto& get_aimid() const noexcept { return aimid_; }
        };

        class thread_subscribers
        {
            std::string thread_id_;
            std::string cursor_;

            std::vector<chat_member_short_info> subscribers_;
            std::shared_ptr<core::archive::persons_map> persons_;

        public:
            thread_subscribers();

            int32_t unserialize(const rapidjson::Value& _node);
            void serialize(core::coll_helper _coll) const;
            void set_id(const std::string& _id);

            const auto& get_persons() const { return persons_; }
            const auto& get_members() const { return subscribers_; }
        };
    }
}
