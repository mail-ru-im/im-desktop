#pragma once

namespace omicron
{
    namespace keys
    {
        constexpr auto one_domain_url = "one_domain_url";
        constexpr auto max_get_url_length = "max_get_url_length";
        constexpr auto feedback_url = "feedback_url";
        constexpr auto history_request_page_size = "history_request_page_size";
        constexpr auto voip_rating_popup_json_config = "voip_rating_popup_json_config";
        constexpr auto voip_config = "voip_config";
        constexpr auto masks_request_timeout_on_fail = "masks_request_timeout_on_fail";

        constexpr auto smartreply_suggests_feature_enabled = "smartreply_suggests_feature_enabled";
        constexpr auto smartreply_suggests_text = "smartreply_suggests_text";
        constexpr auto smartreply_suggests_stickers = "smartreply_suggests_stickers";
        constexpr auto smartreply_suggests_for_quotes = "smartreply_suggests_for_quotes";
        constexpr auto smartreply_suggests_click_hide_timeout = "smartreply_suggests_click_hide_timeout";
        constexpr auto smartreply_suggests_msgid_cache_size = "smartreply_suggests_msgid_cache_size";

        constexpr auto im_stats_send_interval = "im_stats_send_interval";
        constexpr auto im_stats_max_store_interval = "im_stats_max_store_interval";

        //zstd. all these keys violate naming style
        constexpr auto im_zstd_compression_level = "im-zstd-compression-level";
        constexpr auto im_zstd_dict_request_enabled = "im-zstd-dict-request-enabled";
        constexpr auto im_zstd_dict_response_enabled = "im-zstd-dict-response-enabled";
        constexpr auto im_zstd_dict_request = "im-zstd-dict-request";
        constexpr auto im_zstd_dict_response = "im-zstd-dict-response";

        constexpr auto update_alpha_version = "update_alpha_version";
        constexpr auto update_beta_version = "update_beta_version";
        constexpr auto update_release_version = "update_release_version";
        constexpr auto beta_update = "beta_update";

        constexpr auto profile_nickname_minimum_length = "profile_nickname_minimum_length";
        constexpr auto profile_nickname_maximum_length = "profile_nickname_maximum_length";

        constexpr auto polls_enabled = "polls_enabled";
        constexpr auto poll_subscribe_timeout_ms = "poll_subscribe_timeout_ms";
        constexpr auto max_poll_options = "max_poll_options";

        constexpr auto profile_nickname_allowed = "profile_nickname_allowed";
        constexpr auto profile_domain = "profile_domain";
        constexpr auto profile_domain_agent = "profile_domain_agent";

        constexpr auto phone_allowed = "phone_allowed";
        constexpr auto show_attach_phone_number_popup = "show_attach_phone_number_popup";
        constexpr auto attach_phone_number_popup_close_btn = "attach_phone_number_popup_close_btn";
        constexpr auto attach_phone_number_popup_show_timeout = "attach_phone_number_popup_show_timeout";
        constexpr auto attach_phone_number_popup_text = "attach_phone_number_popup_text";
        constexpr auto attach_phone_number_popup_title = "attach_phone_number_popup_title";
        constexpr auto sms_result_waiting_time = "sms_result_waiting_time";
        constexpr auto external_phone_attachment = "external_phone_attachment";

        constexpr auto maximum_undo_size = "maximum_undo_size";
        constexpr auto use_apple_emoji = "use_apple_emoji";
        constexpr auto open_file_on_click = "open_file_on_click";
        constexpr auto show_notification_text = "show_notification_text";
        constexpr auto avatar_change_allowed = "avatar_change_allowed";
        constexpr auto cl_remove_contacts_allowed = "cl_remove_contacts_allowed";
        constexpr auto changeable_name = "changeable_name";
        constexpr auto fs_id_length = "fs_id_length";

        constexpr auto data_visibility_link = "data_visibility_link";
        constexpr auto password_recovery_link = "password_recovery_link";
        constexpr auto security_call_link = "security_call_link";

        constexpr auto server_suggests_enabled = "server_suggests_enabled";
        constexpr auto server_suggests_graceful_time = "server_suggests_graceful_time";
        constexpr auto server_suggests_max_allowed_chars = "server_suggests_max_allowed_chars";
        constexpr auto server_suggests_max_allowed_words = "server_suggests_max_allowed_words";
        constexpr auto suggests_max_allowed_chars = "suggests_max_allowed_chars";
        constexpr auto suggests_max_allowed_words = "suggests_max_allowed_words";

        constexpr auto spell_check_enabled = "spell_check_enabled";
        constexpr auto spell_check_max_suggest_count = "spell_check_max_suggest_count";

        constexpr auto new_message_fields  = "messages_features";
        constexpr auto new_message_parts = "message_part_features";

        constexpr auto favorites_image_id_english = "favorites_image_english";
        constexpr auto favorites_image_id_russian = "favorites_image_russian";

        constexpr auto async_response_timeout = "async_response_timeout";

        constexpr auto voip_call_user_limit = "voip_call_user_limit";
        constexpr auto voip_video_user_limit = "voip_video_user_limit";
        constexpr auto voip_big_conference_boundary = "voip_big_conference_boundary";

