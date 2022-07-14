#pragma once

#include <cstring>
#include <string_view>
#include <algorithm>

#include <boost/functional/hash.hpp>

#include "casefolding.h"

namespace detail
{
    constexpr size_t hash_buf_size(size_t N) { return (N <= sizeof(size_t) ? sizeof(size_t) : N); }

    template<size_t N>
    struct unrolled_tolower
    {
        template<class _Char>
        static void transform(const _Char* _s, _Char* _out)
        {
            std::transform(_s, _s + N, _out, [](_Char c) { return tolowercase(c); });
        }

        template<class _Char>
        static void transform_ascii(const _Char* _s, char* _out)
        {
            std::transform(_s, _s + N, _out, [](_Char c)
            {
                // truncate to ascii, make lower case and return
                return (tolowercase(static_cast<char>(c & 0xFF)));
            });
        }
    };


    template<>
    struct unrolled_tolower<2>
    {
        template<class _Char>
        static void transform(const _Char* __restrict _s, _Char* __restrict _out)
        {
            _out[0] = tolowercase(_s[0]);
            _out[1] = tolowercase(_s[1]);
        }

        template<class _Char>
        static void transform_ascii(const _Char* __restrict _s, char* __restrict _out)
        {
            _out[0] = (tolowercase(static_cast<char>(_s[0] & 0xFF)));
            _out[1] = (tolowercase(static_cast<char>(_s[1] & 0xFF)));
        }
    };

    template<>
    struct unrolled_tolower<3>
    {
        template<class _Char>
        static void transform(const _Char* __restrict _s, _Char* __restrict _out)
        {
            _out[0] = tolowercase(_s[0]);
            _out[1] = tolowercase(_s[1]);
            _out[2] = tolowercase(_s[2]);
        }

        template<class _Char>
        static void transform_ascii(const _Char* __restrict _s, char* __restrict _out)
        {
            _out[0] = (tolowercase(static_cast<char>(_s[0] & 0xFF)));
            _out[1] = (tolowercase(static_cast<char>(_s[1] & 0xFF)));
            _out[2] = (tolowercase(static_cast<char>(_s[1] & 0xFF)));
        }
    };

    template<>
    struct unrolled_tolower<4>
    {
        template<class _Char>
        static void transform(const _Char* __restrict _s, _Char* __restrict _out)
        {
            unrolled_tolower<2>::transform(_s + 0, _out + 0);
            unrolled_tolower<2>::transform(_s + 2, _out + 2);
        }
        template<class _Char>
        static void transform_ascii(const _Char* __restrict _s, char* __restrict _out)
        {
            unrolled_tolower<2>::transform_ascii(_s + 0, _out + 0);
            unrolled_tolower<2>::transform_ascii(_s + 2, _out + 2);
        }
    };


    template<>
    struct unrolled_tolower<5>
    {
        template<class _Char>
        static void transform(const _Char* __restrict _s, _Char* __restrict _out)
        {
            unrolled_tolower<2>::transform(_s + 0, _out + 0);
            unrolled_tolower<3>::transform(_s + 2, _out + 2);
        }

        template<class _Char>
        static void transform_ascii(const _Char* __restrict _s, char* __restrict _out)
        {
            unrolled_tolower<2>::transform_ascii(_s + 0, _out + 0);
            unrolled_tolower<3>::transform_ascii(_s + 2, _out + 2);
        }
    };

    template<>
    struct unrolled_tolower<6>
    {
        template<class _Char>
        static void transform(const _Char* __restrict _s, _Char* __restrict _out)
        {
            unrolled_tolower<3>::transform(_s + 0, _out + 0);
            unrolled_tolower<3>::transform(_s + 3, _out + 3);
        }

        template<class _Char>
        static void transform_ascii(const _Char* __restrict _s, char* __restrict _out)
        {
            unrolled_tolower<3>::transform_ascii(_s + 0, _out + 0);
            unrolled_tolower<3>::transform_ascii(_s + 3, _out + 3);
        }
    };


