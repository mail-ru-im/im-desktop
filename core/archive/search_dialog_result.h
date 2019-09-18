#pragma once

#include "../../corelib/collection_helper.h"

namespace core::archive
{
   using history_message_sptr = std::shared_ptr<class history_message>;
}

namespace core::search
{
    struct searched_message
    {
        archive::history_message_sptr message_;
        std::vector<std::string> highlights_;

        std::string contact_;
        int contact_entry_count_ = -1;
        std::string contact_cursor_;

        void serialize(core::coll_helper _root_coll, const time_t _offset) const;
    };

    struct month_info
    {
        std::string month_;
        int entry_count_ = -1;
    };

    struct search_dialog_results
    {
        std::string keyword_;
        std::string contact_;

        std::vector<month_info> monthly_histogram_;
        std::vector<searched_message> messages_;

        int total_entry_count_ = -1;
        std::string cursor_next_;

        void unserialize_monthly_histogram(const rapidjson::Value& _node);
        void serialize_monthly_histogram(core::coll_helper _root_coll) const;

        void serialize_messages(core::coll_helper _root_coll, const time_t _offset) const;

        void serialize(core::coll_helper _root_coll, const time_t _offset) const;
    };
}
