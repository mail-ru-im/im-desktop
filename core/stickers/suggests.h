#pragma once

#include "../../corelib/collection_helper.h"

namespace core
{
    namespace stickers
    {
        class suggests final
        {
            enum class suggest_type
            {
                suggest_emoji = 1 << 0,
                suggest_word = 1 << 1
            };

            struct sticker_info
            {
                const std::string fs_id_;
                const suggest_type type_;

                sticker_info(std::string&& _fs_id, const suggest_type _suggest_type)
                    : fs_id_(std::move(_fs_id))
                    , type_(_suggest_type)
                {
                }

                bool is_emoji() const noexcept { return type_ == suggest_type::suggest_emoji; }
            };

            struct suggest
            {
                const std::string emoji_;

                std::vector<sticker_info> info_;

                suggest(std::string_view _emoji)
                    : emoji_(_emoji)
                {
                }
            };

            std::vector<suggest> content_;

            struct word_suggest
            {
                std::string word_;

                std::vector<std::string> emoji_list_;

                word_suggest(std::string_view _word, std::vector<std::string>&& _emoji_list)
                    : word_(_word)
                     , emoji_list_(std::move(_emoji_list))
                {

                }
            };

            std::vector<word_suggest> aliases_;

        public:
            suggests();
            ~suggests();

            const std::vector<suggest>& get_suggests() const;

            void serialize(coll_helper _coll) const;
            bool unserialize(const rapidjson::Value& _node);
        };
    }
}

