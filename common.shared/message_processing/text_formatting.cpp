#include "stdafx.h"

#include "text_formatting.h"


namespace core::data::format
{
    bool is_style(format_type _type)
    {
        switch (_type)
        {
        case format_type::bold:
        case format_type::italic:
        case format_type::underline:
        case format_type::strikethrough:
        case format_type::inline_code:
        case format_type::link:
        case format_type::mention:
            return true;
        case format_type::quote:
        case format_type::pre:
        case format_type::ordered_list:
        case format_type::unordered_list:
        case format_type::none:
            return false;
        }
        return false;
    }

    std::string_view to_string(format_type _type)
    {
        switch (_type)
        {
        case format_type::bold:
            return "bold";
        case format_type::italic:
            return "italic";
        case format_type::underline:
            return "underline";
        case format_type::strikethrough:
            return "strikethrough";
        case format_type::inline_code:
            return "inline_code";
        case format_type::link:
            return "link";
        case format_type::mention:
            return "mention";
        case format_type::quote:
            return "quote";
        case format_type::pre:
            return "pre";
        case format_type::ordered_list:
            return "ordered_list";
        case format_type::unordered_list:
            return "unordered_list";
        case format_type::none:
            return "None";
        }
        return {};
    }

    void string_formatting::fix_invalid_ranges(int _string_size)
    {
        auto truncate_range_to_string_size = [_string_size](format_range& _r)
        {
            if (_r.offset_ >= _string_size)
                _r = { 0, 0 };
            else if (auto space_left = _string_size - _r.offset_; space_left < _r.length_)
                _r.length_ = space_left;
        };

        for (auto& info : formats_)
            truncate_range_to_string_size(info.range_);
    }

    string_formatting& string_formatting::operator=(const string_formatting& _other)
    {
        formats_.resize(_other.formats().size());
        std::copy(_other.formats().cbegin(), _other.formats().cend(), formats_.begin());
        return *this;
    }

}
