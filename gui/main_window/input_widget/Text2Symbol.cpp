#include "stdafx.h"

#include "Text2Symbol.h"
#include "utils/StringComparator.h"

namespace
{
    constexpr auto maxEmojiLength = 9;

    inline QString getMyteamString()
    {
        return qsl("Myteam");
    }
}

namespace Emoji
{
    using Text2ReplacementMap = std::map<QStringView, QVariant, Utils::StringComparatorInsensitive>;
    const Text2ReplacementMap& getText2ReplacementMap()
    {
        const static Text2ReplacementMap text2emojiMap =
        {
            {u":)",  QVariant::fromValue(EmojiCode(0x1f642))},
            {u":-)", QVariant::fromValue(EmojiCode(0x1f642))},
            {u":]",  QVariant::fromValue(EmojiCode(0x1f642))},
            {u"(:",  QVariant::fromValue(EmojiCode(0x1f642))},

            {u"=)",  QVariant::fromValue(EmojiCode(0x263a, 0xfe0f))},
            {u"(=",  QVariant::fromValue(EmojiCode(0x263a, 0xfe0f))},

            {u":(",  QVariant::fromValue(EmojiCode(0x2639, 0xfe0f))},
            {u":-(", QVariant::fromValue(EmojiCode(0x2639, 0xfe0f))},
            {u"=(",  QVariant::fromValue(EmojiCode(0x2639, 0xfe0f))},
            {u")=",  QVariant::fromValue(EmojiCode(0x2639, 0xfe0f))},
            {u":[",  QVariant::fromValue(EmojiCode(0x2639, 0xfe0f))},
            {u":c",  QVariant::fromValue(EmojiCode(0x2639, 0xfe0f))},

            {u";-p", QVariant::fromValue(EmojiCode(0x1f61c))},
            {u";p",  QVariant::fromValue(EmojiCode(0x1f61c))},

            {u":-p", QVariant::fromValue(EmojiCode(0x1f61b))},
            {u":p",  QVariant::fromValue(EmojiCode(0x1f61b))},
            {u"=p",  QVariant::fromValue(EmojiCode(0x1f61b))},

            {u":-o", QVariant::fromValue(EmojiCode(0x1f62e))},
            {u":o",  QVariant::fromValue(EmojiCode(0x1f62e))},

            {u":-d", QVariant::fromValue(EmojiCode(0x1f600))},
            {u":d",  QVariant::fromValue(EmojiCode(0x1f600))},
            {u"=d",  QVariant::fromValue(EmojiCode(0x1f600))},

            {u";)",  QVariant::fromValue(EmojiCode(0x1f609))},
            {u";-)", QVariant::fromValue(EmojiCode(0x1f609))},

            {u"b)",  QVariant::fromValue(EmojiCode(0x1f60e))},
            {u"b-)", QVariant::fromValue(EmojiCode(0x1f60e))},

            {u">:(",  QVariant::fromValue(EmojiCode(0x1f620))},
            {u">:-(", QVariant::fromValue(EmojiCode(0x1f620))},
            {u">:o",  QVariant::fromValue(EmojiCode(0x1f620))},
            {u">:-o", QVariant::fromValue(EmojiCode(0x1f620))},

            {u":\\",  QVariant::fromValue(EmojiCode(0x1f615))},
            {u":-\\", QVariant::fromValue(EmojiCode(0x1f615))},
            {u":/",   QVariant::fromValue(EmojiCode(0x1f615))},
            {u"=/",   QVariant::fromValue(EmojiCode(0x1f615))},
            {u":-/",  QVariant::fromValue(EmojiCode(0x1f615))},
            {u"=\\",  QVariant::fromValue(EmojiCode(0x1f615))},
            {u"=-\\", QVariant::fromValue(EmojiCode(0x1f615))},

            {u":'(",  QVariant::fromValue(EmojiCode(0x1f622))},
            {u":'-(", QVariant::fromValue(EmojiCode(0x1f622))},
#ifndef __APPLE__
            {u":’(",  QVariant::fromValue(EmojiCode(0x1f622))},
            {u":’-(", QVariant::fromValue(EmojiCode(0x1f622))},
#endif

            {u"3:)",  QVariant::fromValue(EmojiCode(0x1f608))},
            {u"3:-)", QVariant::fromValue(EmojiCode(0x1f608))},

            {u"0:)",  QVariant::fromValue(EmojiCode(0x1f607))},
            {u"o:)",  QVariant::fromValue(EmojiCode(0x1f607))},
            {u"0:-)", QVariant::fromValue(EmojiCode(0x1f607))},
            {u"o:-)", QVariant::fromValue(EmojiCode(0x1f607))},

            {u":*",  QVariant::fromValue(EmojiCode(0x1f617))},
            {u":-*", QVariant::fromValue(EmojiCode(0x1f617))},
            {u"-3-", QVariant::fromValue(EmojiCode(0x1f617))},

            {u";*",  QVariant::fromValue(EmojiCode(0x1f618))},
            {u";-*", QVariant::fromValue(EmojiCode(0x1f618))},

            {u"^_^", QVariant::fromValue(EmojiCode(0x263a, 0xfe0f))},
            {u"^^",  QVariant::fromValue(EmojiCode(0x263a, 0xfe0f))},
            {u"^~^", QVariant::fromValue(EmojiCode(0x263a, 0xfe0f))},

            {u"-_-", QVariant::fromValue(EmojiCode(0x1f611))},
            {u":-|", QVariant::fromValue(EmojiCode(0x1f610))},
            {u":|",  QVariant::fromValue(EmojiCode(0x1f610))},

            {u">_<", QVariant::fromValue(EmojiCode(0x1f623))},
            {u"><",  QVariant::fromValue(EmojiCode(0x1f623))},
            {u">.<", QVariant::fromValue(EmojiCode(0x1f623))},

            {u"o_o", QVariant::fromValue(EmojiCode(0x1f633))},
            {u"0_0", QVariant::fromValue(EmojiCode(0x1f633))},

            {u"t_t", QVariant::fromValue(EmojiCode(0x1f62d))},
            {u"t-t", QVariant::fromValue(EmojiCode(0x1f62d))},
            {u"tot", QVariant::fromValue(EmojiCode(0x1f62d))},
            {u"t.t", QVariant::fromValue(EmojiCode(0x1f62d))},

            {u"'-_-", QVariant::fromValue(EmojiCode(0x1f613))},
            {u"-_-'", QVariant::fromValue(EmojiCode(0x1f613))},
#ifndef __APPLE__
            {u"’-_-", QVariant::fromValue(EmojiCode(0x1f613))},
            {u"-_-’", QVariant::fromValue(EmojiCode(0x1f613))},
#endif

            {u"<3",         QVariant::fromValue(EmojiCode(0x2764, 0xfe0f))},
            {u"&lt;3",      QVariant::fromValue(EmojiCode(0x2764, 0xfe0f))},
            {u":like:",     QVariant::fromValue(EmojiCode(0x1f44d))},
            {u"(y)",        QVariant::fromValue(EmojiCode(0x1f44d))},
            {u":dislike:",  QVariant::fromValue(EmojiCode(0x1f44e))},
            {u"(n)",        QVariant::fromValue(EmojiCode(0x1f44e))},

            {u":poop:", QVariant::fromValue(EmojiCode(0x1f4a9))},
            {u"<(\")",  QVariant::fromValue(EmojiCode(0x1f427))},

            {u"--", QVariant::fromValue(QString(0x2014))},
            {u"->", QVariant::fromValue(QString(0x2192))},
            {u"<-", QVariant::fromValue(QString(0x2190))},
            {u"<->", QVariant::fromValue(QString(0x2194))},
            {u"myTeam", QVariant::fromValue(getMyteamString())},
            {u"MyTeam", QVariant::fromValue(getMyteamString())},
            {u"myTeam", QVariant::fromValue(getMyteamString())},
            {u"MYteam", QVariant::fromValue(getMyteamString())},
            {u"MYTEAM", QVariant::fromValue(getMyteamString())},
            {u"myteam", QVariant::fromValue(getMyteamString())}
        };

        return text2emojiMap;
    }

    const QVariant& TextSymbolReplacer::getReplacement(QStringView _smile) const
    {
        static const QVariant emptyValue;

        const auto trimmedSmile = _smile.trimmed();
        if (trimmedSmile.isEmpty() || trimmedSmile.size() - 1 > maxEmojiLength)
            return emptyValue;

        const auto& replacementMap = getText2ReplacementMap();
        if (const auto emoji = replacementMap.find(trimmedSmile); emoji != replacementMap.end())
            return emoji->second;

        return emptyValue;
    }

    void TextSymbolReplacer::setAutoreplaceAvailable(const ReplaceAvailable _available)
    {
        replaceAvailable_ = _available;
    }

    bool TextSymbolReplacer::isAutoreplaceAvailable() const
    {
        return replaceAvailable_ == ReplaceAvailable::Available;
    }
}
