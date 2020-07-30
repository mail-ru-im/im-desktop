#pragma once

namespace core
{
    namespace smartreply
    {
        enum class type
        {
            invalid,

            sticker,
            sticker_by_text,
            text,
            audiocall,
            videocall,
        };

        std::string_view type_2_string(const smartreply::type _type) noexcept;
        smartreply::type string_2_type(const std::string_view _str) noexcept;
        std::string_view array_node_name_for(const smartreply::type _type) noexcept;
    }
}