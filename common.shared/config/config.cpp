#include "stdafx.h"

#include "config.h"

#ifdef SUPPORT_EXTERNAL_CONFIG

#include "../core/tools/strings.h"
#include "../core/tools/binary_stream.h"
#include "../core/tools/system.h"
#include "../core/utils.h"
#include <boost/filesystem.hpp>

#endif // SUPPORT_EXTERNAL_CONFIG
#include "../environment.h"

#include "../json_unserialize_helpers.h"
#include "config_data.h"

#include "../spin_lock.h"

namespace config
{
    static auto is_less_by_first = [](const auto& x1, const auto& x2)
    {
        static_assert(std::is_same_v<decltype(x1), decltype(x2)>);
        auto key1 = x1.first;
        auto key2 = x2.first;
        return static_cast<
                std::underlying_type_t<decltype(key1)>>(key1) < static_cast<std::underlying_type_t<decltype(key2)>>(key2);

    };

    static auto get_string(const rapidjson::Value& json_value, std::string_view name)
    {
        return common::json::get_value<std::string>(json_value, name).value_or(std::string());
    }

    static auto get_bool(const rapidjson::Value& json_value, std::string_view name)
    {
        return common::json::get_value<bool>(json_value, name).value_or(false);
    }

    static auto get_int64(const rapidjson::Value& json_value, std::string_view name)
    {
        return common::json::get_value<int64_t>(json_value, name).value_or(0);
    }

    static std::optional<urls_array> parse_urls(const rapidjson::Value& _node)
    {
        if (const auto it = _node.FindMember("urls"); it != _node.MemberEnd())
        {
            urls_array res =
            {
                std::pair(urls::base, get_string(it->value, "base")),
                std::pair(urls::base_binary, get_string(it->value, "base_binary")),
                std::pair(urls::files_parser_url, get_string(it->value, "files_parser_url")),
                std::pair(urls::profile, get_string(it->value, "profile")),
                std::pair(urls::profile_agent, get_string(it->value, "profile_agent")),
                std::pair(urls::auth_mail_ru, get_string(it->value, "auth_mail_ru")),
                std::pair(urls::oauth_url, get_string(it->value, "oauth_url")),
                std::pair(urls::token_url, get_string(it->value, "token_url")),
                std::pair(urls::redirect_uri, get_string(it->value, "redirect_uri")),
                std::pair(urls::r_mail_ru, get_string(it->value, "r_mail_ru")),
                std::pair(urls::win_mail_ru, get_string(it->value, "win_mail_ru")),
                std::pair(urls::read_msg, get_string(it->value, "read_msg")),
                std::pair(urls::stickerpack_share, get_string(it->value, "stickerpack_share")),
                std::pair(urls::cicq_com, get_string(it->value, "c.icq.com")),
                std::pair(urls::update_win_alpha, get_string(it->value, "update_win_alpha")),
                std::pair(urls::update_win_beta, get_string(it->value, "update_win_beta")),
                std::pair(urls::update_win_release, get_string(it->value, "update_win_release")),
                std::pair(urls::update_mac_alpha, get_string(it->value, "update_mac_alpha")),
                std::pair(urls::update_mac_beta, get_string(it->value, "update_mac_beta")),
                std::pair(urls::update_mac_release, get_string(it->value, "update_mac_release")),
                std::pair(urls::update_linux_alpha_32, get_string(it->value, "update_linux_alpha_32")),
                std::pair(urls::update_linux_alpha_64, get_string(it->value, "update_linux_alpha_64")),
                std::pair(urls::update_linux_beta_32, get_string(it->value, "update_linux_beta_32")),
                std::pair(urls::update_linux_beta_64, get_string(it->value, "update_linux_beta_64")),
                std::pair(urls::update_linux_release_32, get_string(it->value, "update_linux_release_32")),
                std::pair(urls::update_linux_release_64, get_string(it->value, "update_linux_release_64")),
                std::pair(urls::omicron, get_string(it->value, "omicron")),
                std::pair(urls::stat_base, get_string(it->value, "stat_base")),
                std::pair(urls::feedback, get_string(it->value, "feedback")),
                std::pair(urls::legal_terms, get_string(it->value, "legal_terms")),
                std::pair(urls::legal_privacy_policy, get_string(it->value, "legal_privacy_policy")),
                std::pair(urls::installer_help, get_string(it->value, "installer_help")),
                std::pair(urls::data_visibility, get_string(it->value, "data_visibility")),
                std::pair(urls::password_recovery, get_string(it->value, "password_recovery")),
                std::pair(urls::zstd_request_dict, get_string(it->value, "zstd_request_dict")),
                std::pair(urls::zstd_response_dict, get_string(it->value, "zstd_response_dict")),
                std::pair(urls::attach_phone, get_string(it->value, "attach_phone_url")),
                std::pair(urls::update_app_url, get_string(it->value, "update_app_url")),
                std::pair(urls::vcs_room, get_string(it->value, "vcs_room")),
                std::pair(urls::dns_cache, get_string(it->value, "dns_cache")),
                std::pair(urls::external_emoji, get_string(it->value, "external_emoji")),
                std::pair(urls::privacy_policy_url, get_string(it->value, "privacy_policy_url")),
                std::pair(urls::terms_of_use_url, get_string(it->value, "terms_of_use_url")),
                std::pair(urls::delete_account_url, get_string(it->value, "delete_account_url")),
                std::pair(urls::delete_account_url_email, get_string(it->value, "delete_account_url_email"))
            };

            if (std::is_sorted(std::cbegin(res), std::cend(res), is_less_by_first))
                return std::make_optional(std::move(res));
            assert(false);
        }
        return {};
    }

