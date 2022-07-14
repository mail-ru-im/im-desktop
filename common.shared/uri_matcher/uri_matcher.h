#pragma once
#include <string>
#include <type_traits>
#include <boost/regex.hpp>

#include "../string_utils.h"
#include "scheme_traits.h"
#include "rk_searcher.h"
#include "uri.h"

template<class _Char>
struct uri_matcher_traits;

template<>
struct uri_matcher_traits<char>
{
    using string_type = std::string;
    using string_view_type = std::string_view;
    using regex_type = boost::regex;

    using scheme_traits = uri_scheme_traits<char>;
    using scheme_mapping = std::pair<string_view_type, scheme_type>;
    using scheme_searcher_type = rk_searcher_sset<scheme_mapping, 4, scheme_traits::count>;
    using domain_searcher_type = rk_searcher_sset<string_view_type, 3, 2048>;

    using category_mapping = std::pair<string_type, category_type>;
    using category_mapping_view = std::pair<string_view_type, category_type>;

    static constexpr size_t max_length = 4096;
    static bool is_whitespace(char c);
    static bool is_alnum(char c);
    static bool is_digit(char c);
    static mmedia_type media_type_from_suffix(string_view_type suffix);

    static const std::vector<category_mapping_view>& builtin_categories_map();

protected:
    static regex_type& regexp_url();
    static regex_type& regexp_email();
    static regex_type& regexp_ipaddress();
    static regex_type& regexp_localhost();
    static scheme_searcher_type& scheme_searcher();
    static domain_searcher_type& domain_searcher();

    static string_view_type open_brackets;
    static string_view_type close_brackets;
};


template<>
struct uri_matcher_traits<wchar_t>
{
    using string_type = std::wstring;
    using string_view_type = std::wstring_view;
    using regex_type = boost::wregex;

    using scheme_traits = uri_scheme_traits<char>;
    using scheme_mapping = std::pair<string_view_type, scheme_type>;
    using scheme_searcher_type = rk_searcher_sset<scheme_mapping, 4, scheme_traits::count, rk_traits_ascii<string_view_type, 4>>;
    using domain_searcher_type = rk_searcher_sset<string_view_type, 3, 2048>;

    using category_mapping = std::pair<string_type, category_type>;
    using category_mapping_view = std::pair<string_view_type, category_type>;

    static constexpr size_t max_length = 4096;
    static bool is_whitespace(wchar_t c);
    static bool is_alnum(wchar_t c);
    static bool is_digit(wchar_t c);
    static mmedia_type media_type_from_suffix(string_view_type _suffix);

    static const std::vector<category_mapping_view>& builtin_categories_map();

protected:
    static regex_type& regexp_url();
    static regex_type& regexp_email();
    static regex_type& regexp_ipaddress();
    static regex_type& regexp_localhost();
    static scheme_searcher_type& scheme_searcher();
    static domain_searcher_type& domain_searcher();

    static string_view_type open_brackets;
    static string_view_type close_brackets;
};


template<
    class _Char,
    class _CharTraits = std::char_traits<_Char>,
    class _MatcherTraits = uri_matcher_traits<_Char>
>
class basic_uri_matcher : public _MatcherTraits
{
public:
    using string_type = std::basic_string<_Char, _CharTraits>;
    using string_view_type = std::basic_string_view<_Char, _CharTraits>;
    using traits_type = _MatcherTraits;
    using char_traits = _CharTraits;

    ~basic_uri_matcher();

    template<class _RandIt, class _OutIt>
    _OutIt search_copy(_RandIt _first, _RandIt _last, _OutIt _out) const;

    template<class _RandIt, class _OutIt>
    _RandIt search(_RandIt _first, _RandIt _last, _OutIt& _result) const;

    template<class _RandIt, class _Result>
    _RandIt search(_RandIt _first, _RandIt _last, _Result&& _result, scheme_type& scheme, const bool _check_punct = true) const;

    void reset();

    template<class _RandIt>
    static bool has_domain(_RandIt _first, _RandIt _last);

    template<class _RandIt>
    static bool has_ipaddr_mask(_RandIt _first, _RandIt _last);

    template<class _RandIt>
    static bool has_ipaddr(_RandIt _first, _RandIt _last);

protected:
    template<class _RandIt>
    static bool has_ipaddr_part(_RandIt _first, _RandIt _last, bool _need_full);

