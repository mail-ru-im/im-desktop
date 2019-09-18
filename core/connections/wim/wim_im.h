#pragma once

#include "../base_im.h"
#include "../../async_task.h"
#include "../../archive/dlg_state.h"
#include "../../archive/history_message.h"
#include "../../archive/opened_dialog.h"
#include "events/fetch_event_diff.h"
#include "../../../corelib/collection_helper.h"
#include "../../tools/event.h"
#include "../../friendly/friendly.h"
#include "../corelib/enumerations.h"

namespace voip_manager{
    struct VoipProtoMsg;
}

CORE_TOOLS_NS_BEGIN

struct url;

typedef std::vector<url> url_vector_t;

CORE_TOOLS_NS_END

namespace core
{
    class async_task;
    class login_info;
    class phone_info;

    enum class typing_status;

    enum class message_read_mode
    {
        read_all,
        read_text
    };

    class masks;

    namespace statistic
    {
        class imstat;
    }

    namespace tools
    {
        class binary_stream;
    }

    namespace archive
    {
        class face;

        class not_sent_message;
        using not_sent_message_sptr = std::shared_ptr<not_sent_message>;

        class history_message;
        using history_message_sptr = std::shared_ptr<history_message>;

        using history_block = std::vector<std::shared_ptr<history_message>>;
        using history_block_sptr = std::shared_ptr<history_block>;

        using headers_list = std::list<message_header>;
        using headers_list_sptr = std::shared_ptr<headers_list>;

        using contact_and_msgs = std::vector<std::pair<std::string, int64_t>>;

        using contact_and_offset = std::tuple<std::string, std::shared_ptr<int64_t>, std::shared_ptr<int64_t>>;
        using contact_and_offsets_v = std::vector<contact_and_offset>;
        using contact_and_offsets_l = std::list<contact_and_offset>;

        struct coded_term;
        struct gallery_state;

        namespace search_dialog
        {
            struct search_dialog_results;
        }
    }

    namespace search
    {
        class search_pattern_history;
        struct search_dialog_results;
        using found_messages = std::unordered_map<std::string, std::vector<archive::history_message_sptr>>;

        using messages = std::vector<std::pair<std::string, archive::history_message_sptr>>;

        struct search_data
        {
            archive::contact_and_offsets_l contact_and_offsets_;
            std::chrono::time_point<std::chrono::system_clock> start_time_;
            std::chrono::time_point<std::chrono::system_clock> last_send_time_;
            int64_t req_id_ = -1;
            int32_t free_threads_count_ = 0;
            int32_t sent_msgs_count_ = 0;
            int32_t not_sent_msgs_count_ = 0;
            messages top_messages_;
            std::map<int64_t, int32_t, std::greater<>> top_messages_ids_;

            std::shared_ptr<search_dialog_results> search_results_;
            int32_t post_results_timer_;

            bool check_req(const int64_t _seq) const noexcept
            {
                return req_id_ != -1 && req_id_ == _seq;
            }

            void reset_post_timer();
        };
    }

    namespace  memory_stats
    {
        class memory_stats_collector;
    }

    namespace wim
    {
        namespace preview_proxy
        {
            class link_meta;
        }

        typedef std::chrono::time_point<std::chrono::system_clock, std::chrono::system_clock::duration> timepoint;

        typedef std::unordered_set<std::string> ignorelist_cache;

        using postponed_for_update_dialogs = std::unordered_set<std::string>;
        using chat_heads_sptr = std::shared_ptr<struct chat_heads>;

        class fetch_event_buddy_list;
        class fetch_event_presence;
        class fetch_event_dlg_state;
        class fetch_event_hidden_chat;
        class fetch_event_diff;
        class fetch_event_my_info;
        class fetch_event_user_added_to_buddy_list;
        class fetch_event_typing;
        class fetch_event_permit;
        class fetch_event_imstate;
        class fetch_event_notification;
        class fetch_event_appsdata;
        class fetch_event_mention_me;
        class fetch_event_chat_heads;
        class fetch_event_gallery_notify;
        class fetch_event_mchat;

        struct auth_parameters;
        struct fetch_parameters;
        struct wim_packet_params;
        struct robusto_packet_params;
        class contactlist;
        class avatar_loader;
        class wim_packet;
        class robusto_packet;
        struct get_history_params;
        struct get_history_batch_params;
        struct set_dlg_state_params;
        class active_dialogs;
        class favorites;
        class my_info_cache;
        class loader;
        class async_loader;
        class send_message;
        class fetch;
        class chat_params;
        class mailbox_storage;
        class search_dialogs;
        class check_nick;
        class get_chat_members;
        class search_chat_members;

        namespace holes
        {
            class request;
            class failed_requests;
        }


        class send_message_handler
        {
        public:

            std::function<void(int32_t _error, const send_message& _packet)> on_result;
        };

        struct dlg_state_cache_item
        {
            std::string aimid_;
            core::icollection* dlg_state_coll_;
            dlg_state_cache_item(const std::string& _aimid, core::icollection* _dlg_state)
                : aimid_(_aimid), dlg_state_coll_(_dlg_state)
            {
                _dlg_state->addref();
            }
        };

        enum class force_save
        {
            no,
            yes
        };

        enum class is_ping
        {
            no = 0,
            yes
        };

        enum class is_login
        {
            no = 0,
            yes
        };

        enum class is_updated_messages
        {
            no = 0,
            yes
        };

        enum class connection_state
        {
            connecting = 0,
            updating = 1,
            online = 2,
            online_from_fetch = 3,
            init = 4
        };

