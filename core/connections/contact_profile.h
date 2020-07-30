#ifndef __CONTACT_PROFILE_H_
#define __CONTACT_PROFILE_H_

#pragma once

#include "../../corelib/collection_helper.h"

namespace core
{
    namespace profile
    {
        enum gender
        {
            unknown		= 0,
            male		= 1,
            female		= 2
        };

        class address
        {
            std::string		city_;
            std::string		state_;
            std::string		country_;

        public:

            bool unserialize(const rapidjson::Value& _node);
            void serialize(coll_helper _coll) const;
        };

        class job
        {
            std::string		industry_;
            std::string		subindustry_;

        public:

            bool unserialize(const rapidjson::Value& _node);
        };

        class email
        {
            std::string		addr_;
            bool			hide_;
            bool			primary_;

        public:
            email() : hide_(false), primary_(false) {}

            bool unserialize(const rapidjson::Value& _node);
        };

        using email_list = std::vector<email>;

        class phone
        {
            std::string		phone_;
            std::string		type_;

        public:

            bool unserialize(const rapidjson::Value& _node);
            void serialize(coll_helper _coll) const;
        };

        using phone_list = std::vector<phone>;

        class interest
        {
            std::string		code_;

        public:

            bool unserialize(const rapidjson::Value& _node);
        };

        using interest_list = std::vector<interest>;

        class info
        {
            std::string			aimid_;
            std::string			first_name_;
            std::string			last_name_;
            std::string			friendly_name_;
            std::string			friendly_;
            std::string			displayid_;
            std::string			relationship_;
            std::string			about_;
            int64_t				birthdate_;
            gender				gender_;
            address				home_address_;
            address				origin_address_;
            std::string			privatekey_;
            email_list			emails_;
            phone_list			phones_;
            interest_list		interests_;
            int32_t				children_;
            std::string			religion_;
            std::string			sex_orientation_;
            bool				smoking_;
            bool                official_;

        public:

            info(std::string _aimid) : aimid_(std::move(_aimid)), birthdate_(0), gender_(gender::unknown), children_(0), smoking_(false), official_(false) {}

            bool unserialize(const rapidjson::Value& _node);
            void serialize(coll_helper _coll) const;

            const std::string& get_aimid() const { return aimid_;  }
            const std::string& get_friendly() const { return friendly_; }
            bool get_official() const { return official_; }
        };

        using profiles_list = std::vector<std::shared_ptr<info>>;
    }
}

#endif //__CONTACT_PROFILE_H_
