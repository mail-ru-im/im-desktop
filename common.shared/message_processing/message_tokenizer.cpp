#include "stdafx.h"

#include "message_tokenizer.h"
#include "../core/tools/strings.h"

namespace fmt = core::data::format;


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

common::tools::message_token::message_token(const url& _url, int _source_offset)
    : type_(type::url)
    , data_(_url)
    , formatted_source_offset_(_source_offset)
{
}

common::tools::message_tokenizer::message_tokenizer(
    const tokenizer_string& _message,
    const std::string& _files_url,
    std::vector<url_parser::compare_item>&& _items)
    : token_text_type_(message_token::type::text)
{
    parse(_message, {}, _files_url, std::move(_items));
}

common::tools::message_tokenizer::message_tokenizer(
    const tokenizer_string& _message,
    const fmt::string_formatting& _formatting,
    const std::string& _files_url,
    std::vector<url_parser::compare_item>&& _items)
    : token_text_type_(message_token::type::formatted_text)
{
    parse(_message, _formatting, _files_url, std::move(_items));
}

bool common::tools::message_tokenizer::has_token() const
{
    return tokens_.size() > 1;
}

const common::tools::message_token& common::tools::message_tokenizer::current() const
{
    return tokens_.front();
}

void common::tools::message_tokenizer::next()
{
    if (has_token())
        tokens_.pop();
}

void common::tools::message_tokenizer::parse(const tokenizer_string& _message, const core::data::format::string_formatting& _formatting, const std::string& _files_url, std::vector<url_parser::compare_item>&& _items)
{
    using namespace core::data::format;
    using manual_url_range = std::pair<fmt::format_range, tokenizer_string_view>;

    auto add_text_token = [this](tokenizer_string&& _s, int _source_offset)
    {
        tokens_.push(message_token(std::move(_s), token_text_type_, _source_offset));
    };

    const auto get_ranges_to_skip_sorted = [](const string_formatting& _f)
    {
        auto result = std::vector<manual_url_range>();

        for (const auto& format : _f.formats())
        {
            if (format.type_ == format_type::link || format.type_ == format_type::mention || format.type_ == format_type::pre)
                result.emplace_back(format.range_, tokenizer_string_view());
        }

        std::sort(result.begin(), result.end(),
            [](const auto& _r1, const auto& _r2) { return _r1.first.offset_ < _r2.first.offset_; });

        return result;
    };

    const auto ranges_to_skip = get_ranges_to_skip_sorted(_formatting);

    auto get_next_range_start = [end_it = ranges_to_skip.cend(), end_pos = _message.size()](const auto it)
    {
        return (it == end_it) ? end_pos : it->first.offset_;
    };

    size_t prev = 0;
    size_t i = 0;
    url_parser_utf16 parser(_files_url);
    parser.add_fixed_urls(std::move(_items));

    auto append_tokens = [&_message, &parser, &prev, &i, this, &add_text_token]()
    {
        const auto url_size = parser.raw_url_length();
        const auto text_size = i - prev - url_size - parser.tail_size();
        if (text_size > 0)
            add_text_token(_message.substr(prev, text_size), prev);

        tokens_.push(message_token(parser.get_url(), i - url_size));
        parser.reset();
        prev += text_size + url_size;
    };

    const auto message_size = _message.size();
    auto range_it = ranges_to_skip.cbegin();
    auto range_start = get_next_range_start(range_it);
    for (i = 0; i < message_size; )
    {
        if (i == range_start)
        {
            const auto skip_range = range_it->first;
            const auto manual_url = range_it->second;

            if (auto n = i - prev; n > 0)
                add_text_token(_message.substr(prev, n), prev);

            parser.reset();
            auto range_text = _message.substr(i, skip_range.length_);
            if (manual_url.empty())
            {
                add_text_token(std::move(range_text), i);
            }
            else
            {
                for (const auto ch : manual_url)
                    parser.process(ch);
                parser.finish();
                if (parser.has_url())
                    add_text_token(std::move(range_text), i);

                parser.reset();
            }
            i += skip_range.length_;
            if (i >= message_size)
                i = message_size;
            prev = i;
            ++range_it;
            range_start = get_next_range_start(range_it);
        }

        if (i < message_size)
            parser.process(_message[i]);
        else
            break;

        if (parser.has_url())
            append_tokens();

        ++i;
    }

    parser.finish();

    if (parser.has_url())
    {
        append_tokens();
        if (prev < i)
            add_text_token(_message.substr(prev, i - prev), prev);
    }
    else
    {
        if (const auto text_size = i - prev; text_size > 0)
            add_text_token(_message.substr(prev, text_size), prev);
    }

    tokens_.push(message_token()); // terminator
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
        assert(!"invalid token type");
    }
    return _out;
}
