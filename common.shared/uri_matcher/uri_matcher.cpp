#include "stdafx.h"
#include "uri_matcher.h"
#include "tld.h"
#include "scheme.h"
#include "regex_strings.h"

namespace
{
    template<class _Char, _Char lo, _Char hi>
    inline bool in_range(_Char c) { return (c >= lo && c <= hi); }

    template<class _Char>
    using mmedia_mapping = std::pair<std::basic_string_view<_Char>, mmedia_type>;

    template<class _Char>
    using category_mapping = std::pair<std::basic_string_view<_Char>, category_type>;

    // built-in url categories mapping
    static const uri_matcher_traits<char>::category_mapping_view URL_CATEGORY_MAP[] =
    {
        { "icq.com/files/", category_type::filesharing },
        { "chat.my.com/files/", category_type::filesharing }
    };

    static const uri_matcher_traits<wchar_t>::category_mapping_view WURL_CATEGORY_MAP[] =
    {
        { L"icq.com/files/", category_type::filesharing },
        { L"chat.my.com/files/", category_type::filesharing }
    };

    // built-in media types mapping
    constexpr size_t max_media_suffix_length = 6;
    constexpr mmedia_mapping<char> MEDIA_SUFFIX_MAP[] =
    {
        // video
        { "3gp", mmedia_type::_3gp },
        { "avi", mmedia_type::avi },
        { "mkv", mmedia_type::mkv },
        { "mov", mmedia_type::mov },
        { "mp4", mmedia_type::mpeg4 },
        { "flv", mmedia_type::flv },
        { "webm", mmedia_type::webm },
        { "wmv", mmedia_type::wmv },
        // images
        { "bmp", mmedia_type::bmp },
        { "gif", mmedia_type::gif },
        { "jpeg", mmedia_type::jpeg },
        { "jpg", mmedia_type::jpg },
        { "svg", mmedia_type::svg },
        { "png", mmedia_type::png },
        { "tiff",  mmedia_type::tiff }
    };
    static_assert(std::size(MEDIA_SUFFIX_MAP) == (size_t)mmedia_type::max_media_type, "mmedia mapping size mismatch");

    constexpr mmedia_mapping<wchar_t> WMEDIA_SUFFIX_MAP[] =
    {
        // video
        { L"3gp", mmedia_type::_3gp },
        { L"avi", mmedia_type::avi },
        { L"mkv", mmedia_type::mkv },
        { L"mov", mmedia_type::mov },
        { L"mp4", mmedia_type::mpeg4 },
        { L"flv", mmedia_type::flv },
        { L"webm", mmedia_type::webm },
        { L"wmv", mmedia_type::wmv },
        // images
        { L"bmp", mmedia_type::bmp },
        { L"gif", mmedia_type::gif },
        { L"jpeg", mmedia_type::jpeg },
        { L"jpg", mmedia_type::jpg },
        { L"svg", mmedia_type::svg },
        { L"png", mmedia_type::png },
        { L"tiff",  mmedia_type::tiff },
    };
    static_assert(std::size(WMEDIA_SUFFIX_MAP) == (size_t)mmedia_type::max_media_type, "mmedia mapping size mismatch");

    template<class _Char>
    using media_type_searcher = rk_searcher_sset<
        mmedia_mapping<_Char>, 3, (size_t)mmedia_type::max_media_type,
        rk_traits_ascii<mmedia_mapping<_Char>, 3>
    >;


}



uri_matcher_traits<char>::string_view_type uri_matcher_traits<char>::open_brackets = "([{«“‘";
uri_matcher_traits<char>::string_view_type uri_matcher_traits<char>::close_brackets = ")]}»”’";

uri_matcher_traits<char>::regex_type& uri_matcher_traits<char>::regexp_url()
{
    static thread_local regex_type rx(latin1::URI_EXPR.begin(), latin1::URI_EXPR.end());
    return rx;
}

uri_matcher_traits<char>::regex_type& uri_matcher_traits<char>::regexp_email()
{
    static thread_local regex_type rx(latin1::EMAIL_EXPR.begin(), latin1::EMAIL_EXPR.end());
    return rx;
}

uri_matcher_traits<char>::regex_type& uri_matcher_traits<char>::regexp_ipaddress()
{
    static thread_local regex_type rx(latin1::IPADDR_EXPR.begin(), latin1::IPADDR_EXPR.end());
    return rx;
}

uri_matcher_traits<char>::regex_type& uri_matcher_traits<char>::regexp_localhost()
{
    static thread_local regex_type rx(latin1::LOCALHOST_EXPR.begin(), latin1::LOCALHOST_EXPR.end());
    return rx;
}

uri_matcher_traits<char>::scheme_searcher_type& uri_matcher_traits<char>::scheme_searcher()
{
    static thread_local scheme_searcher_type searcher(std::begin(SCHEMES), std::end(SCHEMES));
    return searcher;
}

uri_matcher_traits<char>::domain_searcher_type& uri_matcher_traits<char>::domain_searcher()
{
    static thread_local domain_searcher_type searcher(std::begin(TLDS), std::end(TLDS));
    return searcher;
}

bool uri_matcher_traits<char>::is_whitespace(char c)
{
    return (c == ' ' || c == '\t' || c == '\r' || c == '\n' || c == '\v' || c == '\f');
}

bool uri_matcher_traits<char>::is_alnum(char c)
{
    return in_range<char, 'A', 'Z'>(c) ||
           in_range<char, 'a', 'z'>(c) ||
           in_range<char, '0', '9'>(c);
}