    static std::optional<translations_array> parse_translations(const rapidjson::Value& _node)
    {
        if (const auto it = _node.FindMember("translations"); it != _node.MemberEnd())
        {
            translations_array res =
            {
                std::pair(translations::installer_title_win, get_string(it->value, "installer_title_win")),
                std::pair(translations::installer_failed_win, get_string(it->value, "installer_failed_win")),
                std::pair(translations::installer_welcome_win, get_string(it->value, "installer_welcome_win")),
                std::pair(translations::app_title, get_string(it->value, "app_title")),
                std::pair(translations::app_install_mobile, get_string(it->value, "app_install_mobile")),
                std::pair(translations::gdpr_welcome, get_string(it->value, "gdpr_welcome")),
            };

            if (std::is_sorted(std::cbegin(res), std::cend(res), is_less_by_first))
                return std::make_optional(std::move(res));
            assert(false);
        }
        return {};
    }

    static std::optional<features_array> parse_features(const rapidjson::Value& _node)
    {
        if (const auto it = _node.FindMember("features"); it != _node.MemberEnd())
        {
            features_array res =
            {
                std::pair(features::feedback_selected, get_bool(it->value, "feedback_selected")),
                std::pair(features::new_avatar_rapi, get_bool(it->value, "new_avatar_rapi")),
                std::pair(features::ptt_recognition, get_bool(it->value, "ptt_recognition")),
                std::pair(features::snippet_in_chat, get_bool(it->value, "snippet_in_chat")),
                std::pair(features::add_contact, get_bool(it->value, "add_contact")),
                std::pair(features::remove_contact, get_bool(it->value, "remove_contact")),
                std::pair(features::force_avatar_update, get_bool(it->value, "force_avatar_update")),
                std::pair(features::call_quality_stat, get_bool(it->value, "call_quality_stat")),
                std::pair(features::show_data_visibility_link, get_bool(it->value, "show_data_visibility_link")),
                std::pair(features::show_security_call_link, get_bool(it->value, "show_security_call_link")),
                std::pair(features::contact_us_via_mail_default, get_bool(it->value, "contact_us_via_mail_default")),
                std::pair(features::searchable_recents_placeholder, get_bool(it->value, "searchable_recents_placeholder")),
                std::pair(features::auto_accepted_gdpr, get_bool(it->value, "auto_accepted_gdpr")),
                std::pair(features::phone_allowed, get_bool(it->value, "phone_allowed")),
                std::pair(features::show_attach_phone_number_popup, get_bool(it->value, "show_attach_phone_number_popup")),
                std::pair(features::attach_phone_number_popup_modal, get_bool(it->value, "attach_phone_number_popup_modal")),
                std::pair(features::login_by_phone_allowed, get_bool(it->value, "login_by_phone_allowed")),
                std::pair(features::login_by_mail_default, get_bool(it->value, "login_by_mail_default")),
                std::pair(features::login_by_uin_allowed, get_bool(it->value, "login_by_uin_allowed")),
                std::pair(features::login_by_oauth2_allowed, get_bool(it->value, "login_by_oauth2_allowed")),
                std::pair(features::forgot_password, get_bool(it->value, "forgot_password")),
                std::pair(features::explained_forgot_password, get_bool(it->value, "explained_forgot_password")),
                std::pair(features::unknown_contacts, get_bool(it->value, "unknown_contacts")),
                std::pair(features::stranger_contacts, get_bool(it->value, "stranger_contacts")),
                std::pair(features::otp_login, get_bool(it->value, "otp_login")),
                std::pair(features::hiding_message_text_enabled, get_bool(it->value, "hiding_message_text_enabled")),
                std::pair(features::hiding_message_info_enabled, get_bool(it->value, "hiding_message_info_enabled")),
                std::pair(features::changeable_name, get_bool(it->value, "changeable_name")),
                std::pair(features::allow_contacts_rename, get_bool(it->value, "allow_contacts_rename")),
                std::pair(features::avatar_change_allowed, get_bool(it->value, "avatar_change_allowed")),
                std::pair(features::beta_update, get_bool(it->value, "beta_update")),
                std::pair(features::ssl_verify, get_bool(it->value, "ssl_verify")),
                std::pair(features::profile_agent_as_domain_url, get_bool(it->value, "profile_agent_as_domain_url")),
                std::pair(features::need_gdpr_window, get_bool(it->value, "need_gdpr_window")),
                std::pair(features::need_introduce_window, get_bool(it->value, "need_introduce_window")),
                std::pair(features::has_nicknames, get_bool(it->value, "has_nicknames")),
                std::pair(features::zstd_request_enabled, get_bool(it->value, "zstd_request_enabled")),
                std::pair(features::zstd_response_enabled, get_bool(it->value, "zstd_response_enabled")),
                std::pair(features::external_phone_attachment, get_bool(it->value, "external_phone_attachment")),
                std::pair(features::external_url_config, get_bool(it->value, "external_url_config")),
                std::pair(features::omicron_enabled, get_bool(it->value, "omicron_enabled")),
                std::pair(features::sticker_suggests, get_bool(it->value, "sticker_suggests")),
                std::pair(features::sticker_suggests_server, get_bool(it->value, "sticker_suggests_server")),
                std::pair(features::smartreplies, get_bool(it->value, "smartreplies")),
                std::pair(features::spell_check, get_bool(it->value, "spell_check")),
                std::pair(features::favorites_message_onpremise, get_bool(it->value, "favorites_message_onpremise")),
                std::pair(features::info_change_allowed, get_bool(it->value, "info_change_allowed")),
                std::pair(features::call_link_v2_enabled, get_bool(it->value, "call_link_v2_enabled")),
                std::pair(features::vcs_call_by_link_enabled, get_bool(it->value, "vcs_call_by_link_enabled")),
                std::pair(features::vcs_webinar_enabled, get_bool(it->value, "vcs_webinar_enabled")),
                std::pair(features::statuses_enabled, get_bool(it->value, "statuses_enabled")),
                std::pair(features::global_contact_search_allowed, get_bool(it->value, "global_contact_search_allowed")),
                std::pair(features::show_reactions, get_bool(it->value, "show_reactions")),
                std::pair(features::open_icqcom_link, get_bool(it->value, "open_icqcom_link")),
                std::pair(features::statistics, get_bool(it->value, "statistics")),
                std::pair(features::statistics_mytracker, get_bool(it->value, "statistics_mytracker")),
                std::pair(features::force_update_check_allowed, get_bool(it->value, "force_update_check_allowed")),
                std::pair(features::call_room_info_enabled, get_bool(it->value, "call_room_info_enabled")),
                std::pair(features::store_version, get_bool(it->value, "store_version")),
                std::pair(features::otp_login_open_mail_link, get_bool(it->value, "otp_login_open_mail_link")),
                std::pair(features::dns_workaround, get_bool(it->value, "dns_workaround_enabled")),
                std::pair(features::external_emoji, get_bool(it->value, "external_emoji")),
                std::pair(features::show_your_invites_to_group_enabled, get_bool(it->value, "show_your_invites_to_group_enabled")),
                std::pair(features::group_invite_blacklist_enabled, get_bool(it->value, "group_invite_blacklist_enabled")),
                std::pair(features::contact_us_via_backend, get_bool(it->value, "contact_us_via_backend")),
                std::pair(features::update_from_backend, get_bool(it->value, "update_from_backend")),
                std::pair(features::invite_by_sms, get_bool(it->value, "invite_by_sms")),
                std::pair(features::show_sms_notify_setting, get_bool(it->value, "show_sms_notify_setting")),
                std::pair(features::silent_message_delete, get_bool(it->value, "silent_message_delete")),
                std::pair(features::remove_deleted_from_notifications, get_bool(it->value, "remove_deleted_from_notifications")),
                std::pair(features::long_path_tooltip_enabled, get_bool(it->value, "long_path_tooltip_enabled")),
                std::pair(features::installer_crash_send, get_bool(it->value, "installer_crash_send")),
                std::pair(features::custom_statuses_enabled, get_bool(it->value, "custom_statuses_enabled")),
                std::pair(features::formatting_in_bubbles, get_bool(it->value, "formatting_in_bubbles")),
                std::pair(features::formatting_in_input, get_bool(it->value, "formatting_in_input")),
                std::pair(features::apps_bar_enabled, get_bool(it->value, "apps_bar_enabled")),
                std::pair(features::status_in_apps_bar, get_bool(it->value, "status_in_apps_bar")),
                std::pair(features::scheduled_messages_enabled, get_bool(it->value, "scheduled_messages_enabled")),
                std::pair(features::threads_enabled, get_bool(it->value, "threads_enabled")),
                std::pair(features::reminders_enabled, get_bool(it->value, "reminders_enabled")),
                std::pair(features::support_shared_federation_stickerpacks, get_bool(it->value, "support_shared_federation_stickerpacks")),
                std::pair(features::url_ftp_protocols_allowed, get_bool(it->value, "url_ftp_protocols_allowed")),
                std::pair(features::draft_enabled, get_bool(it->value, "draft_enabled")),
                std::pair(features::message_corner_menu, get_bool(it->value, "message_corner_menu")),
                std::pair(features::task_creation_in_chat_enabled, get_bool(it->value, "task_creation_in_chat_enabled")),
                std::pair(features::smartreply_suggests_feature_enabled, get_bool(it->value, "smartreply_suggests_feature_enabled")),
                std::pair(features::smartreply_suggests_text, get_bool(it->value, "smartreply_suggests_text")),
                std::pair(features::smartreply_suggests_stickers, get_bool(it->value, "smartreply_suggests_stickers")),
                std::pair(features::smartreply_suggests_for_quotes, get_bool(it->value, "smartreply_suggests_for_quotes")),
                std::pair(features::compact_mode_by_default, get_bool(it->value, "compact_mode_by_default")),
                std::pair(features::expanded_gallery, get_bool(it->value, "expanded_gallery")),
                std::pair(features::restricted_files_enabled, get_bool(it->value, "restricted_files_enabled")),
                std::pair(features::antivirus_check_enabled, get_bool(it->value, "antivirus_check_enabled")),
                std::pair(features::antivirus_check_progress_visible, get_bool(it->value, "antivirus_check_progress_visible")),
                std::pair(features::external_user_agreement, get_bool(it->value, "external_user_agreement")),
                std::pair(features::user_agreement_enabled, get_bool(it->value, "user_agreement_enabled")),
                std::pair(features::delete_account_enabled, get_bool(it->value, "delete_account_enabled")),
                std::pair(features::delete_account_via_admin, get_bool(it->value, "delete_account_via_admin")),
                std::pair(features::has_registry_about, get_bool(it->value, "has_registry_about")),
                // TODO: remove when deprecated
                std::pair(features::organization_structure_enabled, get_bool(it->value, "organization_structure_enabled")),
                std::pair(features::tasks_enabled, get_bool(it->value, "tasks_enabled")),
                std::pair(features::calendar_enabled, get_bool(it->value, "calendar_enabled")),
                std::pair(features::tarm_mail, get_bool(it->value, "tarm_mail")),
                std::pair(features::tarm_cloud, get_bool(it->value, "tarm_cloud")),
                std::pair(features::tarm_calls, get_bool(it->value, "tarm_calls")),
                std::pair(features::calendar_self_auth, get_bool(it->value, "calendar_self_auth")),
                std::pair(features::mail_enabled, get_bool(it->value, "mail_enabled")),
                std::pair(features::mail_self_auth, get_bool(it->value, "mail_self_auth")),
                std::pair(features::cloud_self_auth, get_bool(it->value, "cloud_self_auth")),
                std::pair(features::digital_assistant_search_positioning, get_bool(it->value, "digital_assistant_search_positioning")),
                std::pair(features::leading_last_name, get_bool(it->value, "leading_last_name")),
                std::pair(features::report_messages_enabled, get_bool(it->value, "report_messages_enabled")),
            };

            if (std::is_sorted(std::cbegin(res), std::cend(res), is_less_by_first))
                return std::make_optional(std::move(res));
            assert(false);
        }
        return {};
    }

