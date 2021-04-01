#pragma once

#include "message_flags.h"

#include "../../corelib/iserializable.h"
#include "../../common.shared/patch_version.h"
#include "../../common.shared/message_processing/text_formatting.h"

#include "../connections/wim/persons.h"
#include "poll.h"
#include "reactions.h"

namespace core
{
    struct icollection;
    class coll_helper;

    enum class message_type;
    enum class chat_event_type;
    enum class voip_event_type;
    enum class file_sharing_base_content_type;

    namespace archive
    {
        class history_patch;
        class message_header;
        class history_message;
        struct person;
        class quote;
        class url_snippet;

        using message_fields_set = std::set<std::string, tools::string_comparator>;
        using history_block = std::vector<std::shared_ptr<history_message>>;
        using history_patch_uptr = std::unique_ptr<history_patch>;
        using mentions_map = std::map<std::string, std::string>;
        using message_header_vec = std::vector<class message_header>;
        using quotes_vec = std::vector<quote>;
        using snippets_vec = std::vector<url_snippet>;

        struct shared_contact_data
        {
            std::string name_;
            std::string phone_;
            std::string sn_;

            void serialize(icollection* _collection) const;
            void serialize(core::tools::tlvpack& _pack) const;
            bool unserialize(icollection* _coll);
            bool unserialize(const rapidjson::Value& _node);
            bool unserialize(const core::tools::tlvpack& _pack);
        };
        using shared_contact = std::optional<shared_contact_data>;

        struct geo_data
        {
            std::string name_;
            double lat_;
            double long_;

            void serialize(icollection* _collection) const;
            void serialize(core::tools::tlvpack& _pack) const;
            bool unserialize(icollection* _coll);
            bool unserialize(const rapidjson::Value& _node);
            bool unserialize(const core::tools::tlvpack& _pack);
        };
        using geo = std::optional<geo_data>;

        struct button_data
        {
            std::string text_;
            std::string url_;
            std::string callback_data_;
            std::string style_;

            void serialize(icollection* _collection) const;
            void unserialize(const rapidjson::Value& _node);
        };

        struct message_reactions
        {
            bool exist_ = false;
            std::optional<reactions_data> data_;

            void serialize(icollection* _collection) const;
            void serialize(core::tools::tlvpack& _pack) const;
            void unserialize(const rapidjson::Value& _node);
            bool unserialize(const core::tools::tlvpack& _pack);
        };
        using reactions = std::optional<message_reactions>;

        //////////////////////////////////////////////////////////////////////////
        // message_header class
        //////////////////////////////////////////////////////////////////////////

        // header store format
        // version		- uint8_t
        // flags		- uint32_t
        // time			- uint64_t
        // msgid		- int64_t
        // data_offset	- int64_t
        // data_size	- uint32_t

        class message_header
        {
            int64_t id_;
            uint8_t version_;
            message_flags flags_;
            uint64_t time_;
            int64_t prev_id_;
            int64_t data_offset_;
            uint32_t data_size_;

            common::tools::patch_version update_patch_version_;

            bool has_shared_contact_with_sn_;
            bool has_poll_with_id_;
            bool has_reactions_;

            message_header_vec modifications_;

            uint32_t min_data_sizeof(uint8_t _version) const noexcept;

        public:

            message_header();
            message_header(message_flags _flags,
                uint64_t _time,
                int64_t _id,
                int64_t _prev_id,
                int64_t _data_offset,
                uint32_t _data_size,
                common::tools::patch_version patch_,
                bool _shared_contact_with_sn,
                bool _has_poll_with_id,
                bool _has_reactions);

            message_header(message_header&&) = default;
            message_header(const message_header&) = default;
            message_header& operator=(const message_header&) = default;
            message_header& operator=(message_header&&) = default;

            uint64_t get_time() const noexcept { return time_; }
            void set_time(uint64_t _value) noexcept { time_ = _value; }

            int64_t get_id() const noexcept  { return id_; }
            void set_id(int64_t _id) noexcept { id_ = _id; }
            bool has_id() const noexcept { return (id_ > 0); }

