#include "stdafx.h"
#include "features.h"

#include "../../common.shared/config/config.h"
#include "../../common.shared/omicron_keys.h"
#include "../../common.shared/string_utils.h"

#include "../zstd_wrap/zstd_dicts/internal_request_dict.h"
#include "../zstd_wrap/zstd_dicts/internal_response_dict.h"

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
        static const auto default_value = config::get().is_on(config::features::zstd_request_enabled);
        return omicronlib::_o(omicron::keys::im_zstd_dict_request_enabled, default_value);
    }

    bool is_zstd_response_enabled()
    {
        static const auto default_value = config::get().is_on(config::features::zstd_response_enabled);
        return omicronlib::_o(omicron::keys::im_zstd_dict_response_enabled, default_value);
    }

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

    bool is_fetch_hotstart_enabled()
    {
        return omicronlib::_o(omicron::keys::fetch_hotstart_enabled, true);
    }
}