    static bool is_common_scheme(scheme_type _scheme);

    template<class _RandIt, class _OutIt>
    static _RandIt put_result(scheme_type, _RandIt _first, _RandIt _last, _OutIt _out);

    template<class _RandIt>
    static _RandIt put_result(scheme_type, _RandIt _first, _RandIt _last, string_type& _result);

    template<class _RandIt>
    static _RandIt put_result(scheme_type, _RandIt _first, _RandIt _last, string_view_type& _result);

    template<class _RandIt, class _UriTraits>
    static _RandIt put_result(scheme_type _scheme_hint, _RandIt _first, _RandIt _last, basic_uri_view<_Char, _UriTraits>& _uri);

    template<class _RandIt>
    static bool validate_authority(_RandIt _first, _RandIt _last);

    template<class _RandIt>
    static bool validate_domain(_RandIt _first, _RandIt _last, _RandIt _tld_first, size_t _tld_length);

    template<class _RandIt>
    _RandIt has_url(_RandIt _first, _RandIt _last, scheme_type _scheme_hint) const;


    template<class _RandIt, class _Result>
    _RandIt do_match_url(_RandIt _first, _RandIt _last, _Result&& _out, scheme_type _scheme_hint) const;

    template<class _RandIt, class _Result>
    _RandIt do_match_url_relaxed(_RandIt _first, _RandIt _last, _Result&& _out) const;

    template<class _RandIt, class _Result>
    _RandIt do_match_localhost(_RandIt _first, _RandIt _last, _Result&& _out, scheme_type _scheme_hint) const;

    static bool is_allowed_trailing(_Char c);

    template<class _RandIt>
    static _RandIt skip_specs(_RandIt _first, _RandIt _last);

    template<class _RandIt>
    static _RandIt scan_first(_RandIt _first, _RandIt _last);

    template<class _RandIt>
    static _RandIt scan_last(_RandIt _first, _RandIt _last);

    template<class _RandIt>
    static _RandIt trim_trailing_punct(_RandIt _first, _RandIt _last);

    template<class _RandIt>
    static bool is_plain_word(_RandIt _first, _RandIt _last);
};


template<class _Char>
class uri_category_matcher
{
    using uri_view_type = basic_uri_view<_Char>;
    using string_type = std::basic_string<_Char>;
    using string_view_type = std::basic_string_view<_Char>;

    using matcher_traits = uri_matcher_traits<_Char>;
    using scheme_traits = uri_scheme_traits<_Char>;
    using category_mapping_type = typename matcher_traits::category_mapping;

    using searcher_type = rk_searcher<category_mapping_type, 4>;

public:

    uri_category_matcher()
          : searcher_(std::begin(matcher_traits::builtin_categories_map()),
                      std::end(matcher_traits::builtin_categories_map()))
    {
    }

    template<class _InIt>
    uri_category_matcher(_InIt _first, _InIt _last, category_type _category)
        : uri_category_matcher()
    {
        for (; _first != _last; ++_first)
            emplace(*_first, _category);
    }

    void emplace(string_view_type _pattern, category_type _category)
    {
        if (_pattern.empty())
            return;
        searcher_.emplace(std::make_pair(_pattern, _category));
    }

    category_type find_category(uri_view_type _uri) const
    {
        if (_uri.host().empty() || !scheme_traits::is_allowed(_uri.scheme()))
            return category_type::undefined;

        if (_uri.scheme() == scheme_type::email)
            return category_type::email;

        auto type = search_category(_uri.data());
        if (type != category_type::undefined)
            return type;

        if (_uri.scheme() == scheme_type::icq)
            return category_type::icqprotocol;

        if (_uri.scheme() == scheme_type::undefined)
        {
            if (_uri.path().empty() && !_uri.has_fragment() && !_uri.has_query() && !_uri.username().empty())
                return category_type::email;
            return category_type::site;
        }

        if (scheme_traits::is_ftp(_uri.scheme()))
            return category_type::ftp;

        if (scheme_traits::is_hypertext(_uri.scheme()))
        {
            const mmedia_type mtype = find_media_type(_uri);
            return media_category(mtype);
        }

        return category_type::site;
    }

