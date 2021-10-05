#pragma once

#include <optional>
#include <vector>
#include <string>
#include <map>

#include "../json_unserialize_helpers.h"


namespace core
{
namespace data
{
    enum class format_type : uint32_t
    {
        none = 0u,

        bold = 1u << 0,
        italic = 1u << 1,
        underline = 1u << 2,
        strikethrough = 1u << 3,
        monospace = 1u << 4,
        link = 1u << 5,

        mention = 1u << 6,

        quote = 1u << 7,
        pre = 1u << 8,
        ordered_list = 1u << 9,
        unordered_list = 1u << 10,
    };


    constexpr std::array<format_type, 11> get_all_format_types() noexcept
    {
        return {
            format_type::bold,
            format_type::italic,
            format_type::underline,
            format_type::strikethrough,
            format_type::monospace,
            format_type::link,
            format_type::mention,
            format_type::quote,
            format_type::pre,
            format_type::ordered_list,
            format_type::unordered_list,
        };
    }

    constexpr std::array<format_type, 7> get_all_styles() noexcept
    {
        return {
            format_type::bold,
            format_type::italic,
            format_type::underline,
            format_type::strikethrough,
            format_type::monospace,
            format_type::link,
            format_type::mention,
        };
    }

    constexpr std::array<format_type, 6> get_all_non_mention_styles() noexcept
    {
        return {
            format_type::bold,
            format_type::italic,
            format_type::underline,
            format_type::strikethrough,
            format_type::monospace,
            format_type::link,
        };
    }

    //! Return true if type is text style, and false if it is none or a block format (like quote or list)
    constexpr bool is_style(format_type _type) noexcept
    {
        using ft = format_type;
        return _type == ft::bold
            || _type == ft::italic
            || _type == ft::underline
            || _type == ft::strikethrough
            || _type == ft::monospace
            || _type == ft::link
            || _type == ft::mention;
    }

    //! Return true if type is text style, and false if it is none or a block format (like quote or list)
    constexpr bool is_style_or_none(format_type _type) noexcept
    {
        return _type == format_type::none || is_style(_type);
    }

    constexpr bool is_block_format(format_type _type) noexcept
    {
        return _type != format_type::none && !is_style(_type);
    }


    std::string_view to_string(format_type _type) noexcept;

    format_type read_format_type_from_string(std::string_view _name) noexcept;

    struct range
    {
        int offset_ = 0;
        int size_ = 0;

        range() = default;
        range(int _offset, int _size) : offset_(_offset), size_(_size) {}

        bool operator==(const range& _other) const { return offset_ == _other.offset_ && size_ == _other.size_; }
        bool operator<(const range&  _other) const { return offset_ < _other.offset_; }

        range intersected(range _other) const;
        int end() const { return offset_ + size_; }
    };

    using format_data = std::optional<std::string>;


    struct range_format
    {
        range_format() = default;
        range_format(format_type _type, range _range, format_data _data = {})
            : type_(_type), range_(_range), data_(_data) {}
        range_format(format_type _type, int _offset, int _length, format_data _data = {})
            : type_(_type), range_{ _offset, _length }, data_(_data) {}

        bool operator<(const range_format& _other) const;
        bool operator==(const range_format& _other) const;
        operator format_type() const noexcept { return type_; }

        format_type type_ = format_type::none;
        range range_;
        format_data data_;
    };


    class format
    {
    public:
        using rapidjson_allocator = rapidjson::MemoryPoolAllocator<rapidjson::CrtAllocator>;

        class builder
        {
        public:
            builder() = default;
            builder(int _reserve_size) : formats_() { formats_.reserve(_reserve_size); }
            builder(const std::vector<range_format>& _formats);
            builder& operator%=(const range_format& _format);
            builder& operator%=(range_format&& _format);
            format finalize() { return { std::move(formats_) }; }

        protected:
            std::vector<range_format> formats_;
        };

    public:
        format() = default;

        format(std::vector<range_format>&& _formats)
        {
            assert(std::all_of(_formats.cbegin(), _formats.cend(), [](const range_format& _ft)
            {
                return _ft.type_ != format_type::none;
            }));

            formats_ = std::move(_formats);
            sort();
        }

        format(range_format&& _format)
            : format(std::vector{ std::move(_format) })
        {
        }

        format(const rapidjson::Value& _node);

        format(const format& _other) = default;

        builder get_builder() const { return builder(formats_); }

        format& operator=(format&& _other) = default;

        format& operator=(const format& _other);

        bool empty() const { return formats_.empty(); }

        //! Chop ranges which exceed string boundaries
        void cut_at(int _string_size);

        void clear() { formats_.clear(); }

        const auto& formats() const { return formats_; }

        bool operator==(const format& _other) const;

        bool operator!=(const format& _other) const { return !operator==(_other); }

        rapidjson::Value serialize(rapidjson_allocator& _a) const;

        void remove_formats(std::function<bool(core::data::format_type)> _pred);

    protected:
        //! Sorted. To speed up comparison
        std::vector<range_format> formats_;

    protected:
        void sort();
    };
}
}

