#include "stdafx.h"

#include "text_formatting.h"
#include "../json_helper.h"

namespace
{
    constexpr std::string_view c_format_type = "type";
    constexpr std::string_view c_format_bold = "bold";
    constexpr std::string_view c_format_italic = "italic";
    constexpr std::string_view c_format_underline = "underline";
    constexpr std::string_view c_format_strikethrough = "strikethrough";
    constexpr std::string_view c_format_link = "link";
    constexpr std::string_view c_format_url = "url";
    constexpr std::string_view c_format_mention = "mention";
    constexpr std::string_view c_format_inline_code = "inline_code";
    constexpr std::string_view c_format_pre = "pre";
    constexpr std::string_view c_format_ordered_list = "ordered_list";
    constexpr std::string_view c_format_unordered_list = "unordered_list";
    constexpr std::string_view c_format_quote = "quote";
    constexpr std::string_view c_format_offset = "offset";
    constexpr std::string_view c_format_length = "length";
    constexpr std::string_view c_format_start_index = "startIndex";
    constexpr std::string_view c_format_data = "data";
    constexpr std::string_view c_format_lang = "lang";
}

namespace core::data
{
    std::string_view to_string(format_type _type) noexcept
    {
        switch (_type)
        {
        case format_type::bold:
            return c_format_bold;
        case format_type::italic:
            return c_format_italic;
        case format_type::underline:
            return c_format_underline;
        case format_type::strikethrough:
            return c_format_strikethrough;
        case format_type::monospace:
            return c_format_inline_code;
        case format_type::link:
            return c_format_link;
        case format_type::mention:
            return c_format_mention;
        case format_type::quote:
            return c_format_quote;
        case format_type::pre:
            return c_format_pre;
        case format_type::ordered_list:
            return c_format_ordered_list;
        case format_type::unordered_list:
            return c_format_unordered_list;
        case format_type::none:
            return "none";
        }
        return {};
    }

    format_type read_format_type_from_string(std::string_view _name) noexcept
    {
        using ftype = core::data::format_type;
        if (_name == c_format_bold)
            return ftype::bold;
        else if (_name == c_format_italic)
            return ftype::italic;
        else if (_name == c_format_inline_code)
            return ftype::monospace;
        else if (_name == c_format_underline)
            return ftype::underline;
        else if (_name == c_format_strikethrough)
            return ftype::strikethrough;
        else if (_name == c_format_link)
            return ftype::link;
        else if (_name == c_format_mention)
            return ftype::mention;
        else if (_name == c_format_pre)
            return ftype::pre;
        else if (_name == c_format_ordered_list)
            return ftype::ordered_list;
        else if (_name == c_format_unordered_list)
            return ftype::unordered_list;
        else if (_name == c_format_quote)
            return ftype::quote;

        im_assert(!"unknown format type name");
        return ftype::none;
    };

    range range::intersected(range _other) const
    {
        const auto right = std::min(end(), _other.end());
        const auto left = std::max(offset_, _other.offset_);
        if (right > left)
            return {left, right - left};
        return {};
    }

    bool range_format::operator<(const range_format& _other) const
    {
        if (range_.offset_ == _other.range_.offset_)
        {
            if (range_.size_ == _other.range_.size_)
            {
                const auto typeNum = static_cast<uint32_t>(type_);
                const auto otherTypeNum = static_cast<uint32_t>(_other.type_);
                if (typeNum == otherTypeNum)
                {
                    if (data_ && _other.data_)
                        return *data_ < *(_other.data_);
                    else if (!data_ && _other.data_)
                        return true;
                    else
                        return false;
                }
                else
                {
                    return typeNum < otherTypeNum;
                }
            }
            else
            {
                return range_.size_ < _other.range_.size_;
            }
        }
        else
        {
            return range_.offset_ < _other.range_.offset_;
        }
    }

    bool range_format::operator==(const range_format& _other) const
    {
        return type_ == _other.type_ && range_ == _other.range_ && data_ == _other.data_;
    }

    void format::cut_at(int _string_size)
    {
        auto wasChanged = false;
        for (auto& info : formats_)
        {
            auto& r = info.range_;
            if (r.offset_ >= _string_size)
            {
                wasChanged = true;
                r = { 0, 0 };
            }
            else if (auto space_left = _string_size - r.offset_; space_left < r.size_)
            {
                wasChanged = true;
                r.size_ = space_left;
            }
        }

        if (wasChanged)
            sort();
    }

