#pragma once

#include "smartreply/smartreply_marker.h"

namespace core
{
    enum class message_type;
    enum class file_sharing_base_content_type;
    class coll_helper;

    namespace tools
    {
        class tlvpack;
    }

    namespace archive
    {
        class history_message;
        class storage;
        class not_sent_message;

        using history_message_sptr = std::shared_ptr<history_message>;
        using history_block = std::vector<history_message_sptr>;
        using history_block_sptr = std::shared_ptr<history_block>;
        using not_sent_message_sptr = std::shared_ptr<not_sent_message>;

        struct message_stat_time
        {
            std::chrono::steady_clock::time_point create_time_;
            bool need_stat_;
            std::string contact_;
            int64_t msg_id_;

            message_stat_time(
                const bool _need_stat = false,
                std::chrono::steady_clock::time_point _create_time = std::chrono::steady_clock::now(),
                std::string_view _contact = std::string_view(),
                const int64_t _msg_id = -1)
                : create_time_(_create_time)
                , need_stat_(_need_stat)
                , contact_(_contact)
                , msg_id_(_msg_id)
            {
            }

            bool operator==(const message_stat_time& _other) const
            {
                return msg_id_ == _other.msg_id_ && contact_ == _other.contact_;
            }

            bool operator!=(const message_stat_time& _other) const
            {
                return !(*this == _other);
            }
        };
        using message_stat_time_v = std::vector<message_stat_time>;

        enum class delete_operation
        {
            min = 0,

            del_up_to = 2,
            del_for_me = 3,
            del_for_all = 4,

            max
        };


        class not_sent_message
        {
        public:
            static not_sent_message_sptr make(const core::tools::tlvpack& _pack);

            static not_sent_message_sptr make(
                const not_sent_message_sptr& _message,
                const uint64_t _message_time);

            static not_sent_message_sptr make(
                const std::string& _aimid,
                std::string _message,
                core::data::format _message_format,
                const message_type _type,
                const uint64_t _message_time,
                std::string _internal_id);

            static not_sent_message_sptr make_outgoing_file_sharing(
                const std::string& _aimid,
                const uint64_t _message_time,
                const std::string& _local_path,
                const core::archive::quotes_vec& _quotes,
                const std::string& _description,
                const core::data::format& _description_format,
                const core::archive::mentions_map& _mentions,
                const std::optional<int64_t>& _duration);

            static not_sent_message_sptr make_incoming_file_sharing(
                const std::string& _aimid,
                const uint64_t _message_time,
                const std::string& _uri,
                std::string _internal_id);

            virtual ~not_sent_message();

            void serialize(core::tools::tlvpack& _pack) const;
            void serialize(core::coll_helper& _coll, const time_t _offset) const;

            const history_message_sptr& get_message() const;

            const std::string& get_aimid() const;

            const std::string& get_internal_id() const;

            const std::string& get_file_sharing_local_path() const;

            const core::archive::quotes_vec& get_quotes() const;
            void attach_quotes(core::archive::quotes_vec _quotes);

            const core::archive::mentions_map& get_mentions() const;
            void set_mentions(core::archive::mentions_map _mentions);

            void set_updated_id(int64_t _id);
            int64_t get_updated_id() const;

            bool is_ready_to_send() const;

            void mark_duplicated();

            void set_failed();

            void update_post_time(const std::chrono::system_clock::time_point& _time_point);

            const std::chrono::system_clock::time_point& get_post_time() const;
            const std::chrono::steady_clock::time_point& get_create_time() const;

            void set_calc_stat(const bool _calc);
            bool is_calc_stat() const;

            bool is_modified() const;
            void set_modified(const bool _modified);

            void set_network_request_id(const std::string& _id);
            const std::string& get_network_request_id() const;

            bool is_deleted() const;
            void set_deleted(const bool _deleted);

            void set_delete_operation(const delete_operation _delete_operation);
            const delete_operation& get_delete_operation() const;

            void set_description(const std::string& _description);
            std::string get_description() const;

            core::data::format get_description_format() const { return message_->description_format(); }
            void set_description_format(const core::data::format& _description_format);

            void set_url(const std::string& _url);
            std::string get_url() const;

            void set_shared_contact(const core::archive::shared_contact& _contact);
            const core::archive::shared_contact& get_shared_contact() const;

            std::optional<int64_t> get_duration() const;
            std::optional<file_sharing_base_content_type> get_base_content_type() const;

            void set_geo(const core::archive::geo& _geo);
            const core::archive::geo& get_geo() const;

            void set_poll(const core::archive::poll& _poll);
            const core::archive::poll get_poll() const;

            void set_task(const core::tasks::task& _task);
            const core::tasks::task& get_task() const;