        //////////////////////////////////////////////////////////////////////////
        // wim_send_thread
        //////////////////////////////////////////////////////////////////////////
        class wim_send_thread : public async_executer, public std::enable_shared_from_this<wim_send_thread>
        {
            struct task_and_params
            {
                std::shared_ptr<wim_packet> task_;
                std::function<void(int32_t)> error_handler_;
                std::shared_ptr<async_task_handlers> callback_handlers_;

                task_and_params(
                    std::shared_ptr<wim_packet> _task,
                    std::function<void(int32_t)> _error_handler,
                    std::shared_ptr<async_task_handlers> _callback_handlers)
                    :
                task_(_task),
                    error_handler_(_error_handler),
                    callback_handlers_(_callback_handlers) {}
            };

            bool is_packet_execute_;
            std::list<task_and_params> packets_queue_;

            std::chrono::system_clock::time_point cancel_packets_time_;

            void execute_packet_from_queue();

            std::shared_ptr<async_task_handlers> post_packet(
                std::shared_ptr<wim_packet> _packet,
                std::function<void(int32_t)> _error_handler,
                std::shared_ptr<async_task_handlers> _handlers);

        public:

            std::shared_ptr<async_task_handlers> post_packet(std::shared_ptr<wim_packet> _packet, std::function<void(int32_t)>&& _error_handler);
            void clear();

            wim_send_thread();
            virtual ~wim_send_thread();
        };



        //////////////////////////////////////////////////////////////////////////
        // fetch_thread
        //////////////////////////////////////////////////////////////////////////
        class fetch_thread : public async_executer
        {
        public:
            fetch_thread() : async_executer("fetch") {}
        };

        class gui_message
        {
            const std::string message_;
            const int64_t seq_;
            const coll_helper data_;

        public:
            gui_message(
                const std::string& _message,
                const int64_t _seq,
                const coll_helper& _data)
                :
                message_(_message)
                , seq_(_seq)
                , data_(_data)
            {
            }

            const std::string& get_message() const { return message_; }
            int64_t get_seq() const { return seq_; }
            const coll_helper& get_data() const { return data_; }
        };

        typedef std::list<gui_message> gui_messages_list;

        //////////////////////////////////////////////////////////////////////////
        // class im
        //////////////////////////////////////////////////////////////////////////
        class im : public base_im , public std::enable_shared_from_this<im>
        {
            friend class core::masks;

            enum class poll_reason
            {
                normal = 0,
                after_network_error = 1,
                after_first_start = 2,
                resend = 3,
            };

            // stop signal
            struct stop_objects
            {
                std::mutex session_mutex_;
                uint64_t active_session_id_ = 0;

                tools::autoreset_event poll_event_;
                tools::autoreset_event startsession_event_;
            };

            std::shared_ptr<stop_objects> stop_objects_;
            std::shared_ptr<wim::contactlist> contact_list_;
            std::shared_ptr<wim::active_dialogs> active_dialogs_;
            std::shared_ptr<wim::my_info_cache> my_info_cache_;
            std::shared_ptr<wim::favorites> favorites_;
            std::shared_ptr<wim::mailbox_storage> mailbox_storage_;

            // authorization parameters
            std::shared_ptr<auth_parameters> auth_params_;
            std::shared_ptr<auth_parameters> attached_auth_params_;

            // wim fetch parameters
            std::shared_ptr<fetch_parameters> fetch_params_;

            // temporary for phone registration
            std::shared_ptr<phone_info> phone_registration_data_;
            std::shared_ptr<phone_info> attached_phone_registration_data_;

            // threads
            std::shared_ptr<wim_send_thread> wim_send_thread_;
            std::shared_ptr<fetch_thread> fetch_thread_;
            std::shared_ptr<async_executer> async_tasks_;

            // files loader/uploader
            std::shared_ptr<loader> files_loader_;
            std::shared_ptr<async_loader> async_loader_;

            // avatar loader
            std::shared_ptr<avatar_loader> avatar_loader_;

            // timers
            uint32_t store_timer_id_;
            uint32_t statistic_timer_id_;
            uint32_t hosts_config_timer_id_;
            uint32_t dlg_state_timer_;
            int32_t stat_timer_id_;
            uint32_t ui_activity_timer_id_;

            bool im_created_;

            // archive
            std::shared_ptr<archive::face> archive_;
            std::shared_ptr<archive::face> get_archive();

            // opened dialog, posted from gui
            std::map<std::string, archive::opened_dialog> opened_dialogs_;

            std::shared_ptr<holes::failed_requests> failed_holes_requests_;

            std::set<std::string> failed_dlg_states_;

            std::set<std::string> failed_set_attention_;

            std::set<std::string> failed_update_messages_;

            std::vector<std::pair<int32_t, int32_t>> failed_sticker_migration_;

            bool sent_pending_messages_active_;
            bool sent_pending_delete_messages_active_;

            // statistic
            std::unique_ptr<statistic::imstat> imstat_;

            // search
            std::unique_ptr<async_executer> history_searcher_;
            search::search_data search_data_;

            std::shared_ptr<search::search_pattern_history> search_history_;
            std::shared_ptr<search::search_pattern_history> get_search_history();

            // need for start session
            timepoint start_session_time_;

            int64_t prefetch_uid_;

            // post messages timer
            int32_t post_messages_timer_;

            int32_t mute_chats_timer_;

            std::chrono::system_clock::time_point last_success_network_post_;
            std::chrono::system_clock::time_point last_check_alt_scheme_reset_;

