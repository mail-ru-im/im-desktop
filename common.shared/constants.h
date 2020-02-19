#pragma once

#define auth_file_name "value.au"
#define auth_export_file_name "exported.json"
#define auth_export_file_name_merge "exportedmerge.json"
#define muted_chats_export_file_name "mutedchats.json"
#define settings_export_file_name "exported_ui.json"
#define core_settings_export_file_name "exported_core.json"
#define fetch_url_file_name "fetch"
#define core_settings_file_name "core.stg"
#define gui_settings_file_name "ui2.stg"
#define theme_settings_file_name "theme.stg"
#define hosts_config_file_name "hosts.stg"
#define omicron_cache_file_name "omicron.stg"

#define settings_language "language"
#define settings_upload_directory "upload_directory"
#define settings_download_directory_old "download_directory"
#define settings_download_directory "user_download_directory"
#define settings_download_directory_save_as "download_directory_save_as"
#define settings_old_recents_stickers "recents_stickers"
#define settings_recents_fs_stickers "recents_fs_stickers"
#define settings_recents_emojis2 "recents_emojis_v3"
#define settings_window_maximized "window_maximized"
#define settings_cl_groups_enabled "cl_groups_enabled"
#define settings_main_window_rect "main_window_rect"
#define settings_favorites_visible "favorites_visible"
#define settings_unimportant_visible "unimportant_visible"
#define settings_splitter_state "splitter_state"
#define settings_splitter_state_scale "splitter_state_scale"
#define settings_recents_mini_mode "recents_mini_mode"

#define settings_startup_at_system_started "startup_at_system_started"
#define settings_start_minimazed "start_minimazed"
#define settings_show_in_taskbar "show_in_taskbar"
#define settings_show_in_menubar "show_in_menubar"
#define settings_sounds_enabled "sounds_enabled"
#define settings_outgoing_message_sound_enabled "outgoing_message_sound_enabled"
#define settings_download_files_automatically "download_files_automatically"
#define settings_key1_to_send_message "key1_to_send_message"
#define settings_key2_to_send_message "key2_to_send_message"
#define settings_show_last_message "settings_show_last_message"
#define settings_show_video_and_images "show_video_and_images"
#define settings_autoplay_video "autoplay_video"
#define settings_hoversound_video "hoversound_video"
#define settings_autoplay_gif "autoplay_gif"
#define settings_scale_coefficient "scale_coefficient"
#define settings_show_popular_contacts "show_popular_contacts"
#define settings_show_groupchat_heads "show_groupchat_heads"
#define settings_show_smartreply "show_smartreply"
#define settings_show_suggests_emoji "show_suggest_emoji"
#define settings_show_suggests_words "show_suggest_words"
#define settings_allow_big_emoji "allow_big_emoji"
#define settings_autoreplace_emoji "autoreplace_emoji"
#define settings_partial_read "partial_read"
#define settings_voip_calls_count_map "voip_calls_count_map"
#define settings_stat_last_posted_times "stat_last_posted_times"
#define settings_shortcuts_close_action "shortcuts_close_action"
#define settings_shortcuts_search_action "shortcuts_search_action"
#define settings_appearance_bold_text "appearance_bold_text"
#define settings_appearance_text_size "appearance_text_size"
#define settings_appearance_last_directory "appearance_last_wp_directory"
#define settings_fast_drop_search_results "fast_drop_search_results"

constexpr bool settings_allow_big_emoji_default() noexcept { return true; }
constexpr bool settings_autoreplace_emoji_default() noexcept { return true; }
constexpr bool settings_hoversound_video_default() noexcept { return true; }
constexpr bool settings_partial_read_default() noexcept { return true; }
constexpr bool settings_show_smartreply_default() noexcept { return true; }
constexpr bool settings_fast_drop_search_default() noexcept { return false; }

