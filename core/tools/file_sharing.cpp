#include "stdafx.h"

#include "strings.h"

#include "file_sharing.h"

#include "../../common.shared/url_parser/url_parser.h"
#include "../../corelib/collection_helper.h"
#include "../configuration/host_config.h"
#include "../common.shared/config/config.h"
#include "../common.shared/string_utils.h"
#include "connections/urls_cache.h"

#include <boost/algorithm/string.hpp>

CORE_TOOLS_NS_BEGIN

namespace
{
    constexpr size_t new_id_min_size() noexcept { return 33; }
}

const std::vector<filesharing_preview_size_info>& get_available_fs_preview_sizes()
{
    static const std::vector<filesharing_preview_size_info> infos =
    {
        { filesharing_preview_size::px192, 192, "192" },
        { filesharing_preview_size::px400, 400, "400" },
        { filesharing_preview_size::px600, 600, "600" },
        { filesharing_preview_size::px800, 800, "800" },
        { filesharing_preview_size::px1024, 1024, "xlarge" },
    };

    return infos;
}

std::string format_file_sharing_preview_uri(const std::string_view _id, const filesharing_preview_size _size)
{
    assert(!_id.empty());
    assert(_size > filesharing_preview_size::min);
    assert(_size < filesharing_preview_size::max);

    std::string result;
    if (_id.size() < new_id_min_size())
        return result;

    const auto& infos = get_available_fs_preview_sizes();
    const auto it = std::find_if(infos.begin(), infos.end(), [_size](const auto& _i) { return _i.size_ == _size; });
    if (it == infos.end())
    {
        assert(!"unknown preview size");
        return result;
    }

    return su::concat(urls::get_url(urls::url_type::files_preview), "/max/", it->url_path_, '/', _id);
}

bool get_content_type_from_uri(std::string_view _uri, Out core::file_sharing_content_type& _type)
{
    if (auto id = tools::parse_new_file_sharing_uri(_uri); id)
        return get_content_type_from_file_sharing_id(*id, Out _type);

    return false;
}

bool get_content_type_from_file_sharing_id(std::string_view _file_id, Out core::file_sharing_content_type& _type)
{
    if (auto res = get_content_type_from_file_sharing_id(_file_id))
    {
        _type = *res;
        return true;
    }
    _type.type_ = core::file_sharing_base_content_type::undefined;
    _type.subtype_ = core::file_sharing_sub_content_type::undefined;
    return false;
}

std::optional<core::file_sharing_content_type> get_content_type_from_file_sharing_id(std::string_view _file_id)
{
    assert(!_file_id.empty());
    core::file_sharing_content_type type;

    const auto id0 = _file_id[0];

    const auto is_gif = ((id0 >= '4') && (id0 <= '5'));
    if (is_gif)
    {
        type.type_ = core::file_sharing_base_content_type::gif;

        if (id0 == '5')
            type.subtype_ = core::file_sharing_sub_content_type::sticker;

        return type;
    }

    const auto is_image = ((id0 >= '0') && (id0 <= '7'));
    if (is_image)
    {
        type.type_ = core::file_sharing_base_content_type::image;

        if (id0 == '2')
            type.subtype_ = core::file_sharing_sub_content_type::sticker;

        return type;
    }

    const auto is_video = (
        ((id0 >= '8') && (id0 <= '9')) ||
        ((id0 >= 'A') && (id0 <= 'F')));
    if (is_video)
    {
        type.type_ = core::file_sharing_base_content_type::video;

        if (id0 == 'D')
            type.subtype_ = core::file_sharing_sub_content_type::sticker;

        return type;
    }

    const auto is_ptt = (
        ((id0 >= 'I') && (id0 <= 'J')));
    if (is_ptt)
    {
        type.type_ = core::file_sharing_base_content_type::ptt;
        return type;
    }

    return std::nullopt;
}

std::optional<std::string_view> parse_new_file_sharing_uri(std::string_view _uri)
{
    assert(!_uri.empty());

    const static auto additional_urls = []() {
        std::vector<std::string> urls;
        if (const auto str = config::get().string(config::values::additional_fs_parser_urls_csv); !str.empty())
        {
            const auto copy = std::string(str);
            boost::split(urls, copy, [](char c) { return c == ','; });
            urls.erase(std::remove_if(urls.begin(), urls.end(), [](const auto& x) { return x.empty(); }), urls.end());
        }
        return urls;
    }();

    common::tools::url_parser parser(config::hosts::get_host_url(config::hosts::host_url_type::files_parse), additional_urls);
    for (auto c : _uri)
        parser.process(c);

    parser.finish();
    if (parser.has_url() && parser.get_url().is_filesharing())
        return get_file_id(_uri);

    return std::nullopt;
}

std::string_view get_file_id(std::string_view _uri)
{
    const auto id = core::tools::trim_right(_uri, '/');

    if (const auto pos = id.rfind('/'); std::string_view::npos != pos)
        return id.substr(pos + 1);

    return std::string_view();
}

CORE_TOOLS_NS_END
