#pragma once

#include "../../../namespaces.h"

CORE_WIM_PREVIEW_PROXY_NS_BEGIN

class link_meta;

typedef std::unique_ptr<link_meta> link_meta_uptr;

CORE_WIM_PREVIEW_PROXY_NS_END

CORE_WIM_NS_BEGIN

enum class path_type
{
    min,

    link_preview,
    link_meta,
    file,

    max
};

struct wim_packet_params;

std::wstring get_path_in_cache(const std::wstring& _cache_dir, std::string_view _uri, const path_type _path_type);

preview_proxy::link_meta_uptr load_link_meta_from_file(const std::wstring &_path, const std::string &_url);

std::string sign_loader_uri(std::string_view _host, const wim_packet_params &_params, str_2_str_map _extra);

CORE_WIM_NS_END