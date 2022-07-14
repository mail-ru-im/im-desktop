#include "stdafx.h"

#include <unordered_map>

#include "message_tokenizer.h"
#include "../core/tools/strings.h"
#include "../uri_matcher/uri_matcher.h"

#include "../core/core.h"
#include "../config/config.h"

namespace
{
    // https://jira.vk.team/browse/IMDESKTOP-18772 Fix view url profile vkteams group, nickname icq and talks vk
    namespace helper_parse_url
    {
        using tokenizer_char = wchar_t;
        using tokenizer_string = std::basic_string<tokenizer_char>;

        enum class domain_type 
        {
            vkteams,
            icq,
            vk
        };

        bool check_punct_url(const tokenizer_string& _url) 
        {
            static std::unordered_map<domain_type, tokenizer_string> domain_url_ =
            {
                { domain_type::vkteams, L"://u.internal.myteam.mail.ru/profile" },
                { domain_type::icq, L"://icq.im" },
                { domain_type::vk, L"://vk.com" }
            };

            // lambda until we not use C++20
            const auto start_with = [](const tokenizer_string& _str, const tokenizer_string& _prefix) -> bool
            {
                return _str.size() >= _prefix.size() && 0 == _str.compare(0, _prefix.size(), _prefix);
            };

            const auto start = _url.find_first_of(L"://");
            if (start > _url.size() || start == tokenizer_string::npos)
                return true;

            const auto path = _url.substr(start, _url.size());
            if (start_with(path, domain_url_[domain_type::vkteams]) || start_with(path, domain_url_[domain_type::icq]))
                return false;
            else if (start_with(path, domain_url_[domain_type::vk]) && path.back() == L'=')
                return false;
            return true;
        }
    }

    /*!
     * Exclude smaller range from a sorted vector of ranges
     * \param ranges vector of splitted ranges
     * \param victim range to exclude
     * \detail
     * Algorithm excludes victim range from list of ranges represented by ranges argument.
     * The common case is when we start from single full range and then exclude some ranges
     * one-by-one.
     * On each call of exclude_range_inplace the ranges vector is being modified in a
     * following way:
     * - the last element is popped from back and stored in a temporary variable.
     * - if offset of victim is same as offset of last popped element, than it trimmed to the length of victim.
     * - otherwise it splits into two (leftwise and rightwise beside the victim).
     * - all the received ranges are stored back into ranges vector.
     * \note: Algorithm rely on that ranges vector is sorted in ascending order by offsets of ranges,
     * and also suppose that victim range is always contained in the very last one element of ranges
     * vector (i.e. in the greatest one). All empty ranges are discarded.
     */
    inline void exclude_range_inplace(std::vector<core::data::range>& _ranges, const core::data::range& _victim)
    {
        im_assert(std::is_sorted(_ranges.begin(), _ranges.end()));
        if (_ranges.empty())
            return;
        auto r = _ranges.back();
        if (_victim.size_ == 0 || _victim.offset_ < r.offset_ || _victim.end() > r.end())
            return;

        _ranges.pop_back(); // get the greatest range

        if (r.offset_ == _victim.offset_) // trim
        {
            r.offset_ = r.offset_ + _victim.size_;
            r.size_ -= _victim.size_;
            if (r.size_ != 0)  // discard empty range
                _ranges.emplace_back(r);
        }
        else // split - warning: the order of emplace_back() calls is important
        {
            size_t offset = r.offset_;
            size_t size = _victim.offset_ - r.offset_;
            if (size != 0) // discard empty empty range
                _ranges.emplace_back(offset, size);

            offset = _victim.offset_ + _victim.size_;
            size = (r.offset_ + r.size_) - offset;
            if (size != 0)  // discard empty empty range
                _ranges.emplace_back(offset, size);
        }
    }
}

common::tools::message_token::message_token()
    : type_(type::undefined)
{
}

common::tools::message_token::message_token(tokenizer_string&& _text, type _type, int _source_offset)
    : type_(_type)
    , data_(std::move(_text))
    , formatted_source_offset_(_source_offset)
{
}

common::tools::message_token::message_token(tokenizer_string&& _url, int32_t scheme_, int _source_offset)
    : type_(type::url)
    , data_(url_view(_url, scheme_))
    , formatted_source_offset_(_source_offset)
{
}

common::tools::message_tokenizer::message_tokenizer(std::u16string_view _message)
    : message_tokenizer(std::wstring{ _message.begin(), _message.end() })
{
}

common::tools::message_tokenizer::message_tokenizer(std::u16string_view _message,
                                                    const core::data::format& _formatting)
    : message_tokenizer(std::wstring{ _message.begin(), _message.end() }, _formatting.formats())
{
}

