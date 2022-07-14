#include "stdafx.h"

#include "../../../core.h"
#include "../../../tools/md5.h"
#include "../../../tools/file_sharing.h"

#include "../wim_packet.h"

#include "preview_proxy.h"

#include "loader_helpers.h"

#include "../common.shared/string_utils.h"

CORE_WIM_NS_BEGIN

namespace
{
    std::string get_uri_ext(const file_key& _file_key)
    {
        if (!_file_key.is_url())
            return {};

        const auto& uri = _file_key.to_string();

        auto ext_end_pos = uri.size();

        if (const auto last_anchor_pos = uri.find_last_of('#'); last_anchor_pos != std::string_view::npos)
            ext_end_pos = last_anchor_pos;

        if (const auto last_question_pos = uri.find_last_of('?'); last_question_pos != std::string_view::npos)
            ext_end_pos = std::min(ext_end_pos, last_question_pos);

        const auto last_slash_pos = uri.find_last_of('/');
        const auto last_dot_pos = uri.find_last_of('.');
        const auto is_ext_found = (last_dot_pos != std::string_view::npos) && (last_dot_pos > last_slash_pos);
        if (!is_ext_found)
            return {};

        const auto ext_begin_pos = last_dot_pos;

        const auto ext_length = ext_end_pos - ext_begin_pos;
        if (const auto is_valid_ext = ext_length <= 5; !is_valid_ext)
            return {};

        return uri.substr(ext_begin_pos, ext_length);
    }

    std::string_view get_path_suffix(const path_type _type)
    {
        im_assert(_type > path_type::min);
        im_assert(_type < path_type::max);

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
            im_assert(!"unexpected path type");
            break;
        }
        return {};
    }

    std::string get_filename_in_cache(const file_key& _file_key)
    {
        const auto& key_string = _file_key.to_string();
        im_assert(!key_string.empty());

        return core::tools::md5(key_string.data(), key_string.size());
    }

} // namespace

file_key::file_key(std::string_view _url)
    : key_(_url)
    , type_(type::url)
{
}

file_key::file_key(const core::tools::filesharing_id& _fs_id)
    : key_(_fs_id.full_id())
    , type_{ type::filesharing_id }
{
}

file_key::file_key(const file_key& _other)
    : key_(_other.key_)
    , type_{ _other.type_ }
{
}

file_key::file_key(file_key&& _other) noexcept
    : key_(std::move(_other.key_))
    , type_{ _other.type_ }
{
    _other.type_ = type::unknown;
}

file_key& file_key::operator=(const file_key& _other)
{
    if (std::addressof(_other) == this)
        return *this;
    key_ = _other.key_;
    type_ = _other.type_;
    return *this;
}

file_key& file_key::operator=(file_key&& _other) noexcept
{
    if (std::addressof(_other) == this)
        return *this;
    key_ = std::move(_other.key_);
    type_ = std::exchange(_other.type_, type::unknown);
    return *this;
}

const std::string& file_key::to_string() const
{
    im_assert(is_valid());
    return key_;
}

bool cancelled_tasks::remove(const file_key& _key)
{
    using it_type = decltype(tasks_)::const_iterator;
    decltype(tasks_.extract(it_type())) to_erase;
    std::scoped_lock lock(mutex_);
    if (auto it = tasks_.find(_key.to_string()); it != tasks_.end())
    {
        to_erase = tasks_.extract(it);
        return true;
    }
    return false;
}

bool cancelled_tasks::remove(const file_key& _key, int64_t _seq)
{
    using it_type = decltype(tasks_)::const_iterator;
    decltype(tasks_.extract(it_type())) to_erase;
    std::scoped_lock lock(mutex_);
    if (const auto it = tasks_.find(_key.to_string()); it != tasks_.end())
    {
        auto& seqs = it->second;
        const auto r = std::remove_if(seqs.begin(), seqs.end(), [_seq](auto x) { return x == _seq; });
        if (r != seqs.end())
        {
            seqs.erase(r, seqs.end());
            if (seqs.empty())
                to_erase = tasks_.extract(it);
            return true;
        }
    }
    return false;
}

void cancelled_tasks::add(const file_key& _key, int64_t _seq)
{
    std::scoped_lock lock(mutex_);
    auto& node = tasks_[_key.to_string()];
    node.push_back(_seq);
}

bool cancelled_tasks::contains(const file_key& _key, int64_t _seq) const
{
    std::scoped_lock lock(mutex_);
    if (const auto it = tasks_.find(_key.to_string()); it != tasks_.end())
    {
        const auto& seqs = it->second;
        return std::any_of(seqs.begin(), seqs.end(), [_seq](auto x) { return x == _seq; });
    }
    return false;
}

std::wstring get_path_in_cache(const std::wstring& _cache_dir, const file_key& _key, const path_type _path_type)
{
    im_assert(!_cache_dir.empty());
    im_assert(!_key.to_string().empty());
    im_assert(_path_type > path_type::min);
    im_assert(_path_type < path_type::max);

    boost::filesystem::wpath path(_cache_dir);

    std::string filename;
    filename += get_filename_in_cache(_key);
    filename += get_path_suffix(_path_type);

    const auto append_extension = (_path_type == path_type::file) || (_path_type == path_type::link_preview);
    if (append_extension)
        filename += get_uri_ext(_key);

    if (_path_type == path_type::link_meta)
    {
        im_assert(!append_extension);
        filename += ".json";
    }

    path /= filename;

    return path.wstring();
}

preview_proxy::link_meta_uptr load_link_meta_from_file(const std::wstring& _path)
{
    im_assert(!_path.empty());

    tools::binary_stream json_file;
    if (!json_file.load_from_file(_path))
        return nullptr;

    const auto file_size = json_file.available();
    if (0 == file_size)
        return nullptr;

    const auto json_str = (char*)json_file.read(file_size);

    std::vector<char> json;
    json.reserve(file_size + 1);

    json.assign(json_str, json_str + file_size);
    json.push_back('\0');

    return preview_proxy::parse_json(json.data());
}

std::string sign_loader_uri(std::string_view _host, const wim_packet_params& _params, str_2_str_map _extra)
{
    im_assert(!_host.empty());

    str_2_str_map p;

    p["aimsid"] = wim_packet::escape_symbols(_params.aimsid_);
    p["k"] = _params.dev_id_; // TODO maybe drop
    p["client"] = "icq"; // TODO maybe drop

    p.merge(std::move(_extra));

    return su::concat(_host, '?', wim_packet::format_get_params(p));
}

CORE_WIM_NS_END