    mmedia_type find_media_type(uri_view_type _uri) const
    {
        if (_uri.path().empty() || _uri.host().empty())
            return mmedia_type::unknown;
        return matcher_traits::media_type_from_suffix(_uri.path_suffix());
    }

private:
    category_type media_category(mmedia_type _mtype) const
    {
        if (_mtype >= mmedia_type::_3gp && _mtype <= mmedia_type::wmv)
            return  category_type::video;
        if (_mtype >= mmedia_type::bmp && _mtype <= mmedia_type::tiff)
            return category_type::image;
        return category_type::site;
    }

    category_type search_category(string_view_type _sv) const
    {
        if (_sv.empty())
            return category_type::undefined;
        return search_category(_sv.begin(), _sv.end());
    }

    template<class _It>
    category_type search_category(_It _first, _It _last) const
    {
        searcher_.reset();
        auto p = searcher_.search(_first, _last);
        return (p.first == _first ? (p.second->second) : category_type::undefined);
    }

private:
    mutable searcher_type searcher_;
};


template<typename _Char, typename _CharTraits, typename _MatcherTraits>
basic_uri_matcher<_Char, _CharTraits, _MatcherTraits>::~basic_uri_matcher()
{
    reset();
}

template<typename _Char, typename _CharTraits, typename _MatcherTraits>
template<class _RandIt, class _OutIt>
_OutIt basic_uri_matcher<_Char, _CharTraits, _MatcherTraits>::search_copy(_RandIt _first, _RandIt _last, _OutIt _out) const
{
    auto p = search(_first, _last, _out);
    while (p != _last)
        p = search(p, _last, _out);

    return _out;
}

template<typename _Char, typename _CharTraits, typename _MatcherTraits>
template<class _RandIt, class _OutIt>
_RandIt basic_uri_matcher<_Char, _CharTraits, _MatcherTraits>::search(_RandIt _first, _RandIt _last, _OutIt& _result) const
{
    scheme_type s;
    return search(_first, _last, _result, s);
}

template<typename _Char, typename _CharTraits, typename _MatcherTraits>
template<class _RandIt, class _Result>
_RandIt basic_uri_matcher<_Char, _CharTraits, _MatcherTraits>::search(_RandIt _first, _RandIt _last, _Result&& _result, scheme_type& scheme, const bool _check_punct) const
{
    static auto comp = [](_Char x, _Char y) {
        return tolowercase(x) == y;
    };

    scheme = scheme_type::undefined;

    if (_first == _last) // O(1)
        return _last;

    _first = scan_first(_first, _last); // remove leading non-alpha numeric chars O(N)
    _last = scan_last(_first, _last); // remove tailing non-alpha numeric chars O(N)
    if (_check_punct)
        _last = trim_trailing_punct(std::make_reverse_iterator(_last), std::make_reverse_iterator(_first)).base(); // trim trailing punctuation
    if (is_plain_word(_first, _last)) // check if we have plain alpha-numeric string
        return _last;

    if (_first == _last) // O(1)
        return _last;

    if (std::distance(_first, _last) > traits_type::max_length)
        return _last;

    traits_type::domain_searcher().reset();
    traits_type::scheme_searcher().reset();

    auto ubegin = _first;
    auto uend = _last;

    // attempt to find scheme
    auto sp = traits_type::scheme_searcher().search_comp(_first, _last, comp); // O(N)
    while (sp.first != _last)
    {
        ubegin = sp.first;
        auto scheme_end = ubegin + sp.second->first.size();
        if (scheme_end == _last)
            return _last; // scheme without any path or query - bailing out

        scheme = sp.second->second;

        const bool is_allowed_scheme = uri_scheme_traits<_Char>::is_allowed(scheme);
        if (!is_allowed_scheme)
            return uend;

        if (!is_common_scheme(scheme) && is_allowed_scheme)
        {
            uend = su::brackets_mismatch(ubegin, uend, traits_type::open_brackets, traits_type::close_brackets);
            if ((size_t)std::distance(ubegin, uend) > sp.second->first.size())
                return put_result(scheme, ubegin, uend, std::forward<_Result>(_result));
        }

        // attempt to find TLD
        auto p = traits_type::domain_searcher().search_comp(_first, _last, comp); // O(N)
        if (p.first != _last) // TLD found
        {
            if (p.first < sp.first) // impossible case: domain before scheme
            {
                _last = sp.first;
                break;
            }
        }

        // see if we have a matching url
        uend = has_url(scheme_end, _last, scheme);
        if (uend != scheme_end) // url is found
        {
            bool has_tld = false;
            while (p.first < uend) // try to validate TLD
            {
                has_tld = validate_domain(scheme_end, uend, p.first, p.second->size());
                if (has_tld)
                    break;
                // we can have false positive: continue searching
                p = traits_type::domain_searcher().search_comp(p.first, uend, comp);
            }

            if (has_tld) // TLD validated successfully
            {
                // try to validate authority
                if (!validate_authority(scheme_end, uend)) // O(N)
                    return uend;

                return put_result(scheme, ubegin, uend, std::forward<_Result>(_result));
            }
            else if (has_ipaddr(scheme_end, uend)) // no TLD, maybe IP address?
            {
                return do_match_url(scheme_end, _last, std::forward<_Result>(_result), scheme);
            }
        }

        // no TLD, no IP address, maybe a localhost?
        uend = do_match_localhost(scheme_end, _last, std::forward<_Result>(_result), scheme);
        if (uend != scheme_end)
            return uend; // url found successfully - return

        // try to find scheme once more
        sp = traits_type::scheme_searcher().search_comp(uend, _last, comp);
    }

    scheme = scheme_type::undefined;

    // at this point we haven't got any scheme - give it a last try
    ubegin = skip_specs(_first, _last); // skip special chars O(N)
    // try to match url with relaxed rules
    uend = do_match_url_relaxed(ubegin, _last, std::forward<_Result>(_result));

    // see if we got the result
    return ((uend != ubegin) ? uend : _last);
}

