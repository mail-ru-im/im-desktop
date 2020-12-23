#include "stdafx.h"
#include "utils/gui_coll_helper.h"
#include "common_phone.h"

namespace Data
{
    PhoneInfo::PhoneInfo(): is_phone_valid_(false)
    {
        //
    }

    PhoneInfo::~PhoneInfo()
    {
        //
    }

    void PhoneInfo::deserialize(core::coll_helper *c)
    {
        if (c)
        {
            info_operator_ = c->get_value_as_string("operator", "");
            info_phone_ = c->get_value_as_string("phone", "");
            info_iso_country_ = c->get_value_as_string("iso_country", "");
            if (c->is_value_exist("printable"))
                printable_ = Utils::toContainerOfString<decltype(printable_)>(c->get_value_as_array("printable"));
            status_ = c->get_value_as_string("status", "");
            trunk_code_ = c->get_value_as_string("trunk_code", "");
            modified_phone_number_ = c->get_value_as_string("modified_phone_number", "");
            if (c->is_value_exist("remaining_lengths"))
            {
                auto remaining_lengths = c->get_value_as_array("remaining_lengths");
                for (int i = 0; remaining_lengths && i < remaining_lengths->size(); ++i)
                {
                    remaining_lengths_.push_back((int)remaining_lengths->get_at(i)->get_as_int64());
                }
            }
            if (c->is_value_exist("prefix_state"))
                prefix_state_ = Utils::toContainerOfString<decltype(prefix_state_)>(c->get_value_as_array("prefix_state"));
            modified_prefix_ = c->get_value_as_string("modified_prefix", "");
            is_phone_valid_ = c->get_value_as_bool("is_phone_valid", false);
        }
    }
}