            message_flags get_flags() const noexcept { return flags_; }
            void set_flags(message_flags _flags) noexcept { flags_ = _flags; }

            int64_t get_data_offset() const noexcept;
            void set_data_offset(int64_t _value) noexcept;
            void invalidate_data_offset() noexcept;
            bool is_message_data_valid() const noexcept;

            uint32_t get_data_size() const noexcept;
            void set_data_size(uint32_t _value) noexcept;

            int64_t get_prev_msgid() const noexcept { return prev_id_; }
            void set_prev_msgid(int64_t _value) noexcept;

            uint8_t get_version() const { return version_; }

            void serialize(core::tools::binary_stream& _data) const;
            bool unserialize(core::tools::binary_stream& _data);

            void merge_with(const message_header &rhs);

            bool is_deleted() const noexcept;
            bool is_restored_patch() const noexcept;
            bool is_modified() const noexcept;
            bool is_updated() const noexcept;
            bool is_patch() const noexcept;
            bool is_updated_message() const noexcept;
            bool is_outgoing() const noexcept;

            void set_updated(bool _value) noexcept;

            const message_header_vec& get_modifications() const;
            bool has_modifications() const;

            const common::tools::patch_version& get_update_patch_version() const noexcept { return update_patch_version_; }
            void set_update_patch_version(common::tools::patch_version _value) { update_patch_version_ = std::move(_value); }
            void increment_offline_version() noexcept { update_patch_version_.increment_offline(); }

            bool has_shared_contact_with_sn() const noexcept;
            bool has_poll_with_id() const noexcept;
            bool has_reactions() const noexcept;

            friend bool operator<(const message_header& _header1, const message_header& _header2) noexcept
            {
                return _header1.get_id() < _header2.get_id();
            }

            friend bool operator<(const message_header& _header1, const int64_t& _id) noexcept
            {
                return _header1.get_id() < _id;
            }
        };
        //////////////////////////////////////////////////////////////////////////



        class sticker_data
        {
            std::string		id_;

        public:
            sticker_data();

            sticker_data(std::string _id);

            const std::string& get_id() const { return id_; }

            static std::pair<int32_t, int32_t> get_ids(const std::string& _id);

            void serialize(icollection* _collection);
            void serialize(core::tools::tlvpack& _pack);
            int32_t unserialize(const rapidjson::Value& _node);
            int32_t unserialize(core::tools::tlvpack& _pack);
        };

        class mult_data
        {
        public:
            void serialize(icollection* _collection) {}
            void serialize(core::tools::tlvpack& _pack) {}
            int32_t unserialize(const rapidjson::Value& _node) { return 0; }
            int32_t unserialize(core::tools::tlvpack& _pack) { return 0; }
        };

        class voip_data
            : public core::iserializable
            , public core::tools::iserializable_tlv
        {
        public:
            voip_data();

            void apply_persons(const archive::persons_map &_persons);

            bool unserialize(const rapidjson::Value &_node, const std::string &_sender_aimid);

            virtual void serialize(Out coll_helper &_coll) const override;
            virtual bool unserialize(const coll_helper &_coll) override;

            virtual void serialize(Out core::tools::tlvpack &_pack) const override;
            virtual bool unserialize(const core::tools::tlvpack &_pack) override;

            int32_t is_incoming() const;

        private:
            voip_event_type type_;
            std::string sid_;
            std::string sender_aimid_;
            std::string sender_friendly_;
            int32_t duration_sec_;
            int32_t is_incoming_;
            bool is_video_;
            std::vector<std::string> conf_members_;

            virtual void unserialize_duration(const rapidjson::Value &_node);
            void unserialize_conf_members(const tools::tlvpack& _pack);
        };

        using voip_data_uptr = std::unique_ptr<voip_data>;

        class chat_data
        {
            std::string	sender_;
            core::archive::person friendly_;
            std::string	name_;

        public:

            void apply_persons(const archive::persons_map &_persons);

