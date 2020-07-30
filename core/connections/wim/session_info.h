#pragma once

namespace core
{
    class coll_helper;

    namespace wim
    {
        struct session_info
        {
            std::string os_;
            std::string client_name_;
            std::string location_;
            std::string ip_;
            std::string hash_;
            int64_t started_time_ = -1;
            bool is_current_ = false;

            void serialize(core::coll_helper _coll) const;
            int32_t unserialize(const rapidjson::Value& _node);
        };
    }
}