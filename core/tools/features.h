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

    std::chrono::seconds get_link_metainfo_repeat_interval();
    std::chrono::milliseconds get_metainfo_repeat_interval();
    std::chrono::milliseconds get_preview_repeat_interval();
}
