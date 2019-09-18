#include "stdafx.h"
#include "search_dialog_result.h"
#include "history_message.h"

#include "../tools/json_helper.h"

namespace core::search
{
    void searched_message::serialize(core::coll_helper _root_coll, const time_t _offset) const
    {
        coll_helper coll(_root_coll->create_collection(), true);

        ifptr<iarray> hl_array(_root_coll->create_array());
        hl_array->reserve((int32_t)highlights_.size());
        for (const auto& hl : highlights_)
        {
            ifptr<ivalue> hl_value(_root_coll->create_value());
            hl_value->set_as_string(hl.c_str(), (int32_t)hl.length());

            hl_array->push_back(hl_value.get());
        }
        coll.set_value_as_array("highlights", hl_array.get());

        if (message_)
            message_->serialize(coll.get(), _offset);

        _root_coll.set_value_as_collection("message", coll.get());
    }

    // ------------------------------------------------------------------------------

    void search_dialog_results::unserialize_monthly_histogram(const rapidjson::Value& _node)
    {
        const auto node_hist = _node.FindMember("monthlyHistogram");
        if (node_hist == _node.MemberEnd() || !node_hist->value.IsArray())
            return;

        monthly_histogram_.reserve(node_hist->value.Size());

        for (auto iter = node_hist->value.Begin(), iter_end = node_hist->value.End(); iter != iter_end; ++iter)
        {
            month_info mi;
            const auto valid =
                tools::unserialize_value(*iter, "month", mi.month_) &&
                tools::unserialize_value(*iter, "entryCount", mi.entry_count_);

            if (valid)
                monthly_histogram_.emplace_back(std::move(mi));
        }
    }

    void search_dialog_results::serialize_monthly_histogram(core::coll_helper _root_coll) const
    {
        if (monthly_histogram_.empty())
            return;

        ifptr<iarray> array(_root_coll->create_array());
        array->reserve((int32_t)monthly_histogram_.size());
        for (const auto& m : monthly_histogram_)
        {
            core::coll_helper coll(_root_coll->create_collection(), true);

            coll.set_value_as_string("month", m.month_);
            coll.set_value_as_int("count", m.entry_count_);

            ifptr<ivalue> val(_root_coll->create_value());
            val->set_as_collection(coll.get());
            array->push_back(val.get());
        }

        _root_coll.set_value_as_array("monthly_histogram", array.get());
    }

    void search_dialog_results::serialize_messages(core::coll_helper _root_coll, const time_t _offset) const
    {
        if (messages_.empty())
            return;

       ifptr<iarray> array(_root_coll->create_array());
       array->reserve((int32_t)messages_.size());
       for (const auto& m : messages_)
       {
           if (!m.message_ || m.highlights_.empty())
               continue;

           coll_helper entry_coll(_root_coll->create_collection(), true);

           ifptr<iarray> hl_array(_root_coll->create_array());
           hl_array->reserve((int32_t)m.highlights_.size());
           for (const auto& hl : m.highlights_)
           {
               ifptr<ivalue> hl_value(_root_coll->create_value());
               hl_value->set_as_string(hl.c_str(), (int32_t)hl.length());

               hl_array->push_back(hl_value.get());
           }
           entry_coll.set_value_as_array("highlights", hl_array.get());

           entry_coll.set_value_as_string("contact", m.contact_);
           if (m.contact_entry_count_ != -1)
           {
               entry_coll.set_value_as_int("contact_entry_count", m.contact_entry_count_);

               if (!m.contact_cursor_.empty())
                   entry_coll.set_value_as_string("cursor", m.contact_cursor_);
           }

           coll_helper msg_coll(_root_coll->create_collection(), true);
           m.message_->serialize(msg_coll.get(), _offset);
           entry_coll.set_value_as_collection("message", msg_coll.get());

           ifptr<ivalue> val(_root_coll->create_value());
           val->set_as_collection(entry_coll.get());
           array->push_back(val.get());
       }
       _root_coll.set_value_as_array("messages", array.get());
    }

    void search_dialog_results::serialize(core::coll_helper _root_coll, const time_t _offset) const
    {
        serialize_monthly_histogram(_root_coll);
        serialize_messages(_root_coll, _offset);

        if (!cursor_next_.empty())
        {
            assert(!messages_.empty());
            _root_coll.set_value_as_string("cursor_next", cursor_next_);
        }

        _root_coll.set_value_as_int("total_count", total_entry_count_);
    }
}
