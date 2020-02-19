#pragma once

#include "../async_task.h"
#include "../namespaces.h"

#include "options.h"
#include "storage.h"

namespace common
{
    namespace tools
    {
        struct url;

        using url_vector_t = std::vector<url>;
    }
}

namespace core
{
    class coll_helper;

    namespace tools
    {
        class binary_stream;
    }

    namespace archive
    {
        class contact_archive;
        class image_data;
        class message_header;
        class history_message;
        class dlg_state;
        struct dlg_state_changes;
        class archive_hole;
        class not_sent_message;
        class pending_operations;
        class delete_message;
        class gallery_storage;
        struct gallery_state;
        struct gallery_entry_id;
        struct gallery_item;

        struct message_stat_time;
        using message_stat_time_v = std::vector<message_stat_time>;

        using not_sent_message_sptr = std::shared_ptr<not_sent_message>;
        using history_message_sptr = std::shared_ptr<history_message>;
        using archives_map = std::unordered_map<std::string, std::shared_ptr<contact_archive>>;
        using history_block = std::vector<history_message_sptr>;
        using history_block_sptr = std::shared_ptr<history_block>;
        using image_list = std::list<image_data>;
        using headers_list = std::list<message_header>;
        using msgids_list = std::vector<int64_t>;
        using contact_and_msgs = std::vector<std::pair<std::string, int64_t>>;

        using contact_and_offset = std::tuple<std::string, std::shared_ptr<int64_t>, std::shared_ptr<int64_t>>;
        using contact_and_offsets_v = std::vector<contact_and_offset>;

        using error_vector = std::vector<std::pair<int64_t, int32_t>>;

        enum class first_load
        {
            no,
            yes
        };

        struct request_headers_handler
        {
            using on_result_type = std::function<void(std::shared_ptr<headers_list>, first_load)>;
            on_result_type on_result;

            request_headers_handler()
            {
                on_result = [](std::shared_ptr<headers_list>, first_load){};
            }
        };

        struct request_buddies_handler
        {
            using on_result_type = std::function<void(std::shared_ptr<history_block>, first_load, std::shared_ptr<error_vector>)>;
            on_result_type on_result;

            request_buddies_handler()
            {
                on_result = [](std::shared_ptr<history_block>, first_load, std::shared_ptr<error_vector>){};
            }
        };

        struct filter_deleted_handler
        {
            using on_result_type = std::function<void(const std::vector<int64_t>&, first_load)>;
            on_result_type on_result;

            filter_deleted_handler()
            {
                on_result = [](const std::vector<int64_t>&, first_load) {};
            }
        };

        struct request_dlg_state_handler
        {
            using on_result_type = std::function<void(const dlg_state& _state)>;
            on_result_type on_result;

            request_dlg_state_handler()
            {
                on_result = [](const dlg_state& _state){};
            }
        };

        struct request_dlg_states_handler
        {
            using on_result_type = std::function<void(const std::vector<dlg_state>& _states)>;
            on_result_type on_result;

            request_dlg_states_handler()
            {
                on_result = [](const std::vector<dlg_state>& _states){};
            }
        };

        struct set_dlg_state_handler
        {
            using on_result_type = std::function<void(const dlg_state& _state, const dlg_state_changes& _changes)>;
            on_result_type on_result;

            set_dlg_state_handler()
            {
                on_result = [](const dlg_state&, const dlg_state_changes&){};
            }
        };

        struct request_next_hole_handler
        {
            using on_result_type = std::function<void(std::shared_ptr<archive_hole> _hole)>;
            on_result_type on_result;

            request_next_hole_handler()
            {
                on_result = [](std::shared_ptr<archive_hole> _hole){};
            }
        };

        struct validate_hole_request_handler
        {
            using on_result_type = std::function<void(int64_t _from)>;
            on_result_type on_result;

            validate_hole_request_handler()
            {
                on_result = [](int64_t _from){};
            }
        };

        struct update_history_handler
        {
            using on_result_type = std::function<void(std::shared_ptr<headers_list> _new_ids, const dlg_state &_state, const archive::dlg_state_changes&, core::archive::storage::result_type)>;
            on_result_type on_result;
        };

        struct get_locale_handler
        {
            using on_result_type = std::function<void(std::optional<std::string>)>;
            on_result_type on_result;
        };

        struct not_sent_messages_handler
        {
            using on_result_type = std::function<void(not_sent_message_sptr _message)>;
            on_result_type on_result;

            not_sent_messages_handler()
            {
                on_result = [](not_sent_message_sptr _message){};
            }
        };

