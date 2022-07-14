#include "stdafx.h"
#include "scheme.h"
#include "scheme_traits.h"
#include "casefolding.h"
#include "../core/tools/features.h"
#include "../config/config.h"
#include "../string_utils.h"

#include <algorithm>

namespace
{
    template<class _Char>
    using scheme_mapping = std::pair<std::basic_string_view<_Char>, scheme_type>;

    template<class _Char, size_t N>
    static std::basic_string_view<_Char> find_signature(int16_t _scheme, const scheme_mapping<_Char>(&_mapping)[N])
    {
        if (_scheme < 0 || _scheme >= (int16_t)N)
            return {};
        return _mapping[_scheme].first;
    }

    template<class _Char, size_t N>
    static std::basic_string_view<_Char> find_name(int16_t _scheme, const scheme_mapping<_Char>(&_mapping)[N])
    {
        if (_scheme < 0 || _scheme >= (int16_t)N)
            return {};
        return _mapping[_scheme].first.substr(0, SCHEME_OFFSETS[_scheme]);
    }

    template<class _Char, size_t N>
    static scheme_type find_scheme(std::basic_string_view<_Char> _scheme, const scheme_mapping<_Char>(&_mapping)[N])
    {
        static auto compare = [](_Char x, _Char y) { return (x == tolowercase(y)); };
        for (size_t i = 0; i < N; ++i)
        {
            if (std::equal(_mapping[i].first.begin(), _mapping[i].first.begin() + SCHEME_OFFSETS[i],
                           _scheme.begin(), _scheme.end(), compare))
                return static_cast<scheme_type>(i);
        }
        return undefined;
    }
}

bool  uri_scheme_traits_base::is_allowed(scheme_type _scheme)
{
    static const auto allowed_schemes = []()
    {
        std::vector<scheme_type> results;
        const auto scheme_csv = config::get().string(config::values::register_url_scheme_csv); //url1,descr1,url2,desc2,..
        std::vector<std::string_view> splitted;
        splitted.reserve(std::count(scheme_csv.begin(), scheme_csv.end(), ',') + 1);
        su::split(scheme_csv, std::back_inserter(splitted), ',');
        im_assert(splitted.size() % 2 == 0);
        for (size_t i = 0, n = splitted.size(); i < n; ++i)
        {
            if (((i % 2) == 0) && !splitted[i].empty())
            {
                auto s = find_scheme(splitted[i], SCHEMES);
                if (s != scheme_type::undefined)
                    results.emplace_back(s); // add allowed schemes
            }
        }
        return results;
    }();

    if (_scheme >= scheme_type::undefined && _scheme < scheme_type::icq)
    {
        if (!features::is_url_ftp_protocols_allowed())
            return !(_scheme >= sftp && _scheme <= ftp);
        return true;
    }

    return (std::find(allowed_schemes.begin(), allowed_schemes.end(), _scheme) != allowed_schemes.end());
}


uri_scheme_traits<char>::string_view_type uri_scheme_traits<char>::signature(int16_t _scheme)
{
    return find_signature(_scheme, SCHEMES);
}

uri_scheme_traits<char>::string_view_type uri_scheme_traits<char>::name(int16_t _scheme)
{
    return find_name(_scheme, SCHEMES);
}

scheme_type uri_scheme_traits<char>::value(uri_scheme_traits<char>::string_view_type _scheme)
{
    return find_scheme(_scheme, SCHEMES);
}


uri_scheme_traits<char16_t>::string_view_type uri_scheme_traits<char16_t>::signature(int16_t _scheme)
{
    return find_signature(_scheme, U16SCHEMES);
}

uri_scheme_traits<char16_t>::string_view_type uri_scheme_traits<char16_t>::name(int16_t _scheme)
{
    return find_name(_scheme, U16SCHEMES);
}

scheme_type uri_scheme_traits<char16_t>::value(uri_scheme_traits<char16_t>::string_view_type _scheme)
{
    return find_scheme(_scheme, U16SCHEMES);
}


uri_scheme_traits<wchar_t>::string_view_type uri_scheme_traits<wchar_t>::signature(int16_t _scheme)
{
    return find_signature(_scheme, WSCHEMES);
}

uri_scheme_traits<wchar_t>::string_view_type uri_scheme_traits<wchar_t>::name(int16_t _scheme)
{
    return find_name(_scheme, WSCHEMES);
}

scheme_type uri_scheme_traits<wchar_t>::value(uri_scheme_traits<wchar_t>::string_view_type _scheme)
{
    return find_scheme(_scheme, WSCHEMES);
}



