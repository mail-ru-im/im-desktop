#include "stdafx.h"

#include "../../../core.h"
#include "../../../tools/md5.h"

#include "../wim_packet.h"

#include "preview_proxy.h"

#include "loader_helpers.h"

#include "../common.shared/string_utils.h"

CORE_WIM_NS_BEGIN

namespace
{
    std::string_view get_uri_ext(std::string_view _uri)
    {
        auto ext_end_pos = _uri.size();

        if (const auto last_anchor_pos = _uri.find_last_of('#'); last_anchor_pos != std::string_view::npos)
            ext_end_pos = last_anchor_pos;

        if (const auto last_question_pos = _uri.find_last_of('?'); last_question_pos != std::string_view::npos)
            ext_end_pos = std::min(ext_end_pos, last_question_pos);

        const auto last_slash_pos = _uri.find_last_of('/');
        const auto last_dot_pos = _uri.find_last_of('.');
        const auto is_ext_found = (last_dot_pos != std::string_view::npos) && (last_dot_pos > last_slash_pos);
        if (!is_ext_found)
            return std::string_view();

        const auto ext_begin_pos = last_dot_pos;

        const auto ext_length = ext_end_pos - ext_begin_pos;
        if (const auto is_valid_ext = ext_length <= 5; !is_valid_ext)
            return std::string_view();

        return _uri.substr(ext_begin_pos, ext_length);
    }

    std::string_view get_path_suffix(const path_type _type)
    {
        assert(_type > path_type::min);
        assert(_type < path_type::max);

        switch (_type)
        {
        case path_type::link_preview:
            return "lp";

        case path_type::link_meta:
            return "lm";

        case path_type::file:
            // no suffix for the gentlemen please
            break;

        default:
            assert(!"unexpected path type");
            break;
        }
        return {};
    }

    std::string get_filename_in_cache(std::string_view _uri)
    {
        assert(!_uri.empty());

        return core::tools::md5(
            _uri.data(),
            int32_t(_uri.size()));
    }

}

std::wstring get_path_in_cache(const std::wstring& _cache_dir, std::string_view _uri, const path_type _path_type)
{
    assert(!_cache_dir.empty());
    assert(!_uri.empty());
    assert(_path_type > path_type::min);
    assert(_path_type < path_type::max);

    boost::filesystem::wpath path(_cache_dir);

    std::string filename;
    filename += get_filename_in_cache(_uri);
    filename += get_path_suffix(_path_type);

    const auto append_extension = (_path_type == path_type::file) || (_path_type == path_type::link_preview);
    if (append_extension)
        filename += get_uri_ext(_uri);

    if (_path_type == path_type::link_meta)
    {
        assert(!append_extension);
        filename += ".json";
    }

    path /= filename;

    return path.wstring();
}

preview_proxy::link_meta_uptr load_link_meta_from_file(const std::wstring &_path, const std::string &_uri)
{
    assert(!_path.empty());
    assert(!_uri.empty());

    tools::binary_stream json_file;
    if (!json_file.load_from_file(_path))
    {
        return nullptr;
    }

    const auto file_size = json_file.available();
    if (file_size == 0)
    {
        return nullptr;
    }

    const auto json_str = (char*)json_file.read(file_size);

    std::vector<char> json;
    json.reserve(file_size + 1);

    json.assign(json_str, json_str + file_size);
    json.push_back('\0');

    return preview_proxy::parse_json(json.data(), _uri);
}

std::string sign_loader_uri(std::string_view _host, const wim_packet_params &_params, str_2_str_map _extra)
{
    assert(!_host.empty());

    str_2_str_map p;

    // todo: remove when /misc/preview/getPreview will work without 'a'
    const time_t ts = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now()) - _params.time_offset_;
    p["a"] = _params.a_token_;
    p["ts"] = std::to_string(ts);
    // !!!

    p["aimsid"] = wim_packet::escape_symbols(_params.aimsid_);
    p["k"] = _params.dev_id_;
    p["client"] = "icq";

    p.merge(std::move(_extra));

    return su::concat(_host, '?', wim_packet::format_get_params(p));
}

CORE_WIM_NS_END