            // dlg_state cache
            bool dlg_state_agregate_mode_;
            std::chrono::system_clock::time_point agregate_start_time_;
            std::chrono::system_clock::time_point last_network_activity_time_;
            std::list<dlg_state_cache_item> cached_dlg_states_;
            // syncronized with dlg_state messages
            gui_messages_list gui_messages_queue_;

            friendly_container friendly_container_;
            postponed_for_update_dialogs postponed_for_update_dialogs_;

            std::map<std::string, chat_heads_sptr> chat_heads_;
            int32_t chat_heads_timer_;

            bool ui_active_;

            int dlg_states_count_;

            connection_state active_connection_state_;

            std::unique_ptr<accept_agreement_info> resend_gdpr_info_;
            std::atomic<bool> suppress_gdpr_resend_;

            std::chrono::system_clock::time_point last_send_stat_time_;

            std::chrono::system_clock::time_point last_ui_activity_;

            bool waiting_for_local_pin_;
            std::string local_pin_hash_;

            std::shared_ptr<async_task_handlers> post_wim_packet(std::shared_ptr<wim_packet> _packet);
            std::shared_ptr<async_task_handlers> post_robusto_packet(std::shared_ptr<robusto_packet> packet);

            void store_auth_parameters();
            void load_auth_and_fetch_parameters();
            void store_fetch_parameters();
            std::wstring get_auth_parameters_filename();
            std::wstring get_auth_parameters_filename_exported();
            std::wstring get_auth_parameters_filename_merge();
            std::wstring get_fetch_parameters_filename();
            std::wstring get_exported_muted_chats_filename();

            void add_postponed_for_update_dialog(const std::string& _contact);
            void remove_postponed_for_update_dialog(const std::string& _contact);
            void process_postponed_for_update_dialog(const std::string& _contact);

            void save_my_info();
            void save_contact_list(force_save _force);
            void save_active_dialogs();
            void save_favorites();
            void save_mailboxes();
            void save_search_history();

            std::shared_ptr<async_task_handlers> load_active_dialogs();
            std::shared_ptr<async_task_handlers> load_contact_list();
            std::shared_ptr<async_task_handlers> load_my_info();
            std::shared_ptr<async_task_handlers> load_favorites();
            std::shared_ptr<async_task_handlers> load_mailboxes();

            // stickers
            void load_stickers_data(int64_t _seq, const std::string& _size, const bool _up_to_date);
            void download_stickers_metadata(int64_t _seq, const std::string& _size, const bool _up_to_date);
            void download_stickers_metafile(int64_t _seq, const std::string& _size, const std::string& _md5);


            void download_stickers(const bool _recursive = false);
            void load_stickers_suggests();
            void post_stickers_suggests_2_gui();


            void get_stickers_meta(int64_t _seq, const std::string& _size) override;
            void get_sticker(const int64_t _seq, const int32_t _set_id, const int32_t _sticker_id, std::string_view _fs_id, const core::sticker_size _size) override;
            void get_stickers_pack_info(const int64_t _seq, const int32_t _set_id, const std::string& _store_id, const std::string& _file_id) override;
            void add_stickers_pack(const int64_t _seq, const int32_t _set_id, std::string _store_id) override;
            void remove_stickers_pack(const int64_t _seq, const int32_t _set_id, std::string _store_id) override;
            void get_set_icon_big(const int64_t _seq, const int32_t _set_id) override;
            void clean_set_icon_big(const int64_t _seq, const int32_t _set_id) override;
            void post_stickers_meta_to_gui(int64_t _seq, const std::string& _size);
            void post_stickers_store_to_gui(int64_t _seq);
            void post_stickers_search_result_to_gui(int64_t _seq);
            void reload_stickers_meta(const int64_t _seq, const std::string& _size);
            void get_stickers_store(const int64_t _seq) override;
            void search_stickers_store(const int64_t _seq, const std::string &_search_term) override;
            void set_sticker_order(const int64_t _seq, const std::vector<int32_t>& _values) override;
            void get_fs_stickers_by_ids(const int64_t _seq, std::vector<std::pair<int32_t, int32_t>> _values) override;

            void get_chat_home(int64_t _seq, const std::string& _tag) override;
            void get_chat_info(int64_t _seq, const std::string& _aimid, const std::string& _stamp, int32_t _limit) override;
            void resolve_pending(int64_t _seq, const std::string& _aimid, const std::vector<std::string>& _contact, bool _approve) override;
            void get_chat_member_info(int64_t _seq, const std::string& _aimid, const std::vector<std::string>& _members) override;
            void get_chat_members_page(int64_t _seq, std::string_view _aimid, std::string_view _role, uint32_t _page_size) override;
            void get_chat_members_next_page(int64_t _seq, std::string_view _aimid, std::string_view _cursor, uint32_t _page_size) override;
            void on_get_chat_members_result(int64_t _seq, const std::shared_ptr<get_chat_members>& _packet, uint32_t _page_size, int32_t _error);
            void search_chat_members_page(int64_t _seq, std::string_view _aimid, std::string_view _role, std::string_view _keyword, uint32_t _page_size) override;
            void search_chat_members_next_page(int64_t _seq, std::string_view _aimid, std::string_view _cursor, uint32_t _page_size) override;
            void on_search_chat_members_result(int64_t _seq, const std::shared_ptr<search_chat_members>& _packet, uint32_t _page_size, int32_t _error);
            void get_chat_contacts(int64_t _seq, const std::string& _aimid, const std::string& _cursor, uint32_t _page_size) override;

