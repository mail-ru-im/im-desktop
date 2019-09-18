#ifndef __BASE_IM_H_
#define __BASE_IM_H_

#pragma once

#include "../../common.shared/patch_version.h"

#include "accept_agreement_info.h"
#include "wim/privacy_settings.h"
#include "archive/history_message.h"

namespace voip_manager {
    class VoipManager;
    struct VoipProtoMsg;
    struct WindowParams;
    struct VoipProxySettings;
    enum AvatarThemeType : unsigned short;
}

namespace core
{
    class login_info;
    class im_login_id;
    class phone_info;
    class search_params;
    enum class file_sharing_function;
    enum class message_type;
    enum class sticker_size;
    enum class profile_state;
    enum class typing_status;
    enum class message_read_mode;
    class auto_callback;

    class masks;

    namespace wim
    {
        class im;
        class wim_packet;
        struct wim_packet_params;
        class chat_params;
        struct auth_parameters;
        enum class is_ping;
        enum class is_login;
        enum class is_updated_messages;
    }

    namespace archive
    {
        class message_header;
        class quote;
        struct shared_contact_data;

        struct gallery_entry_id;
        typedef std::vector<int64_t> msgids_list;
        typedef std::vector<quote> quotes_vec;
        typedef std::map<std::string, std::string> mentions_map;
        using shared_contact = std::optional<shared_contact_data>;

        struct message_pack
        {
            std::string message_;
            quotes_vec quotes_;
            mentions_map mentions_;
            std::string internal_id_;
            core::message_type type_;
            uint64_t message_time_ = 0;
            std::string description_;
            std::string url_;
            common::tools::patch_version version_;
            shared_contact shared_contact_;
        };
    }

    namespace stickers
    {
        class face;
    }

    namespace themes
    {
        class wallpaper_loader;
    }

    namespace  memory_stats
    {
        class memory_stats_collector;
        struct request_handle;
        struct partial_response;
        using request_id = int64_t;
    }

    class base_im
    {
        std::shared_ptr<voip_manager::VoipManager> voip_manager_;
        int32_t id_;

        // stickers
        // use it only this from thread
        std::shared_ptr<stickers::face> stickers_;

        std::shared_ptr<themes::wallpaper_loader> wp_loader_;

    protected:
        std::shared_ptr<core::memory_stats::memory_stats_collector> memory_stats_collector_;
        std::shared_ptr<masks> masks_;

        std::wstring get_contactlist_file_name() const;
        std::wstring get_my_info_file_name() const;
        std::wstring get_active_dilaogs_file_name() const;
        std::wstring get_favorites_file_name() const;
        std::wstring get_mailboxes_file_name() const;
        std::wstring get_im_data_path() const;
        std::wstring get_avatars_data_path() const;
        std::wstring get_file_name_by_url(const std::string& _url) const;
        std::wstring get_masks_path() const;
        std::wstring get_stickers_path() const;
        std::wstring get_im_downloads_path(const std::string &alt) const;
        std::wstring get_content_cache_path() const;
        std::wstring get_search_history_path() const;

        virtual std::string _get_protocol_uid() = 0;
        void set_id(int32_t _id);
        int32_t get_id() const;

        void create_masks(std::weak_ptr<wim::im> _im);

        std::shared_ptr<stickers::face> get_stickers();
        std::shared_ptr<themes::wallpaper_loader> get_wallpaper_loader();

        std::vector<std::string> get_audio_devices_names();
        std::vector<std::string> get_video_devices_names();

    public:
        base_im(const im_login_id& _login,
                std::shared_ptr<voip_manager::VoipManager> _voip_manager,
                std::shared_ptr<memory_stats::memory_stats_collector> _memory_stats_collector);
        virtual ~base_im();

        virtual void connect() = 0;
        virtual std::wstring get_im_path() const = 0;

        // login functions
        virtual void login(int64_t _seq, const login_info& _info) = 0;
        virtual void login_normalize_phone(int64_t _seq, const std::string& _country, const std::string& _raw_phone, const std::string& _locale, bool _is_login) = 0;
        virtual void login_get_sms_code(int64_t _seq, const phone_info& _info, bool _is_login) = 0;
        virtual void login_by_phone(int64_t _seq, const phone_info& _info) = 0;

        virtual void start_attach_uin(int64_t _seq, const login_info& _info, const wim::wim_packet_params& _from_params) = 0;
        virtual void start_attach_phone(int64_t _seq, const phone_info& _info) = 0;