            const std::string& get_sender() const { return sender_; }
            void set_sender(const std::string_view _sender) { sender_ = _sender; }

            const std::string& get_name() const { return name_; }

            const core::archive::person& get_friendly() const { return friendly_; }
            void set_friendly(const core::archive::person& _friendly) { friendly_ = _friendly; }

            void serialize(core::tools::tlvpack& _pack);
            void serialize(icollection* _collection);
            int32_t unserialize(const rapidjson::Value& _node);
            int32_t unserialize(core::tools::tlvpack& _pack);
        };

        typedef std::unique_ptr<class file_sharing_data> file_sharing_data_uptr;

        class file_sharing_data
        {
        public:
            file_sharing_data(std::string_view _local_path, std::string_view _uri);
            file_sharing_data(std::string_view _local_path, std::string_view _uri, std::optional<int64_t> duration, std::optional<file_sharing_base_content_type> base_content_type);

            file_sharing_data(icollection* _collection);

            file_sharing_data(const core::tools::tlvpack &_pack);

            file_sharing_data();
            ~file_sharing_data();

            bool contents_equal(const file_sharing_data& _rhs) const;

            const std::string& get_local_path() const;

            const std::string& get_uri() const;

            bool is_empty() const noexcept { return uri_.empty() && local_path_.empty(); };

            void serialize(Out icollection* _collection, const std::string &_internal_id, const bool _is_outgoing) const;

            void serialize(Out core::tools::tlvpack& _pack) const;

            std::string to_log_string() const;

            const std::optional<int64_t>& get_duration() const noexcept;
            const std::optional<file_sharing_base_content_type>& get_base_content_type() const noexcept;

        private:
            std::string uri_;
            std::string local_path_;
            std::optional<int64_t> duration_;
            std::optional<file_sharing_base_content_type> base_content_type_;
        };

        using chat_event_data_sptr = std::shared_ptr<class chat_event_data>;

        class chat_event_data
        {
        public:
            static chat_event_data_sptr make_added_to_buddy_list(const std::string &_sender_aimid);
            static chat_event_data_sptr make_mchat_event(const rapidjson::Value& _node);
            static chat_event_data_sptr make_modified_event(const rapidjson::Value& _node);
            static chat_event_data_sptr make_from_tlv(const tools::tlvpack& _pack);
            static chat_event_data_sptr make_simple_event(const chat_event_type _type);
            static chat_event_data_sptr make_generic_event(const rapidjson::Value& _text_node);
            static chat_event_data_sptr make_generic_event(std::string _text);
            static chat_event_data_sptr make_status_reply_event(const rapidjson::Value& _node);
            static chat_event_data_sptr make_custom_status_reply_event(const rapidjson::Value& _node);

            void apply_persons(const archive::persons_map &_persons);
            bool contents_equal(const chat_event_data& _rhs) const;

            void serialize(Out icollection* _collection) const;
            void serialize(Out tools::tlvpack& _pack) const;

            bool is_type_deleted() const noexcept;
            void set_captcha_present(bool _is_captcha_present) { is_captcha_present_ = _is_captcha_present; }

            archive::persons_map get_persons() const;

        private:
            chat_event_data(const chat_event_type _type);
            chat_event_data(const tools::tlvpack &_pack);

            void deserialize_chat_modifications(const tools::tlvpack &_pack);
            void deserialize_mchat_members(const tools::tlvpack &_pack);
            void deserialize_mchat_members_aimids(const tools::tlvpack &_pack);
            void deserialize_status_reply(const tools::tlvpack& _pack);

            chat_event_type get_type() const;
            bool has_generic_text() const;
            bool has_mchat_members() const;
            bool has_chat_modifications() const;
            bool has_sender_aimid() const;

            void serialize_chat_modifications(Out coll_helper &_coll) const;
            void serialize_chat_modifications(Out tools::tlvpack &_pack) const;
            void serialize_mchat_members(Out coll_helper &_coll) const;
            void serialize_mchat_members_aimids(Out coll_helper &_coll) const;
            void serialize_mchat_members(Out tools::tlvpack &_pack) const;
            void serialize_mchat_members_aimids(Out tools::tlvpack &_pack) const;
            void serialize_status_reply(Out coll_helper& _coll) const;
            void serialize_status_reply(Out tools::tlvpack& _pack) const;

