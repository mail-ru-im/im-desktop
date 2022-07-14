#pragma once
#include "scheme.h"
#include <string_view>

template<class _Char>
struct uri_scheme_traits;


struct uri_scheme_traits_base
{
    static bool is_ftp(scheme_type _t)
    {
        return (_t >= sftp && _t <= rdp);
    }

    static bool is_hypertext(scheme_type _t)
    {
        return (_t == scheme_type::http || _t == scheme_type::https);
    }

    static bool is_allowed(scheme_type _scheme);
};

template<>
struct uri_scheme_traits<char> : public uri_scheme_traits_base
{
    using string_view_type = std::string_view;
    static string_view_type signature(int16_t _scheme);
    static string_view_type name(int16_t _scheme);
    static scheme_type value(string_view_type _scheme);

    static constexpr size_t count = std::size(SCHEMES);
};


template<>
struct uri_scheme_traits<char16_t> : public uri_scheme_traits_base
{
    using string_view_type = std::u16string_view;
    static string_view_type signature(int16_t _scheme);
    static string_view_type name(int16_t _scheme);
    static scheme_type value(string_view_type _scheme);

    static constexpr size_t count = std::size(U16SCHEMES);
};


template<>
struct uri_scheme_traits<wchar_t> : public uri_scheme_traits_base
{
    using string_view_type = std::wstring_view;
    static string_view_type signature(int16_t _scheme);
    static string_view_type name(int16_t _scheme);
    static scheme_type value(string_view_type _scheme);

    static constexpr size_t count = std::size(WSCHEMES);
};

