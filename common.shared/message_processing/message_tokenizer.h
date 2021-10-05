#pragma once

#include <queue>

#include <boost/variant.hpp>

#include "../url_parser/url_parser.h"
#include "text_formatting.h"

namespace common
{
    namespace tools
    {
        using tokenizer_char = char16_t;
        using tokenizer_string = std::basic_string<tokenizer_char>;
        using tokenizer_string_view = std::basic_string_view<tokenizer_char>;

        struct message_token final
        {
            enum class type : int
            {
                undefined,
                text,
                formatted_text,
                url,
            };

            using data_t = boost::variant<tokenizer_string, url>;

            message_token();
            explicit message_token(tokenizer_string&& _text, type _type = type::text, int _source_offset = -1);
            explicit message_token(const url& _url, int _source_offset = -1);

            type type_;
            data_t data_;
            int formatted_source_offset_ = -1;
        };

        class message_tokenizer final
        {
        public:
            message_tokenizer(const tokenizer_string& _message, const std::string& _files_url, std::vector<url_parser::compare_item>&& _items);
            message_tokenizer(const tokenizer_string& _message, const core::data::format& _formatting, const std::string& _files_url, std::vector<url_parser::compare_item>&& _items);

            bool has_token() const;
            const message_token& current() const;

            void next();

            message_token::type token_text_type() const { return token_text_type_; }

        private:
            void parse(const tokenizer_string& _message, const core::data::format& _formatting, const std::string& _files_url, std::vector<url_parser::compare_item>&& _items);

            std::queue<message_token> tokens_;
            message_token::type token_text_type_;
        };
    }
}

std::ostream& operator<<(std::ostream& _out, common::tools::message_token::type _type);