            void create_chat(int64_t _seq, const std::string& _aimid, const std::string& _name, std::vector<std::string> _members, core::wim::chat_params _params) override;

            void mod_chat_params(int64_t _seq, const std::string& _aimid, core::wim::chat_params _params) override;
            void mod_chat_name(int64_t _seq, const std::string& _aimid, const std::string& _name) override;
            void mod_chat_about(int64_t _seq, const std::string& _aimid, const std::string& _about) override;
            void mod_chat_rules(int64_t _seq, const std::string& _aimid, const std::string& _rules) override;
            void mod_chat_public(int64_t _seq, const std::string& _aimid, bool _public) override;
            void mod_chat_join(int64_t _seq, const std::string& _aimid, bool _approved) override;
            void mod_chat_link(int64_t _seq, const std::string& _aimid, bool _link) override;
            void mod_chat_ro(int64_t _seq, const std::string& _aimid, bool _ro) override;

            void set_attention_attribute(int64_t _seq, std::string _aimid, bool _value, const bool _resume) override;

            void block_chat_member(int64_t _seq, const std::string& _aimid, const std::string& _contact, bool _block) override;
            void set_chat_member_role(int64_t _seq, const std::string& _aimid, const std::string& _contact, const std::string& _role) override;
            void pin_chat_message(int64_t _seq, const std::string& _aimid, int64_t _message, bool _is_unpin) override;

            void get_mentions_suggests(int64_t _seq, const std::string& _aimid, const std::string& _pattern) override;

            void check_themes_meta_updates(int64_t _seq) override;
            void download_wallpapers_recursively();
            void get_theme_wallpaper(const std::string_view _wp_id) override;
            void get_theme_wallpaper_preview(const std::string_view _wp_id) override;
            void set_user_wallpaper(const std::string_view _wp_id, tools::binary_stream _image) override;
            void remove_user_wallpaper(const std::string_view _wp_id) override;

            void load_cached_objects();
            void save_cached_objects(force_save _force = force_save::no);

            void post_my_info_to_gui();
            void post_contact_list_to_gui();
            void post_ignorelist_to_gui(int64_t _seq);
            void post_active_dialogs_to_gui();
            void post_active_dialogs_are_empty_to_gui();

            virtual std::string get_login() override;
            virtual void logout(std::function<void()> _on_result) override;

            void login_by_password(
                int64_t _seq,
                const std::string& login,
                const std::string& password,
                const bool save_auth_data,
                const bool start_session,
                const bool _from_export_login,
                const token_type _token_type = token_type::basic);

            void login_by_agent_token(
                const std::string& _login,
                const std::string& _token,
                const std::string& _guid);

            void login_by_password_and_attach_uin(
                int64_t _seq,
                const std::string& login,
                const std::string& password,
                const wim_packet_params& _from_params);

            void start_session(is_ping _is_ping = is_ping::no, is_login _is_login = is_login::no) override;

            void phoneinfo(int64_t seq, const std::string &phone, const std::string &gui_locale) override;

            void cancel_requests();
            bool is_session_valid(uint64_t _session_id);
            void poll(const bool _is_first, const poll_reason _reason, const int32_t _failed_network_error_count = 0);

            void dispatch_events(std::shared_ptr<fetch> _fetch_packet, std::function<void(int32_t)> _on_complete);

            void schedule_store_timer();
            void stop_store_timer();

            void stop_waiters();

            loader& get_loader();
            async_loader& get_async_loader();

            // statistic
            void schedule_statistic_timer();
            void stop_statistic_timer();
            void send_statistic_if_needed();

            void schedule_ui_activity_timer();
            void stop_ui_activity_timer();

            void on_im_created();

            virtual std::wstring get_im_path() const override;

            std::shared_ptr<avatar_loader> get_avatar_loader();

            void get_contact_avatar(int64_t _seq, const std::string& _contact, int32_t _avatar_size, bool _force) override;
            void show_contact_avatar(int64_t _seq, const std::string& _contact, int32_t _avatar_size) override;
            void remove_contact_avatars(int64_t _seq, const std::string& _contact) override;

            void send_message_to_contact(
                const int64_t _seq,
                const std::string& _contact,
                core::archive::message_pack _pack) override;

            void send_message_to_contacts(
                const int64_t _seq,
                const std::vector<std::string>& _contacts,
                core::archive::message_pack _pack) override;

            void update_message_to_contact(
                const int64_t _seq,
                const int64_t _id,
                const std::string& _contact,
                core::archive::message_pack _pack) override;

            void send_message_typing(const int64_t _seq, const std::string& _contact, const core::typing_status& _status, const std::string& _id) override;

            void send_feedback(const int64_t, const std::string &url, const std::map<std::string, std::string> &fields, const std::vector<std::string> &files) override;

            void set_state(const int64_t _seq, const core::profile_state _state) override;

            void post_pending_messages(const bool _recurcive = false);
            void post_pending_delete_messages(const bool _recurcive = false);

            std::shared_ptr<async_task_handlers> send_pending_message_async(
                const int64_t _seq,
                const archive::not_sent_message_sptr& _message);

            std::shared_ptr<send_message_handler> send_message_async(const int64_t _seq,
                const int64_t _updated_id,
                const std::string& _contact,
                const std::string& _message,
                const std::string& _internal_id,
                const core::message_type _type,
                const core::archive::quotes_vec& _quotes,
                const core::archive::mentions_map& _mentions,
                const std::string& _description,
                const std::string& _url, const archive::shared_contact& _shared_contact);