    static std::optional<values_array> parse_values(const rapidjson::Value& _node)
    {
        if (const auto it = _node.FindMember("values"); it != _node.MemberEnd())
        {
            values_array res =
            {
                std::pair(values::product_path, get_string(it->value, "product_path")),
                std::pair(values::product_path_mac, get_string(it->value, "product_path_mac")),
                std::pair(values::app_name_win, get_string(it->value, "app_name_win")),
                std::pair(values::app_name_mac, get_string(it->value, "app_name_mac")),
                std::pair(values::app_name_linux, get_string(it->value, "app_name_linux")),
                std::pair(values::user_agent_app_name, get_string(it->value, "user_agent_app_name")),
                std::pair(values::client_b64, get_string(it->value, "client_b64")),
                std::pair(values::client_id, get_string(it->value, "client_id")),
                std::pair(values::client_rapi, get_string(it->value, "client_rapi")),
                std::pair(values::oauth_type, get_string(it->value, "oauth_type")),
                std::pair(values::oauth_scope, get_string(it->value, "oauth_scope")),
                std::pair(values::product_name, get_string(it->value, "product_name")),
                std::pair(values::product_name_short, get_string(it->value, "product_name_short")),
                std::pair(values::product_name_full, get_string(it->value, "product_name_full")),
                std::pair(values::main_instance_mutex_linux, get_string(it->value, "main_instance_mutex_linux")),
                std::pair(values::main_instance_mutex_win, get_string(it->value, "main_instance_mutex_win")),
                std::pair(values::crossprocess_pipe, get_string(it->value, "crossprocess_pipe")),
                std::pair(values::register_url_scheme_csv, get_string(it->value, "register_url_scheme_csv")),
                std::pair(values::installer_shortcut_win, get_string(it->value, "installer_shortcut_win")),
                std::pair(values::installer_menu_folder_win, get_string(it->value, "installer_menu_folder_win")),
                std::pair(values::installer_product_exe_win, get_string(it->value, "installer_product_exe_win")),
                std::pair(values::installer_exe_win, get_string(it->value, "installer_exe_win")),
                std::pair(values::installer_hkey_class_win, get_string(it->value, "installer_hkey_class_win")),
                std::pair(values::installer_main_instance_mutex_win, get_string(it->value, "installer_main_instance_mutex_win")),
                std::pair(values::company_name, get_string(it->value, "company_name")),
                std::pair(values::app_user_model_win, get_string(it->value, "app_user_model_win")),
                std::pair(values::feedback_version_id, get_string(it->value, "feedback_version_id")),
                std::pair(values::feedback_platform_id, get_string(it->value, "feedback_platform_id")),
                std::pair(values::feedback_aimid_id, get_string(it->value, "feedback_aimid_id")),
                std::pair(values::feedback_selected_id, get_string(it->value, "feedback_selected_id")),
                std::pair(values::omicron_dev_id_win, get_string(it->value, "omicron_dev_id_win")),
                std::pair(values::omicron_dev_id_mac, get_string(it->value, "omicron_dev_id_mac")),
                std::pair(values::omicron_dev_id_linux, get_string(it->value, "omicron_dev_id_linux")),
                std::pair(values::dev_id_win, get_string(it->value, "dev_id_win")),
                std::pair(values::dev_id_mac, get_string(it->value, "dev_id_mac")),
                std::pair(values::dev_id_linux, get_string(it->value, "dev_id_linux")),
                std::pair(values::additional_fs_parser_urls_csv, get_string(it->value, "additional_fs_parser_urls_csv")),
                std::pair(values::uins_for_send_log_csv, get_string(it->value, "uins_for_send_log_csv")),
                std::pair(values::zstd_compression_level, get_int64(it->value, "zstd_compression_level")),
                std::pair(values::server_search_messages_min_symbols, get_int64(it->value, "server_search_messages_min_symbols")),
                std::pair(values::server_search_contacts_min_symbols, get_int64(it->value, "server_search_contacts_min_symbols")),
                std::pair(values::voip_call_user_limit, get_int64(it->value, "voip_call_user_limit")),
                std::pair(values::voip_video_user_limit, get_int64(it->value, "voip_video_user_limit")),
                std::pair(values::voip_big_conference_boundary, get_int64(it->value, "voip_big_conference_boundary")),
                std::pair(values::maximum_history_file_size, get_int64(it->value, "maximum_history_file_size")),
                std::pair(values::external_config_preset_url, get_string(it->value, "external_config_preset_url")),
                std::pair(values::client_api_version, get_int64(it->value, "client_api_version")),
                std::pair(values::server_api_version, get_int64(it->value, "client_api_version")), // client_api_version is correct
                std::pair(values::server_mention_timeout, get_int64(it->value, "server_mention_timeout")),
                std::pair(values::support_uin, get_string(it->value, "support_uin")),
                std::pair(values::mytracker_app_id_win, get_string(it->value, "mytracker_app_id_win")),
                std::pair(values::mytracker_app_id_mac, get_string(it->value, "mytracker_app_id_mac")),
                std::pair(values::mytracker_app_id_linux, get_string(it->value, "mytracker_app_id_linux")),
                std::pair(values::status_banner_emoji_csv, get_string(it->value, "status_banner_emoji_csv")),
                std::pair(values::draft_timeout_sec, get_int64(it->value, "draft_timeout")),
                std::pair(values::draft_max_len, get_int64(it->value, "draft_max_len")),
                std::pair(values::smartreply_suggests_click_hide_timeout, get_int64(it->value, "smartreply_suggests_click_hide_timeout")),
                std::pair(values::smartreply_suggests_msgid_cache_size, get_int64(it->value, "smartreply_suggests_msgid_cache_size")),
                std::pair(values::base_retry_interval_sec, get_int64(it->value, "base_retry_interval_sec")),
                std::pair(values::product_path_old, get_string(it->value, "product_path_old")),
                std::pair(values::product_name_old, get_string(it->value, "product_name_old")),
                std::pair(values::crossprocess_pipe_old, get_string(it->value, "crossprocess_pipe_old")),
                std::pair(values::main_instance_mutex_win_old, get_string(it->value, "main_instance_mutex_win_old")),
                std::pair(values::bots_commands_disabled, get_string(it->value, "bots_commands_disabled")),
                std::pair(values::service_apps_order, std::string{}), // must be empty
                std::pair(values::service_apps_config, get_string(it->value, "service-apps-config")),
                std::pair(values::service_apps_desktop, get_string(it->value, "service-apps-desktop")),
                std::pair(values::custom_miniapps, get_string(it->value, "custom-miniapps")),
                std::pair(values::wim_parallel_packets_count, get_int64(it->value, "wim_parallel_packets_count")),
                std::pair(values::digital_assistant_bot_aimid, get_int64(it->value, "digital_assistant_bot_aimid")),
                std::pair(values::additional_theme, get_int64(it->value, "additional_theme")),
                std::pair(values::max_delete_files, get_int64(it->value, "max_delete_files")),
                std::pair(values::delete_files_older_sec, get_int64(it->value, "delete_files_older_sec")),
                std::pair(values::cleanup_period_sec, get_int64(it->value, "cleanup_period_sec")),
            };

            if (std::is_sorted(std::cbegin(res), std::cend(res), is_less_by_first))
                return std::make_optional(std::move(res));
            assert(!"Product config not sorted");
        }
        return {};
    }

