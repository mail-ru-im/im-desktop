#pragma once

#include "../../../corelib/collection_helper.h"

namespace core::wim
{
    struct id_info_response
    {
        enum class id_type { invalid, chat, user };

        id_type type_ = id_type::invalid;

        std::string sn_;
        std::string name_;
        std::string description_;

        // optional user fields
        std::string nick_;

        // optional chat fields
        std::string stamp_;
        int64_t member_count_ = 0;

        void serialize(core::coll_helper _root_coll) const;
        int32_t unserialize(const rapidjson::Value& _node);
    };
}