bool uri_matcher_traits<char>::is_digit(char c)
{
    return in_range<char, '0', '9'>(c);
}


mmedia_type uri_matcher_traits<char>::media_type_from_suffix(string_view_type _suffix)
{
    static thread_local media_type_searcher<char> searcher(std::begin(MEDIA_SUFFIX_MAP), std::end(MEDIA_SUFFIX_MAP));
    if (_suffix.size() >= max_media_suffix_length || _suffix.size() < media_type_searcher<wchar_t>::hash_width)
        return mmedia_type::unknown;
    searcher.reset();
    auto p = searcher.search(_suffix.begin(), _suffix.end(), max_media_suffix_length);  // O(1) amort
    return (p.first == _suffix.begin() ? p.second->second : mmedia_type::unknown);
}

const std::vector<uri_matcher_traits<char>::category_mapping_view>& uri_matcher_traits<char>::builtin_categories_map()
{
    using mapping_t = uri_matcher_traits<char>::category_mapping_view;
    static std::vector<mapping_t> mapping(std::begin(URL_CATEGORY_MAP), std::end(URL_CATEGORY_MAP));
    return mapping;
}


uri_matcher_traits<wchar_t>::string_view_type uri_matcher_traits<wchar_t>::open_brackets = L"([{«“‘";
uri_matcher_traits<wchar_t>::string_view_type uri_matcher_traits<wchar_t>::close_brackets = L")]}»”’";

uri_matcher_traits<wchar_t>::regex_type& uri_matcher_traits<wchar_t>::regexp_url()
{
    static thread_local regex_type rx(unicode::URI_EXPR.begin(), unicode::URI_EXPR.end());
    return rx;
}

uri_matcher_traits<wchar_t>::regex_type& uri_matcher_traits<wchar_t>::regexp_email()
{
    static thread_local regex_type rx(unicode::EMAIL_EXPR.begin(), unicode::EMAIL_EXPR.end());
    return rx;
}

uri_matcher_traits<wchar_t>::regex_type& uri_matcher_traits<wchar_t>::regexp_ipaddress()
{
    static thread_local regex_type rx(unicode::IPADDR_EXPR.begin(), unicode::IPADDR_EXPR.end());
    return rx;
}

uri_matcher_traits<wchar_t>::regex_type& uri_matcher_traits<wchar_t>::regexp_localhost()
{
    static thread_local regex_type rx(unicode::LOCALHOST_EXPR.begin(), unicode::LOCALHOST_EXPR.end());
    return rx;
}

uri_matcher_traits<wchar_t>::scheme_searcher_type& uri_matcher_traits<wchar_t>::scheme_searcher()
{
    static thread_local scheme_searcher_type searcher(std::begin(WSCHEMES), std::end(WSCHEMES));
    return searcher;
}

uri_matcher_traits<wchar_t>::domain_searcher_type& uri_matcher_traits<wchar_t>::domain_searcher()
{
    static thread_local domain_searcher_type searcher(std::begin(WTLDS), std::end(WTLDS));
    return searcher;
}

bool uri_matcher_traits<wchar_t>::is_whitespace(wchar_t c)
{
    return (c == ' ' || c == '\t' ||
            c == '\r' || c == '\n' ||
            c == '\v' || c == '\f' ||
            c == 0xFEFF || c == 0xFFFE); // include BOM as a whitespace
}

bool uri_matcher_traits<wchar_t>::is_alnum(wchar_t c)
{
    return (c != 0xFEFF && c != 0xFFFE && c != 0x00AB && c != 0x00BB &&
            c != 0x201C && c != 0x201D && c != 0x2018 && c != 0x2019) &&
            ((c == 0x2028 || c == 0x2029 || c == 0x202F || c == 0x3000) ||
            in_range<wchar_t, wchar_t('A'), wchar_t('Z')>(c) ||
            in_range<wchar_t, wchar_t('a'), wchar_t('z')>(c) ||
            in_range<wchar_t, wchar_t('0'), wchar_t('9')>(c) ||
            in_range<wchar_t, 0x00A0, 0xD7FF>(c) ||
            in_range<wchar_t, 0xF900, 0xFDCF>(c) ||
            in_range<wchar_t, 0xFDF0, 0xFFEF>(c) ||
            in_range<wchar_t, 0xD800, 0xDFFF>(c));
}

bool uri_matcher_traits<wchar_t>::is_digit(wchar_t c)
{
    return in_range<wchar_t, L'0', L'9'>(c);
}

mmedia_type uri_matcher_traits<wchar_t>::media_type_from_suffix(string_view_type _suffix)
{
    static thread_local media_type_searcher<wchar_t> searcher(std::begin(WMEDIA_SUFFIX_MAP), std::end(WMEDIA_SUFFIX_MAP));
    if (_suffix.size() >= max_media_suffix_length || _suffix.size() < media_type_searcher<wchar_t>::hash_width)
        return mmedia_type::unknown;
    searcher.reset();
    auto p = searcher.search(_suffix.begin(), _suffix.end(), max_media_suffix_length); // O(1) amort
    return (p.first == _suffix.begin() ? p.second->second : mmedia_type::unknown);
}

const std::vector<uri_matcher_traits<wchar_t>::category_mapping_view>& uri_matcher_traits<wchar_t>::builtin_categories_map()
{
    using mapping_t = uri_matcher_traits<wchar_t>::category_mapping_view;
    static std::vector<mapping_t> mapping(std::begin(WURL_CATEGORY_MAP), std::end(WURL_CATEGORY_MAP));
    return mapping;
}