    configuration::configuration(std::string_view json, bool _is_debug)
        : is_debug_(_is_debug)
    {
        spin_lock_ = std::make_unique<common::tools::spin_lock>();
        if (rapidjson::Document doc; !doc.Parse(json.data(), json.size()).HasParseError())
        {
            auto urls = parse_urls(doc);
            if (!urls)
                return;
            c_.urls = std::move(*urls);

            auto values = parse_values(doc);
            if (!values)
                return;
            c_.values = std::move(*values);

            auto features = parse_features(doc);
            if (!features)
                return;
            c_.features = std::move(*features);

            auto translations = parse_translations(doc);
            if (!translations)
                return;
            c_.translations = std::move(*translations);

            is_valid_ = true;

            if (is_on(config::features::external_url_config))
                e_ = std::make_shared<external_configuration>();
        }
    }

    bool configuration::is_external_config_enabled() const noexcept
    {
        return c_.features[static_cast<size_t>(config::features::external_url_config)].second;
    }

    std::shared_ptr<external_configuration> configuration::get_external() const
    {
        std::scoped_lock lock(*spin_lock_);
        return e_;
    }

    void configuration::set_external(std::shared_ptr<external_configuration> _e, bool _is_develop)
    {
        std::scoped_lock lock(*spin_lock_);
        e_ = std::move(_e);
        is_develop_ = _is_develop;
    }

