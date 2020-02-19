#include "stdafx.h"

#include "suggests.h"

#include "../tools/json_helper.h"

using namespace core;
using namespace stickers;

suggests::suggests() = default;

suggests::~suggests() = default;

const std::vector<suggests::suggest>& suggests::get_suggests() const
{
    return content_;
}

void suggests::serialize(coll_helper _coll) const
{
    ifptr<iarray> suggests_array(_coll->create_array());

    for (const auto& _suggest : content_)
    {
        coll_helper coll_suggest(_coll->create_collection(), true);

        ifptr<iarray> stickers_array(_coll->create_array());

        for (const auto& _sticker_info : _suggest.info_)
        {
            coll_helper coll_info(_coll->create_collection(), true);

            coll_info.set_value_as_string("fs_id", _sticker_info.fs_id_);
            coll_info.set_value_as_string("type", ((_sticker_info.type_ == suggest_type::suggest_emoji) ? "emoji" : "word"));

            ifptr<ivalue> val(_coll->create_value());
            val->set_as_collection(coll_info.get());

            stickers_array->push_back(val.get());
        }

        coll_suggest.set_value_as_string("emoji", _suggest.emoji_);
        coll_suggest.set_value_as_array("stickers", stickers_array.get());

        ifptr<ivalue> val(_coll->create_value());
        val->set_as_collection(coll_suggest.get());
        suggests_array->push_back(val.get());
    }

    _coll.set_value_as_array("suggests", suggests_array.get());

    ifptr<iarray> alias_array(_coll->create_array());

    for (const auto& _alias : aliases_)
    {
        coll_helper coll_alias(_coll->create_collection(), true);

        coll_alias.set_value_as_string("word", _alias.word_);

        ifptr<iarray> emoji_array(_coll->create_array());

        for (const auto& _emoji : _alias.emoji_list_)
        {
            ifptr<ivalue> val_emoji(_coll->create_value());
            val_emoji->set_as_string(_emoji.c_str(), _emoji.size());

            emoji_array->push_back(val_emoji.get());
        }

        coll_alias.set_value_as_array("emojies", emoji_array.get());

        ifptr<ivalue> val(_coll->create_value());
        val->set_as_collection(coll_alias.get());

        alias_array->push_back(val.get());
    }

    _coll.set_value_as_array("aliases", alias_array.get());
}

bool suggests::unserialize(const rapidjson::Value& _node)
{
    const auto parse_stickers = [this](const rapidjson::Value& _node_data, suggest_type _type)
    {
        for (const auto& field : _node_data.GetObject())
        {
            if (!field.value.IsArray())
                continue;

            suggest sgst(rapidjson_get_string_view(field.name));

            sgst.info_.reserve(field.value.Size());

            for (const auto& sticker : field.value.GetArray())
                sgst.info_.emplace_back(rapidjson_get_string(sticker), _type);

            content_.push_back(std::move(sgst));
        }
    };

    content_.clear();
    aliases_.clear();

    if (const auto iter_data = _node.FindMember("emoji"); iter_data != _node.MemberEnd() && iter_data->value.IsObject())
        parse_stickers(iter_data->value, suggest_type::suggest_emoji);

    if (const auto iter_words = _node.FindMember("words"); iter_words != _node.MemberEnd() && iter_words->value.IsObject())
        parse_stickers(iter_words->value, suggest_type::suggest_word);

    if (const auto iter_alias = _node.FindMember("alias"); iter_alias != _node.MemberEnd() && iter_alias->value.IsObject())
    {
        for (const auto& lang : iter_alias->value.GetObject())
        {
            if (!lang.value.IsObject())
                continue;

            for (const auto& field : lang.value.GetObject())
            {
                if (!field.value.IsArray())
                    continue;

                const auto members_count = field.value.Size();

                if (members_count <= 0)
                    continue;

                std::vector<std::string> emoji_list;
                emoji_list.reserve(members_count);

                for (const auto& emoji : field.value.GetArray())
                {
                    if (!emoji.IsString())
                        continue;

                    emoji_list.push_back(rapidjson_get_string(emoji));
                }

                aliases_.emplace_back(rapidjson_get_string_view(field.name), std::move(emoji_list));
            }
        }
    }

    return true;
}
