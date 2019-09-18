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
#define settings_recents_emojis "recents_emojis_v2"
#define settings_recents_emojis2 "recents_emojis_v3"
#define settings_window_maximized "window_maximized"
#define settings_cl_groups_enabled "cl_groups_enabled"
#define settings_main_window_rect "main_window_rect"
#define settings_favorites_visible "favorites_visible"
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
#define settings_autoplay_gif "autoplay_gif"
#define settings_scale_coefficient "scale_coefficient"
#define settings_show_popular_contacts "show_popular_contacts"
#define settings_show_groupchat_heads "show_groupchat_heads"
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

constexpr bool settings_allow_big_emoji_deafult() noexcept { return true; }
constexpr bool settings_autoreplace_emoji_deafult() noexcept { return true; }
constexpr bool settings_partial_read_deafult() noexcept { return true; }

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

#define setting_mplayer_volume "mplayer_volume"
#define setting_mplayer_mute "mplayer_mute"

#define product_path_icq_w L"ICQ"
#define product_path_icq_a "ICQ"
#define product_path_agent_w L"Mail.Ru/Agent"
#define product_path_agent_a "Mail.Ru/Agent"
#define product_path_biz_w L"Mail.Ru/Myteam"
#define product_path_biz_a "Mail.Ru/Myteam"
#define product_path_dit_w L"Messenger"
#define product_path_dit_a "Messenger"
#define product_path_agent_mac_w L"Mail.Ru Agent"
#define product_path_agent_mac_a "Mail.Ru Agent"
#define product_name_icq_mac_a "ICQ"
#define product_name_agent_mac_a "Mail.ru Agent"

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
        inline constexpr size_t max_patterns_count() noexcept
        {
            return 10;
        }
    }
}

namespace feature
{
    constexpr bool default_one_domain_feature() noexcept { return true; }
    constexpr long long default_im_stats_send_interval() noexcept { return 1800LL; }
    constexpr long long default_im_stats_max_store_interval() noexcept { return 0LL; }
    constexpr int default_profile_nickname_minimum_length() noexcept { return 5; }
    constexpr int default_profile_nickname_maximum_length() noexcept { return 30; }
    constexpr bool default_profile_nickname_allowed() noexcept { return true; }
    constexpr const char* default_profile_domain() noexcept { return "icq.im"; }
    constexpr const char* default_profile_myteam_domain() noexcept { return "myteam.mail.ru/profile"; }
    constexpr const char* default_no_account_link() noexcept { return "https://biz.mail.ru/myteam"; }

    constexpr bool default_use_apple_emoji() noexcept { return true; }
    constexpr bool default_open_file_on_click() noexcept { return true; }
    constexpr bool default_force_show_chat_popup() noexcept { return true; }
    constexpr bool default_biz_phone_allowed() noexcept { return false; }
    constexpr long long default_history_request_page_size() noexcept { return 100; }
    constexpr bool default_new_avatar_rapi() noexcept { return false; }

    constexpr const char* default_data_visibility_link_icq() noexcept { return "https://icq.com/help/visibility/"; }
    constexpr const char* default_data_visibility_link_agent() noexcept { return "https://agent.mail.ru/help/visibility/"; }
    constexpr const char* default_password_recovery_link_icq() noexcept { return "https://icq.com/password/"; }
    constexpr const char* default_password_recovery_link_agent() noexcept { return "https://e.mail.ru/cgi-bin/passremind"; }
    constexpr const char* default_security_call_link() noexcept { return "https://icq.com/security-calls"; }
}
