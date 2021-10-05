#pragma once

#include "../../../corelib/collection_helper.h"
#include "user_agreement_info.h"
#include "status.h"

namespace core
{
    namespace wim
    {
        class my_info
        {
            std::string aimId_;
            std::string nick_;
            std::string friendly_;
            std::string state_;
            std::string userType_;
            std::string phoneNumber_;
            uint32_t	flags_ = 0;
            std::string	largeIconId_;
            user_agreement_info user_agreement_info_;
            bool hasMail_ = false;
            bool official_ = false;
            bool trusted_ = false;

        public:
            my_info();

            int32_t unserialize(const rapidjson::Value& _node);
            int32_t serialize(rapidjson::Value& _node, rapidjson_allocator& _a) const;

            void serialize(core::coll_helper _coll);

            bool is_phone_number_exists() const;

            bool official() const noexcept { return official_; }

            const std::string& get_aimid() const { return aimId_; }
            const std::string& get_friendly() const { return friendly_; }
            const std::string& get_nick() const { return nick_; }
            const std::string& get_phone_number() const { return phoneNumber_; }
            const user_agreement_info& get_user_agreement_info() const { return user_agreement_info_; }

            bool is_trusted() const noexcept { return trusted_; }
            void set_trusted(bool _trusted) { trusted_ = _trusted; }

            bool operator==(const my_info& _right) const;
            bool operator!=(const my_info& _right) const;
        };

        class my_info_cache
        {
            bool changed_ = false;

            std::shared_ptr<my_info> info_;
            status status_;

        public:
            my_info_cache();

            bool is_changed() const noexcept { return changed_; }
            void set_changed(bool _changed) { changed_ = _changed; }

            bool is_phone_number_exists() const;

            const std::shared_ptr<my_info>& get_info() const noexcept { return info_; }
            void set_info(const std::shared_ptr<my_info>& _info);

            const status& get_status() const noexcept { return status_; }
            void set_status(status _status);

            int32_t serialize(rapidjson::Value& _node, rapidjson_allocator& _a) const;

            int32_t load(std::wstring_view _filename);
        };
    }
}
