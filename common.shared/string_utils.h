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
}