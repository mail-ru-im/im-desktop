#pragma once

#include "../omicron_keys.h"

namespace feature
{
    namespace smartreply
    {
        struct omicron_field
        {
            std::string_view name_;
            std::variant<bool, int> default_value_;

            template <typename T> constexpr T def() const noexcept
            {
                if (auto ptr = std::get_if<T>(&default_value_))
                    return *ptr;
                return T{};
            }
            constexpr const char* name() const noexcept { return name_.data(); }
        };

        constexpr omicron_field is_enabled() { return { omicron::keys::smartreply_suggests_feature_enabled, true }; }           // whole feature switch
        constexpr omicron_field text_enabled() { return { omicron::keys::smartreply_suggests_text, true }; }                     // is text suggests enabled
        constexpr omicron_field sticker_enabled() { return { omicron::keys::smartreply_suggests_stickers, true }; }              // is sticker suggests enabled
        constexpr omicron_field is_enabled_for_quotes() { return { omicron::keys::smartreply_suggests_for_quotes, true }; }      // is smartreplies for quotes enabled
        constexpr omicron_field click_hide_timeout() { return { omicron::keys::smartreply_suggests_click_hide_timeout, 150 }; } // timeout after click, milliseconds
        constexpr omicron_field msgid_cache_size() { return { omicron::keys::smartreply_suggests_msgid_cache_size, 1 }; }        // cache size for when storing smartreplies in memory
    }
}