            void post_not_sent_message_to_gui(int64_t _seq, const archive::not_sent_message_sptr& _message);

            void connect() override;

            // login functions
            void login(int64_t _seq, const login_info& _info) override;
            void login_get_sms_code(int64_t seq, const phone_info& _info, bool _is_login) override;
            void login_by_phone(int64_t _seq, const phone_info& _info) override;

            void start_attach_phone(int64_t _seq, const phone_info& _info) override;
            void start_attach_uin(int64_t _seq, const login_info& _info, const wim_packet_params& _from_params) override;

            void erase_auth_data() override;

            void sign_url(int64_t _seq, const std::string& unsigned_url) override;

            // history functions

            void get_archive_messages(int64_t _seq, const std::string& _contact, int64_t _from, int64_t _count_early, int64_t _count_later, int32_t _recursion, bool _need_prefetch, bool _first_request, bool _after_search);
            void get_archive_messages(int64_t _seq, const std::string& _contact, int64_t _from, int64_t _count_early, int64_t _count_later, bool _need_prefetch, bool _first_request, bool _after_search) override;
            std::shared_ptr<async_task_handlers> get_archive_messages_get_messages(
                int64_t _seq,
                const std::string& _contact,
                int64_t _from,
                int64_t _count_early,
                int64_t _count_later,
                int32_t _recursion,
                bool _need_prefetch,
                bool _first_request,
                bool _after_search);

            void get_archive_messages_buddies(int64_t _seq, const std::string& _contact, std::shared_ptr<archive::msgids_list> _ids, is_updated_messages _is_updated_messages) override;
            void get_messages_for_update(const std::string& _contact) override;

            void get_mentions(int64_t _seq, std::string_view _contact) override;

            std::shared_ptr<async_task_handlers> get_history_from_server(const get_history_params& _params);
            std::shared_ptr<async_task_handlers> get_history_from_server(const std::string_view _contact, const std::string_view _patch_version, std::vector<int64_t> _ids);
            std::shared_ptr<async_task_handlers> get_history_from_server(const std::string_view _contact, const std::string_view _patch_version, const int64_t _id, const int32_t _context_early, const int32_t _context_after, int64_t _seq);
            std::shared_ptr<async_task_handlers> get_history_from_server_batch(get_history_batch_params _params);

            void get_message_context(const int64_t _seq, const std::string& _contact, const int64_t _msgid) override;

            std::shared_ptr<async_task_handlers> set_dlg_state(set_dlg_state_params _params);
            void add_opened_dialog(const std::string& _contact) override;
            void remove_opened_dialog(const std::string& _contact) override;
            bool has_opened_dialogs(const std::string& _contact) const;
            void set_last_read(const std::string& _contact, int64_t _message, message_read_mode _mode) override;
            void set_last_read_mention(const std::string& _contact, int64_t _message) override;
            void set_last_read_partial(const std::string& _contact, int64_t _message) override;
            void hide_dlg_state(const std::string& _contact) override;

            void clear_last_message_in_dlg_state(const std::string& _contact);

            void delete_archive_message(const int64_t _seq, const std::string &_contact_aimid, const int64_t _message_id, const std::string& _internal_id, const bool _for_all) override;
            void delete_archive_message_local(const int64_t _seq, const std::string &_contact_aimid, const int64_t _message_id, const std::string& _internal_id, const bool _for_all);
            void delete_archive_messages_from(const int64_t _seq, const std::string &_contact_aimid, const int64_t _from_id) override;
            void delete_archive_all_messages(const int64_t _seq, std::string_view _contact_aimid) override;

            void search_dialogs_local(const std::string& search_patterns, const std::vector<std::string>& _aimids) override;
            void search_contacts_local(const std::vector<std::vector<std::string>>& _search_patterns, int64_t _seq, unsigned _fixed_patterns_count, const std::string& _pattern) override;
            void setup_search_dialogs_params(int64_t _req_id) override;
            void clear_search_dialogs_params() override;

            void add_search_pattern_to_history(const std::string_view _search_pattern, const std::string_view _contact) override;
            void remove_search_pattern_from_history(const std::string_view _search_pattern, const std::string_view _contact) override;
            void load_search_patterns(const std::string_view _contact);

            void download_failed_holes();
            void download_holes(const std::string& _contact);
            void download_holes(const std::string& _contact, int64_t _depth);
            void download_holes(const std::string& _contact, int64_t _from, int64_t _depth, int32_t _recursion = 0);

            std::string _get_protocol_uid() override;

            void update_active_dialogs(const std::string& _aimid, archive::dlg_state& _state);
            void hide_chat_async(const std::string& _contact, const int64_t _last_msg_id, std::function<void(int32_t)> _on_result = std::function<void(int32_t)>());
            void hide_chat(const std::string& _contact) override;
            void mute_chat(const std::string& _contact, bool _mute) override;
            std::shared_ptr<async_task_handlers> mute_chat_async(const std::string& _contact, bool _mute);
            void mute_chats(std::shared_ptr<std::list<std::string>> _chats);
            void add_contact(int64_t _seq, const std::string& _aimid, const std::string& _group, const std::string& _auth_message) override;
            void remove_contact(int64_t _seq, const std::string& _aimid) override;
            void rename_contact(int64_t _seq, const std::string& _aimid, const std::string& _friendly) override;
            void ignore_contact(int64_t _seq, const std::string& _aimid, bool ignore) override;
            void get_ignore_list(int64_t _seq) override;
            void favorite(const std::string& _contact) override;
            void unfavorite(const std::string& _contact) override;
            void update_outgoing_msg_count(const std::string& _aimid, int _count) override;