        virtual std::string get_login() = 0;
        virtual void logout(std::function<void()> _on_result) = 0;

        virtual wim::wim_packet_params make_wim_params() = 0;
        virtual wim::wim_packet_params make_wim_params_general(bool _is_current_auth_params) = 0;

        virtual void erase_auth_data() = 0; // when logout
        virtual void start_session(wim::is_ping _is_ping, wim::is_login _is_login) = 0;
        virtual void handle_net_error(int32_t err) = 0;

        virtual void phoneinfo(int64_t _seq, const std::string &phone, const std::string &gui_locale) = 0;

        // messaging functions
        virtual void send_message_to_contact(
            const int64_t _seq,
            const std::string& _contacts,
            core::archive::message_pack _pack) = 0;

        virtual void send_message_to_contacts(
            const int64_t _seq,
            const std::vector<std::string>& _contact,
            core::archive::message_pack _pack) = 0;

        virtual void update_message_to_contact(
            const int64_t _seq,
            const int64_t _id,
            const std::string& _contact,
            core::archive::message_pack _pack) = 0;


        virtual void send_message_typing(const int64_t _seq, const std::string& _contact, const core::typing_status& _status, const std::string& _id) = 0;

        // feedback
        virtual void send_feedback(const int64_t, const std::string &url, const std::map<std::string, std::string> &fields, const std::vector<std::string> &files) = 0;

        // state
        virtual void set_state(const int64_t, const core::profile_state) = 0;

        // group chat
        virtual void remove_members(int64_t _seq, const std::string& _aimid, const std::string& _m_chat_members) = 0;
        virtual void add_members(int64_t _seq, const std::string& _aimid, const std::string& _m_chat_members_to_add) = 0;
        virtual void add_chat(int64_t _seq, const std::string& _m_chat_name, std::vector<std::string> _m_chat_members) = 0;
        virtual void modify_chat(int64_t _seq, const std::string& _aimid, const std::string& _m_chat_name) = 0;

        // mrim
        virtual void get_mrim_key(int64_t _seq, const std::string& _email) = 0;

        // avatar function
        virtual void get_contact_avatar(int64_t _seq, const std::string& _contact, int32_t _avatar_size, bool _force) = 0;
        virtual void show_contact_avatar(int64_t _seq, const std::string& _contact, int32_t _avatar_size) = 0;
        virtual void remove_contact_avatars(int64_t _seq, const std::string& _contact) = 0;

        // history functions
        virtual void get_archive_messages(int64_t _seq_, const std::string& _contact, int64_t _from, int64_t _count_early, int64_t _count_later, bool _need_prefetch, bool _first_request, bool _after_search) = 0;
        virtual void get_archive_messages_buddies(int64_t _seq_, const std::string& _contact, std::shared_ptr<archive::msgids_list> _ids, wim::is_updated_messages) = 0;
        virtual void set_last_read(const std::string& _contact, int64_t _message, message_read_mode _mode) = 0;
        virtual void set_last_read_mention(const std::string& _contact, int64_t _message) = 0;
        virtual void set_last_read_partial(const std::string& _contact, int64_t _message) = 0;
        virtual void hide_dlg_state(const std::string& _contact) = 0;
        virtual void delete_archive_message(const int64_t _seq, const std::string &_contact_aimid, const int64_t _message_id, const std::string& _internal_id, const bool _for_all) = 0;
        virtual void delete_archive_messages_from(const int64_t _seq, const std::string &_contact_aimid, const int64_t _from_id) = 0;
        virtual void delete_archive_all_messages(const int64_t _seq, std::string_view _contact_aimid) = 0;
        virtual void get_messages_for_update(const std::string& _contact) = 0;

        virtual void get_mentions(int64_t _seq, std::string_view _contact) = 0;

        virtual void get_message_context(int64_t _seq, const std::string& _contact, const int64_t _msgid) = 0;

        virtual void add_opened_dialog(const std::string& _contact) = 0;
        virtual void remove_opened_dialog(const std::string& _contact) = 0;