        struct pending_messages_handler
        {
            using on_result_type = std::function<void(const std::list<not_sent_message_sptr>& _messages)>;
            on_result_type on_result;

            pending_messages_handler()
            {
                on_result = [](const std::list<not_sent_message_sptr>&){};
            }
        };

        struct has_not_sent_handler
        {
            using on_result_type = std::function<void(bool _has)>;
            on_result_type on_result;

            has_not_sent_handler()
            {
                on_result = [](bool){};
            }
        };

        struct request_history_file_handler
        {
            using on_result_type = std::function<void(std::shared_ptr<contact_and_msgs>, std::shared_ptr<contact_and_offsets_v>, std::shared_ptr<tools::binary_stream> _data)>;
            on_result_type on_result;

            request_history_file_handler()
            {
                on_result = [](std::shared_ptr<contact_and_msgs>, std::shared_ptr<contact_and_offsets_v>, std::shared_ptr<tools::binary_stream>){};
            }
        };

        struct request_msg_ids_handler
        {
            using on_result_type = std::function<void(std::shared_ptr<std::vector<int64_t>>)>;
            on_result_type on_result;

            request_msg_ids_handler()
            {
                on_result = [](std::shared_ptr<std::vector<int64_t>>){};
            }
        };

        struct get_mentions_handler
        {
            using on_result_type = std::function<void(history_block_sptr, first_load)>;
            on_result_type on_result;

            get_mentions_handler()
            {
                on_result = [](history_block_sptr, first_load) {};
            }
        };

        struct find_previewable_links_handler
        {
            using on_result_type = std::function<void(const common::tools::url_vector_t &_uris)>;
            on_result_type on_result_;
        };

        struct request_gallery_state_handler
        {
            using on_result_type = std::function<void(const archive::gallery_state& _state)>;
            on_result_type on_result;

            request_gallery_state_handler()
            {
                on_result = [](const archive::gallery_state& _state) {};
            }
        };

        struct request_gallery_set_state_handler
        {
            using on_result_type = std::function<void()>;
            on_result_type on_result;

            request_gallery_set_state_handler()
            {
                on_result = []() {};
            }
        };

        struct request_merge_gallery_from_server
        {
            using on_result_type = std::function<void(const std::vector<archive::gallery_item>& _changes)>;
            on_result_type on_result;

            request_merge_gallery_from_server()
            {
                on_result = [](const std::vector<archive::gallery_item>& _changes) {};
            }
        };

        struct request_gallery_holes
        {
            using on_result_type = std::function<void(bool _result, const archive::gallery_entry_id& _from, const archive::gallery_entry_id& _till)>;
            on_result_type on_result;

            request_gallery_holes()
            {
                on_result = [](bool _result, const archive::gallery_entry_id& _from, const archive::gallery_entry_id& _till) {};
            }
        };

        struct request_gallery_entries
        {
            using on_result_type = std::function<void(const std::vector<archive::gallery_item>& _entries, int _index, int _total)>;
            on_result_type on_result;

            request_gallery_entries()
            {
                on_result = [](const std::vector<archive::gallery_item>& _entries, int _index, int _total) {};
            }
        };

        struct request_gallery_entries_page
        {
            using on_result_type = std::function<void(const std::vector<archive::gallery_item>& _entries, bool _exhausted)>;
            on_result_type on_result;

            request_gallery_entries_page()
            {
                on_result = [](const std::vector<archive::gallery_item>& _entries, bool _exhausted) {};
            }
        };

        struct memory_usage
        {
            std::function<void(
                const int64_t _memory_usage_index,
                const int64_t _memory_usage_gallery)> on_result = [](const int64_t, const int64_t){};
        };

        class local_history : public std::enable_shared_from_this<local_history>
        {
            archives_map archives_;

            const std::wstring archive_path_;

            std::unique_ptr<pending_operations> po_;

            message_stat_time_v stat_message_times_;

            std::shared_ptr<contact_archive> get_contact_archive(const std::string& _contact);

            const pending_operations& get_pending_operations() const;
            pending_operations& get_pending_operations();

            void stat_write_sent_time(const message_stat_time& _stime, const std::chrono::steady_clock::time_point& _correct_time);
            void stat_write_delivered_time(message_stat_time& _stime, const std::chrono::steady_clock::time_point& _correct_time);

            void on_dlgstate_check_message_delivered(const std::string_view _contact, const int64_t _last_delivered_msgid, const std::chrono::steady_clock::time_point& _correct_time);
            void stat_clear_message_delivered(const std::string_view _contact);