            chat_event_type type_;
            bool is_captcha_present_;
            bool is_channel_;
            std::string sender_aimid_;
            std::string sender_friendly_;

            struct
            {
                string_vector_t members_;
                string_vector_t members_friendly_;
                std::string requested_by_;
                std::string requested_by_friendly_;
            } mchat_;

            struct
            {
                std::string new_name_;
                std::string new_description_;
                std::string new_rules_;
                std::string new_stamp_;
                bool new_join_moderation_;
                bool new_public_;
            } chat_;

            struct
            {
                std::string sender_status_;
                std::string sender_status_description_;
                std::string owner_status_;
                std::string owner_status_descriprion_;
            } status_reply_;

            std::string generic_;
        };

        using history_message_sptr = std::shared_ptr<class history_message>;

        class format_data : public core::data::format::string_formatting
        {
        public:
            format_data() = default;
            format_data(const core::data::format::string_formatting& _data) { *static_cast<core::data::format::string_formatting*>(this) = _data; }
            void serialize(icollection* _collection, std::string_view _name) const;
            void serialize(tools::tlvpack& _pack) const;
            void unserialize(const rapidjson::Value& _node);
            bool unserialize(core::tools::tlvpack& _pack);
            void unserialize(const coll_helper& _coll, std::string_view _name);
        };
        using format_data_uptr = std::unique_ptr<format_data>;

        class history_message
        {
            int64_t msgid_;
            int64_t prev_msg_id_;
            message_flags flags_;
            time_t time_;
            std::string text_;
            format_data_uptr format_;
            int64_t data_offset_;
            uint32_t data_size_;
            std::string internal_id_;
            std::string sender_friendly_;
            common::tools::patch_version update_patch_version_;

            std::unique_ptr<sticker_data> sticker_;
            std::unique_ptr<mult_data> mult_;
            voip_data_uptr voip_;
            std::unique_ptr<chat_data> chat_;
            file_sharing_data_uptr file_sharing_;
            chat_event_data_sptr chat_event_;
            quotes_vec quotes_;
            mentions_map mentions_;
            snippets_vec snippets_;
            std::string description_;
            format_data_uptr description_format_;
            std::string url_;
            shared_contact shared_contact_;
            geo geo_;
            poll poll_;
            std::string json_;
            std::string sender_aimid_;
            bool unsupported_;
            bool hide_edit_;
            std::vector<std::vector<button_data>> buttons_;
            std::string buttons_json_;
            reactions reactions_;

            void copy(const history_message& _message);

            void init_default();

            void reset_extended_data();

            void set_deleted(const bool _deleted);

            void set_restored_patch(const bool _restored);

            void set_modified(const bool _modified);

            void set_updated(const bool _updated);

            void set_clear(const bool _clear);

            void set_patch(const bool _patch);

        public:

            void merge(const history_message& _message);

            static history_message_sptr make_deleted_patch(const int64_t _archive_id, std::string_view _internal_id);
            static history_message_sptr make_modified_patch(const int64_t _archive_id, std::string_view _internal_id, chat_event_data_sptr _chat_event = chat_event_data_sptr());
            static history_message_sptr make_updated_patch(const int64_t _archive_id, std::string_view _internal_id);
            static history_message_sptr make_updated(const int64_t _archive_id, std::string_view _internal_id);
            static history_message_sptr make_clear_patch(const int64_t _archive_id, std::string_view _internal_id);
            static history_message_sptr make_set_reactions_patch(const int64_t _archive_id, std::string_view _internal_id);

            static void make_deleted(history_message_sptr _message);
            static void make_modified(history_message_sptr _message);

            history_message();
            history_message(const history_message& _message);
            history_message& operator=(const history_message& _message);
            ~history_message();