        virtual void get_stickers_meta(int64_t _seq, const std::string& _size) = 0;
        virtual void get_sticker(const int64_t _seq, const int32_t _set_id, const int32_t _sticker_id, std::string_view _fs_id, const core::sticker_size _size) = 0;
        virtual void get_stickers_pack_info(const int64_t _seq, const int32_t _set_id, const std::string& _store_id, const std::string& _file_id) = 0;
        virtual void add_stickers_pack(const int64_t _seq, const int32_t _set_id, std::string _store_id) = 0;
        virtual void remove_stickers_pack(const int64_t _seq, const int32_t _set_id, std::string _store_id) = 0;
        virtual void get_stickers_store(const int64_t _seq) = 0;
        virtual void search_stickers_store(const int64_t _seq, const std::string& _search_term) = 0;
        virtual void get_set_icon_big(const int64_t _seq, const int32_t _set_id) = 0;
        virtual void clean_set_icon_big(const int64_t _seq, const int32_t _set_id) = 0;
        virtual void set_sticker_order(const int64_t _seq, const std::vector<int32_t>& _values) = 0;
        virtual void get_fs_stickers_by_ids(const int64_t _seq, std::vector<std::pair<int32_t, int32_t>> _values) = 0;

        virtual void get_chat_home(int64_t _seq, const std::string& _tag) = 0;
        virtual void get_chat_info(int64_t _seq, const std::string& _aimid, const std::string& _stamp, int32_t _limit) = 0;
        virtual void get_chat_member_info(int64_t _seq, const std::string& _aimid, const std::vector<std::string>& _members) = 0;
        virtual void get_chat_members_page(int64_t _seq, std::string_view _aimid, std::string_view _role, uint32_t _page_size) = 0;
        virtual void get_chat_members_next_page(int64_t _seq, std::string_view _aimid, std::string_view _cursor, uint32_t _page_size) = 0;
        virtual void search_chat_members_page(int64_t _seq, std::string_view _aimid, std::string_view _role, std::string_view _keyword, uint32_t _page_size) = 0;
        virtual void search_chat_members_next_page(int64_t _seq, std::string_view _aimid, std::string_view _cursor, uint32_t _page_size) = 0;
        virtual void get_chat_contacts(int64_t _seq, const std::string& _aimid, const std::string& _cursor, uint32_t _page_size) = 0;

        virtual void check_themes_meta_updates(int64_t _seq) = 0;
        virtual void get_theme_wallpaper(const std::string_view _wp_id) = 0;
        virtual void get_theme_wallpaper_preview(const std::string_view _wp_id) = 0;
        virtual void set_user_wallpaper(const std::string_view _wp_id, tools::binary_stream _image) = 0;
        virtual void remove_user_wallpaper(const std::string_view _wp_id) = 0;

        virtual void resolve_pending(int64_t _seq, const std::string& _aimid, const std::vector<std::string>& _contact, bool _approve) = 0;

        virtual void create_chat(int64_t _seq, const std::string& _aimid, const std::string& _name, std::vector<std::string> _members, core::wim::chat_params _params) = 0;

        virtual void mod_chat_params(int64_t _seq, const std::string& _aimid, core::wim::chat_params _params) = 0;
        virtual void mod_chat_name(int64_t _seq, const std::string& _aimid, const std::string& _name) = 0;
        virtual void mod_chat_about(int64_t _seq, const std::string& _aimid, const std::string& _about) = 0;
        virtual void mod_chat_rules(int64_t _seq, const std::string& _aimid, const std::string& _rules) = 0;
        virtual void mod_chat_public(int64_t _seq, const std::string& _aimid, bool _public) = 0;
        virtual void mod_chat_join(int64_t _seq, const std::string& _aimid, bool _approved) = 0;
        virtual void mod_chat_link(int64_t _seq, const std::string& _aimid, bool _link) = 0;
        virtual void mod_chat_ro(int64_t _seq, const std::string& _aimid, bool _ro) = 0;

        virtual void set_attention_attribute(int64_t _seq, std::string _aimid, const bool _value, const bool _resume) = 0;

        virtual void block_chat_member(int64_t _seq, const std::string& _aimid, const std::string& _contact, bool _block) = 0;
        virtual void set_chat_member_role(int64_t _seq, const std::string& _aimid, const std::string& _contact, const std::string& _role) = 0;
        virtual void pin_chat_message(int64_t _seq, const std::string& _aimid, int64_t _message, bool _is_unpin) = 0;

        virtual void get_mentions_suggests(int64_t _seq, const std::string& _aimid, const std::string& _pattern) = 0;

