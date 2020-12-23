#include "stdafx.h"

#include "config.h"

#ifdef SUPPORT_EXTERNAL_CONFIG

#include "../core/tools/strings.h"
#include "../core/tools/binary_stream.h"
#include "../core/tools/system.h"
#include "../core/utils.h"
#include "../common.h"

#include <boost/filesystem.hpp>

#endif // SUPPORT_EXTERNAL_CONFIG

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
                std::pair(features::forgot_password, get_bool(it->value, "forgot_password")),
                std::pair(features::explained_forgot_password, get_bool(it->value, "explained_forgot_password")),
                std::pair(features::unknown_contacts, get_bool(it->value, "unknown_contacts")),
                std::pair(features::stranger_contacts, get_bool(it->value, "stranger_contacts")),
                std::pair(features::otp_login, get_bool(it->value, "otp_login")),
                std::pair(features::show_notification_text, get_bool(it->value, "show_notification_text")),
                std::pair(features::changeable_name, get_bool(it->value, "changeable_name")),
                std::pair(features::avatar_change_allowed, get_bool(it->value, "avatar_change_allowed")),
                std::pair(features::beta_update, get_bool(it->value, "beta_update")),
                std::pair(features::ssl_verify_peer, get_bool(it->value, "ssl_verify_peer")),
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
                std::pair(features::external_config_use_preset_url, get_bool(it->value, "external_config_use_preset_url")),
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
                std::pair(values::client_rapi, get_string(it->value, "client_rapi")),
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
                std::pair(values::updater_main_instance_mutex_win, get_string(it->value, "updater_main_instance_mutex_win")),
                std::pair(values::company_name, get_string(it->value, "company_name")),
                std::pair(values::app_user_model_win, get_string(it->value, "app_user_model_win")),
                std::pair(values::flurry_key, get_string(it->value, "flurry_key")),
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
                std::pair(values::external_config_preset_url, get_string(it->value, "external_config_preset_url")),
                std::pair(values::server_api_version, get_int64(it->value, "server_api_version")),
                std::pair(values::server_mention_timeout, get_int64(it->value, "server_mention_timeout")),
                std::pair(values::support_uin, get_string(it->value, "support_uin")),
                std::pair(values::mytracker_app_id_win, get_string(it->value, "mytracker_app_id_win")),
                std::pair(values::mytracker_app_id_mac, get_string(it->value, "mytracker_app_id_mac")),
                std::pair(values::mytracker_app_id_linux, get_string(it->value, "mytracker_app_id_linux")),
            };

            if (std::is_sorted(std::cbegin(res), std::cend(res), is_less_by_first))
                return std::make_optional(std::move(res));
            assert(false);
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

    configuration::configuration() = default;

    std::shared_ptr<external_configuration> configuration::get_external() const
    {
        std::scoped_lock lock(*spin_lock_);
        return e_;
    }

    void configuration::set_external(std::shared_ptr<external_configuration> _e)
    {
        std::scoped_lock lock(*spin_lock_);
        e_ = std::move(_e);
    }

    configuration::~configuration() = default;

    bool configuration::is_valid() const noexcept
    {
        return is_valid_;
    }

    const value_type& configuration::value(values _v) const noexcept
    {
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
        if (!c_.features[static_cast<size_t>(config::features::external_url_config)].second || config::features::external_url_config == _v)
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

    bool configuration::is_overridden(features _v) const noexcept
    {
        if (!c_.features[static_cast<size_t>(config::features::external_url_config)].second || config::features::external_url_config == _v)
            return false;

        if (const auto external = get_external(); external)
        {
            const auto& features = external->features;
            return std::any_of(features.begin(), features.end(), [_v](auto x) { return x.first == _v; });
        }

        return false;
    }

    static configuration make_config()
    {
        auto local_config = configuration(config_json(), false);

#ifdef SUPPORT_EXTERNAL_CONFIG
        if (auto v = local_config.value(platform::is_apple() ? values::product_path_mac : values::product_path); auto path = std::get_if<std::string>(&v))
        {
            auto debug_config_path = core::utils::get_product_data_path(core::tools::from_utf8(*path));

            boost::system::error_code error_code;
            const auto extrenal_config_file_name = boost::filesystem::canonical(debug_config_path, Out error_code) / L"config.json";

            if (error_code == boost::system::errc::success)
            {
                if (core::tools::binary_stream bstream; bstream.load_from_file(extrenal_config_file_name.wstring()))
                {
                    const auto size = bstream.available();
                    if (auto extrernal_config = configuration(std::string_view(bstream.read(size), size_t(size)), true); extrernal_config.is_valid())
                        return extrernal_config;
                }
            }
        }
#endif // SUPPORT_EXTERNAL_CONFIG

        return local_config;
    }
}

static config::configuration& get_impl()
{
    static auto c = config::make_config();
    return c;
}

const config::configuration& config::get()
{
    return get_impl();
}

bool config::is_overridden(features _v)
{
    return config::get().is_overridden(_v);
}

void config::reset_external()
{
    set_external({});
}

void config::set_external(std::shared_ptr<external_configuration> _f)
{
    get_impl().set_external(std::move(_f));
}