    configuration::~configuration() = default;

    void configuration::set_develop_command_line_flag(bool _flag)
    {
        has_develop_command_line_flag_ = _flag;
    }

    bool configuration::is_valid() const noexcept
    {
        return is_valid_;
    }

    const value_type& configuration::value(values _v) const noexcept
    {
        if (is_external_config_enabled())
        {
            if (const auto external = get_external(); external)
            {
                const auto& values = external->values;
                const auto it = std::find_if(values.begin(), values.end(), [_v](auto x) { return x.first == _v; });
                if (it != values.end())
                    return it->second;
            }
        }

        return c_.values[static_cast<size_t>(_v)].second;
    }

    std::string_view configuration::string(values _v) const noexcept
    {
        if (const auto& v = value(_v); auto ptr = std::get_if<std::string>(&v))
            return std::string_view(*ptr);

        return {};
    }

    std::string_view configuration::url(urls _v) const noexcept
    {
        return c_.urls[static_cast<size_t>(_v)].second;
    }

    std::string_view configuration::translation(translations _v) const noexcept
    {
        return c_.translations[static_cast<size_t>(_v)].second;
    }

    bool configuration::is_on(features _v) const noexcept
    {
        auto default_value = c_.features[static_cast<size_t>(_v)].second;
        if (!is_external_config_enabled() || config::features::external_url_config == _v)
            return default_value;

        if (const auto external = get_external(); external)
        {
            const auto& features = external->features;
            const auto it = std::find_if(features.begin(), features.end(), [_v](auto x) { return x.first == _v; });
            if (it != features.end())
                return it->second;
        }

        return default_value;
    }

