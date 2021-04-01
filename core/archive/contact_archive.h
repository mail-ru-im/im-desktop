#pragma once

#include "storage.h"

namespace core
{
    namespace archive
    {
        class history_message;
        class archive_index;
        class messages_data;
        class message_header;
        class dlg_state;
        struct dlg_state_changes;
        class archive_hole;
        enum class archive_hole_error;
        class archive_state;
        class image_data;
        class mentions_me;
        class gallery_storage;
        struct gallery_state;
        struct gallery_entry_id;
        struct gallery_item;
        class reactions_storage;
        struct reactions_data;

        using image_list = std::list<image_data>;
        using history_block = std::vector<std::shared_ptr<history_message>>;
        using history_block_sptr = std::shared_ptr<history_block>;
        using msgids_list = std::vector<int64_t>;
        using headers_list = std::list<message_header>;
        using error_vector = std::vector<std::pair<int64_t, int32_t>>;

        class contact_archive
        {
            const std::wstring path_;

            std::unique_ptr<archive_index> index_;
            std::unique_ptr<messages_data> data_;
            std::unique_ptr<archive_state> state_;
            std::unique_ptr<mentions_me> mentions_;
            std::unique_ptr<gallery_storage> gallery_;
            std::unique_ptr<reactions_storage> reactions_;

            bool local_loaded_;

            mutable struct load_metrics
            {
                int64_t index_ms_;
                int64_t mentions_ms_;
                int64_t gallery_ms_;
            } load_metrics_;

            bool get_messages_buddies_from_ids(const archive::msgids_list& _ids, history_block& _messages, error_vector& _errors) const;
            bool get_messages_buddies_from_headers(const headers_list& _headers, history_block& _messages, error_vector& _errors) const;

        public:

            enum class get_message_policy
            {
                get_all,
                skip_patches_and_deleted
            };

            void get_messages(int64_t _from, int64_t _count_early, int64_t _count_later, history_block& _messages, get_message_policy policy) const;
            void get_messages_index(int64_t _from, int64_t _count_early, int64_t _count_later, headers_list& _headers) const;

            bool get_messages_buddies(const std::shared_ptr<archive::msgids_list>& _ids, const std::shared_ptr<history_block>& _messages, const std::shared_ptr<error_vector>& _errors) const;
            bool get_messages_buddies(const headers_list& _headers, const std::shared_ptr<history_block>& _messages, const std::shared_ptr<error_vector>& _errors) const;

            static bool get_history_file(const std::wstring& _file_name, core::tools::binary_stream& _data
                , const std::shared_ptr<int64_t>& _offset, const std::shared_ptr<int64_t>& _remaining_size, int64_t& _cur_index, const std::shared_ptr<int64_t>& _mode);

            history_block get_mentions() const;

            void filter_deleted(std::vector<int64_t>& _ids) const;

            std::vector<int64_t> get_messages_for_update() const;
            archive::archive_hole_error get_next_hole(int64_t _from, archive_hole& _hole, int64_t _depth) const;
            int64_t validate_hole_request(const archive_hole& _hole, const int32_t _count) const;

            std::optional<std::string> get_locale() const;

            const dlg_state& get_dlg_state() const;
            void set_dlg_state(const dlg_state& _state, Out dlg_state_changes& _changes);
            void clear_dlg_state();

            void insert_history_block(
                history_block_sptr _data,
                Out headers_list& _inserted_messages,
                Out dlg_state& _updated_state,
                Out dlg_state_changes& _state_changes,
                Out archive::storage::result_type& _result,
                int64_t _from,
                bool _has_older_message_id);

            void update_message_data(const history_message& _message);

            void drop_history();

            int32_t load_from_local(/*out*/ bool& _first_load);
            void load_gallery_state_from_local();
            std::string get_load_metrics_for_log() const;

            bool need_optimize() const;
            void optimize();
            void free();

            void delete_messages_up_to(const int64_t _up_to);

            contact_archive(std::wstring _archive_path, const std::string& _contact_id);
            ~contact_archive();

            void add_mention(const std::shared_ptr<archive::history_message>& _message);
            void update_attention_attribute(const bool _value);
            void merge_server_gallery(const archive::gallery_storage& _gallery, const archive::gallery_entry_id& _from, const archive::gallery_entry_id& _till, std::vector<archive::gallery_item>& _changes);
            archive::gallery_state get_gallery_state() const;
            void set_gallery_state(const archive::gallery_state& _state, bool _store_patch_version);
            bool get_gallery_holes(archive::gallery_entry_id& _from, archive::gallery_entry_id& _till) const;
            std::vector<archive::gallery_item> get_gallery_entries(const archive::gallery_entry_id& _from, const std::vector<std::string>& _types, int _page_size, bool& _exhausted);
            std::vector<archive::gallery_item> get_gallery_entries_by_msg(int64_t _msg_id, const std::vector<std::string>& _types, int& _index, int& _total);
            void clear_hole_request();
            void make_gallery_hole(int64_t _from, int64_t _till);
            void make_holes();
            bool is_gallery_hole_requested() const;
            void invalidate_message_data(const std::vector<int64_t>& _ids);
            void invalidate_message_data(int64_t _from, int64_t _before_count, int64_t _after_count);
            void get_memory_usage(int64_t& _index_size, int64_t& _gallery_size) const;
            void get_reactions(const msgids_list& _ids_to_load, /*out*/ std::vector<reactions_data>& _reactions, /*out*/ msgids_list& _missing);
            void insert_reactions(std::vector<reactions_data>& _reactions);
        };

        std::wstring_view db_filename() noexcept;
        std::wstring_view index_filename() noexcept;
        std::wstring_view dlg_state_filename() noexcept;
        std::wstring_view cache_filename() noexcept;
        std::wstring_view mentions_filename() noexcept;
        std::wstring_view gallery_cache_filename() noexcept;
        std::wstring_view gallery_state_filename() noexcept;
        std::wstring_view reactions_index_filename() noexcept;
        std::wstring_view reactions_data_filename() noexcept;
    }
}
