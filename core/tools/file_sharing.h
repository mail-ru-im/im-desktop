#pragma once

#include "../../corelib/enumerations.h"

#include "../namespaces.h"

CORE_TOOLS_NS_BEGIN

// https://confluence.mail.ru/pages/viewpage.action?pageId=161199966
enum class filesharing_preview_size
{
    min,

    px192, // 192x192

    px400, // 400x400
    small = px400,

    px600, // 600x600
    medium = px600,

    px800, // 800x800
    large = px800,

    px1024, // 1024x1024
    xlarge = px1024,

    max
};

struct filesharing_preview_size_info
{
    filesharing_preview_size size_;
    int32_t max_side_;
    std::string_view url_path_;
};

class filesharing_id
{
public:
    filesharing_id() = default;
    explicit filesharing_id(std::string_view _file_id, std::string_view _source_id = {});
    const std::string& full_id() const { return full_id_; }
    std::string_view file_id() const { return file_id_; }
    const std::optional<std::string_view>& source_id() const { return source_id_; }

    bool is_empty() const { return file_id_.empty(); }
    void set_source_id(std::string_view _source_id);

    filesharing_id(const filesharing_id& _other);
    filesharing_id(filesharing_id&& _other) noexcept;

    filesharing_id& operator=(const filesharing_id& _other);
    filesharing_id& operator=(filesharing_id&& _other) noexcept;

    static filesharing_id from_filesharing_uri(std::string_view _uri);

    std::string file_hash_for_subscription() const;
    std::string file_url_part() const;

private:
    friend bool operator==(const filesharing_id& _lhs, const filesharing_id& _rhs)
    {
        return _lhs.full_id_ == _rhs.full_id_;
    }

private:
    std::string full_id_;
    std::string_view file_id_;
    std::optional<std::string_view> source_id_;
};

const std::vector<filesharing_preview_size_info>& get_available_fs_preview_sizes();

std::string format_file_sharing_preview_uri(const filesharing_id& _id, const filesharing_preview_size _size);

bool get_content_type_from_uri(std::string_view _uri, Out core::file_sharing_content_type& _type);

bool get_content_type_from_file_sharing_id(std::string_view _file_id, Out core::file_sharing_content_type& _type);

std::optional<core::file_sharing_content_type> get_content_type_from_file_sharing_id(std::string_view _file_id);

std::optional<filesharing_id> parse_new_file_sharing_uri(std::string_view _uri);

std::string_view get_file_id(std::string_view _uri);

std::optional<std::string_view> get_source_id(std::string_view _uri);

CORE_TOOLS_NS_END