            void serialize(icollection* _collection, const time_t _offset, bool _serialize_message = true) const;
            void serialize(core::tools::binary_stream& _data) const;
            void serialize_call(core::tools::binary_stream& _data, const std::string& _aimid) const;

            int32_t unserialize(const rapidjson::Value& _node, const std::string &_sender_aimid);
            int32_t unserialize(core::tools::binary_stream& _data);
            int32_t unserialize_call(core::tools::binary_stream& _data, std::string& _aimid);

            static void jump_to_text_field(core::tools::binary_stream& _stream, uint32_t& length);
            static int64_t get_id_field(core::tools::binary_stream& _stream);
            static bool is_sticker(core::tools::binary_stream& _stream);
            static bool is_chat_event(core::tools::binary_stream& _stream);

            void set_msgid(const int64_t _msgid) noexcept { msgid_ = _msgid; }
            int64_t get_msgid() const noexcept { return msgid_; }
            bool has_msgid() const noexcept { return msgid_ > 0; }

            message_flags get_flags() const noexcept;
            time_t get_time() const noexcept { return time_; }

            int64_t get_data_offset() const noexcept { return data_offset_; }
            void set_data_offset(int64_t _value) noexcept { data_offset_ = _value; }

            uint32_t get_data_size() const noexcept { return data_size_; }
            void set_data_size(uint32_t _value) noexcept { data_size_ = _value; }

            int64_t get_prev_msgid() const noexcept { return prev_msg_id_; }
            void set_prev_msgid(int64_t _value) noexcept;
            bool has_prev_msgid() const noexcept { return (prev_msg_id_ > 0); }

            const common::tools::patch_version& get_update_patch_version() const { return update_patch_version_; }
            void set_update_patch_version(common::tools::patch_version _value);
            void increment_offline_version();

            const std::string& get_text() const noexcept;
            void set_text(std::string _text) noexcept;
            bool has_text() const noexcept;

            bool has_format() const noexcept { return format_.operator bool(); }
            void set_format(const core::data::format::string_formatting& _format);
            const format_data get_format() const { return has_format() ? *format_ : format_data(); }

            archive::chat_data* get_chat_data() noexcept;
            const archive::chat_data* get_chat_data() const noexcept;
            void set_chat_data(const chat_data& _data);

            void set_internal_id(std::string _internal_id) { internal_id_ = std::move(_internal_id); }
            const std::string& get_internal_id() const noexcept { return internal_id_; }
            bool has_internal_id() const noexcept { return !internal_id_.empty(); }

            void set_sender_friendly(const std::string& _friendly) { sender_friendly_ = _friendly; }
            const std::string& get_sender_friendly() const noexcept { return sender_friendly_; }
            bool has_sender_friendly() const noexcept { return !sender_friendly_.empty(); }

            void set_time(uint64_t time) { time_ = time; }

            void set_description(const std::string& _description) { description_ = _description; }
            const std::string& description() const { return description_; }
            void set_description_format(const core::data::format::string_formatting& _format);

            core::data::format::string_formatting description_format() const { return description_format_ ? *description_format_ : core::data::format::string_formatting(); }

            void set_url(const std::string& _url) { url_ = _url; }
            std::string url() const { return url_; }

            bool is_outgoing() const;
            void set_outgoing(const bool _outgoing);

            bool is_deleted() const;
            bool is_modified() const;
            bool is_updated() const;
            bool is_clear() const;
            bool is_patch() const;
            bool is_chat_event_deleted() const;
            bool is_restored_patch() const;

            void apply_header_flags(const message_header &_header);
            void apply_modifications(const history_block &_modifications);

            const quotes_vec& get_quotes() const;
            void attach_quotes(quotes_vec _quotes);

            const mentions_map& get_mentions() const;
            void set_mentions(mentions_map _mentions);

            const snippets_vec& get_snippets() const;
            void set_snippets(snippets_vec _snippets);

            bool is_sms() const noexcept { return false; }
            bool is_sticker() const noexcept { return (bool)sticker_; }
            bool is_file_sharing() const noexcept { return (bool)file_sharing_; }
            bool is_chat_event() const noexcept { return (bool)chat_event_; }
            bool is_voip_event() const noexcept { return (bool)voip_; }