template<typename _Char, typename _CharTraits, typename _MatcherTraits>
void basic_uri_matcher<_Char, _CharTraits, _MatcherTraits>::reset()
{
    traits_type::scheme_searcher().reset();
    traits_type::domain_searcher().reset();
}

template<typename _Char, typename _CharTraits, typename _MatcherTraits>
template<class _RandIt>
bool basic_uri_matcher<_Char, _CharTraits, _MatcherTraits>::has_domain(_RandIt _first, _RandIt _last)
{
    static auto comp = [](_Char x, _Char y) {
        return tolowercase(x) == y;
    };

    traits_type::domain_searcher().reset();
    auto p = traits_type::domain_searcher().search_comp(_first, _last, comp);
    while (p.first != _last)
    {
        if (validate_domain(_first, _last, p.first, p.second->size()))
            return true;
        // we can have false positive: continue searching
        p = traits_type::domain_searcher().search_comp(p.first, _last, comp);
    }
    return false;
}

template<typename _Char, typename _CharTraits, typename _MatcherTraits>
template<class _RandIt>
bool basic_uri_matcher<_Char, _CharTraits, _MatcherTraits>::has_ipaddr_part(_RandIt _first, _RandIt _last, bool _need_full)
{
    uint32_t b = 0; int n = 0, sections = 1;
    for (auto it = _first; it != _last; ++it)
    {
        if (sections > 4)
            return false;

        _Char c = *it;
        if (traits_type::is_digit(c))
        {
            if (n > 0 && b == 0)
                return false;
            if (n > 3)
                return false;

            b = b * 10 + (c - '0');
            ++n;
        }
        else if (c == '.')
        {
            if (b > 255)
                return false;

            b = 0; n = 0;
            ++sections;
        }
        else
        {
            if (b > 255)
                return false;
            break;
        }
    }
    if (b > 255)
        return false;
    if (sections != 4 && _need_full)
        return false;
    return true;
}

template<typename _Char, typename _CharTraits, typename _MatcherTraits>
template<class _RandIt>
bool basic_uri_matcher<_Char, _CharTraits, _MatcherTraits>::has_ipaddr_mask(_RandIt _first, _RandIt _last)
{
    return has_ipaddr_part(_first, _last, false);
}

template<typename _Char, typename _CharTraits, typename _MatcherTraits>
template<class _RandIt>
bool basic_uri_matcher<_Char, _CharTraits, _MatcherTraits>::has_ipaddr(_RandIt _first, _RandIt _last)
{
    return has_ipaddr_part(_first, _last, true);
}

template<typename _Char, typename _CharTraits, typename _MatcherTraits>
bool basic_uri_matcher<_Char, _CharTraits, _MatcherTraits>::is_common_scheme(scheme_type _scheme)
{
    return (_scheme > scheme_type::undefined && _scheme < scheme_type::icq);
}

