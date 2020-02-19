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
}