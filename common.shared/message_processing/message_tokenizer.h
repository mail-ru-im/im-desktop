#pragma once

#include <queue>

#include <boost/variant.hpp>
#include <variant>

#include "text_formatting.h"

namespace common
{
    namespace tools
    {
        using tokenizer_char = wchar_t;
        using tokenizer_string = std::basic_string<tokenizer_char>;
        using tokenizer_string_view = std::basic_string_view<tokenizer_char>;

        struct url_view
        {
            tokenizer_string text_;
            int32_t scheme_;
            url_view(const tokenizer_string& _u, int32_t _scheme)
                : text_(_u)
                , scheme_(_scheme)
            {}
        };

        struct message_token final
        {
            enum class type : int
            {
                undefined,
                text,
                formatted_text,
                url,
            };

            using data_t = boost::variant<tokenizer_string, url_view>;

            message_token();
            message_token(tokenizer_string&& _text, type _type, int _source_offset);
            message_token(tokenizer_string&& _url, int32_t scheme_, int _source_offset);

            type type_;
            data_t data_;
            int formatted_source_offset_ = -1;
        };

        class message_tokenizer final
        {
        public:
            message_tokenizer(std::u16string_view _message);
            message_tokenizer(std::u16string_view _message, const core::data::format& _formatting);
            message_tokenizer(std::u16string_view _message, const std::vector<core::data::range_format>& _formats);

            message_tokenizer(const tokenizer_string& _message);
            message_tokenizer(const tokenizer_string& _message, const std::vector<core::data::range_format>& _formats);

            bool has_token() const;
            const message_token& current() const;

            void next();

            message_token::type token_text_type() const { return token_text_type_; }

        private:
            void parse_message(const tokenizer_string& _message, const std::vector<core::data::range_format>& _formats);
            void parse_range(const core::data::range& _range, const tools::tokenizer_string& _message);

            std::deque<message_token> tokens_;
            message_token::type token_text_type_;
        };
    }
}

std::ostream& operator<<(std::ostream& _out, common::tools::message_token::type _type);
