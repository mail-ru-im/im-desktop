#pragma once

namespace features
{
    int get_zstd_compression_level();
    bool is_zstd_request_enabled();
    bool is_zstd_response_enabled();
#ifndef STRIP_ZSTD
    std::string get_zstd_dict_request();
    std::string get_zstd_dict_response();
#endif // !STRIP_ZSTD

    bool smartreply_suggests_stickers_enabled();
    bool smartreply_suggests_text_enabled();

    bool is_fetch_hotstart_enabled();
    bool is_statistics_mytracker_enabled();
    bool is_dns_workaround_enabled();
    bool is_fallback_to_ip_mode_enabled();

    bool is_webp_preview_accepted();
    bool is_webp_original_accepted();

    bool is_update_from_backend_enabled();

    bool is_notify_network_change_enabled();

    bool is_invite_by_sms_enabled();
    bool should_show_sms_notify_setting();

    size_t get_max_parallel_sticker_downloads();

    bool is_silent_delete_enabled();

    bool is_url_ftp_protocols_allowed();

    bool is_draft_enabled();
    int draft_maximum_length();

    std::chrono::seconds get_link_metainfo_repeat_interval();
    std::chrono::milliseconds get_metainfo_repeat_interval();
    std::chrono::milliseconds get_preview_repeat_interval();
    std::chrono::seconds get_oauth2_refresh_interval();
    bool is_oauth2_login_allowed();

    std::chrono::seconds task_cache_lifetime();
    bool is_threads_enabled();

    bool is_restricted_files_enabled();
    bool trust_status_default();
    bool is_antivirus_check_enabled();
}