            std::shared_ptr<async_task_handlers> post_dlg_state_to_gui(
                const std::string& _contact,
                const bool _add_to_active_dialogs = false,
                const bool _serialize_message = true,
                const bool _force = false /*need for favorites*/,
                const bool _load_from_local = false);

            void post_dlg_state_to_gui(const std::string& _aimid, core::icollection* _dlg_state);
            void post_gallery_state_to_gui(const std::string& _aimid, const archive::gallery_state& _gallery_state);
            void sync_message_with_dlg_state_queue(std::string_view _message, const int64_t _seq, coll_helper _data);

            void post_queued_messages_to_gui();
            void post_queued_objects_to_gui();

            void post_cached_dlg_states_to_gui();
            void remove_from_cached_dlg_states(const std::string& _aimid);
            void stop_dlg_state_timer();
            void check_need_agregate_dlg_state();

            void post_history_search_result_msg_to_gui(const std::string_view _contact, const archive::history_message_sptr _msg, const std::string_view _term);
            void post_history_search_result_empty();

            // ------------------------------------------------------------------------------
            // files functions
            void upload_file_sharing(
                const int64_t _seq,
                const std::string& _contact,
                const std::string& _file_name,
                std::shared_ptr<core::tools::binary_stream> _data,
                const std::string& _extension,
                const core::archive::quotes_vec& _quotes,
                const std::string& _description,
                const core::archive::mentions_map& _mentions,
                const std::optional<int64_t>& _duration) override;

            void get_file_sharing_preview_size(
                const int64_t _seq,
                const std::string& _file_url,
                const int32_t _original_size) override;

            void download_file_sharing_metainfo(
                const int64_t _seq,
                const std::string& _file_url) override;

            void download_file_sharing(
                const int64_t _seq,
                const std::string& _contact,
                const std::string& _file_url,
                const bool _force_request_metainfo,
                const std::string& _filename,
                const std::string& _download_dir,
                bool _raise_priority) override;

            void download_image(const int64_t _seq,
                const std::string& _image_url,
                const std::string& _forced_path,
                const bool _download_preview,
                const int32_t _preview_width,
                const int32_t _preview_height,
                const bool _ext_resource,
                const bool _raise_priority,
                const bool _with_data) override;

            void download_link_preview(
                const int64_t _seq,
                const std::string& _contact_aimid,
                const std::string& _url,
                const int32_t _preview_width,
                const int32_t _preview_height) override;

            void cancel_loader_task(const std::string& _url) override;

            void abort_file_sharing_download(const std::string& _url) override;

            void abort_file_sharing_upload(
                const int64_t _seq,
                const std::string & _contact,
                const std::string &_process_seq) override;

            void raise_download_priority(const int64_t _task_id) override;

            void contact_switched(const std::string &_contact_aimid) override;

            void download_link_preview_image(
                const int64_t _seq,
                const std::string& _contact_aimid,
                const preview_proxy::link_meta &_meta,
                const int32_t _preview_width,
                const int32_t _preview_height);

            void download_link_preview_favicon(
                const int64_t _seq,
                const std::string& _contact_aimid,
                const preview_proxy::link_meta &_meta);

            void set_played(const std::string& url, bool played) override;
            void speech_to_text(int64_t _seq, const std::string& _url, const std::string& _locale) override;

            void upload_file_sharing_internal(const archive::not_sent_message_sptr& _not_sent);

            void resume_failed_network_requests();
            void resume_file_sharing_uploading(const archive::not_sent_message_sptr &_not_sent);
            void resume_all_file_sharing_uploading();
            void resume_stickers_migration();
            void resume_download_stickers();
            void resume_download_masks();
            void resume_failed_avatars();
            void resume_failed_user_agreements();
            void send_user_agreement(const accept_agreement_info& _info, int64_t _seq = 0);
            void resume_failed_dlg_states();
            void resume_failed_set_attention();
            void resume_failed_update_messages();
            // ------------------------------------------------------------------------------

            // group chat
            void remove_members(int64_t _seq, const std::string& _aimid, const std::string& _m_chat_members_to_remove) override;
            void add_members(int64_t _seq, const std::string& _aimid, const std::string& _m_chat_members_to_add) override;
            void add_chat(int64_t _seq, const std::string& _m_chat_name, std::vector<std::string> _m_chat_members) override;
            void modify_chat(int64_t _seq, const std::string& _aimid, const std::string& _m_chat_name) override;

            // mrim
            void get_mrim_key(int64_t _seq, const std::string& _email) override;

            static void on_created_groupchat(std::shared_ptr<diffs_map> _diff);

            void on_history_patch_version_changed(const std::string& _aimid, const std::string& _history_patch_version);
            void on_del_up_to_changed(const std::string& _aimid, const int64_t _del_up_to, const auto_callback_sptr& _on_complete);

            void merge_account(const auth_parameters& _merge_params);
            void merge_account();
            void attach_uin(int64_t _seq);
            void attach_phone(int64_t _seq, const auth_parameters& auth_params, const phone_info& _info);

            void apply_exported_settings();
            void apply_exported_muted_chats();
            void apply_exported_muted_chats_internal();

            void attach_uin_finished();
            void attach_phone_finished();

            void update_profile(int64_t _seq, const std::vector<std::pair<std::string, std::string>>& _field) override;

            // alpha chats
            void join_live_chat(int64_t _seq, const std::string& _stamp) override;

