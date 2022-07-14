#ifndef __CORE_H__
#define __CORE_H__

#pragma once
#define BOOST_STACKTRACE_GNU_SOURCE_NOT_REQUIRED

#include "../corelib/core_face.h"
#include "../common.shared/common_defs.h"
#include "tools/binary_stream.h"
#include "Voip/VoipManagerDefines.h"
#include <stack>
#include <optional>
#include <boost/stacktrace.hpp>

namespace coretools
{
    class settings;
}

namespace voip_manager {
    class VoipManager;
}

namespace core
{
    class main_thread;
    class im_container;
    class coll_helper;
    class core_settings;
    class gui_settings;
    class theme_settings;
    class base_im;
    class scheduler;
    class async_task;
    class async_executer;
    struct async_task_handlers;
    class im_login_id;
    class network_log;
    struct proxy_settings;
    class proxy_settings_manager;
    class hosts_config;
    class zstd_helper;
    class network_change_notifier;

    using stack_ptr = std::shared_ptr<boost::stacktrace::stacktrace>;
    using stack_vec = std::vector<stack_ptr>;

    class stacked_task
    {
    public:
        stacked_task() = default;

        stacked_task(std::function<void()> _func);

        operator bool() const noexcept;

        void execute();

        const stack_vec& get_stack() const;

    private:
        std::function<void()> func_;
        stack_vec stack_;
        bool need_stack_ = false;
    };

    namespace update
    {
        class updater;
    }

    namespace stats
    {
        class statistics;
        class im_stats;
    }

    namespace dump
    {
        class report_sender;
    }

    namespace stats
    {
        enum class stats_event_names;
        enum class im_stat_event_names;
    }

    namespace memory_stats
    {
        class memory_stats_collector;
    }

    namespace themes
    {
        using wallpaper_info_v = std::vector<struct wallpaper_info>;
    }

    class core_dispatcher
    {
        main_thread* core_thread_;
        std::unique_ptr<network_log> network_log_;
        std::unique_ptr<scheduler> scheduler_;

        std::shared_ptr<im_container> im_container_;
        std::shared_ptr<voip_manager::VoipManager> voip_manager_;

        std::shared_ptr<core::core_settings> settings_;
        std::shared_ptr<core::gui_settings> gui_settings_;
        std::shared_ptr<core::theme_settings> theme_settings_;
        std::shared_ptr<core::stats::statistics> statistics_;
        std::shared_ptr<core::stats::im_stats> im_stats_;
        std::shared_ptr<core::proxy_settings_manager> proxy_settings_manager_;

#ifndef STRIP_NET_CHANGE_NOTIFY
        std::shared_ptr<core::network_change_notifier> network_change_notifier_;
#endif //!STRIP_NET_CHANGE_NOTIFY

        std::shared_ptr<dump::report_sender> report_sender_;

        // memory usage stats
        std::shared_ptr<memory_stats::memory_stats_collector> memory_stats_collector_;

#ifndef STRIP_ZSTD
        // zstd compress/decompress helper
        std::shared_ptr<zstd_helper> zstd_helper_;
#endif // !STRIP_ZSTD

        // gui interfaces
        iconnector* gui_connector_;
        icore_factory* core_factory_;
        std::unique_ptr<async_executer> async_executer_;

        // updater
        std::unique_ptr<update::updater> updater_;

        common::core_gui_settings core_gui_settings_;

        std::atomic_uchar cl_search_count_;

        uint32_t delayed_stat_timer_id_;
        uint32_t delayed_post_timer_id_;
        uint32_t omicron_update_timer_id_;

        void load_gui_settings();
        void post_gui_settings();
        void post_logins();
        void post_app_config();
        void on_message_update_gui_settings_value(int64_t _seq, coll_helper _params);
        void on_message_log(coll_helper _params) const;
        void on_message_network_log(coll_helper _params);
        void on_message_profiler_proc_start(coll_helper _params) const;
        void on_message_profiler_proc_stop(coll_helper _params) const;

        void load_theme_settings();
        void post_theme_settings_and_meta();

        void on_message_update_theme_settings_value(int64_t _seq, coll_helper _params);
        void on_message_set_default_theme_id(int64_t _seq, coll_helper _params);
        void on_message_set_default_wallpaper_id(int64_t _seq, coll_helper _params);
        void on_message_set_wallpaper_urls(int64_t _seq, coll_helper _params);
        void on_feedback(int64_t _seq, coll_helper& _params);
        void on_misc_support(int64_t _seq, coll_helper& _params);
        void on_create_logs_archive(int64_t _seq, coll_helper& _params);

        void load_statistics();
        void load_im_stats();

        void update_parellel_packets_count();

        void setup_memory_stats_collector();

        void post_install_action();

        void start_omicron_service();

        bool is_network_log_valid() const;
        network_log& get_network_log();

        bool get_gui_settings_bool_value(std::string_view _name, const bool _default) const;