#define settings_microphone "microphone"
#define settings_microphone_gain "microphone_gain"
#define settings_speakers "speakers"
#define settings_webcam "webcam"
#define settings_microphone_is_default "default_microphone"
#define settings_speakers_is_default "default_speakers"

#define settings_search_history_patterns "search_history_patterns"

#define settings_notify_new_messages "notify_new_messages"
#define settings_notify_new_mail_messages "notify_new_mail_messages"
#define settings_hide_message_notification "hide_message_notification"
#define settings_alert_tray_icon "alert_tray_icon"

#define settings_keep_logged_in "keep_logged_in"

#define settings_feedback_email "feedback_email"

#define settings_mac_accounts_migrated "mac_accounts_migrated"
#define settings_need_show_promo "need_show_promo"

#define settings_proxy_type "proxy_settings_type"
#define settings_proxy_address "proxy_settings_address"
#define settings_proxy_port "proxy_settings_port"
#define settings_proxy_username "proxy_settings_username"
#define settings_proxy_password "proxy_settings_password"

#define login_page_last_entered_uin "login_page_last_entered_uin"
#define login_page_last_entered_country_code "login_page_last_entered_country_code"
#define login_page_last_entered_phone "login_page_last_entered_phone"
#define login_page_last_login_type "login_page_last_login_type"
#define login_page_need_fill_profile "login_page_need_fill_profile"

#define setting_mplayer_volume "mplayer_volume"
#define setting_mplayer_mute "mplayer_mute"

#define setting_show_forward_author "show_forward_author"

#define setting_local_pin_timeout "local_pin_timeout"

#define release_notes_sha1 "release_notes_sha1"
#define first_run "first_run"
#define last_version "last_version"

constexpr const char* get_global_wp_id_setting_field() noexcept
{
    return "global_wp_id";
}

namespace core
{
    namespace search
    {
        constexpr size_t max_patterns_count() noexcept { return 10; }
        constexpr size_t archive_block_size() noexcept { return 4 * 1024 * 1024; }
    }
}

namespace feature
{
    constexpr long long default_im_stats_send_interval() noexcept { return 1800LL; }
    constexpr long long default_im_stats_max_store_interval() noexcept { return 0LL; }
    constexpr int default_profile_nickname_minimum_length() noexcept { return 5; }
    constexpr int default_profile_nickname_maximum_length() noexcept { return 30; }
    constexpr bool default_profile_nickname_allowed() noexcept { return true; }

    constexpr bool default_use_apple_emoji() noexcept { return true; }
    constexpr bool default_open_file_on_click() noexcept { return true; }
    constexpr bool default_external_phone_attachment() noexcept { return false; }
    constexpr long long default_history_request_page_size() noexcept { return 100; }
    constexpr int default_show_timeout_attach_phone_number_popup() noexcept { return 86400; }
    constexpr const char* default_voip_rating_popup_json_config() noexcept { return "{}"; }
    constexpr bool default_polls_enabled() noexcept { return true; }
    constexpr int default_poll_subscribe_timeout_ms() noexcept { return 60000; }
    constexpr int default_max_poll_options() noexcept { return 10; }

    constexpr int default_stickers_suggest_time_interval() noexcept { return 501; }
    constexpr bool default_stickers_server_suggest_enabled() noexcept { return true; }
    constexpr int default_stickers_server_suggests_max_allowed_chars() noexcept { return 50; }
    constexpr int default_stickers_server_suggests_max_allowed_words() noexcept { return 5; }
    constexpr int default_stickers_local_suggests_max_allowed_chars() noexcept { return 50; }
    constexpr int default_stickers_local_suggests_max_allowed_words() noexcept { return 10; }

    constexpr const char* default_security_call_link() noexcept { return "https://icq.com/security-calls"; }
    constexpr int default_fs_id_length() noexcept { return 33; }
    constexpr int default_im_zstd_compression_level() noexcept { return 3; }
    constexpr const char* default_dev_id() noexcept { return "dev_icq_id"; }
}