common::tools::message_tokenizer::message_tokenizer(std::u16string_view _message, const std::vector<core::data::range_format>& _formats)
    : message_tokenizer(std::wstring{ _message.begin(), _message.end() }, _formats)
{
}

common::tools::message_tokenizer::message_tokenizer(const tokenizer_string& _message)
    : token_text_type_(message_token::type::text)
{
    parse_message(_message, {});
}

common::tools::message_tokenizer::message_tokenizer(const tokenizer_string& _message,
                                                    const std::vector<core::data::range_format>& _formats)
    : token_text_type_(message_token::type::formatted_text)
{
    parse_message(_message, _formats);
}

bool common::tools::message_tokenizer::has_token() const
{
    return !tokens_.empty();
}

const common::tools::message_token& common::tools::message_tokenizer::current() const
{
    return tokens_.front();
}

void common::tools::message_tokenizer::next()
{
    if (has_token())
        tokens_.pop_front();
}

void common::tools::message_tokenizer::parse_message(const tokenizer_string& _message, const std::vector<core::data::range_format>& _formats)
{
    using core::data::range;
    using ftype = core::data::format_type;

    static auto is_excluded = [](const core::data::range_format& _fmt) -> bool
    {
        return (_fmt.type_ == ftype::link || _fmt.type_ == ftype::mention || _fmt.type_ == ftype::pre);
    };

    range _message_range = { 0, (int)_message.size() }; // full range of message

    if (_formats.empty())
    {
        // no formats - process the whole message
        parse_range(_message_range, _message);
    }
    else
    {
        // formats must be sorted - see format class in text_formatting.cpp
        im_assert(std::is_sorted(_formats.begin(), _formats.end()));

        std::vector<range> unformatted_ranges = { _message_range };

        for (const auto& _fmt : _formats)
        {
            if (is_excluded(_fmt)) // excluded formats
            {
                exclude_range_inplace(unformatted_ranges, _fmt.range_);
                tokens_.emplace_back(_message.substr(_fmt.range_.offset_, _fmt.range_.size_), token_text_type_, _fmt.range_.offset_);
            }
        }

        // process unformatted ranges
        if (!unformatted_ranges.empty())
        {
            for (const auto& range : unformatted_ranges)
                parse_range(range, _message);
        }
    }

    std::sort(tokens_.begin(), tokens_.end(), [](const auto& x, const auto& y) { return x.formatted_source_offset_ < y.formatted_source_offset_; });
}

void common::tools::message_tokenizer::parse_range(const core::data::range& _range, const common::tools::tokenizer_string& _message)
{
    using matcher_type = basic_uri_matcher<tokenizer_char>;

    tokenizer_string_view sv = _message; // get the string view from message to avoid allocations on substr()
    sv = sv.substr(_range.offset_, _range.size_);

    tokenizer_string_view result;

    scheme_type scheme;
    matcher_type matcher;
    size_t pos = _range.offset_;

    auto p = matcher.search(sv.begin(), sv.end(), result, scheme, helper_parse_url::check_punct_url({ sv.begin(), sv.end() }));
    while (p != sv.end())
    {
        // compute size and offset
        size_t size = result.size();
        size_t offset = _range.offset_ + std::distance(sv.begin(), p - size);
        if (offset > pos) // add whitespaces tokens
            tokens_.emplace_back(_message.substr(pos, offset - pos), token_text_type_, pos);
        pos = offset + size; // adjust current position

        if (!result.empty())
        {
            // add url token
            tokens_.emplace_back((tokenizer_string)result, (int32_t)scheme, offset);
            result = {};
        }

        p = matcher.search(p, sv.end(), result, scheme);
    }

    // process the last result
    if (!result.empty())
    {
        // compute size and offset
        size_t size = result.size();
        size_t offset = _range.offset_ + std::distance(sv.begin(), p - size);
        if (offset > pos) // add the whitespaces token
            tokens_.emplace_back(_message.substr(pos, offset - pos), token_text_type_, pos);
        tokens_.emplace_back((tokenizer_string)result, (int32_t)scheme, offset);
        pos = offset + size; // adjust current position
    }

    if (pos < (size_t)_range.end()) // add trailing whitespaces token
        tokens_.emplace_back(_message.substr(pos, _range.end() - pos), token_text_type_, pos);
}

std::ostream& operator<<(std::ostream& _out, common::tools::message_token::type _type)
{
    switch (_type)
    {
    case common::tools::message_token::type::undefined:
        _out << "undefined";
        break;
    case common::tools::message_token::type::text:
        _out << "text";
        break;
    case common::tools::message_token::type::url:
        _out << "url";
        break;
    case common::tools::message_token::type::formatted_text:
        _out << "formatted_text";
        break;
    default:
        _out << "#unknown";
        im_assert(!"invalid token type");
    }
    return _out;
}
