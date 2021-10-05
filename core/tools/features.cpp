#include "stdafx.h"
#include "features.h"

#include "../../common.shared/config/config.h"
#include "../../common.shared/omicron_keys.h"
#include "../../common.shared/string_utils.h"

#ifndef STRIP_ZSTD
#include "../zstd_wrap/zstd_dicts/internal_request_dict.h"
#include "../zstd_wrap/zstd_dicts/internal_response_dict.h"
#endif // !STRIP_ZSTD

#include "../../libomicron/include/omicron/omicron.h"

#include "../configuration/app_config.h"

namespace
{
    bool myteam_config_or_omicron_feature_enabled(config::features _feature, const char* _omicron_key)
    {
        const auto config_value = config::get().is_on(_feature);
        const auto omicron_value = omicronlib::_o(_omicron_key, config_value);
        return config::is_overridden(_feature) ? config_value || omicron_value : omicron_value;
    }
}

namespace features
{
    int get_zstd_compression_level()
    {
        static const auto default_value = config::get().number<int64_t>(config::values::zstd_compression_level).value_or(feature::default_im_zstd_compression_level());
        return omicronlib::_o(omicron::keys::im_zstd_compression_level, default_value);
    }

    bool is_zstd_request_enabled()
    {
#ifndef STRIP_ZSTD
        const auto default_value = config::get().is_on(config::features::zstd_request_enabled) && core::configuration::get_app_config().is_net_compression_enabled();
        return omicronlib::_o(omicron::keys::im_zstd_dict_request_enabled, default_value);
#else
        return false;
#endif // !STRIP_ZSTD
    }

    bool is_zstd_response_enabled()
    {
#ifndef STRIP_ZSTD
        const auto default_value = config::get().is_on(config::features::zstd_response_enabled) && core::configuration::get_app_config().is_net_compression_enabled();
        return omicronlib::_o(omicron::keys::im_zstd_dict_response_enabled, default_value);
#else
        return false;
#endif // !STRIP_ZSTD
    }

#ifndef STRIP_ZSTD
    std::string get_zstd_dict_request()
    {
        static const auto default_link = []() -> std::string {
            const auto link = config::get().url(config::urls::zstd_request_dict);
            return !link.empty() ? su::concat("https://", link) : zstd_internal::get_request_dict_name();
        }();
        return omicronlib::_o(omicron::keys::im_zstd_dict_request, default_link);
    }

    std::string get_zstd_dict_response()
    {
        static const auto default_link = []() -> std::string {
            const auto link = config::get().url(config::urls::zstd_response_dict);
            return !link.empty() ? su::concat("https://", link) : zstd_internal::get_response_dict_name();
        }();
        return omicronlib::_o(omicron::keys::im_zstd_dict_response, default_link);
    }
#endif // !STRIP_ZSTD

    bool smartreply_suggests_stickers_enabled()
    {
        return myteam_config_or_omicron_feature_enabled(config::features::smartreply_suggests_stickers, omicron::keys::smartreply_suggests_stickers);
    }

    bool smartreply_suggests_text_enabled()
    {
        return myteam_config_or_omicron_feature_enabled(config::features::smartreply_suggests_text, omicron::keys::smartreply_suggests_text);
    }

    bool is_fetch_hotstart_enabled()
    {
        return omicronlib::_o(omicron::keys::fetch_hotstart_enabled, true);
    }

    bool is_statistics_mytracker_enabled()
    {
        static const auto default_value = config::get().is_on(config::features::statistics_mytracker);
        return omicronlib::_o(omicron::keys::statistics_mytracker, default_value);
    }

    bool is_dns_workaround_enabled()
    {
        static const auto default_value = config::get().is_on(config::features::dns_workaround);
        return omicronlib::_o(omicron::keys::dns_workaround_option, default_value);
    }