            std::shared_ptr<async_task_handlers> send_timezone();

            void update_friendly(const friendly_container::friendly_updates& _updates);

            // prefetching

            void history_search_one_batch(std::shared_ptr<archive::coded_term> _cterm, std::shared_ptr<archive::contact_and_msgs> _archive
                , std::shared_ptr<tools::binary_stream> _data, int64_t _seq
                , int64_t _min_id);

            void post_unignored_contact_to_gui(const std::string& _aimid);


            void post_presence_to_gui(const std::string& _aimid);
            void post_outgoing_count_to_gui(const std::string& _aimid);
            void post_connection_state_to_gui(const std::string_view _state);
            void set_connection_state(const connection_state _state);

            void send_stat_if_needed();
            void download_gallery_holes(const std::string& _aimid);

            void check_ui_activity();

        public:

            void post_voip_msg_to_server(const voip_manager::VoipProtoMsg& msg) override;
            void post_voip_alloc_to_server(const std::string& data) override;

            std::shared_ptr<wim::wim_packet> prepare_voip_msg(const std::string& data) override;
            std::shared_ptr<wim::wim_packet> prepare_voip_pac(const voip_manager::VoipProtoMsg& data) override;

            void login_normalize_phone(int64_t _seq, const std::string& _country, const std::string& _raw_phone, const std::string& _locale, bool _is_login) override;
            std::string get_contact_friendly_name(const std::string& contact_login) override;

            // events
            void on_login_result(int64_t _seq, int32_t err, bool _from_exported_data);
            void on_login_result_attach_uin(int64_t _seq, int32_t err, const auth_parameters& auth_params, const wim_packet_params& _from_params);

            void handle_net_error(int32_t err) override;

            void on_event_buddies_list(fetch_event_buddy_list* _event, std::shared_ptr<auto_callback> _on_complete);
            void on_event_presence(fetch_event_presence* _event, std::shared_ptr<auto_callback> _on_complete);

            void on_event_dlg_state(fetch_event_dlg_state* _event, auto_callback_sptr _on_complete);
            void on_event_dlg_state_local_state_loaded(
                const auto_callback_sptr& _on_complete,
                const std::string& _aimid,
                const archive::dlg_state& _local_dlg_state,
                archive::dlg_state _server_dlg_state,
                const archive::history_block_sptr& _tail_messages,
                const archive::history_block_sptr& _intro_messages
                );
            void on_event_dlg_state_process_messages(
                const archive::history_block& _tail_messages,
                const archive::history_block& _intro_messages,
                const std::string& _aimid,
                InOut archive::dlg_state& _server_dlg_state
                );
            void on_event_dlg_state_local_state_updated(
                const auto_callback_sptr& _on_complete,
                const archive::history_block_sptr& _tail_messages,
                const archive::history_block_sptr& _intro_messages,
                const std::string& _aimid,
                const archive::dlg_state& _local_dlg_state,
                const bool _patch_version_changed,
                const bool _del_up_to_changed,
                const bool _last_msg_id_changed
                );
            void on_event_dlg_state_history_updated(
                const auto_callback_sptr& _on_complete,
                const bool _patch_version_changed,
                const archive::dlg_state& _local_dlg_state,
                const std::string& _aimid,
                const int64_t _last_message_id,
                const int32_t _count,
                const bool _del_up_to_changed,
                const bool _last_msg_id_changed
                );

            void on_event_hidden_chat(fetch_event_hidden_chat* _event, std::shared_ptr<auto_callback> _on_complete);
            void on_event_diff(fetch_event_diff* _event, std::shared_ptr<auto_callback> _on_complete);
            void on_event_my_info(fetch_event_my_info* _event, std::shared_ptr<auto_callback> _on_complete);
            void on_event_user_added_to_buddy_list(fetch_event_user_added_to_buddy_list* _event, std::shared_ptr<auto_callback> _on_complete);
            void on_event_typing(fetch_event_typing* _event, std::shared_ptr<auto_callback> _on_complete);
            void on_event_permit(fetch_event_permit* _event, std::shared_ptr<auto_callback> _on_complete);
            void on_event_imstate(fetch_event_imstate* _event, std::shared_ptr<auto_callback> _on_complete);
            void on_event_notification(fetch_event_notification* _event, std::shared_ptr<auto_callback> _on_complete);
            void on_event_appsdata(fetch_event_appsdata* _event, std::shared_ptr<auto_callback> _on_complete);
            void on_event_mention_me(fetch_event_mention_me* _event, std::shared_ptr<auto_callback> _on_complete);
            void on_event_chat_heads(fetch_event_chat_heads* _event, std::shared_ptr<auto_callback> _on_complete);
            void on_event_gallery_notify(fetch_event_gallery_notify* _event, std::shared_ptr<auto_callback> _on_complete);
            void on_event_mchat(fetch_event_mchat* _event, std::shared_ptr<auto_callback> _on_complete);

            void insert_friendly(const std::string& _uin, const std::string& _friendly_name, const std::string& _nick, bool _official, friendly_source _type, friendly_add_mode _mode = friendly_add_mode::insert_or_update);
            void insert_friendly(const std::string& _uin, const std::string& _friendly_name, const std::string& _nick, std::optional<bool> _official, friendly_source _type, friendly_add_mode _mode = friendly_add_mode::insert_or_update);
            void insert_friendly(const core::archive::persons_map& _map, friendly_source _type, friendly_add_mode _mode = friendly_add_mode::insert_or_update);
            void insert_friendly(const std::shared_ptr<core::archive::persons_map>& _map, friendly_source _type, friendly_add_mode _mode = friendly_add_mode::insert_or_update);