    bool configuration::is_debug() const noexcept
    {
        return is_debug_;
    }

    bool configuration::is_develop() const noexcept
    {
        return is_develop_;
    }

    bool configuration::has_develop_cli_flag() const noexcept
    {
        return environment::is_develop() && has_develop_command_line_flag_;
    }

    bool configuration::is_overridden(features _v) const noexcept
    {
        if (!is_external_config_enabled() || config::features::external_url_config == _v)
            return false;

        if (const auto external = get_external())
        {
            const auto& features = external->features;
            return std::any_of(features.begin(), features.end(), [_v](auto x) { return x.first == _v; });
        }

        return false;
    }

    bool configuration::is_overridden(values _v) const noexcept
    {
        if (!is_external_config_enabled())
            return false;

        if (const auto external = get_external(); external)
        {
            const auto& values = external->values;
            return std::any_of(values.begin(), values.end(), [_v](auto x) { return x.first == _v; });
        }
        return false;
    }

    void configuration::set_profile_link(const std::wstring& _profile)
    {
        current_profile_link_ = _profile;
    }

    std::wstring configuration::get_profile_link(bool _secure) const
    {
        if (_secure)
            return L"https://" + current_profile_link_ + L'/';
        else
            return L"http://" + current_profile_link_ + L'/';
    }