        // search functions
        virtual void search_dialogs_local(const std::string& search_patterns, const std::vector<std::string>& _aimids) = 0;
        virtual void search_contacts_local(const std::vector<std::vector<std::string>>& search_patterns, int64_t _req_id, unsigned fixed_patterns_count, const std::string& pattern) = 0;
        virtual void setup_search_dialogs_params(int64_t _req_id) = 0;
        virtual void clear_search_dialogs_params() = 0;

        virtual void add_search_pattern_to_history(const std::string_view _search_pattern, const std::string_view _contact) = 0;
        virtual void remove_search_pattern_from_history(const std::string_view _search_pattern, const std::string_view _contact) = 0;

        // cl
        virtual std::string get_contact_friendly_name(const std::string& contact_login) = 0;
        virtual void hide_chat(const std::string& _contact) = 0;
        virtual void mute_chat(const std::string& _contact, bool _mute) = 0;
        virtual void add_contact(int64_t _seq, const std::string& _aimid, const std::string& _group, const std::string& _auth_message) = 0;
        virtual void remove_contact(int64_t _seq, const std::string& _aimid) = 0;
        virtual void rename_contact(int64_t _seq, const std::string& _aimid, const std::string& _friendly) = 0;
        virtual void ignore_contact(int64_t _seq, const std::string& _aimid, bool ignore) = 0;
        virtual void get_ignore_list(int64_t _seq) = 0;
        virtual void favorite(const std::string& _contact) = 0;
        virtual void unfavorite(const std::string& _contact) = 0;
        virtual void update_outgoing_msg_count(const std::string& _aimid, int _count) = 0;

        // voip
        //virtual void on_peer_list_updated(const std::vector<std::string>& peers) = 0;
        virtual void on_voip_call_request_calls();
        virtual void on_voip_call_set_proxy(const voip_manager::VoipProxySettings& proxySettings);
        virtual void on_voip_call_start(std::string contact, bool video, bool attach);
        virtual void on_voip_add_window(voip_manager::WindowParams& windowParams);
        virtual void on_voip_remove_window(void* hwnd);
        virtual void on_voip_call_end(std::string contact, bool busy);
        virtual void on_voip_call_accept(std::string contact, bool video);
        virtual void on_voip_call_stop();
        virtual void on_voip_proto_msg(bool allocate, const char* data, unsigned len, std::shared_ptr<auto_callback> _on_complete);
        virtual void on_voip_proto_ack(const voip_manager::VoipProtoMsg& msg, bool success);
        virtual void on_voip_update();
        virtual void on_voip_reset();

        virtual bool on_voip_avatar_actual_for_voip(const std::string& contact, unsigned avatar_size);
        virtual void on_voip_user_update_avatar(const std::string& contact, const unsigned char* data, unsigned size, unsigned avatar_h, unsigned avatar_w, voip_manager::AvatarThemeType theme);
        virtual void on_voip_user_update_avatar_no_video(const std::string& contact, const unsigned char* data, unsigned size, unsigned h, unsigned w, voip_manager::AvatarThemeType theme);
        //virtual void on_voip_user_update_avatar_camera_off(const std::string& contact, const unsigned char* data, unsigned size, unsigned h, unsigned w, voip_manager::AvatarThemeType theme);
        //virtual void on_voip_user_update_avatar_no_camera(const std::string& contact, const unsigned char* data, unsigned size, unsigned h, unsigned w, voip_manager::AvatarThemeType theme);
        virtual void on_voip_user_update_avatar_text(const std::string& contact, const unsigned char* data, unsigned size, unsigned h, unsigned w, voip_manager::AvatarThemeType theme);
        virtual void on_voip_user_update_avatar_text_header(const std::string& contact, const unsigned char* data, unsigned size, unsigned h, unsigned w, voip_manager::AvatarThemeType theme);
        virtual void on_voip_user_update_avatar_background(const std::string& contact, const unsigned char* data, unsigned size, unsigned h, unsigned w, voip_manager::AvatarThemeType theme);
        virtual void on_voip_window_update_background(void* hwnd, const unsigned char* data, unsigned size, unsigned w, unsigned h);
        virtual void on_voip_window_set_offsets(void* hwnd, unsigned l, unsigned t, unsigned r, unsigned b);
        virtual void on_voip_window_set_primary(void* hwnd, const std::string& contact);
        virtual void on_voip_window_set_conference_layout(void* hwnd, int);
        virtual void on_voip_window_set_hover(void* hwnd, bool);
        virtual void on_voip_window_set_large(void* hwnd, bool);

