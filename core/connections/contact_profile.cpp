#include "stdafx.h"
#include "contact_profile.h"
#include "wim/persons.h"
#include "../tools/json_helper.h"

namespace core
{
    namespace profile
    {
        bool address::unserialize(const rapidjson::Value& _node)
        {
            tools::unserialize_value(_node, "city", city_);
            tools::unserialize_value(_node, "state", state_);
            tools::unserialize_value(_node, "country", country_);

            return true;
        }

        void address::serialize(coll_helper _coll) const
        {
            _coll.set_value_as_string("city", city_);
            _coll.set_value_as_string("state", state_);
            _coll.set_value_as_string("country", country_);
        }

        bool phone::unserialize(const rapidjson::Value &_node)
        {
            tools::unserialize_value(_node, "number", phone_);
            tools::unserialize_value(_node, "type", type_);

            return true;
        }

        void phone::serialize(coll_helper _coll) const
        {
            _coll.set_value_as_string("phone", phone_);
            _coll.set_value_as_string("type", type_);
        }

        bool info::unserialize(const rapidjson::Value& _root)
        {
            const auto root_end = _root.MemberEnd();
            const auto iter_phones = _root.FindMember("abPhones");
            if (iter_phones != root_end && iter_phones->value.IsArray())
            {
                phones_.reserve(iter_phones->value.Size());
                for (auto iter_phone = iter_phones->value.Begin(), iter_phone_end = iter_phones->value.End(); iter_phone != iter_phone_end; ++iter_phone)
                {
                    phone p;
                    if (p.unserialize(*iter_phone))
                        phones_.push_back(std::move(p));
                }
            }

            official_ = wim::is_official(_root);
            tools::unserialize_value(_root, "friendly", friendly_);

            const auto iter_profile = _root.FindMember("profile");
            if (iter_profile == root_end || !iter_profile->value.IsObject())
                return false;

            const rapidjson::Value& _node = iter_profile->value;

            tools::unserialize_value(_node, "firstName", first_name_);
            tools::unserialize_value(_node, "lastName", last_name_);
            tools::unserialize_value(_node, "friendlyName", friendly_name_);
            tools::unserialize_value(_node, "displayId", displayid_);

            std::string sex;
            tools::unserialize_value(_node, "gender", sex);
            if (sex == "male")
                gender_ = gender::male;
            else if (sex == "female")
                gender_ = gender::female;
            else
                gender_ = gender::unknown;


            tools::unserialize_value(_node, "relationshipStatus", relationship_);
            tools::unserialize_value(_node, "birthDate", birthdate_);

            const auto node_end = _node.MemberEnd();
            const auto iter_home_address = _node.FindMember("homeAddress");
            if (iter_home_address != node_end && iter_home_address->value.IsArray())
            {
                if (iter_home_address->value.Size() > 0)
                    home_address_.unserialize(*iter_home_address->value.Begin());
            }

            tools::unserialize_value(_node, "aboutMe", about_);

            return true;
        }

        void info::serialize(coll_helper _coll) const
        {
            _coll.set_value_as_string("aimid", aimid_);
            _coll.set_value_as_string("firstname", first_name_);
            _coll.set_value_as_string("lastname", last_name_);
            _coll.set_value_as_string("friendly", friendly_);
            _coll.set_value_as_string("friendly_name", friendly_name_);
            _coll.set_value_as_string("displayid", displayid_);
            _coll.set_value_as_string("relationship", relationship_);
            _coll.set_value_as_string("about", about_);
            _coll.set_value_as_int64("birthdate", birthdate_);
            std::string_view sgender;
            if (gender_ == gender::male)
                sgender = "male";
            else if (gender_ == gender::female)
                sgender = "female";
            _coll.set_value_as_string("gender", sgender);

            coll_helper coll_address(_coll->create_collection(), true);
            home_address_.serialize(coll_address);
            _coll.set_value_as_collection("homeaddress", coll_address.get());

            ifptr<iarray> phones(_coll->create_array());
            phones->reserve((int32_t)phones_.size());
            for (const auto& phone : phones_)
            {
                coll_helper coll_phone(_coll->create_collection(), true);
                phone.serialize(coll_phone);

                ifptr<ivalue> val(_coll->create_value());
                val->set_as_collection(coll_phone.get());

                phones->push_back(val.get());
            }
            _coll.set_value_as_array("phones", phones.get());
        }
    }
}

