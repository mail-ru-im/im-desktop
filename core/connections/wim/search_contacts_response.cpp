#include "stdafx.h"
#include "search_contacts_response.h"
#include "../../tools/json_helper.h"

namespace core::wim
{
    bool search_contacts_response::unserialize_contacts(const rapidjson::Value& _node)
    {
        auto contacts_node = _node.FindMember("data");
        if (contacts_node != _node.MemberEnd())
        {
            if (!contacts_node->value.IsArray())
                return false;

            contacts_data_.clear();
            for (const auto& node : contacts_node->value.GetArray())
            {
                contact_chunk c;
                tools::unserialize_value(node, "sn", c.aimid_);
                tools::unserialize_value(node, "stamp", c.stamp_);
                tools::unserialize_value(node, "type", c.type_);
                tools::unserialize_value(node, "score", c.score_);
                {
                    auto a = node.FindMember("anketa");
                    if (a != node.MemberEnd() && a->value.IsObject())
                    {
                        const auto& sub_node = a->value;
                        tools::unserialize_value(sub_node, "firstName", c.first_name_);
                        tools::unserialize_value(sub_node, "lastName", c.last_name_);
                        tools::unserialize_value(sub_node, "friendly", c.friendly_);
                        tools::unserialize_value(sub_node, "city", c.city_);
                        tools::unserialize_value(sub_node, "state", c.state_);
                        tools::unserialize_value(sub_node, "country", c.country_);
                        tools::unserialize_value(sub_node, "gender", c.gender_);

                        {
                            auto b = sub_node.FindMember("birthDate");
                            if (b != sub_node.MemberEnd() && b->value.IsObject())
                            {
                                const auto& sub_sub_node = b->value;
                                tools::unserialize_value(sub_sub_node, "year", c.birthdate_.year_);
                                tools::unserialize_value(sub_sub_node, "month", c.birthdate_.month_);
                                tools::unserialize_value(sub_sub_node, "day", c.birthdate_.day_);
                            }
                        }
                        tools::unserialize_value(sub_node, "aboutMeCut", c.about_);
                    }
                    //tools::unserialize_value(sub_node, "mutualCount", c.mutual_friend_count_);//TODO fix name when server side is done
                }

                if (const auto iter_hl = node.FindMember("highlight"); iter_hl != node.MemberEnd() || iter_hl->value.IsArray())
                {
                    c.highlights_.reserve(iter_hl->value.Size());
                    for (const auto& hl : iter_hl->value.GetArray())
                        c.highlights_.push_back(rapidjson_get_string(hl));
                }

                contacts_data_.emplace_back(std::move(c));
            }
        }
        return true;
    }

    bool search_contacts_response::unserialize_chats(const rapidjson::Value& _node)
    {
        auto chats_node = _node.FindMember("chats");
        if (chats_node != _node.MemberEnd())
        {
            if (!chats_node->value.IsArray())
                return false;

            chats_data_.clear();
            chats_data_.reserve(chats_node->value.Size());

            for (const auto& c_node : chats_node->value.GetArray())
            {
                chat_info c;
                c.unserialize(c_node);
                chats_data_.emplace_back(std::move(c));
            }
        }
        return true;
    }

    void search_contacts_response::serialize_contacts(core::coll_helper _root_coll) const
    {
        if (contacts_data_.empty())
            return;

        ifptr<iarray> array(_root_coll->create_array());
        array->reserve((int32_t)contacts_data_.size());
        for (const auto& c : contacts_data_)
        {
            coll_helper coll(_root_coll->create_collection(), true);

            coll.set_value_as_string("aimid", c.aimid_);
            coll.set_value_as_string("stamp", c.stamp_);
            coll.set_value_as_string("type", c.type_);
            if (c.score_ >= 0)
                coll.set_value_as_int("score", c.score_);
            coll.set_value_as_string("first_name", c.first_name_);
            coll.set_value_as_string("last_name", c.last_name_);
            coll.set_value_as_string("nick_name", c.nick_name_);
            coll.set_value_as_string("friendly", c.friendly_);
            coll.set_value_as_string("city", c.city_);
            coll.set_value_as_string("state", c.state_);
            coll.set_value_as_string("country", c.country_);
            coll.set_value_as_string("gender", c.gender_);
            if (c.birthdate_.year_ >= 0 && c.birthdate_.month_ >= 0 && c.birthdate_.day_ >= 0)
            {
                coll_helper b(_root_coll->create_collection(), true);
                b.set_value_as_int("year", c.birthdate_.year_);
                b.set_value_as_int("month", c.birthdate_.month_);
                b.set_value_as_int("day", c.birthdate_.day_);
                coll.set_value_as_collection("birthdate", b.get());
            }
            coll.set_value_as_string("about", c.about_);
            coll.set_value_as_int("mutual_count", c.mutual_friend_count_);

            if (!c.highlights_.empty())
            {
                ifptr<iarray> hl_array(_root_coll->create_array());
                hl_array->reserve((int32_t)c.highlights_.size());
                for (const auto& hl : c.highlights_)
                {
                    ifptr<ivalue> hl_value(_root_coll->create_value());
                    hl_value->set_as_string(hl.c_str(), (int32_t)hl.length());

                    hl_array->push_back(hl_value.get());
                }
                coll.set_value_as_array("highlights", hl_array.get());
            }

            ifptr<ivalue> val(_root_coll->create_value());
            val->set_as_collection(coll.get());
            array->push_back(val.get());
        }

        _root_coll.set_value_as_array("data", array.get());
    }

    void core::wim::search_contacts_response::serialize_chats(core::coll_helper _root_coll) const
    {
        if (chats_data_.empty())
            return;

        ifptr<iarray> array(_root_coll->create_array());
        array->reserve((int32_t)chats_data_.size());

        for (const auto& c : chats_data_)
        {
            coll_helper coll(_root_coll->create_collection(), true);
            c.serialize(coll);

            ifptr<ivalue> val(_root_coll->create_value());
            val->set_as_collection(coll.get());
            array->push_back(val.get());
        }

        _root_coll.set_value_as_array("chats", array.get());
    }

    bool search_contacts_response::unserialize(const rapidjson::Value& _node)
    {
        tools::unserialize_value(_node, "finish", finish_);
        tools::unserialize_value(_node, "newTag", next_tag_);

        auto res = unserialize_contacts(_node);
        res |= unserialize_chats(_node);

        if (res)
        {
            persons_ = std::make_shared<core::archive::persons_map>(parse_persons(_node));

            for (const auto& c : chats_data_)
            {
                const auto& p = c.get_persons();
                if (p)
                    persons_->insert(p->begin(), p->end());
            }

            // unique nicks are now in persons array, extract them from there
            for (const auto& p_it : *persons_)
            {
                const auto it = std::find_if(contacts_data_.begin(), contacts_data_.end(), [&p_it](const auto& _c) { return _c.aimid_ == p_it.first; });
                if (it != contacts_data_.end())
                    it->nick_name_ = p_it.second.nick_;
            }
        }

        return res;
    }

    void search_contacts_response::serialize(core::coll_helper _root_coll) const
    {
        serialize_contacts(_root_coll);
        serialize_chats(_root_coll);

        _root_coll.set_value_as_bool("finish", finish_);
        _root_coll.set_value_as_string("next_tag", next_tag_);
    }
}