template<typename _Char, typename _CharTraits, typename _MatcherTraits>
template<class _RandIt, class _OutIt>
_RandIt basic_uri_matcher<_Char, _CharTraits, _MatcherTraits>::put_result(scheme_type, _RandIt _first, _RandIt _last, _OutIt _out)
{
    _last = su::brackets_mismatch(_first, _last, traits_type::open_brackets, traits_type::close_brackets);
    *_out = { _first, _last };
    ++_out;
    return _last;
}

template<typename _Char, typename _CharTraits, typename _MatcherTraits>
template<class _RandIt>
_RandIt basic_uri_matcher<_Char, _CharTraits, _MatcherTraits>::put_result(scheme_type, _RandIt _first, _RandIt _last, string_type& _result)
{
    _last = su::brackets_mismatch(_first, _last, traits_type::open_brackets, traits_type::close_brackets);
    _result = { _first, _last };
    return _last;
}

template<typename _Char, typename _CharTraits, typename _MatcherTraits>
template<class _RandIt>
_RandIt basic_uri_matcher<_Char, _CharTraits, _MatcherTraits>::put_result(scheme_type, _RandIt _first, _RandIt _last, string_view_type& _result)
{
    _last = su::brackets_mismatch(_first, _last, traits_type::open_brackets, traits_type::close_brackets);
    _result = string_view_type{ std::addressof(*_first), (size_t)std::distance(_first, _last) };
    return _last;
}

template<typename _Char, typename _CharTraits, typename _MatcherTraits>
template<class _RandIt, class _UriTraits>
_RandIt basic_uri_matcher<_Char, _CharTraits, _MatcherTraits>::put_result(scheme_type _scheme_hint, _RandIt _first, _RandIt _last, basic_uri_view<_Char, _UriTraits>& _uri)
{
    _last = su::brackets_mismatch(_first, _last, traits_type::open_brackets, traits_type::close_brackets);
    string_view_type sv = string_view_type{ std::addressof(*_first), (size_t)std::distance(_first, _last) };
    _uri.assign(_scheme_hint, sv);
    return _last;
}

template<typename _Char, typename _CharTraits, typename _MatcherTraits>
template<class _RandIt>
bool basic_uri_matcher<_Char, _CharTraits, _MatcherTraits>::validate_authority(_RandIt _first, _RandIt _last)
{
    if (_first == _last)
        return false;
    auto end = std::find(_first, _last, _Char('/'));
    auto mid = std::find(_first, _last, _Char('@'));
    if (mid != end)
        return true;
    _Char c = *_first;
    return (c != _Char('.') && c != _Char('-'));
}

template<typename _Char, typename _CharTraits, typename _MatcherTraits>
template<class _RandIt>
bool basic_uri_matcher<_Char, _CharTraits, _MatcherTraits>::validate_domain(_RandIt _first, _RandIt _last, _RandIt _tld_first, size_t _tld_length)
{
    if (_tld_first != _last) // has TLD
    {
        // verify that TLD is on its place
        auto end = std::find(_first, _last, _Char('/'));
        auto mid = std::find(_first, end, _Char('@'));
        auto beg = mid == end ? _first : mid;
        end = std::find_if(beg, end, [](_Char c) { return c == _Char(':') || c == _Char('?') || c == _Char('#'); });
        return (_tld_first == (end - _tld_length));
    }
    return false;
}

template<typename _Char, typename _CharTraits, typename _MatcherTraits>
template<class _RandIt>
_RandIt basic_uri_matcher<_Char, _CharTraits, _MatcherTraits>::has_url(_RandIt _first, _RandIt _last, scheme_type _scheme_hint) const
{
    boost::match_results<_RandIt> results;
    try
    {
        if (!boost::regex_search(_first, _last, results, _scheme_hint == scheme_type::email ? traits_type::regexp_email() : traits_type::regexp_url()))
            return _first;
    }
    catch (const std::exception&)
    {
        return _first;
    }

    if (results.position() != 0)
        return _first;

    return (_first + results.length());
}