        virtual void on_voip_device_changed(const std::string& dev_type, const std::string& uid);

        virtual void on_voip_switch_media(bool video);
        virtual void on_voip_volume_change(int vol);
        virtual void on_voip_mute_switch();
        virtual void on_voip_set_mute(bool mute);
        virtual void on_voip_mute_incoming_call_sounds(bool mute);
        virtual void on_voip_minimal_bandwidth_switch();

        virtual void on_voip_load_mask(const std::string& path);
        virtual void on_voip_init_mask_engine();

        virtual void voip_set_model_path(const std::string& _local_path);
        virtual bool has_created_call();

        virtual void post_voip_msg_to_server(const voip_manager::VoipProtoMsg& msg) = 0;
        virtual void post_voip_alloc_to_server(const std::string& data) = 0;

        virtual std::shared_ptr<wim::wim_packet> prepare_voip_msg(const std::string& data) = 0;

        virtual std::shared_ptr<wim::wim_packet> prepare_voip_pac(const voip_manager::VoipProtoMsg& data) = 0;

        // files functions
        virtual void upload_file_sharing(
            const int64_t _seq,
            const std::string& _contact,
            const std::string& _file_name,
            std::shared_ptr<core::tools::binary_stream> _data,
            const std::string& _extension,
            const core::archive::quotes_vec& _quotes,
            const std::string& _description,
            const core::archive::mentions_map& _mentions,
            const std::optional<int64_t>& _duration) = 0;

        virtual void get_file_sharing_preview_size(
            const int64_t _seq,
            const std::string& _file_url,
            const int32_t _original_size) = 0;

        virtual void download_file_sharing_metainfo(
            const int64_t _seq,
            const std::string& _file_url) = 0;

        virtual void download_file_sharing(
            const int64_t _seq,
            const std::string& _contact,
            const std::string& _file_url,
            const bool _force_request_metainfo,
            const std::string& _filename,
            const std::string& _download_dir,
            bool _raise_priority) = 0;

        virtual void download_image(
            const int64_t _seq,
            const std::string& _file_url,
            const std::string& _destination,
            const bool _download_preview,
            const int32_t _preview_width,
            const int32_t _preview_height,
            const bool _ext_resource,
            const bool _raise_priority,
            const bool _with_data) = 0;

        virtual void download_link_preview(
            const int64_t _seq,
            const std::string& _contact_aimid,
            const std::string& _url,
            const int32_t _preview_width,
            const int32_t _preview_height) = 0;

        virtual void cancel_loader_task(const std::string& _url) = 0;

        virtual void abort_file_sharing_download(const std::string& _url) = 0;

        virtual void abort_file_sharing_upload(
            const int64_t _seq,
            const std::string & _contact,
            const std::string &_process_seq) = 0;
        virtual void raise_download_priority(
            const int64_t _task_id) = 0;

        virtual void contact_switched(const std::string &_contact_aimid) = 0;

        virtual void set_played(const std::string& url, bool played) = 0;
        virtual void speech_to_text(int64_t _seq, const std::string& _url, const std::string& _locale) = 0;

        // search for contacts
        virtual void search_contacts_server(int64_t _seq, const std::string_view _keyword, const std::string_view _phone) = 0;
        virtual void syncronize_address_book(const int64_t _seq, const std::string _keyword, const std::vector<std::string> _phone) = 0;

        virtual void search_dialogs_one(const int64_t _seq, const std::string_view _keyword, const std::string_view _contact) = 0;
        virtual void search_dialogs_all(const int64_t _seq, const std::string_view _keyword) = 0;
        virtual void search_dialogs_continue(const int64_t _seq, const std::string_view _cursor, const std::string_view _contact) = 0;

        virtual void get_profile(int64_t _seq, const std::string& _contact) = 0;

        // tools
        virtual void sign_url(int64_t _seq, const std::string& unsigned_url) = 0;

        virtual void update_profile(int64_t _seq, const std::vector<std::pair<std::string, std::string>>& _field) = 0;

        // live chats
        virtual void join_live_chat(int64_t _seq, const std::string& _stamp) = 0;

        virtual void set_avatar(const int64_t _seq, tools::binary_stream image, const std::string& _aimId, const bool _chat) = 0;

        virtual void save_auth_params_to_file(std::shared_ptr<core::wim::auth_parameters> auth_params, const std::wstring& _file_name, std::function<void()> _on_result) = 0;
        virtual void save_auth_to_export(std::function<void()> _on_result) = 0;