            void cleanup_stat_message_delivered();

        public:

            enum class has_older_message_id
            {
                no,
                yes
            };

            local_history(const std::wstring& _archive_path);
            virtual ~local_history();

            void optimize_contact_archive(const std::string& _contact);
            void free_dialog(const std::string& _contact);

            void get_messages_buddies(const std::string& _contact, std::shared_ptr<archive::msgids_list> _ids, /*out*/ std::shared_ptr<history_block> _messages, /*out*/ bool& _first_load, /*out*/ std::shared_ptr<error_vector> _errors);
            bool get_messages(const std::string& _contact, int64_t _from, int64_t _count_early, int64_t _count_later, /*out*/ std::shared_ptr<history_block> _messages, /*out*/ bool& _first_load, /*out*/ std::shared_ptr<error_vector> _errors);
            bool get_history_file(const std::string& _contact, /*out*/ core::tools::binary_stream& _history_archive
                , std::shared_ptr<int64_t> _offset, std::shared_ptr<int64_t> _remaining_size, int64_t& _cur_index, std::shared_ptr<int64_t> _mode);

            dlg_state get_dlg_state(const std::string& _contact);

            std::vector<dlg_state> get_dlg_states(const std::vector<std::string>& _contacts);

            void set_dlg_state(const std::string& _contact, const dlg_state& _state, const std::chrono::steady_clock::time_point& _correct_time, Out dlg_state& _result, Out dlg_state_changes& _changes);
            bool clear_dlg_state(const std::string& _contact);

            std::vector<int64_t> get_messages_for_update(const std::string& _contact);

            void filter_deleted(const std::string& _contact, /*in-out*/std::vector<int64_t>& _ids,/*out*/ bool& _first_load);
            history_block get_mentions(const std::string& _contact, bool& _first_load);

            std::shared_ptr<archive_hole> get_next_hole(const std::string& _contact, int64_t _from, int64_t _depth = -1);
            int64_t validate_hole_request(const std::string& _contact, const archive_hole& _hole_request, const int32_t _count);

            void update_history(
                const std::string& _contact,
                history_block_sptr _data,
                Out headers_list& _inserted_messages,
                Out dlg_state& _state,
                Out dlg_state_changes& _state_changes,
                Out archive::storage::result_type& _result,
                int64_t _from,
                has_older_message_id _has_older_msdid
                );

            void update_message_data(const std::string& _contact, const history_message& _message);
            void drop_history(const std::string& _contact);

            not_sent_message_sptr get_first_message_to_send();
            not_sent_message_sptr get_not_sent_message_by_iid(const std::string& _iid);
            int32_t insert_not_sent_message(const std::string& _contact, const not_sent_message_sptr& _msg);
            bool update_if_exist_not_sent_message(const std::string& _contact, const not_sent_message_sptr& _msg);

            bool get_first_message_to_delete(std::string& _contact, delete_message& _message) const;
            bool get_messages_to_delete(std::string& _contact, std::vector<delete_message>& _message) const;
            int32_t insert_pending_delete_message(const std::string& _contact, delete_message _message);
            int32_t remove_pending_delete_message(const std::string& _contact, const delete_message& _message);

            int32_t remove_messages_from_not_sent(
                const std::string& _contact,
                const bool _remove_if_modified,
                const std::shared_ptr<archive::history_block>& _data,
                const std::chrono::steady_clock::time_point& _correct_time);

            int32_t remove_messages_from_not_sent(
                const std::string& _contact,
                const bool _remove_if_modified,
                const std::shared_ptr<archive::history_block>& _data1,
                const std::shared_ptr<archive::history_block>& _data2,
                const std::chrono::steady_clock::time_point& _correct_time);
            void mark_message_duplicated(const std::string& _message_internal_id);
            void update_message_post_time(
                const std::string& _message_internal_id,
                const std::chrono::system_clock::time_point& _time_point);

            bool has_not_sent_messages(const std::string& _contact);
            void get_not_sent_messages(const std::string& _contact, /*out*/ std::shared_ptr<history_block> _messages);
            void get_pending_file_sharing(std::list<not_sent_message_sptr>& _messages);
            not_sent_message_sptr update_pending_with_imstate(
                const std::string& _message_internal_id,
                const int64_t& _hist_msg_id,
                const int64_t& _before_hist_msg_id,
                const std::chrono::steady_clock::time_point& _correct_time);

            void failed_pending_message(const std::string& _message_internal_id);

            void delete_messages_up_to(const std::string& _contact, const int64_t _id);

