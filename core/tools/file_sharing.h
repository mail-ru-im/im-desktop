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

struct filesharing_id
{
    std::string file_id_;
    std::optional<std::string> source_id_;

private:
    friend bool operator==(const filesharing_id& _lhs, const filesharing_id& _rhs)
    {
        return _lhs.file_id_ == _rhs.file_id_ && _lhs.source_id_ == _rhs.source_id_;
    }
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