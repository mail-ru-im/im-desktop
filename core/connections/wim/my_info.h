#pragma once

#include "../../../corelib/collection_helper.h"
#include "../../archive/storage.h"
#include "user_agreement_info.h"

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
            bool hasMail_ = false;
            bool official_ = false;
            user_agreement_info user_agreement_info_;

        public:
            my_info();

            int32_t unserialize(const rapidjson::Value& _node);
            void serialize(core::coll_helper _coll);

            void serialize(core::tools::binary_stream& _data) const;
            bool unserialize(core::tools::binary_stream& _data);

            bool is_phone_number_exists() const;

            bool official() const noexcept { return official_; }

            const std::string& get_aimid() const { return aimId_; }
            const std::string& get_friendly() const { return friendly_; }
            const std::string& get_nick() const { return nick_; }
            const std::string& get_phone_number() const { return phoneNumber_; }
            const user_agreement_info& get_user_agreement_info() const { return user_agreement_info_; }
        };

        class my_info_cache
        {
            bool changed_;

            std::shared_ptr<my_info> info_;

        public:
            my_info_cache();

            bool is_changed() const;
            bool is_phone_number_exists() const;

            std::shared_ptr<my_info> get_info() const;
            void set_info(std::shared_ptr<my_info> _info);

            int32_t save(const std::wstring& _filename);
            int32_t load(const std::wstring& _filename);
        };
    }
}