template<typename _Char, typename _CharTraits, typename _MatcherTraits>
template<class _RandIt, class _Result>
_RandIt basic_uri_matcher<_Char, _CharTraits, _MatcherTraits>::do_match_url(_RandIt _first, _RandIt _last, _Result&& _out, scheme_type _scheme_hint) const
{
    boost::match_results<_RandIt> results;
    try
    {
        if (!boost::regex_search(_first, _last, results, _scheme_hint == scheme_type::email ? traits_type::regexp_email() : traits_type::regexp_url()))
            return _first;
    }
    catch (const std::exception&)
    {
        return _first;
    }

    if (results.position() != 0)
        return _first;

    size_t scheme_length = uri_scheme_traits<_Char>::signature(_scheme_hint).size();
    return put_result(_scheme_hint, _first - scheme_length, _first + results.length(), std::forward<_Result>(_out));
}

template<typename _Char, typename _CharTraits, typename _MatcherTraits>
template<class _RandIt, class _Result>
_RandIt basic_uri_matcher<_Char, _CharTraits, _MatcherTraits>::do_match_url_relaxed(_RandIt _first, _RandIt _last, _Result&& _out) const
{
    boost::match_results<_RandIt> results;
    try
    {
        if (!boost::regex_search(_first, _last, results, traits_type::regexp_url()))
            return _first;
    }
    catch (const std::exception&)
    {
        return _first;
    }

    auto pos = results.position();
    auto length = results.length();
    auto ubegin = _first + pos;
    auto uend = ubegin + length;
    if (has_domain(ubegin, uend) || has_ipaddr(ubegin, _last))
        return put_result(scheme_type::undefined, ubegin, uend, std::forward<_Result>(_out));

    return _first;
}

template<typename _Char, typename _CharTraits, typename _MatcherTraits>
template<class _RandIt, class _Result>
_RandIt basic_uri_matcher<_Char, _CharTraits, _MatcherTraits>::do_match_localhost(_RandIt _first, _RandIt _last, _Result&& _out, scheme_type _scheme_hint) const
{
    if (_scheme_hint != scheme_type::http && _scheme_hint != scheme_type::https)
        return _first;

    boost::match_results<_RandIt> results;
    try
    {
        if (!boost::regex_search(_first, _last, results, traits_type::regexp_localhost()))
            return _first;
    }
    catch (const std::exception&)
    {
        return _first;
    }

    if (results.position() != 0)
        return _first;

    size_t scheme_length = uri_scheme_traits<_Char>::signature(_scheme_hint).size();
    return put_result(_scheme_hint, _first - scheme_length, _first + results.length(), std::forward<_Result>(_out));
}

template<typename _Char, typename _CharTraits, typename _MatcherTraits>
bool basic_uri_matcher<_Char, _CharTraits, _MatcherTraits>::is_allowed_trailing(_Char c)
{
    return (c == _Char('#') || c == _Char('/') || c == _Char(')') || c == _Char('}'));
}

template<typename _Char, typename _CharTraits, typename _MatcherTraits>
template<class _RandIt>
_RandIt basic_uri_matcher<_Char, _CharTraits, _MatcherTraits>::skip_specs(_RandIt _first, _RandIt _last)
{
    return std::find_if(_first, _last, &traits_type::is_alnum);
}

template<typename _Char, typename _CharTraits, typename _MatcherTraits>
template<class _RandIt>
_RandIt basic_uri_matcher<_Char, _CharTraits, _MatcherTraits>::scan_first(_RandIt _first, _RandIt _last)
{
    return std::find_if(_first, _last, &traits_type::is_alnum);
}

template<typename _Char, typename _CharTraits, typename _MatcherTraits>
template<class _RandIt>
_RandIt basic_uri_matcher<_Char, _CharTraits, _MatcherTraits>::scan_last(_RandIt _first, _RandIt _last)
{
    return std::find_if(_first, _last, &traits_type::is_whitespace);
}

template<typename _Char, typename _CharTraits, typename _MatcherTraits>
template<class _RandIt>
_RandIt basic_uri_matcher<_Char, _CharTraits, _MatcherTraits>::trim_trailing_punct(_RandIt _first, _RandIt _last)
{
    return std::find_if(_first, _last, [](_Char c) { return (traits_type::is_alnum(c) || is_allowed_trailing(c)); });
}

template<typename _Char, typename _CharTraits, typename _MatcherTraits>
template<class _RandIt>
bool basic_uri_matcher<_Char, _CharTraits, _MatcherTraits>::is_plain_word(_RandIt _first, _RandIt _last)
{
    return std::all_of(_first, _last, &traits_type::is_alnum);
}