    static std::unique_ptr<configuration> g_config;

    static configuration& get_impl()
    {
        if (!g_config)
            g_config = std::make_unique<configuration>(config_json(), false);
        return *g_config;
    }

    const configuration& get()
    {
        return get_impl();
    }

    configuration& get_mutable()
    {
        return get_impl();
    }

    bool try_replace_with_develop_config()
    {
#ifdef SUPPORT_EXTERNAL_CONFIG
        if constexpr (!environment::is_release())
        {
            const auto debug_config_path = core::utils::get_product_data_path();
            boost::system::error_code error_code;
            const auto extrenal_config_file_name = boost::filesystem::canonical(debug_config_path, Out error_code) / L"config.json";

            if (boost::system::errc::success == error_code)
            {
                if (core::tools::binary_stream bstream; bstream.load_from_file(extrenal_config_file_name.wstring()))
                {
                    const auto size = bstream.available();
                    if (auto external_config = std::make_unique<configuration>(std::string_view(bstream.read(size), size_t(size)), true); external_config->is_valid())
                    {
                        external_config->set_develop_command_line_flag(get().has_develop_cli_flag());
                        g_config = std::move(external_config);
                        return true;
                    }
                }
            }
        }
#endif // SUPPORT_EXTERNAL_CONFIG
        return false;
    }

    bool is_overridden(features _v)
    {
        return get().is_overridden(_v);
    }

    bool is_overridden(values _v)
    {
        return get().is_overridden(_v);
    }

    void reset_external()
    {
        set_external({}, false);
    }

    void reset_config()
    {
        g_config.reset();
    }

    void set_external(std::shared_ptr<external_configuration> _f, bool _is_develop)
    {
        get_impl().set_external(std::move(_f), _is_develop);
    }
}