    template<>
    struct unrolled_tolower<7>
    {
        template<class _Char>
        static void transform(const _Char* __restrict _s, _Char* __restrict _out)
        {
            unrolled_tolower<4>::transform(_s + 0, _out + 0);
            unrolled_tolower<3>::transform(_s + 4, _out + 4);
        }
        template<class _Char>
        static void transform_ascii(const _Char* __restrict _s, char* __restrict _out)
        {
            unrolled_tolower<4>::transform_ascii(_s + 0, _out + 0);
            unrolled_tolower<3>::transform_ascii(_s + 4, _out + 4);
        }
    };

    template<>
    struct unrolled_tolower<8>
    {
        template<class _Char>
        static void transform(const _Char* __restrict _s, _Char* __restrict _out)
        {
            unrolled_tolower<4>::transform(_s + 0, _out + 0);
            unrolled_tolower<4>::transform(_s + 4, _out + 4);
        }
        template<class _Char>
        static void transform_ascii(const _Char* __restrict _s, char* __restrict _out)
        {
            unrolled_tolower<4>::transform_ascii(_s + 0, _out + 0);
            unrolled_tolower<4>::transform_ascii(_s + 4, _out + 4);
        }
    };
}


template<size_t N, class T>
struct fixed_hash : // most generic version: fallback to boost::hash<T>
    public boost::hash<T>
{
    using argument_type = T;
    std::size_t operator()(const argument_type& s) const
    {
        return boost::hash_range(std::begin(s), std::begin(s) + N);
    }
};


template<class T>
struct fixed_hash<0, T> {};

template<class T>
struct fixed_hash<1, T> {};

template<size_t N, class _Char>
struct string_fixed_hash_base
{
protected:
    static size_t compute_hash(_Char* _buf)
    {
        if constexpr (sizeof(_Char) * N <= sizeof(size_t))
            return bytes_as_uint(_buf);
        else
            return boost::hash_range(_buf, _buf + N);
    }

private:
    static size_t bytes_as_uint(const _Char* _s)
    {
        size_t result = 0;
        std::memcpy(&result, _s, sizeof(result));
        return result;
    }
};

template<size_t N, class _Char, class _Traits>
struct fixed_hash<N, std::basic_string_view<_Char, _Traits>> :
    string_fixed_hash_base<N, _Char>
{
    static constexpr size_t size = N;

    using base_type = string_fixed_hash_base<N, _Char>;
    using argument_type = std::basic_string_view<_Char, _Traits>;

    std::size_t operator()(argument_type _s) const
    {
        _Char buf[detail::hash_buf_size(N)] = { 0 };
        detail::unrolled_tolower<N>::transform(_s.data(), buf);
        return base_type::compute_hash(buf);
    }
};

template<size_t N, class _Char, class _Traits, class _Alloc>
struct fixed_hash<N, std::basic_string<_Char, _Traits, _Alloc>> :
    public fixed_hash<N, std::basic_string_view<_Char, _Traits>>
{
    static constexpr size_t size = N;

    using argument_type = std::basic_string<_Char, _Traits, _Alloc>;
    using view_type = std::basic_string_view<_Char, _Traits>;
    using base_type = fixed_hash<N, view_type>;

    std::size_t operator()(const argument_type& _s) const
    {
        return base_type::operator()((view_type)_s);
    }
};


template<size_t, class>
struct fixed_hash_ascii;

template<size_t N, class _Char, class _Traits>
struct fixed_hash_ascii<N, std::basic_string_view<_Char, _Traits>> :
    string_fixed_hash_base<N, char>
{
    static constexpr size_t size = N;

    using base_type = string_fixed_hash_base<N, char>;
    using argument_type = std::basic_string_view<_Char, _Traits>;

    std::size_t operator()(argument_type _s) const
    {
        char buf[detail::hash_buf_size(N)] = { 0 };
        detail::unrolled_tolower<N>::transform_ascii(_s.data(), buf);
        return base_type::compute_hash(buf);
    }
};