    public:

        core_dispatcher();
        virtual ~core_dispatcher();

        void start(const common::core_gui_settings&);
        std::string get_uniq_device_id() const;
        void execute_core_context(stacked_task _func);
        void on_external_config_changed();
        void on_omicron_updated(const std::string& _data);

        void write_data_to_network_log(tools::binary_stream _data);
        void write_string_to_network_log(std::string_view _text);
        std::stack<std::wstring> network_log_file_names_history_copy();

        uint32_t add_timer(stacked_task _func, std::chrono::milliseconds _timeout);
        uint32_t add_single_shot_timer(stacked_task _func, std::chrono::milliseconds _timeout);
        void stop_timer(uint32_t _id);

        std::shared_ptr<async_task_handlers> run_async(std::function<int32_t()> task);

        icollection* create_collection();

        void link_gui(icore_interface* _core_face, const common::core_gui_settings& _settings);
        void unlink_gui();

        void post_message_to_gui(std::string_view _message, int64_t _seq, icollection* _message_data);
        void receive_message_from_gui(std::string_view _message, int64_t _seq, icollection* _message_data);

        const common::core_gui_settings& get_core_gui_settings() const;
        void set_recent_avatars_size(int32_t _size);

        bool is_valid_cl_search() const;
        void begin_cl_search();
        unsigned end_cl_search();

        void unlogin(const bool _is_auth_error, const bool _force_clean_local_data = false, std::string_view _reason_to_log = {});

        std::string get_root_login();
        std::string get_login_after_start() const;
        std::string get_contact_friendly_name(unsigned _id, const std::string& _contact_login);
        std::shared_ptr<base_im> find_im_by_id(unsigned _id) const;

        void update_login(im_login_id& _login);
        void replace_uin_in_login(im_login_id& old_login, im_login_id& new_login);

#ifndef STRIP_VOIP
        void post_voip_message(unsigned _id, const voip_manager::VoipProtoMsg& msg);
        void post_voip_alloc(unsigned _id, const char* _data, size_t _len);
#endif

        void insert_event(core::stats::stats_event_names _event);
        void insert_event(core::stats::stats_event_names _event, core::stats::event_props_type&& _props);

        void insert_im_stats_event(core::stats::im_stat_event_names _event);
        void insert_im_stats_event(core::stats::im_stat_event_names _event, core::stats::event_props_type&& _props);

        void start_session_stats(bool _delayed);
        bool is_stats_enabled() const;

        bool is_im_stats_enabled() const;

        void on_thread_finish();

        // may be empty
        const std::shared_ptr<core::stats::statistics>& get_statistics() const;

        proxy_settings get_proxy_settings() const;
        bool try_to_apply_alternative_settings();

        void post_proxy_to_gui(const proxy_settings& _proxy_settings);

        void set_user_proxy_settings(const proxy_settings& _user_proxy_settings);

        bool locale_was_changed() const;
        void set_locale(const std::string& _locale);
        std::string get_locale() const;

        std::thread::id get_core_thread_id() const;
        bool is_core_thread() const;

        void save_themes_etag(std::string_view etag);
        std::string load_themes_etag();

        themes::wallpaper_info_v get_wallpaper_urls() const;

        // We save fix voip mute flag in settings,
        // Because we want to call this fix once.
        void set_voip_mute_fix_flag(bool bValue);
        bool get_voip_mute_fix_flag() const;

        void update_outgoing_msg_count(const std::string& _aimid, int _count);

        void set_last_stat_send_time(int64_t _time);
        int64_t get_last_stat_send_time();

        void set_local_pin_enabled(bool _enable);
        bool get_local_pin_enabled() const;

        void set_install_beta_updates(bool _enable);
        bool get_install_beta_updates() const;

        void set_local_pin(const std::string& _password);

        std::string get_local_pin_hash() const;
        std::string get_local_pin_salt() const;
        bool verify_local_pin(const std::string& _password) const;

        void set_external_config_url(std::string_view _url);
        std::string get_external_config_url() const;

        int64_t get_voip_init_memory_usage() const;

        void check_update(int64_t _seq, std::optional<std::string> _url);

        bool is_keep_local_data() const;
        void remove_local_data(const std::string_view _aimid, bool _is_exit = false);

        bool is_smartreply_available() const;
        bool is_suggests_available() const;

        void update_gui_settings();

        void reset_connection();

        void check_if_network_change_notifier_available();
        bool is_network_change_notifier_valid() const;

        void update_omicron(bool _need_add_account);

        void try_send_crash_report();

        void notify_history_dropped(const std::string& _aimid);

#ifndef STRIP_ZSTD
        const std::shared_ptr<zstd_helper>& get_zstd_helper() const;
#endif // !STRIP_ZSTD
    };

    extern std::unique_ptr<core::core_dispatcher>		g_core;
}



#endif // __CORE_H__