            void set_smartreply_marker(const smartreply::marker_opt& _marker);
            const smartreply::marker_opt& get_smartreply_marker() const;

            void set_draft_delete_time(const std::optional<int64_t>& _draft_delete_time);
            const std::optional<int64_t>& get_draft_delete_time() const;

        private:

            std::string aimid_;

            std::shared_ptr<history_message> message_;

            bool duplicated_;

            std::chrono::system_clock::time_point post_time_;

            bool calc_sent_stat_;

            std::chrono::steady_clock::time_point create_time_;

            int64_t updated_id_;

            bool modified_;

            std::string network_request_id_;

            bool deleted_;

            delete_operation delete_operation_;
            smartreply::marker_opt smartreply_marker_;

            std::optional<int64_t> draft_delete_time_;

            not_sent_message();

            not_sent_message(
                const not_sent_message_sptr& _message,
                const uint64_t _message_time);

            not_sent_message(
                const std::string& _aimid,
                std::string _message,
                core::data::format _message_format,
                const message_type _type,
                const uint64_t _message_time,
                std::string&& _internal_id);

            void copy_from(const not_sent_message_sptr& _message);

            bool unserialize(const core::tools::tlvpack& _pack);
        };

        typedef std::list<not_sent_message_sptr> not_sent_messages_list;


        class delete_message
        {
            enum delete_message_fields
            {
                min = 0,

                contact = 1,
                message_id = 2,
                internal_id = 3,
                operation = 4,

                max
            };


            std::string aimid_;

            int64_t message_id_;

            std::string internal_id_;

            delete_operation operation_;

        public:

            bool operator==(const delete_message& _other) const;

            int64_t get_message_id() const;
            const std::string& get_internal_id() const;

            delete_operation get_operation() const;

            delete_message(const std::string& _aimid, const int64_t _message_id, const std::string& _internal_id, const delete_operation _operation);
            delete_message();

            delete_message(const delete_message&) = default;
            delete_message(delete_message&&) = default;
            delete_message& operator=(delete_message&&) = default;
            delete_message& operator=(const delete_message&) = default;

            const std::string& get_aimid() const;

            void serialize(core::tools::tlvpack& _pack) const;

            static std::unique_ptr<delete_message> make(const core::tools::tlvpack& _pack);
        };


        typedef std::list<std::unique_ptr<delete_message>> delete_messages_list;


        class pending_operations
        {
            std::map<std::string, not_sent_messages_list> pending_messages_;

            std::map<std::string, delete_messages_list> pending_delete_messages_;

            std::unique_ptr<storage> pending_sent_storage_;

            std::unique_ptr<storage> pending_delete_storage_;

            bool is_loaded_;

        public:
            pending_operations(
                const std::wstring& _file_pending_sent,
                const std::wstring& _file_pending_delete);

            ~pending_operations();

            void load_if_need();

            message_stat_time_v remove(const std::string &_aimid, const bool _remove_if_modified, const history_block_sptr &_data);
            void remove(const std::string& _internal_id);

            void drop(const std::string& _aimid);

            void insert_message(const std::string &_aimid, const not_sent_message_sptr &_message);
            bool update_message_if_exist(const std::string &_aimid, const not_sent_message_sptr &_message);
            bool exist_message(const std::string& _aimid) const;
            not_sent_message_sptr get_first_ready_to_send() const;
            void get_pending_file_sharing_messages(not_sent_messages_list& _messages) const;
            not_sent_message_sptr get_message_by_internal_id(const std::string& _iid) const;
            void mark_duplicated(const std::string& _message_internal_id);

            void update_message_post_time(
                const std::string& _message_internal_id,
                const std::chrono::system_clock::time_point& _time_point);

            not_sent_message_sptr update_with_imstate(
                const std::string& _message_internal_id,
                const int64_t& _hist_msg_id,
                const int64_t& _before_hist_msg_id);

            void failed_pending_message(const std::string& _message_internal_id);
            void get_messages(const std::string &_aimid, Out history_block &_messages);

            int32_t insert_deleted_message(const std::string& _contact, delete_message _message);
            int32_t remove_deleted_message(const std::string& _contact, const delete_message& _message);
            bool get_first_pending_delete_message(std::string& _contact, delete_message& _message) const;
            bool get_pending_delete_messages(std::string& _contact, std::vector<delete_message>& _messages) const;

            template <class container_type_>
            bool save(storage& _storage, const container_type_& _pendings) const;

            template <class container_type_, class entry_type_>
            bool load(storage& _storage, container_type_& _pendings);

            bool save_pending_sent_messages();
            bool load_pending_sent_messages();
            bool save_pending_delete_messages();
            bool load_pending_delete_messages();
        };

    }
}
