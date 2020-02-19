#pragma once

#include "../../../namespaces.h"

CORE_WIM_PREVIEW_PROXY_NS_BEGIN

using link_meta_uptr = std::unique_ptr<class link_meta>;

using int32_t_opt = std::optional<int32_t>;

using preview_size = std::tuple<int32_t, int32_t>;

using origin_size = std::tuple<int32_t, int32_t>;

class link_meta final
{
public:
    link_meta(
        const std::string &_title,
        const std::string &_annotation,
        const std::string &_preview_uri,
        const std::string &_download_uri,
        const std::string &_favicon_uri,
        const std::string &_site_name,
        const std::string &_content_type,
        const preview_size &_preview_size,
        const int64_t _file_size,
        const origin_size& _origin_size,
        const std::string& _filename);

    ~link_meta();

    std::string get_preview_uri(
        const int32_t_opt _width = int32_t_opt(),
        const int32_t_opt _height = int32_t_opt()) const;

    const std::string& get_annotation() const;

    const std::string& get_content_type() const;

    const std::string& get_download_uri() const;

    const std::string& get_favicon_uri() const;

    int64_t get_file_size() const;

    preview_size get_preview_size() const;

    origin_size get_origin_size() const;

    const std::string& get_site_name() const;

    const std::string& get_title() const;

    const std::string& get_filename() const;

    bool has_favicon_uri() const;

    bool has_preview_uri() const;

private:
    std::string preview_uri_;

    std::string download_uri_;

    std::string title_;

    std::string annotation_;

    std::string favicon_uri_;

    std::string site_name_;

    std::string content_type_;

    preview_size preview_size_;

    int64_t file_size_;

    origin_size origin_size_;

    std::string filename_;
};

enum class favicon_size
{
    undefined,

    min,

    small,
    med,

    max,
};

str_2_str_map format_get_preview_params(
    std::string_view _uri_to_preview,
    const int32_t_opt _width = int32_t_opt(),
    const int32_t_opt _height = int32_t_opt(),
    const bool _crop = false,
    const favicon_size _favicon_size = favicon_size::small);

str_2_str_map format_get_url_content_params(std::string_view _uri);

link_meta_uptr parse_json(InOut char *_json, const std::string &_uri);

namespace uri
{
    std::string get_preview();
    std::string_view get_url_content();
}

CORE_WIM_PREVIEW_PROXY_NS_END
