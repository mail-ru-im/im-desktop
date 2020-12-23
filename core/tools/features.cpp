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
        static const auto default_value = config::get().is_on(config::features::zstd_request_enabled);
        return omicronlib::_o(omicron::keys::im_zstd_dict_request_enabled, default_value);
#else
        return false;
#endif // !STRIP_ZSTD
    }

    bool is_zstd_response_enabled()
    {
#ifndef STRIP_ZSTD
        static const auto default_value = config::get().is_on(config::features::zstd_response_enabled);
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
        return omicronlib::_o(omicron::keys::webp_preview_accepted, false);
    }

    bool is_webp_original_accepted()
    {
        return omicronlib::_o(omicron::keys::webp_original_accepted, false);
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
        auto value = config::get().is_on(config::features::silent_message_delete);
        value = config::is_overridden(config::features::silent_message_delete) ? value : omicronlib::_o(omicron::keys::silent_message_delete, value);
        return value;
    }
}
