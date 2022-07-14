#pragma once

#include "../../../namespaces.h"

namespace core::tools
{
    class filesharing_id;
}

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

struct file_key
{
    explicit file_key() = default;
    explicit file_key(std::string_view _url);
    explicit file_key(const core::tools::filesharing_id& _fs_id);
    file_key(const file_key& _other);
    file_key(file_key&& _other) noexcept;
    file_key& operator=(const file_key& _other);
    file_key& operator=(file_key&& _other) noexcept;
    const std::string& to_string() const;
    bool is_url() const noexcept { return type::url == type_; }
    bool is_valid() const noexcept { return type::unknown != type_; }

private:
    enum class type
    {
        unknown,
        url,
        filesharing_id
    };

    std::string key_;
    type type_ = type::unknown;
};

struct cancelled_tasks
{
    bool remove(const file_key& _key);
    bool remove(const file_key& _key, int64_t _seq);
    void add(const file_key& _key, int64_t _seq);
    bool contains(const file_key& _key, int64_t _seq) const;

private:
    std::unordered_map<std::string, std::vector<int64_t>> tasks_;
    mutable std::mutex mutex_;
};

struct wim_packet_params;

std::wstring get_path_in_cache(const std::wstring& _cache_dir, const file_key& _key, const path_type _path_type);

preview_proxy::link_meta_uptr load_link_meta_from_file(const std::wstring& _path);

std::string sign_loader_uri(std::string_view _host, const wim_packet_params& _params, str_2_str_map _extra);

CORE_WIM_NS_END
