#include "stdafx.h"

#include "strings.h"

#include "file_sharing.h"

#include "../../common.shared/url_parser/url_parser.h"
#include "../../corelib/collection_helper.h"
#include "../configuration/app_config.h"

CORE_TOOLS_NS_BEGIN

using namespace boost::xpressive;

namespace
{
    #define NET_URI_PREFIX "^http(s?)://files\\.icq\\.net/get"

    #define COM_URI_PREFIX "^http(s?)://icq\\.com/files"

    const auto NEW_ID_LENGTH_MIN = 33;
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
    if (_id.length() < NEW_ID_LENGTH_MIN)
        return result;

    const auto& infos = get_available_fs_preview_sizes();
    const auto it = std::find_if(infos.begin(), infos.end(), [_size](const auto& _i) { return _i.size_ == _size; });
    if (it == infos.end())
    {
        assert(!"unknown preview size");
        return result;
    }

    result += std::string_view("https://");
    result += core::configuration::get_app_config().get_url_files();
    result += std::string_view("/preview/max/");
    result += it->url_path_;
    result += '/';
    result += _id;
    return result;
}

bool get_content_type_from_uri(const std::string& _uri, Out core::file_sharing_content_type& _type)
{
    std::string id;

    if (!tools::parse_new_file_sharing_uri(_uri, Out id))
        return false;

    return get_content_type_from_file_sharing_id(id, Out _type);
}

bool get_content_type_from_file_sharing_id(const std::string& _file_id, Out core::file_sharing_content_type& _type)
{
    assert(!_file_id.empty());

    _type.type_ = core::file_sharing_base_content_type::undefined;
    _type.subtype_ = core::file_sharing_sub_content_type::undefined;

    const auto id0 = _file_id[0];

    const auto is_gif = ((id0 >= '4') && (id0 <= '5'));
    if (is_gif)
    {
        _type.type_ = core::file_sharing_base_content_type::gif;

        if (id0 == '5')
            _type.subtype_ = core::file_sharing_sub_content_type::sticker;

        return true;
    }

    const auto is_image = ((id0 >= '0') && (id0 <= '7'));
    if (is_image)
    {
        _type.type_ = core::file_sharing_base_content_type::image;

        if (id0 == '2')
            _type.subtype_ = core::file_sharing_sub_content_type::sticker;

        return true;
    }

    const auto is_video = (
        ((id0 >= '8') && (id0 <= '9')) ||
        ((id0 >= 'A') && (id0 <= 'F')));
    if (is_video)
    {
        _type.type_ = core::file_sharing_base_content_type::video;

        if (id0 == 'D')
            _type.subtype_ = core::file_sharing_sub_content_type::sticker;

        return true;
    }

    const auto is_ptt = (
        ((id0 >= 'I') && (id0 <= 'J')));
    if (is_ptt)
    {
        _type.type_ = core::file_sharing_base_content_type::ptt;
        return true;
    }

    return false;
}

bool parse_new_file_sharing_uri(const std::string &_uri, Out std::string &_fileId)
{
    assert(!_uri.empty());

    common::tools::url_parser parser(core::configuration::get_app_config().get_url_files_get());
    for (size_t i = 0; i < _uri.length(); ++i)
        parser.process(_uri[i]);

    parser.finish();
    if (parser.has_url() && parser.get_url().is_filesharing())
    {
        _fileId = get_file_id(_uri);
        return true;
    }

    return false;
}

std::string_view get_file_id(std::string_view _uri)
{
    const auto id = core::tools::trim_right(_uri, '/');

    if (const auto pos = id.rfind('/'); std::string_view::npos != pos)
        return id.substr(pos + 1);

    return std::string_view();
}

CORE_TOOLS_NS_END