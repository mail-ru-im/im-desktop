#pragma once

#include "storage.h"

namespace core::archive
{
    struct thread_update;

    using msgids_list = std::vector<int64_t>;

    class thread_update_storage
    {
    public:
        thread_update_storage(std::wstring _index_path, std::wstring _data_path);

        void get_thread_updates(const msgids_list& _ids_to_load, /*out*/ std::vector<thread_update>& _thread_updates, /*out*/ msgids_list& _missing);
        void insert_thread_updates(const std::vector<thread_update>& _thread_updates);

    private:
        struct index_record
        {
            int64_t offset_ = 0;
            int64_t msg_id_ = 0;

            void serialize(core::tools::binary_stream& _data) const;
            bool unserialize(core::tools::binary_stream& _data);
        };

        void load_index();
        void save_index_records(const std::vector<index_record>& _records);

        index_record add_record(const thread_update& _update);
        index_record add_record(int64_t _msgid, core::tools::binary_stream _update_data);
        std::optional<index_record> update_record_at(const thread_update& _update, int64_t _offset);

        std::unordered_map<int64_t, int64_t> index_offsets_;
        std::unordered_map<int64_t, int64_t> index_;
        std::unique_ptr<storage> index_storage_;
        std::unique_ptr<storage> data_storage_;
        bool index_loaded_ = false;
    };
}
