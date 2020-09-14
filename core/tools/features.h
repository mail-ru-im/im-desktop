#pragma once

namespace features
{
    int get_zstd_compression_level();
    bool is_zstd_request_enabled();
    bool is_zstd_response_enabled();
    std::string get_zstd_dict_request();
    std::string get_zstd_dict_response();

    bool is_fetch_hotstart_enabled();
    bool is_statistics_mytracker_enabled();
    bool is_dns_workaround_enabled();
}
