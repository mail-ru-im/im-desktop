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

        constexpr auto reactions_initial_set = "reactions_initial_set";
        constexpr auto show_reactions = "show_reactions";

        constexpr auto subscr_renew_interval_status = "subscr_renew_interval_status";
        constexpr auto subscr_renew_interval_antivirus = "subscr_renew_interval_antivirus";
        constexpr auto subscr_renew_interval_call_room_info = "subscr_renew_interval_call_room_info";

        constexpr auto statuses_json = "statuses_json";
        constexpr auto statuses_enabled = "statuses_enabled";

        constexpr auto global_contact_search_allowed = "global_contact_search_allowed";
        constexpr auto force_update_check_allowed = "force_update_check_allowed";

        constexpr auto call_room_info_enabled = "call_room_info_enabled";
        constexpr auto has_connect_by_ip_option = "has_connect_by_ip_option";
    }
}
