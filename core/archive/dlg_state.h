#pragma once

#include <stdint.h>
#include "history_message.h"
#include "../../common.shared/patch_version.h"

//////////////////////////////////////////////////////////////////////////
// dlg_state class
//////////////////////////////////////////////////////////////////////////
namespace core
{
    struct icollection;

    namespace archive
    {
        class storage;
        class history_message;

        using dlg_state_head = std::pair<std::string, std::string>;

        class dlg_state
        {
            std::optional<uint32_t> unread_count_;
            std::optional<int32_t> unread_mentions_count_;
            std::optional<std::string> info_version_;
            int64_t last_read_mention_;
            int64_t last_msgid_;
            int64_t yours_last_read_;
            int64_t del_up_to_;
            int64_t theirs_last_read_;
            int64_t theirs_last_delivered_;
            int64_t hidden_msg_id_;

            bool visible_;
            bool fake_;
            bool official_;
            bool suspicious_;
            bool attention_;
            bool stranger_;

            std::unique_ptr<history_message> last_message_;
            std::unique_ptr<history_message> pinned_message_;

            std::string friendly_;

            // patch version from getWimHistory (stored in db)
            common::tools::patch_version history_patch_version_;

            // current patch version (not stored in db)
            std::string dlg_state_patch_version_;

            // heads
            std::vector<dlg_state_head> heads_;

        public:

            dlg_state();
            dlg_state(const dlg_state& _state);
            ~dlg_state();

            dlg_state& operator=(const dlg_state& _state);

            void copy_from(const dlg_state& _state);

            void set_unread_count(uint32_t _unread_count) noexcept { unread_count_ = _unread_count; }
            void set_unread_count(std::optional<uint32_t> _unread_count) noexcept { unread_count_ = _unread_count; }
            uint32_t get_unread_count() const noexcept { return unread_count_.value_or(0); }
            bool has_unread_count() const noexcept { return unread_count_.has_value(); }

            void set_last_msgid(const int64_t _value);
            int64_t get_last_msgid() const noexcept { return last_msgid_; }
            bool has_last_msgid() const noexcept { return last_msgid_ > 0; }
            void clear_last_msgid() noexcept { last_msgid_ = -1; }

            void set_yours_last_read(int64_t _val) noexcept { yours_last_read_ = _val; }
            int64_t get_yours_last_read() const { return yours_last_read_; }
            bool has_yours_last_read() const noexcept { return yours_last_read_ > 0; }

            void set_theirs_last_read(int64_t _val) noexcept { theirs_last_read_ = _val; }
            int64_t get_theirs_last_read() const noexcept { return theirs_last_read_; }

            void set_theirs_last_delivered(int64_t _val) noexcept { theirs_last_delivered_ = _val; }
            int64_t	get_theirs_last_delivered() const noexcept { return theirs_last_delivered_; }

            void set_hidden_msg_id(int64_t _val) noexcept { hidden_msg_id_ = _val; }
            int64_t get_hidden_msg_id() const noexcept { return hidden_msg_id_; }

            void set_visible(bool _val) noexcept { visible_ = _val; }
            bool get_visible() const noexcept { return visible_; }

            void set_fake(bool _val) noexcept { fake_ = _val; }
            bool get_fake() const noexcept { return fake_; }

            void set_friendly(const std::string& _friendly) {friendly_ = _friendly;}
            const std::string& get_friendly() const noexcept {return friendly_;}

            void set_official(bool _official) noexcept {official_ = _official;}
            bool get_official() const noexcept {return official_;}

            void set_suspicious(bool _suspicious) { suspicious_ = _suspicious; }
            bool get_suspicious() const { return suspicious_; }

            void set_attention(bool _attention) noexcept { attention_ = _attention; }
            bool get_attention() const noexcept { return attention_; }

            void set_stranger(bool _stranger) { stranger_ = _stranger; }
            bool get_stranger() const { return stranger_; }

            const history_message& get_last_message() const;
            void set_last_message(const history_message& _message);
            bool has_last_message() const;
            void clear_last_message();

            const history_message& get_pinned_message() const;
            void set_pinned_message(const history_message& _message);
            bool has_pinned_message() const;
            void clear_pinned_message();

            const common::tools::patch_version& get_history_patch_version() const;
            common::tools::patch_version get_history_patch_version(std::string_view _default) const;
            bool has_history_patch_version() const;
            void set_history_patch_version(common::tools::patch_version _patch_version);
            void set_history_patch_version(std::string_view _patch_version);
            void reset_history_patch_version();

            const std::string& get_dlg_state_patch_version() const;
            void set_dlg_state_patch_version(std::string _patch_version);

            int64_t get_del_up_to() const;
            void set_del_up_to(const int64_t _msg_id);
            bool has_del_up_to() const;

            bool is_empty() const;

            void set_unread_mentions_count(const int32_t _count);
            void set_unread_mentions_count(std::optional<int32_t> _count);
            int32_t get_unread_mentions_count() const;
            bool has_unread_mentions_count() const;

            void set_last_read_mention(int64_t _val) noexcept { last_read_mention_ = _val; }
            int64_t get_last_read_mention() const noexcept { return last_read_mention_; }
            bool has_last_read_mention() const noexcept { return last_read_mention_ > 0; }

            void set_info_version(const std::string& _info_version) noexcept { info_version_ = _info_version; }
            void set_info_version(const std::optional<std::string>& _info_version) noexcept { info_version_ = _info_version; }
            const std::optional<std::string>& get_info_version() const noexcept { return info_version_; }
            bool has_info_version() const { return info_version_.has_value(); }


            void set_heads(std::vector<dlg_state_head>&& _heads);
            void set_heads(const std::vector<dlg_state_head>& _heads);
            const std::vector<dlg_state_head>& get_heads() const;

            void serialize(icollection* _collection, const time_t _offset, const time_t _last_successful_fetch, const bool _serialize_message = true) const;
            void serialize(core::tools::binary_stream& _data) const;

            bool unserialize(core::tools::binary_stream& _data);
        };
        //////////////////////////////////////////////////////////////////////////

        struct dlg_state_changes
        {
            dlg_state_changes();

            bool del_up_to_changed_;

            bool history_patch_version_changed_;

            bool initial_fill_;

            bool last_message_changed_;
        };

        class archive_state
        {
            std::unique_ptr<dlg_state> state_;
            std::unique_ptr<storage> storage_;
            const std::string contact_id_;

            bool load();

        public:

            archive_state(std::wstring _file_name, std::string _contact_id);
            ~archive_state();

            bool merge_state(const dlg_state& _new_state, Out dlg_state_changes& _changes);

            bool save();

            const dlg_state& get_state();
            void set_state(const dlg_state& _state, Out dlg_state_changes& _changes);
            void update_attention_attribute(const bool _value);
            void clear_state();
        };
    }
}
