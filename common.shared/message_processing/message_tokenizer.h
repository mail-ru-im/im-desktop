#pragma once

#include <queue>

#include <boost/variant.hpp>

#include "../url_parser/url_parser.h"

namespace common
{
    namespace tools
    {
        struct message_token final
        {
            enum class type : int
            {
                undefined,
                text,
                url
            };

            typedef boost::variant<std::string, url> data_t;

            message_token();
            explicit message_token(std::string&& _text, type _type = type::text);
            explicit message_token(const url& _url);

            type type_;
            data_t data_;
        };

        class message_tokenizer final
        {
        public:
            explicit message_tokenizer(const std::string& _message, const std::string& _files_url);
            message_tokenizer(const std::string& _message, const std::string& _files_url, std::vector<url_parser::compare_item> &&_items);

            bool has_token() const;
            const message_token& current() const;

            void next();

        private:
            std::queue<message_token> tokens_;
        };
    }
}

std::ostream& operator<<(std::ostream& _out, common::tools::message_token::type _type);