        // masks
        virtual void get_mask_id_list(int64_t _seq) = 0;
        virtual void get_mask_preview(int64_t _seq, const std::string& mask_id) = 0;
        virtual void get_mask_model(int64_t _seq) = 0;
        virtual void get_mask(int64_t _seq, const std::string& mask_id) = 0;
        virtual void get_existent_masks(int64_t _seq) = 0;

        virtual bool has_valid_login() const = 0;
        virtual void merge_exported_account(core::wim::auth_parameters& _params) = 0;

        virtual void get_code_by_phone_call(const std::string& _ivr_url) = 0;

        virtual void get_attach_phone_info(const int64_t _seq, const std::string& _locale, const std::string& _lang) = 0;
        virtual void get_voip_calls_quality_popup_conf(const int64_t _seq, const std::string& _locale, const std::string& _lang) = 0;
        virtual void get_logs_path(const int64_t _seq) = 0;
        virtual void remove_content_cache(const int64_t _seq) = 0;
        virtual void clear_avatars(const int64_t _seq) = 0;

        virtual void report_contact(int64_t _seq, const std::string& _aimid, const std::string& _reason, const bool _ignore_and_close) = 0;
        virtual void report_stickerpack(const int32_t _id, const std::string& _reason) = 0;
        virtual void report_sticker(const std::string& _id, const std::string& _reason, const std::string& _aimid, const std::string& _chatId) = 0;
        virtual void report_message(const int64_t _id, const std::string& _text, const std::string& _reason, const std::string& _aimid, const std::string& _chatId) = 0;

        virtual void user_accept_gdpr(int64_t _seq, const accept_agreement_info& _accept_info) = 0;

        virtual void send_voip_calls_quality_report(int _score, const std::string& _survey_id,
                                                    const std::vector<std::string>& _reasons, const std::string& _aimid);

        virtual void send_stat() = 0;

        virtual void get_dialog_gallery(const int64_t _seq,
                                        const std::string& _aimid,
                                        const std::vector<std::string>& _type,
                                        const archive::gallery_entry_id _after,
                                        const int _page_size) = 0;

        virtual void get_dialog_gallery_by_msg(const int64_t _seq, const std::string& _aimid, const std::vector<std::string>& _type, int64_t _msg_id) = 0;

        virtual void request_gallery_state(const std::string& _aimId) = 0;
        virtual void get_gallery_index(const std::string& _aimId, const std::vector<std::string>& _type, int64_t _msg, int64_t _seq) = 0;
        virtual void make_gallery_hole(const std::string& _aimid, int64_t _from, int64_t _till) = 0;

        virtual void request_memory_usage(const int64_t _seq) = 0;
        virtual void report_memory_usage(const int64_t _seq,
                                         memory_stats::request_id _req_handle,
                                         const memory_stats::partial_response& _partial_response) = 0;
        virtual void get_ram_usage(const int64_t _seq) = 0;

        virtual void make_archive_holes(const int64_t _seq, const std::string& _archive) = 0;
        virtual void invalidate_archive_data(const int64_t _seq, const std::string& _archive, std::vector<int64_t> _ids) = 0;
        virtual void invalidate_archive_data(const int64_t _seq, const std::string& _archive, int64_t _from, int64_t _before_count, int64_t _after_count) = 0;
        virtual void on_ui_activity(const int64_t _time) = 0;

        virtual void close_stranger(const std::string& _contact) = 0;

        virtual void on_local_pin_set(const std::string& _password) = 0;
        virtual void on_local_pin_entered(const int64_t _seq, const std::string& _password) = 0;
        virtual void on_local_pin_disable(const int64_t _seq) = 0;

        virtual void get_id_info(const int64_t _seq, const std::string_view _id) = 0;
        virtual void get_user_info(const int64_t _seq, const std::string& _aimid) = 0;
        virtual void get_user_last_seen(const int64_t _seq, const std::vector<std::string>& _ids) = 0;

        virtual void set_privacy_settings(const int64_t _seq, core::wim::privacy_settings _settings) = 0;
        virtual void get_privacy_settings(const int64_t _seq) = 0;

        virtual void on_nickname_check(const int64_t _seq, const std::string& _nickname, bool _set_nick) = 0;
        virtual void on_get_common_chats(const int64_t _seq, const std::string& _sn) = 0;
    };

}

#endif //#__BASE_IM_H_
