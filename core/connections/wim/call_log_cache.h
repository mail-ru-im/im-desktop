#pragma once

#include "../../archive/history_message.h"
#include "../../archive/storage.h"
#include "persons.h"

namespace core
{
    struct icollection;

    namespace archive
    {
        struct call_info
        {
            std::string aimid_;
            history_message message_;

            int32_t unserialize(const rapidjson::Value& _node, const archive::persons_map& _persons);
            int32_t unserialize(core::tools::binary_stream& _data);

            void serialize(icollection* _collection, const time_t _offset) const;
            void serialize(core::tools::binary_stream& _data) const;
        };

        class call_log_cache
        {
        public:
            call_log_cache(const std::wstring& _filename);

            call_log_cache() = default;
            ~call_log_cache() = default;

            int32_t unserialize(const rapidjson::Value& _node, const archive::persons_map& _persons);
            void serialize(icollection* _collection, const time_t _offset) const;

            int32_t load();

            void merge(const call_log_cache& _other);
            bool merge(const call_info& _call);

            bool remove_by_id(const std::string& _contact, int64_t _msg_id);
            bool remove_till(const std::string& _contact, int64_t _del_up_to);
            bool remove_contact(const std::string& _contact);

        private:
            bool save_cache() const;
            bool load_cache();

            std::vector<call_info> calls_;
            std::unique_ptr<storage> storage_;
        };
    }
}