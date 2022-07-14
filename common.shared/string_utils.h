#pragma once

#include <string_view>
#include <string>

namespace su
{
    inline namespace impl
    {
        [[nodiscard]] constexpr size_t size(std::string_view s) noexcept
        {
            return s.size();
        }

        [[nodiscard]] constexpr size_t size(char) noexcept
        {
            return 1;
        }

        [[nodiscard]] constexpr size_t size(std::wstring_view s) noexcept
        {
            return s.size();
        }

        [[nodiscard]] constexpr size_t size(wchar_t) noexcept
        {
            return 1;
        }

        template<typename T, typename ...Args>
        T concat_impl(Args&&... args)
        {
            T res;
            res.reserve((size(args) + ...));
            (res.operator+=(std::forward<Args>(args)), ...);
            return res;
        }

        template<typename T, typename U, typename V>
        T join_impl(U first, U last, V sep)
        {
            T res;

            if (first != last)
            {
                res += *first;
                ++first;
            }

            for (; first != last; ++first)
            {
                res += sep;
                res += *first;
            }

            return res;
        }
    }

    template<typename ...Args>
    [[nodiscard]] std::string concat(Args&&... args)
    {
        return concat_impl<std::string>(std::forward<Args>(args)...);
    }

    template<typename ...Args>
    [[nodiscard]] std::wstring wconcat(Args&&... args)
    {
        return concat_impl<std::wstring>(std::forward<Args>(args)...);
    }

    template<typename T>
    [[nodiscard]] std::string join(T first, T last, std::string_view sep)
    {
        return join_impl<std::string>(first, last, sep);
    }

    template<typename T>
    [[nodiscard]] std::wstring join(T first, T last, std::wstring_view sep)
    {
        return join_impl<std::wstring>(first, last, sep);
    }

    template<typename T, typename U>
    [[nodiscard]] bool starts_with(T source, U prefix)
    {
        if (std::size(prefix) > std::size(source))
            return false;
        return std::equal(std::cbegin(prefix), std::cend(prefix), std::cbegin(source));
    }

    template<typename T, typename U>
    [[nodiscard]] bool ends_with(T _source, U _suffix)
    {
        if (std::size(_suffix) > std::size(_source))
            return false;
        return std::equal(std::crbegin(_suffix), std::crend(_suffix), std::crbegin(_source));
    }

    enum class token_compress
    {
        off = 0,
        on = 1
    };

    // like boost::split, but much more faster
    template<class _Char, class _Traits, class _Pred, class _OutIt>
    _OutIt split_if(std::basic_string_view<_Char, _Traits> source, _OutIt out, _Pred p, token_compress mode = token_compress::on)
    {
        auto first = source.begin();
        auto eos = source.end();
        while (first != eos)
        {
            auto second = std::find_if(first, eos, p);
            if (mode == token_compress::on && first == second)
            {
                first = ++second;
                continue;
            }

            *out = { std::addressof(*first), static_cast<size_t>(std::distance(first, second)) };
            ++out;

            if (second == eos)
                break;

            first = ++second;
        }

        return out;
    }

    // wrapper for split_if, provided for convenience
    template<class _Char, class _Traits, class _OutIt>
    _OutIt split(std::basic_string_view<_Char, _Traits> source, _OutIt out, _Char delim = _Char(' '), token_compress mode = token_compress::off)
    {
        return split_if(source, out, [delim](auto c) { return c == delim; }, mode);
    }


    /*!
     * \brief find position of first unbalanced bracket
     */
    template<class _InIt, class _Char, class _Traits>
    _InIt brackets_mismatch(_InIt _first, _InIt _last,
        std::basic_string_view<_Char, _Traits> open_set,
        std::basic_string_view<_Char, _Traits> close_set)
    {
        // Maximum amount of stack allocated memory:
        // 512 bytes is enough for the most cases
        constexpr size_t MAX_LOCAL_STACKALLOC = 512;
        constexpr size_t npos = std::basic_string_view<_Char, _Traits>::npos;

        if (open_set.size() != close_set.size())
            return _last;

        size_t* counters = nullptr;
        const size_t n = open_set.size();
        const size_t sz = n * sizeof(size_t);
        if (sz > MAX_LOCAL_STACKALLOC)
            counters = new size_t[n];
        else
            counters = (size_t*)alloca(n * sizeof(size_t));

#if defined(__GNUC__)
        // workaround for bug in GCC libstdc++ (see
        // https://travisdowns.github.io/blog/2020/01/20/zero.html for details)
        __builtin_memset(counters, 0, sizeof(size_t) * n);
#else
        std::fill_n(counters, n, 0);
#endif

        /*
         * Process the string from left to right. Maintain a counters of
         * unmatched opening parentheses.
         * - If we see an opening parenthesis, increment the counter.
         * - If we see a closing parenthesis and the counter is non-zero, decrement the counter.
         * - If we see a closing parenthesis and the counter is zero, return the current position
         * as a mismatched closing parenthesis.
         */
        for (; _first != _last; ++_first)
        {
            auto i = open_set.find(*_first);
            if (i != npos)
                ++counters[i];

            i = close_set.find(*_first);
            if (i != npos)
            {
                if (counters[i] != 0)
                    --counters[i];
                else
                    break;
            }
        }

        if (sz > MAX_LOCAL_STACKALLOC)
            delete[] counters;

        return _first;
    }
}