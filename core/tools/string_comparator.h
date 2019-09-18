#pragma once

namespace core
{
    namespace tools
    {
        struct string_comparator
        {
            using is_transparent = std::true_type;

            bool operator()(std::string_view lhs, std::string_view rhs) const noexcept { return lhs < rhs; }
        };
    }
}