#include "stdafx.h"

#include "strings.h"

#include "file_sharing.h"

#include "../../common.shared/uri_matcher/uri_matcher.h"
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

std::string filesharing_id::file_hash_for_subscription() const
{
    return std::string{ source_id_ ? su::concat(file_id_, "#", *source_id_) : file_id_ };
}

std::string filesharing_id::file_url_part() const
{
    return std::string{ source_id_ ? su::concat(file_id_, "?source=", *source_id_) : file_id_ };
}

filesharing_id::filesharing_id(std::string_view _file_id, std::string_view _source_id)
    : full_id_{ _source_id.empty() ? _file_id : su::concat(_file_id, _source_id) }
    , file_id_{ full_id_.data(), _file_id.size() }
    , source_id_{ _source_id.empty() ? std::nullopt : std::make_optional<std::string_view>(full_id_.data() + _file_id.size(), _source_id.size()) }
{
    im_assert(!file_id_.empty());
}

void filesharing_id::set_source_id(std::string_view _source_id)
{
    im_assert(!_source_id.empty());
    if (_source_id.empty())
        return;
    const auto file_id_size = file_id_.size();
    full_id_ = su::concat(file_id_, _source_id);
    file_id_ = std::string_view(full_id_.data(), file_id_size);
    source_id_ = std::string_view(full_id_.data() + file_id_size, _source_id.size());
}

filesharing_id::filesharing_id(const filesharing_id& _other)
    : full_id_{ _other.full_id_ }
    , file_id_{ full_id_.data(), _other.file_id().size() }
    , source_id_{ _other.source_id() ? std::make_optional<std::string_view>(full_id_.data() + file_id_.size(), _other.source_id()->size()) : std::nullopt }
{
}

filesharing_id::filesharing_id(filesharing_id&& _other) noexcept
    : full_id_{ std::move(_other.full_id_) }
    , file_id_{ std::move(_other.file_id_) }
    , source_id_{ std::move(_other.source_id_) }
{
}

filesharing_id& filesharing_id::operator=(const filesharing_id& _other)
{
    if (std::addressof(_other) == this)
        return *this;
    full_id_ = _other.full_id_;
    file_id_ = std::string_view(full_id_.data(), _other.file_id().size());
    source_id_ = _other.source_id() ? std::make_optional<std::string_view>(full_id_.data() + file_id_.size(), _other.source_id()->size()) : std::nullopt;
    return *this;
}

filesharing_id& filesharing_id::operator=(filesharing_id&& _other) noexcept
{
    if (std::addressof(_other) == this)
        return *this;
    file_id_ = std::move(_other.file_id_);
    source_id_ = std::move(_other.source_id_);
    full_id_ = std::move(_other.full_id_);
    return *this;
}

filesharing_id filesharing_id::from_filesharing_uri(std::string_view _uri)
{
    const auto fsId = parse_new_file_sharing_uri(_uri);
    return fsId ? *fsId : filesharing_id{};
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

std::string format_file_sharing_preview_uri(const filesharing_id& _id, const filesharing_preview_size _size)
{
    im_assert(!_id.is_empty());
    im_assert(_size > filesharing_preview_size::min);
    im_assert(_size < filesharing_preview_size::max);

    std::string result;
    if (_id.file_id().size() < new_id_min_size())
        return result;

    const auto& infos = get_available_fs_preview_sizes();
    const auto it = std::find_if(infos.begin(), infos.end(), [_size](const auto& _i) { return _i.size_ == _size; });
    if (it == infos.end())
    {
        im_assert(!"unknown preview size");
        return result;
    }

    return su::concat(urls::get_url(urls::url_type::files_preview), "/max/", it->url_path_, '/', _id.file_id(), _id.source_id() ? su::concat("?source=", *_id.source_id()) : "");
}

bool get_content_type_from_uri(std::string_view _uri, Out core::file_sharing_content_type& _type)
{
    if (auto id = tools::parse_new_file_sharing_uri(_uri); id)
        return get_content_type_from_file_sharing_id(id->file_id(), Out _type);

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
    if (_file_id.empty())
        return {};

    const auto id0 = _file_id.front();
    core::file_sharing_content_type type;

    const auto is_one_of = [id0](std::string_view _values)
    {
        return _values.find(id0) != std::string_view::npos;
    };

    if (is_one_of("45")) // gif
    {
        type.type_ = core::file_sharing_base_content_type::gif;
        if (id0 == '5')
            type.subtype_ = core::file_sharing_sub_content_type::sticker;
    }
    else if (is_one_of("IJ")) // ptt
    {
        type.type_ = core::file_sharing_base_content_type::ptt;
    }
    else if (is_one_of("L")) // lottie sticker
    {
        type.type_ = core::file_sharing_base_content_type::lottie;
        type.subtype_ = core::file_sharing_sub_content_type::sticker;
    }
    else if (is_one_of("01234567")) // image
    {
        type.type_ = core::file_sharing_base_content_type::image;
        if (id0 == '2')
            type.subtype_ = core::file_sharing_sub_content_type::sticker;
    }
    else if (is_one_of("89ABCDEF")) // video
    {
        type.type_ = core::file_sharing_base_content_type::video;
        if (id0 == 'D')
            type.subtype_ = core::file_sharing_sub_content_type::sticker;
    }
    return type;
}

std::optional<filesharing_id> parse_new_file_sharing_uri(std::string_view _uri)
{
    using namespace config::hosts;
    using uri_matcher = basic_uri_matcher<char> ;
    using uri_view = basic_uri_view<char>;
    using category_matcher = uri_category_matcher<char>;

    im_assert(!_uri.empty());

    const static auto additional_urls = []()
    {
        std::vector<std::string_view> urls;
        if (const auto str = config::get().string(config::values::additional_fs_parser_urls_csv); !str.empty())
        {
            urls.reserve(std::count(str.begin(), str.end(), ',') + 1);
            su::split(str, std::back_inserter(urls), ',', su::token_compress::on);
        }
        return urls;
    }();

    scheme_type scheme;
    uri_matcher matcher;

    uri_view uview;
    matcher.search(_uri.begin(), _uri.end(), uview, scheme);

    if (uview.empty())
        return std::nullopt;

    category_matcher categorizer(additional_urls.begin(), additional_urls.end(), category_type::filesharing);
    categorizer.emplace(get_host_url(host_url_type::files_parse), category_type::filesharing);

    if (categorizer.find_category(uview) == category_type::filesharing)
    {
        const auto file_id = get_file_id(_uri);
        const auto source = get_source_id(_uri);
        return source ? std::make_optional<filesharing_id>(file_id, *source) : std::make_optional<filesharing_id>(file_id);
    }

    return std::nullopt;
}

std::string_view get_file_id(std::string_view _uri)
{
    const auto id = core::tools::trim_right_copy(_uri, '/');

    if (const auto lastSlash = id.rfind('/'); std::string_view::npos != lastSlash)
    {
        const auto lastQuestion = id.rfind('?');
        const auto idLength = lastQuestion == std::string_view::npos ? std::string_view::npos : (lastQuestion - lastSlash - 1);
        return id.substr(lastSlash + 1, idLength);
    }

    return {};
}

std::optional<std::string_view> get_source_id(std::string_view _uri)
{
    const auto id = core::tools::trim_right_copy(_uri, '/');

    if (const auto pos = id.rfind('='); std::string_view::npos != pos)
        return id.substr(pos + 1);

    return std::nullopt;
}

CORE_TOOLS_NS_END