            void search_contacts_server(const int64_t _seq, const std::string_view _keyword, const std::string_view _phone) override;

            void syncronize_address_book(const int64_t _seq, const std::string _keyword, const std::vector<std::string> _phone) override;

            void search_dialogs_one(const int64_t _seq, const std::string_view _keyword, const std::string_view _contact) override;
            void search_dialogs_all(const int64_t _seq, const std::string_view _keyword) override;
            void search_dialogs_continue(const int64_t _seq, const std::string_view _cursor, const std::string_view _contact) override;
            void on_search_dialogs_result(const int64_t _seq, const std::shared_ptr<search_dialogs>& _packet, const int32_t _error);

            void get_profile(int64_t _seq, const std::string& _contact) override;

            void set_resend_gdpr_info(std::unique_ptr<accept_agreement_info> _info);

            virtual wim_packet_params make_wim_params() override;
            virtual wim_packet_params make_wim_params_general(bool _is_current_auth_params) override;
            wim_packet_params make_wim_params_from_auth(const auth_parameters& _auth_params);

            im(const im_login_id& _login, std::shared_ptr<voip_manager::VoipManager> _voip_manager, std::shared_ptr<memory_stats::memory_stats_collector> _memory_stats_collector);
            virtual ~im();

            void init_failed_requests();

            void set_avatar(const int64_t _seq, tools::binary_stream image, const std::string& _aimId, const bool _chat) override;

            void save_auth_params_to_file(std::shared_ptr<auth_parameters> auth_params, const std::wstring& _file_name, std::function<void()> _on_result) override;
            void save_auth_to_export(std::function<void()> _on_result) override;

            void get_mask_id_list(int64_t _seq) override;
            void get_mask_preview(int64_t _seq, const std::string& mask_id) override;
            void get_mask_model(int64_t _seq) override;
            void get_mask(int64_t _seq, const std::string& mask_id) override;
            void get_existent_masks(int64_t _seq) override;

            bool has_valid_login() const override;
            void merge_exported_account(core::wim::auth_parameters& _params) override;

            void get_code_by_phone_call(const std::string& _ivr_url) override;

            void get_attach_phone_info(const int64_t _seq, const std::string& _locale, const std::string& _lang) override;
            void get_voip_calls_quality_popup_conf(const int64_t _seq, const std::string& _locale, const std::string& _lang) override;

            void get_logs_path(const int64_t _seq) override;
                        void remove_content_cache(const int64_t _seq) override;
            void clear_avatars(const int64_t _seq) override;

            void report_contact(int64_t _seq, const std::string& _aimid, const std::string& _reason, const bool _ignore_and_close) override;
            void report_stickerpack(const int32_t _id, const std::string& _reason) override;
            void report_sticker(const std::string& _id, const std::string& _reason, const std::string& _aimid, const std::string& _chatId) override;
            void report_message(const int64_t _id, const std::string& _text, const std::string& _reason, const std::string& _aimid, const std::string& _chatId) override;

            void user_accept_gdpr(int64_t _seq, const accept_agreement_info& _accept_info) override;

            void send_stat() override;

            void schedule_stat_timer(bool _first_send);

            void stop_stat_timer();

            void get_dialog_gallery(const int64_t _seq,
                                    const std::string& _aimid,
                                    const std::vector<std::string>& _type,
                                    const archive::gallery_entry_id _after,
                                    const int _page_size) override;

            void get_dialog_gallery_by_msg(const int64_t _seq, const std::string& _aimid, const std::vector<std::string>& _type, int64_t _msg_id) override;

            void request_gallery_state(const std::string& _aimId) override;
            void get_gallery_index(const std::string& _aimId, const std::vector<std::string>& _type, int64_t _msg, int64_t _seq) override;
            void make_gallery_hole(const std::string& _aimid, int64_t _from, int64_t _till) override;

            void request_memory_usage(const int64_t _seq) override;
            void report_memory_usage(const int64_t _seq,
                                             core::memory_stats::request_id _req_id,
                                             const core::memory_stats::partial_response& _partial_response) override;
            void get_ram_usage(const int64_t _seq) override;

            void make_archive_holes(const int64_t _seq, const std::string& _archive) override;
            void invalidate_archive_data(const int64_t _seq, const std::string& _archive, std::vector<int64_t> _ids) override;
            void invalidate_archive_data(const int64_t _seq, const std::string& _archive, int64_t _from, int64_t _before_count, int64_t _after_count) override;

            void serialize_heads(const chat_heads_sptr& _heads);
            void on_ui_activity(const int64_t _time) override;

            void close_stranger(const std::string& _contact) override;

            void on_local_pin_set(const std::string& _password) override;
            void on_local_pin_entered(const int64_t _seq, const std::string& _password) override;
            void on_local_pin_disable(const int64_t _seq) override;

            void get_id_info(const int64_t _seq, const std::string_view _id) override;
            void get_user_info(const int64_t _seq, const std::string& _aimid) override;

            void set_privacy_settings(const int64_t _seq, privacy_settings _settings) override;
            void get_privacy_settings(const int64_t _seq) override;

            void get_user_last_seen(const int64_t _seq, const std::vector<std::string>& _ids) override;
            void on_nickname_check(const int64_t _seq, const std::string& _nickname, bool _set_nick) override;
            void on_get_common_chats(const int64_t _seq, const std::string& _sn) override;
        };
    }
}
