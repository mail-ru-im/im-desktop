#pragma once

namespace core
{
    struct icollection;

    namespace archive
    {
        class storage;

        struct gallery_entry_id
        {
            gallery_entry_id() = default;
            gallery_entry_id(int64_t _msg_id, int64_t _seq);

            bool empty() const;
            bool valid() const;
            bool operator>(const gallery_entry_id& _other) const;
            bool operator<(const gallery_entry_id& _other) const;
            bool operator==(const gallery_entry_id& _other) const;
            bool operator!=(const gallery_entry_id& _other) const;

            void serialize(core::tools::binary_stream& _data) const;
            bool unserialize(core::tools::binary_stream& _data);

            int64_t msg_id_ = 0;
            int64_t seq_ = 0;

            bool is_first_ = false;
        };

        struct gallery_item
        {
            bool operator<(const gallery_item& _other) const;
            bool operator==(const gallery_item& _other) const;

            void serialize(core::tools::binary_stream& _data) const;
            void serialize(icollection* _collection) const;
            bool unserialize(core::tools::binary_stream& _data);

            gallery_entry_id id_;
            gallery_entry_id next_;
            std::string url_;
            std::string caption_;
            std::string type_;
            std::string sender_;
            bool outgoing_ = false;
            time_t time_ = 0;
            std::string action_;

            int64_t get_memory_usage() const;
        };

        struct gallery_state
        {
            void serialize(icollection* _collection, const std::string& _aimid) const;
            bool operator==(const gallery_state& _other) const;
            bool operator!=(const gallery_state& _other) const;

            std::string patch_version_;
            gallery_entry_id last_entry_;
            gallery_entry_id first_entry_ = gallery_entry_id(-1, -1);
            int images_count_ = 0;
            int videos_count_ = 0;
            int files_count_ = 0;
            int links_count_ = 0;
            int ptt_count_ = 0;
            int audio_count_ = 0;
            bool patch_version_changed = false;
        };

        struct gallery_patch
        {
            int64_t msg_id_ = 0;
            std::string type_;
        };

        typedef std::deque<gallery_item> gallery_items_block;

        class gallery_storage
        {
        public:
            gallery_storage(const std::wstring& _cache_file_name, const std::wstring& _state_file_name);
            gallery_storage(const std::string& _aimid);
            gallery_storage();

            gallery_storage(const gallery_storage& _other);
            ~gallery_storage();

            void load_from_local();
            bool load_state_from_local();
            void free();

            void set_aimid(const std::string& _aimid);
            void set_my_aimid(const std::string& _aimid);

            void set_first_entry_reached(bool _reached);

            std::string get_aimid() const;
            gallery_state get_gallery_state() const;

            void merge_from_server(const gallery_storage& _other, const gallery_entry_id& _from, const gallery_entry_id& _till, std::vector<gallery_item>& _changes);
            void set_state(const gallery_state& _state, bool _store_patch_version);

            int32_t unserialize(const rapidjson::Value& _node, bool _parse_for_patches);

            std::vector<gallery_patch> get_patch() const;
            gallery_items_block get_items() const;

            std::vector<gallery_item> get_items(const gallery_entry_id& _from, const std::vector<std::string>& _types, int _page_size, bool& _exhausted);
            std::vector<gallery_item> get_items(int64_t _msg_id, const std::vector<std::string>& _types, int& _index, int& _total);

            bool get_next_hole(gallery_entry_id& _from, gallery_entry_id& _till);
            bool erased() const;

            void clear_hole_request();
            void make_hole(int64_t, int64_t);

            int64_t get_memory_usage() const;

        private:
            bool load_cache_from_local();
            bool save_cache();
            bool save_state();

            bool check_consistency();
            void clear_gallery();

        private:
            std::string aimid_;
            std::string my_aimid_;
            gallery_state state_;
            gallery_items_block items_;
            std::vector<gallery_patch> patches_;
            gallery_entry_id older_entry_id_;
            bool first_entry_reached_;
            bool loaded_from_local_;
            int64_t delUpTo_;

            std::unique_ptr<storage> cache_storage_;
            std::unique_ptr<storage> state_storage_;

            bool hole_requested_;
            bool first_load_;

            std::chrono::system_clock::time_point last_consistency_check_time_;
        };
    }
}