    core::data::format::format(const rapidjson::Value& _node)
    {
        im_assert(_node.IsObject());

        if (!_node.IsObject())
            return;

        const auto read_range = [](const rapidjson::Value& _value)
        {
            im_assert(_value.IsObject());
            auto result = core::data::range();
            tools::unserialize_value(_value, c_format_offset, result.offset_);
            tools::unserialize_value(_value, c_format_length, result.size_);
            tools::unserialize_value(_value, c_format_start_index, result.start_index_);
            return result;
        };

        const auto read_data = [](core::data::format_type _type, const auto& _format_params) -> core::data::format_data
        {
            auto data_str = std::string();
            if (_type == core::data::format_type::link)
                tools::unserialize_value(_format_params, c_format_url, data_str);
            else if (_type == core::data::format_type::pre)
                tools::unserialize_value(_format_params, c_format_pre, data_str);

            if (data_str.empty())
                return {};

            return { std::move(data_str) };
        };

        for (const auto& field : _node.GetObject())
        {
            const auto type = read_format_type_from_string(rapidjson_get_string_view(field.name));
            im_assert(field.value.IsArray());
            for (const auto& format_params : field.value.GetArray())
                formats_.emplace_back(type, read_range(format_params), read_data(type, format_params));
        }
    }

    format& format::operator=(const format& _other)
    {
        formats_.resize(_other.formats().size());
        std::copy(_other.formats().cbegin(), _other.formats().cend(), formats_.begin());
        return *this;
    }

    bool format::operator==(const format& _other) const
    {
        im_assert(std::is_sorted(formats_.cbegin(), formats_.cend()));
        im_assert(std::is_sorted(_other.formats_.cbegin(), _other.formats_.cend()));

        return formats_ == _other.formats();
    }

    rapidjson::Value core::data::format::serialize(rapidjson_allocator& _a) const
    {
        auto serialize_format_entry = [&_a](const core::data::range_format& _entry)
        {
            rapidjson::Value result(rapidjson::Type::kObjectType);
            result.AddMember("offset", _entry.range_.offset_, _a);
            result.AddMember("length", _entry.range_.size_, _a);
            if (_entry.range_.start_index_ > 1)
                result.AddMember("startIndex", _entry.range_.start_index_, _a);

            if (_entry.data_)
            {
                if (_entry.type_ == core::data::format_type::link)
                    result.AddMember("url", *_entry.data_, _a);

                if (_entry.type_ == core::data::format_type::pre)
                    result.AddMember("lang", *_entry.data_, _a);
            }

            return result;
        };

        rapidjson::Value result(rapidjson::Type::kObjectType);

        std::unordered_map<core::data::format_type, rapidjson::Value> arrays_map;
        for (const auto& entry : formats())
        {
            if (arrays_map.find(entry.type_) == arrays_map.end())
                arrays_map.emplace(entry.type_, rapidjson::Type::kArrayType);

            arrays_map[entry.type_].PushBack(serialize_format_entry(entry), _a);
        }

        for (auto& [format_type, arr] : arrays_map)
            result.AddMember(rapidjson::Value(core::data::to_string(format_type).data(), _a), arr, _a);

        return result;
    }

    void format::remove_formats(std::function<bool(core::data::format_type)> _pred)
    {
        if (!_pred)
            return;

        if (auto it = std::remove_if(formats_.begin(), formats_.end(), _pred); it != formats_.end())
            formats_.erase(it, formats_.end());
    }

    void format::add_start_index_to_ordered_list(int _start_index)
    {
        for (auto& f : formats_)
        {
            if (f.type_ == format_type::ordered_list)
            {
                f.range_.start_index_ = _start_index;
                return;//since formats are sorted we need to apply startIndex only to the first ordered_list
            }
        }
    }

    void format::sort()
    {
        std::sort(formats_.begin(), formats_.end());
    }

    format::builder::builder(const std::vector<range_format>& _formats)
    {
        formats_.resize(_formats.size());
        std::copy(_formats.cbegin(), _formats.cend(), formats_.begin());
    }

    format::builder& format::builder::operator%=(const range_format& _format)
    {
        formats_.emplace_back(_format);
        return *this;
    }

    format::builder& format::builder::operator%=(range_format&& _format)
    {
        formats_.push_back(std::move(_format));
        return *this;
    }

}
