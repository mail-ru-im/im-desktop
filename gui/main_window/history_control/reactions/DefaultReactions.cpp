#include "stdafx.h"

#include "utils/features.h"
#include "DefaultReactions.h"
#include "cache/emoji/EmojiCode.h"
#include "utils/InterConnector.h"
#include "../common.shared/json_unserialize_helpers.h"


namespace
{
    const std::vector<Ui::ReactionWithTooltip>& defaultReactionsWithTooltipSet()
    {
        static const std::vector<Ui::ReactionWithTooltip> reactions =
        {
            { Emoji::EmojiCode::toQString(Emoji::EmojiCode(0x1f44d)),           QT_TRANSLATE_NOOP("reactions", "Like") },
            { Emoji::EmojiCode::toQString(Emoji::EmojiCode(0x2764, 0xfe0f)),    QT_TRANSLATE_NOOP("reactions", "Super") },
            { Emoji::EmojiCode::toQString(Emoji::EmojiCode(0x1f923)),           QT_TRANSLATE_NOOP("reactions", "Funny") },
            { Emoji::EmojiCode::toQString(Emoji::EmojiCode(0x1f633)),           QT_TRANSLATE_NOOP("reactions", "Oops") },
            { Emoji::EmojiCode::toQString(Emoji::EmojiCode(0x1f622)),           QT_TRANSLATE_NOOP("reactions", "Sad") },
            { Emoji::EmojiCode::toQString(Emoji::EmojiCode(0x1f621)),           QT_TRANSLATE_NOOP("reactions", "Scandalous") },
        };

        return reactions;
    }
}

namespace Ui
{

class DefaultReactions_p
{
public:
    std::vector<ReactionWithTooltip> reactions_;

    void updateDefaultReactions()
    {
        // json format example:  "[{\"👍\" : \"tooltip\"}, {\"❤️\" : \"tooltip\"}, {\"🤣\" : \"tooltip\"}, {\"😳\" : \"tooltip\"}, {\"😢\" : \"tooltip\"}, {\"😡\" : \"tooltip\"}]"

        auto json = Features::getReactionsJson();
        if (json.empty())
        {
            reactions_ = defaultReactionsWithTooltipSet();
            return;
        }

        rapidjson::Document doc;
        doc.Parse(json);

        if (doc.HasParseError() || !doc.IsArray())
        {
            reactions_ = defaultReactionsWithTooltipSet();
            return;
        }

        const auto jsonArray = doc.GetArray();

        reactions_.clear();
        reactions_.reserve(jsonArray.Size());

        for (const auto& item : jsonArray)
        {
            for (const auto& field : item.GetObject())
            {
                ReactionWithTooltip reaction;
                auto name_view = common::json::rapidjson_get_string_view(field.name);
                if (!name_view.empty())
                    reaction.reaction_ = QString::fromUtf8(name_view.data(), name_view.size());

                if (field.value.IsString())
                {
                    auto value_view = common::json::rapidjson_get_string_view(field.value);
                    if (!value_view.empty())
                        reaction.tooltip_ = QString::fromUtf8(value_view.data(), value_view.size());
                }

                reactions_.push_back(std::move(reaction));
            }
        }
    }
};


DefaultReactions::DefaultReactions()
    : d(std::make_unique<DefaultReactions_p>())
{
    d->updateDefaultReactions();

    connect(&Utils::InterConnector::instance(), &Utils::InterConnector::omicronUpdated, this, &DefaultReactions::onOmicronUpdated);
}

DefaultReactions::~DefaultReactions() = default;

DefaultReactions* DefaultReactions::instance()
{
    static DefaultReactions defaultReactions;
    return &defaultReactions;
}

const std::vector<ReactionWithTooltip>& DefaultReactions::reactionsWithTooltip() const
{
    return d->reactions_;
}

void DefaultReactions::onOmicronUpdated()
{
    d->updateDefaultReactions();
}

}
