#pragma once

#include "storage.h"

namespace core::archive
{
    using msgids_list = std::vector<int64_t>;

    struct reactions_data
    {
        struct reaction
        {
            std::string reaction_;
            int64_t count_ = 0;

            void serialize(icollection* _collection) const;
            void serialize(core::tools::tlvpack& _pack) const;
            bool unserialize(core::tools::tlvpack& _pack);
            bool unserialize(const rapidjson::Value& _node);
        };

        int64_t msg_id_ = 0;
        std::string chat_id_;
        std::string my_reaction_;
        std::vector<reaction> reactions_;

        void serialize(icollection* _collection) const;
        void serialize(core::tools::binary_stream& _data) const;
        bool unserialize(core::tools::binary_stream& _data);
        bool unserialize(const rapidjson::Value& _node);
    };

    class reactions_storage
    {
    public:
        reactions_storage(std::wstring _index_path, std::wstring _data_path);

        void get_reactions(const msgids_list& _ids_to_load, /*out*/ std::vector<reactions_data>& _reactions, /*out*/ msgids_list& _missing);
        void insert_reactions(std::vector<reactions_data>& _reactions);

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

        index_record add_record(const reactions_data& _reactions);
        std::optional<index_record> update_record_at(const reactions_data& _reactions, int64_t _offset);

        std::unordered_map<int64_t, int64_t> index_offsets_;
        std::unordered_map<int64_t, int64_t> index_;
        std::unique_ptr<storage> index_storage_;
        std::unique_ptr<storage> data_storage_;
        bool index_loaded_ = false;
    };
}

