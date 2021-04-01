#pragma once

#include <optional>
#include <vector>
#include <string>
#include <map>
#include <cassert>


namespace core
{
namespace data
{
namespace format
{
    enum class format_type : uint32_t
    {
        none = 0u,
        bold = 1u << 0,
        italic = 1u << 1,
        underline = 1u << 2,
        strikethrough = 1u << 3,
        inline_code = 1u << 4,
        link = 1u << 5,
        mention = 1u << 6,
        quote = 1u << 7,
        pre = 1u << 8,
        ordered_list = 1u << 9,
        unordered_list = 1u << 10,
    };

    //! Return true if type is text style, and false if it is a block format (like quote or list)
    bool is_style(format_type _type);

    std::string_view to_string(format_type _type);

    struct format_range
    {
        int offset_ = 0;
        int length_ = 0;

        bool operator==(const format_range& _other) const { return offset_ == _other.offset_ && length_ == _other.length_; }
        bool operator<(const format_range&  _other) const { return offset_ < _other.offset_; }
    };

    using format_data = std::optional<std::string>;


    struct format
    {
        format() = default;
        format(format_type _type, format_range _range, format_data _data = {})
            : type_(_type), range_(_range), data_(_data) {}

        format_type type_ = format_type::none;
        format_range range_;
        format_data data_;
    };


    class string_formatting
    {
    public:
        inline bool empty() const { return formats_.empty(); }

        inline void add(const format& _info) { formats_.emplace_back(_info); }

        //! Limit ranges which exceed string boundaries
        void fix_invalid_ranges(int _string_size);

        inline void clear() { formats_.clear(); }

        inline const auto& formats() const { return formats_; }

        inline auto& formats() { return formats_; }

        string_formatting& operator=(const string_formatting& _other);

    protected:
        std::vector<format> formats_;
    };
}
}
}