    bool is_fallback_to_ip_mode_enabled()
    {
        return omicronlib::_o(omicron::keys::fallback_to_ip_mode_enabled, true);
    }

    bool is_webp_preview_accepted()
    {
        return omicronlib::_o(omicron::keys::webp_preview_accepted, true);
    }

    bool is_webp_original_accepted()
    {
        return omicronlib::_o(omicron::keys::webp_original_accepted, true);
    }

    bool is_update_from_backend_enabled()
    {
        return config::get().is_on(config::features::update_from_backend);
    }

    bool is_notify_network_change_enabled()
    {
        return omicronlib::_o(omicron::keys::notify_network_change, true);
    }

    bool is_invite_by_sms_enabled()
    {
        return omicronlib::_o(omicron::keys::invite_by_sms, config::get().is_on(config::features::invite_by_sms));
    }

    bool should_show_sms_notify_setting()
    {
        return omicronlib::_o(omicron::keys::show_sms_notify_setting, config::get().is_on(config::features::show_sms_notify_setting));
    }

    size_t get_max_parallel_sticker_downloads()
    {
        return omicronlib::_o(omicron::keys::max_parallel_sticker_downloads, 10);
    }

    bool is_silent_delete_enabled()
    {
        return myteam_config_or_omicron_feature_enabled(config::features::silent_message_delete, omicron::keys::silent_message_delete);
    }

    bool is_url_ftp_protocols_allowed()
    {
        static const auto default_value = config::get().is_on(config::features::url_ftp_protocols_allowed);
        return omicronlib::_o(omicron::keys::url_ftp_protocols_allowed, default_value);
    }

    bool is_draft_enabled()
    {
        return omicronlib::_o(omicron::keys::draft_enabled, config::get().is_on(config::features::draft_enabled));
    }

    int draft_maximum_length()
    {
        const auto default_value = config::get().number<int64_t>(config::values::draft_max_len).value_or(feature::default_draft_max_len());
        return omicronlib::_o(omicron::keys::draft_max_len, default_value);
    }

    std::chrono::seconds get_link_metainfo_repeat_interval()
    {
        return std::chrono::seconds(omicronlib::_o(omicron::keys::link_metainfo_repeat_interval, 60));
    }

    std::chrono::milliseconds get_metainfo_repeat_interval()
    {
        return std::chrono::milliseconds(omicronlib::_o(omicron::keys::metainfo_repeat_interval, 1000));
    }

    std::chrono::milliseconds get_preview_repeat_interval()
    {
        return std::chrono::milliseconds(omicronlib::_o(omicron::keys::preview_repeat_interval, 1000));
    }

    std::chrono::seconds get_oauth2_refresh_interval()
    {
        return std::chrono::seconds(omicronlib::_o(omicron::keys::oauth2_refresh_interval, 604800)); // 604800 secs = 7 days
    }

    bool is_oauth2_login_allowed()
    {
        return omicronlib::_o(omicron::keys::login_by_oauth2_allowed, config::get().is_on(config::features::login_by_oauth2_allowed));
    }

    std::chrono::seconds task_cache_lifetime()
    {
        constexpr std::chrono::seconds default_duration = std::chrono::hours(24*7);
        return std::chrono::seconds(omicronlib::_o(omicron::keys::task_cache_lifetime, default_duration.count()));
    }

    bool is_threads_enabled()
    {
        return myteam_config_or_omicron_feature_enabled(config::features::threads_enabled, omicron::keys::threads_enabled);
    }

    bool is_restricted_files_enabled()
    {
        return myteam_config_or_omicron_feature_enabled(config::features::restricted_files_enabled, omicron::keys::restricted_files_enabled);
    }

    bool trust_status_default()
    {
        return !is_restricted_files_enabled();
    }

    bool is_antivirus_check_enabled()
    {
        return myteam_config_or_omicron_feature_enabled(config::features::antivirus_check_enabled, omicron::keys::antivirus_check_enabled);
    }
}