            int32_t get_outgoing_msg_count(const std::string& _contact);

            std::optional<std::string> get_locale(const std::string& _contact);

            static void serialize(std::shared_ptr<headers_list> _headers, coll_helper& _coll);
            static void serialize_headers(std::shared_ptr<archive::history_block> _data, coll_helper& _coll);

            void add_mention(const std::string& _contact, const std::shared_ptr<archive::history_message>& _message);
            void update_attention_attribute(const std::string& _contact, const bool _value);
            void merge_server_gallery(const std::string& _constact, const archive::gallery_storage& _gallery, const archive::gallery_entry_id& _from, const archive::gallery_entry_id& _till, std::vector<archive::gallery_item>& _changes);
            void get_gallery_state(const std::string& _aimid, archive::gallery_state& _state);
            void set_gallery_state(const std::string& _aimid, const archive::gallery_state& _state, bool _store_patch_version);
            void get_gallery_holes(const std::string& _aimid, bool& result, archive::gallery_entry_id& _from, archive::gallery_entry_id& _till);
            bool get_gallery_entries(const std::string& _aimid, const archive::gallery_entry_id& _from, const std::vector<std::string>& _types, int _page_size, std::vector<archive::gallery_item>& _entries);
            void get_gallery_entries_by_msg(const std::string& _aimid, const std::vector<std::string>& _types, int64_t _msg_id, std::vector<archive::gallery_item>& _entries, int& _index, int& _total);
            void clear_hole_request(const std::string& _aimId);
            void make_gallery_hole(const std::string& _aimId, int64_t _from, int64_t _till);
            void make_holes(const std::string& _aimId);
            void invalidate_message_data(const std::string& _aimId, const std::vector<int64_t>& _ids);
            void invalidate_message_data(const std::string& _aimId, int64_t _from, int64_t _before_count, int64_t _after_count);
            void get_memory_usage(int64_t& _index_size, int64_t& _gallery_size);
        };

        class face : public std::enable_shared_from_this<face>
        {
            std::shared_ptr<local_history> history_cache_;
            std::shared_ptr<core::async_executer> thread_;

        public:

            explicit face(const std::wstring& _archive_path);

            void free_dialog(const std::string& _contact);

            std::shared_ptr<update_history_handler> update_history(const std::string& _contact, const std::shared_ptr<archive::history_block>& _data, int64_t _from = -1, local_history::has_older_message_id _has_older_msgid = local_history::has_older_message_id::yes);
            std::shared_ptr<async_task_handlers> update_message_data(const std::string& _contact, const history_message& _message);
            std::shared_ptr<async_task_handlers> drop_history(const std::string& _contact);
            std::shared_ptr<request_buddies_handler> get_messages_buddies(const std::string& _contact, std::shared_ptr<archive::msgids_list> _ids);
            std::shared_ptr<request_buddies_handler> get_messages(const std::string& _contact, int64_t _from, int64_t _count_early, int64_t _count_later);
            std::shared_ptr<request_msg_ids_handler> get_messages_for_update(const std::string& _contact);
            std::shared_ptr<get_mentions_handler> get_mentions(std::string_view _contact);
            std::shared_ptr<filter_deleted_handler> filter_deleted_messages(const std::string& _contact, std::vector<int64_t>&& _ids);

            std::shared_ptr<request_history_file_handler> get_history_block(std::shared_ptr<contact_and_offsets_v> _contacts
                , std::shared_ptr<contact_and_msgs> _archive, std::shared_ptr<tools::binary_stream> _data, std::function<bool()> _cancel);

            std::shared_ptr<request_dlg_state_handler> get_dlg_state(std::string_view _contact, bool _debug = false);

            std::shared_ptr<request_dlg_states_handler> get_dlg_states(const std::vector<std::string>& _contacts);

            std::shared_ptr<set_dlg_state_handler> set_dlg_state(const std::string& _contact, const dlg_state& _state);
            std::shared_ptr<async_task_handlers> clear_dlg_state(const std::string& _contact);
            std::shared_ptr<request_next_hole_handler> get_next_hole(const std::string& _contact, int64_t _from, int64_t _depth = -1);
            std::shared_ptr<validate_hole_request_handler> validate_hole_request(const std::string& _contact, const archive_hole& _hole_request, const int32_t _count);

            std::shared_ptr<async_task_handlers> sync_with_history();