        constexpr auto fetch_hotstart_enabled = "fetch_hotstart_enabled";
        constexpr auto fetch_timeout = "fetch_timeout";

        constexpr auto vcs_call_by_link_enabled = "vcs_call_by_link_enabled";
        constexpr auto vcs_webinar_enabled = "vcs_webinar_enabled";
        constexpr auto vcs_room = "vcs_room";

        constexpr auto reactions_initial_set = "reactions_initial_set_json";
        constexpr auto show_reactions = "show_reactions";

        constexpr auto subscr_renew_interval_status = "subscr_renew_interval_status";
        constexpr auto subscr_renew_interval_antivirus = "subscr_renew_interval_antivirus";
        constexpr auto subscr_renew_interval_call_room_info = "subscr_renew_interval_call_room_info";
        constexpr auto subscr_renew_interval_thread = "subscr_renew_interval_thread";
        constexpr auto subscr_renew_interval_task = "subscr_renew_interval_task";

        constexpr auto statuses_json = "statuses_json";
        constexpr auto statuses_enabled = "statuses_enabled";
        constexpr auto custom_statuses_enabled = "custom_statuses_enabled";

        constexpr auto global_contact_search_allowed = "global_contact_search_allowed";

        constexpr auto block_stranger_button_text = "block_stranger_button_text";

        constexpr auto force_update_check_allowed = "force_update_check_allowed";

        constexpr auto webp_preview_accepted = "webp_preview_accepted";
        constexpr auto webp_original_accepted = "webp_original_accepted";
        constexpr auto webp_screenshot_enabled = "webp_screenshot_enabled";
        constexpr auto webp_max_file_size_to_convert = "webp_max_file_size_to_convert";

        constexpr auto call_room_info_enabled = "call_room_info_enabled";
        constexpr auto statistics_mytracker = "statistics_mytracker";
        constexpr auto dns_workaround_option = "dns_workaround_enabled";
        constexpr auto dns_resolve_timeout_sec = "dns_resolve_timeout_sec";
        constexpr auto fallback_to_ip_mode_enabled = "fallback_to_ip_mode_enabled";
        constexpr auto external_emoji_url = "external_emoji_url";

        constexpr auto ivr_login_enabled = "ivr_login_enabled";
        constexpr auto ivr_resend_count_to_show = "ivr_resend_count_to_show";
        constexpr auto show_your_invites_to_group_enabled = "show_your_invites_to_group_enabled";
        constexpr auto group_invite_blacklist_enabled = "group_invite_blacklist_enabled";
        constexpr auto notify_network_change = "notify_network_change";

        constexpr auto invite_by_sms = "invite_by_sms";
        constexpr auto show_sms_notify_setting = "show_sms_notify_setting";

        constexpr auto animated_stickers_in_picker_disabled = "animated_stickers_in_picker_disabled";
        constexpr auto animated_stickers_in_chat_disabled = "animated_stickers_in_chat_disabled";

        constexpr auto contact_list_smooth_scrolling_enabled = "contact_list_smooth_scrolling_enabled";

        constexpr auto max_parallel_sticker_downloads = "max_parallel_sticker_downloads";

        constexpr auto silent_message_delete = "silent_message_delete";

        constexpr auto background_ptt_play_enabled = "background_ptt_play_enabled";

        constexpr auto remove_deleted_from_notifications = "remove_deleted_from_notifications";

        constexpr auto long_path_tooltip_enabled = "long_path_tooltip_enabled";
        constexpr auto status_banner_emoji_csv = "status_banner_emoji_csv";

        constexpr auto formatting_in_bubbles = "formatting_in_bubbles";
        constexpr auto formatting_in_input = "formatting_in_input";

        constexpr auto link_metainfo_repeat_interval = "link_metainfo_repeat_interval";
        constexpr auto metainfo_repeat_interval = "metainfo_repeat_interval";
        constexpr auto preview_repeat_interval = "preview_repeat_interval";
        constexpr auto login_by_oauth2_allowed = "login_by_oauth2_allowed";
        constexpr auto oauth2_refresh_interval = "oauth2_refresh_interval";

        constexpr auto threads_enabled = "threads_enabled";
        constexpr auto organization_structure_enabled = "organization_structure_enabled";

        constexpr auto url_ftp_protocols_allowed = "url_ftp_protocols_allowed";

        constexpr auto draft_enabled = "draft_enabled";
        constexpr auto draft_timeout = "draft_timeout";
        constexpr auto draft_max_len = "draft_max_len";

        constexpr auto tasks_enabled = "tasks_enabled";
        constexpr auto task_creation_in_chat_enabled = "task_creation_in_chat_enabled";
        constexpr auto task_cache_lifetime = "task_cache_lifetime";

        constexpr auto expanded_gallery_enabled = "expanded_gallery_enabled";

        constexpr auto restricted_files_enabled = "restricted_files_enabled";
        constexpr auto antivirus_check_enabled = "antivirus_check_enabled";

        constexpr auto calendar_enabled = "calendar_enabled";
        constexpr auto base_retry_interval_sec = "base_retry_interval_sec";
    }
}
