#include "stdafx.h"

#include "message_tokenizer.h"

common::tools::message_token::message_token()
    : type_(type::undefined)
{
}

common::tools::message_token::message_token(std::string&& _text, type _type)
    : type_(_type)
    , data_(std::move(_text))
{
}

common::tools::message_token::message_token(const url& _url)
    : type_(type::url)
    , data_(_url)
{
}

common::tools::message_tokenizer::message_tokenizer(
    const std::string& _message,
    const std::string& _files_url,
    std::vector<url_parser::compare_item>&& _items)
{
    size_t prev = 0;
    size_t i = 0;

    url_parser parser(_files_url);

    parser.add_fixed_urls(std::move(_items));

    auto append_tokens = [&_message, &parser, &prev, &i, this]()
    {
        const auto url_size = parser.raw_url_length();
        const auto text_size = i - prev - url_size - parser.tail_size();
        if (text_size > 0)
            tokens_.push(message_token(_message.substr(prev, text_size)));

        tokens_.push(message_token(parser.get_url()));

        parser.reset();

        prev += (text_size + url_size);
    };


    for (i = 0; i < _message.size(); ++i)
    {
        if (_message[i] == '@' && _message[i + 1] == '[' && i < _message.size() - 2)
        {
            for (auto j = i + 2; j < _message.size(); ++j)
            {
                if (_message[j] == ']')
                {
                    i = j;
                    break;
                }
            }
        }

        parser.process(_message[i]);

        if (parser.has_url())
            append_tokens();
    }

    parser.finish();

    if (parser.has_url())
    {
        append_tokens();

        if (prev < i)
            tokens_.push(message_token(_message.substr(prev, i - prev)));
    }
    else
    {
        const auto text_size = i - prev;
        if (text_size > 0)
            tokens_.push(message_token(_message.substr(prev, i - prev)));
    }

    tokens_.push(message_token()); // terminator
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
    default:
        _out << "#unknown";
        assert(!"invalid token type");
    }
    return _out;
}