            std::shared_ptr<not_sent_messages_handler> get_pending_message();
            std::shared_ptr<not_sent_messages_handler> get_not_sent_message_by_iid(const std::string& _iid);
            std::shared_ptr<async_task_handlers> insert_not_sent_message(const std::string& _contact, const not_sent_message_sptr& _msg);
            std::shared_ptr<async_task_handlers> update_if_exist_not_sent_message(const std::string& _contact, const not_sent_message_sptr& _msg);

            void get_pending_delete_message(std::function<void(const bool _empty, const std::string& _contact, const delete_message& _message)> _callback);
            void get_pending_delete_messages(std::function<void(const bool _empty, const std::string& _contact, const std::vector<delete_message>& _messages)> _callback);
            void insert_pending_delete_message(const std::string& _contact, delete_message _message);
            void remove_pending_delete_message(const std::string& _contact, const delete_message& _message);

            [[nodiscard]] std::shared_ptr<get_locale_handler> get_locale(std::string_view _contact);

            std::shared_ptr<async_task_handlers> remove_messages_from_not_sent(
                const std::string& _contact,
                const bool _remove_if_modified,
                const std::shared_ptr<archive::history_block>& _data);

            std::shared_ptr<async_task_handlers> remove_messages_from_not_sent(
                const std::string& _contact,
                const bool _remove_if_modified,
                const std::shared_ptr<archive::history_block>& _data1,
                const std::shared_ptr<archive::history_block>& _data2);

            std::shared_ptr<async_task_handlers> remove_message_from_not_sent(
                const std::string& _contact,
                const bool _remove_if_modified,
                const history_message_sptr _data);

            std::shared_ptr<async_task_handlers> mark_message_duplicated(const std::string& _message_internal_id);

            std::shared_ptr<async_task_handlers> update_message_post_time(
                const std::string& _message_internal_id,
                const std::chrono::system_clock::time_point& _time_point);

            std::shared_ptr<not_sent_messages_handler> update_pending_messages_by_imstate(
                const std::string& _message_internal_id,
                const int64_t& _hist_msg_id,
                const int64_t& _before_hist_msg_id);

            std::shared_ptr<async_task_handlers> failed_pending_message(const std::string& _message_internal_id);

            std::shared_ptr<has_not_sent_handler> has_not_sent_messages(const std::string& _contact);
            std::shared_ptr<request_buddies_handler> get_not_sent_messages(const std::string& _contact, bool _debug = false);
            std::shared_ptr<pending_messages_handler> get_pending_file_sharing();

            std::shared_ptr<async_task_handlers> delete_messages_up_to(const std::string& _contact, const int64_t _id);

            std::shared_ptr<async_task_handlers> add_mention(const std::string& _contact, const std::shared_ptr<archive::history_message>& _message);

            static void serialize(std::shared_ptr<headers_list> _headers, coll_helper& _coll);
            static void serialize_headers(std::shared_ptr<archive::history_block> _data, coll_helper& _coll);

            std::shared_ptr<async_task_handlers> update_attention_attribute(const std::string& _aimid, const bool _value);

            std::shared_ptr<request_merge_gallery_from_server> merge_gallery_from_server(const std::string& _aimid, const archive::gallery_storage& _gallery, const archive::gallery_entry_id& _from, const archive::gallery_entry_id& _till);

            std::shared_ptr<request_gallery_state_handler> get_gallery_state(const std::string& _aimid);

            std::shared_ptr<async_task_handlers> set_gallery_state(const std::string& _aimid, const archive::gallery_state& _state, bool _store_patch_version);

            std::shared_ptr<request_gallery_holes> get_gallery_holes(const std::string& _aimid);

            std::shared_ptr<request_gallery_entries_page> get_gallery_entries(const std::string& _aimid, const archive::gallery_entry_id& _from, const std::vector<std::string> _types, int _page_size);

            std::shared_ptr<request_gallery_entries> get_gallery_entries_by_msg(const std::string& _aimid, const std::vector<std::string> _types, int64_t _msg_id);

            std::shared_ptr<async_task_handlers> clear_hole_request(const std::string& _aimId);

            std::shared_ptr<async_task_handlers> make_gallery_hole(const std::string& _aimId, int64_t _from, int64_t _till);

            std::shared_ptr<async_task_handlers> make_holes(const std::string& _aimid);
            std::shared_ptr<async_task_handlers> invalidate_message_data(const std::string& _aimid, std::vector<int64_t> _ids);
            std::shared_ptr<async_task_handlers> invalidate_message_data(const std::string& _aimid, int64_t _from, int64_t _before_count, int64_t _after_count);

            std::shared_ptr<memory_usage> get_memory_usage();
        };
    }
}