            bool init_file_sharing_from_local_path(const std::string& _local_path);
            bool init_file_sharing_from_local_path(const std::string& _local_path, std::optional<int64_t> _duration, std::optional<file_sharing_base_content_type> _base_content_type);
            void init_file_sharing_from_link(const std::string &_uri);
            void init_file_sharing(file_sharing_data&&);
            void init_sticker_from_text(std::string _text);
            const file_sharing_data_uptr& get_file_sharing_data() const;

            const chat_event_data_sptr& get_chat_event_data() const;
            voip_data_uptr& get_voip_data();

            message_type get_type() const noexcept;

            bool contents_equal(const history_message& _msg) const;

            void apply_persons_to_quotes(const archive::persons_map & _persons);
            void apply_persons_to_mentions(const archive::persons_map & _persons);
            void apply_persons_to_voip(const archive::persons_map & _persons);

            void set_shared_contact(const shared_contact& _contact);
            const shared_contact& get_shared_contact() const;

            void set_geo(const geo& _geo);
            const geo& get_geo() const;

            void set_poll(const poll& _poll);
            const poll& get_poll() const;

            void set_poll_id(const std::string& _poll_id);

            bool has_shared_contact_with_sn() const;
            bool has_poll_with_id() const;
            bool has_reactions() const;

            const reactions& get_reactions() const;
        };

        class quote
        {
            std::string text_;
            std::optional<format_data> format_;
            std::string sender_;
            std::string chat_;
            std::string chat_stamp_;
            std::string chat_name_;
            std::string senderFriendly_;
            std::string url_;
            std::string description_;
            std::optional<format_data> description_format_;
            int32_t time_;
            int32_t setId_;
            int32_t stickerId_;
            int64_t msg_id_;
            bool is_forward_;
            shared_contact shared_contact_;
            geo geo_;
            poll poll_;

        public:
            quote();

            void serialize(icollection* _collection) const;
            void serialize(core::tools::tlvpack& _pack) const;
            void unserialize(icollection* _coll);
            bool unserialize(const rapidjson::Value& _node, bool _is_forward, const message_fields_set& _new_fields);
            void unserialize(const core::tools::tlvpack &_pack);

            const std::string& get_text() const { return text_; }
            const std::optional<format_data>& get_format() const { return format_; }
            const std::string& get_sender() const { return sender_; }
            const std::string& get_chat() const { return chat_; }
            const std::string& get_sender_friendly() const { return senderFriendly_; }
            bool is_forward() const { return is_forward_; }
            void set_sender_friendly(const std::string& _friendly) { senderFriendly_ = _friendly; }
            int32_t get_time() const { return time_; }
            void set_time(const int32_t _time) { time_ = _time; }
            int64_t get_msg_id() const { return msg_id_; }
            std::string get_type() const { return is_forward_ ? "forward" : "quote"; }
            std::string get_sticker() const;
            const std::string& get_stamp() const { return chat_stamp_; }
            const std::string& get_chat_name() const { return chat_name_; }
            const std::string& get_url() const { return url_; }
            const std::string& get_description() const { return description_; }
            format_data get_description_format() const { return description_format_ ? *description_format_ : format_data(); }
            const shared_contact& get_shared_contact() const { return shared_contact_; }
            const geo& get_geo() const { return geo_; }
            const poll& get_poll() const { return poll_; }
        };

        class url_snippet
        {
        public:
            url_snippet(std::string_view _url = std::string_view());

            void serialize(icollection* _collection) const;
            void serialize(core::tools::tlvpack& _pack) const;
            void unserialize(const rapidjson::Value& _node);
            void unserialize(const core::tools::tlvpack& _pack);

            const std::string& get_url() const { return url_; }

        private:
            std::string url_;
            std::string content_type_;

            std::string preview_url_;
            int preview_width_;
            int preview_height_;

            std::string title_;
            std::string description_;
        };
    }
}
