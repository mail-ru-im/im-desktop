#include "stdafx.h"

#include "gui_settings.h"

#include "Text2Symbol.h"
#include "HistoryTextEdit.h"

namespace
{
    constexpr auto maxEmojiLength = 9;
}

namespace Emoji
{
    const Text2ReplacementMap& getText2ReplacementMap()
    {
        const static Text2ReplacementMap text2emojiMap =
        {
            {qsl(":)"),  QVariant::fromValue(EmojiCode(0x1f642))},
            {qsl(":-)"), QVariant::fromValue(EmojiCode(0x1f642))},
            {qsl(":]"),  QVariant::fromValue(EmojiCode(0x1f642))},
            {qsl(":)"),  QVariant::fromValue(EmojiCode(0x1f642))},
            {qsl("(:"),  QVariant::fromValue(EmojiCode(0x1f642))},

            {qsl("=)"),  QVariant::fromValue(EmojiCode(0x263a, 0xfe0f))},
            {qsl("(="),  QVariant::fromValue(EmojiCode(0x263a, 0xfe0f))},

            {qsl(":("),  QVariant::fromValue(EmojiCode(0x2639, 0xfe0f))},
            {qsl(":-("), QVariant::fromValue(EmojiCode(0x2639, 0xfe0f))},
            {qsl("=("),  QVariant::fromValue(EmojiCode(0x2639, 0xfe0f))},
            {qsl(")="),  QVariant::fromValue(EmojiCode(0x2639, 0xfe0f))},
            {qsl(":["),  QVariant::fromValue(EmojiCode(0x2639, 0xfe0f))},
            {qsl(":c"),  QVariant::fromValue(EmojiCode(0x2639, 0xfe0f))},

            {qsl(";-p"), QVariant::fromValue(EmojiCode(0x1f61c))},
            {qsl(";p"),  QVariant::fromValue(EmojiCode(0x1f61c))},

            {qsl(":-p"), QVariant::fromValue(EmojiCode(0x1f61b))},
            {qsl(":p"),  QVariant::fromValue(EmojiCode(0x1f61b))},
            {qsl("=p"),  QVariant::fromValue(EmojiCode(0x1f61b))},
                         
            {qsl(":-o"), QVariant::fromValue(EmojiCode(0x1f62e))},
            {qsl(":o"),  QVariant::fromValue(EmojiCode(0x1f62e))},
                         
            {qsl(":-d"), QVariant::fromValue(EmojiCode(0x1f600))},
            {qsl(":d"),  QVariant::fromValue(EmojiCode(0x1f600))},
            {qsl("=d"),  QVariant::fromValue(EmojiCode(0x1f600))},
                         
            {qsl(";)"),  QVariant::fromValue(EmojiCode(0x1f609))},
            {qsl(";-)"), QVariant::fromValue(EmojiCode(0x1f609))},
                         
            {qsl("b)"),  QVariant::fromValue(EmojiCode(0x1f60e))},
            {qsl("b-)"), QVariant::fromValue(EmojiCode(0x1f60e))},

            {qsl(">:("),  QVariant::fromValue(EmojiCode(0x1f620))},
            {qsl(">:-("), QVariant::fromValue(EmojiCode(0x1f620))},
            {qsl(">:o"),  QVariant::fromValue(EmojiCode(0x1f620))},
            {qsl(">:-o"), QVariant::fromValue(EmojiCode(0x1f620))},

            {qsl(":\\"),  QVariant::fromValue(EmojiCode(0x1f615))},
            {qsl(":-\\"), QVariant::fromValue(EmojiCode(0x1f615))},
            {qsl(":/"),   QVariant::fromValue(EmojiCode(0x1f615))},
            {qsl("=/"),   QVariant::fromValue(EmojiCode(0x1f615))},
            {qsl(":-/"),  QVariant::fromValue(EmojiCode(0x1f615))},
            {qsl("=\\"),  QVariant::fromValue(EmojiCode(0x1f615))},
            {qsl("=-\\"), QVariant::fromValue(EmojiCode(0x1f615))},

            {qsl(":'("),  QVariant::fromValue(EmojiCode(0x1f622))},
            {qsl(":'-("), QVariant::fromValue(EmojiCode(0x1f622))},
#ifndef __APPLE__
            {qsl(":’("),  QVariant::fromValue(EmojiCode(0x1f622))},
            {qsl(":’-("), QVariant::fromValue(EmojiCode(0x1f622))},
#endif         

            {qsl("3:)"),  QVariant::fromValue(EmojiCode(0x1f608))},
            {qsl("3:-)"), QVariant::fromValue(EmojiCode(0x1f608))},

            {qsl("0:)"),  QVariant::fromValue(EmojiCode(0x1f607))},
            {qsl("o:)"),  QVariant::fromValue(EmojiCode(0x1f607))},
            {qsl("0:-)"), QVariant::fromValue(EmojiCode(0x1f607))},
            {qsl("o:-)"), QVariant::fromValue(EmojiCode(0x1f607))},

            {qsl(":*"),  QVariant::fromValue(EmojiCode(0x1f617))},
            {qsl(":-*"), QVariant::fromValue(EmojiCode(0x1f617))},
            {qsl("-3-"), QVariant::fromValue(EmojiCode(0x1f617))},

            {qsl(";*"),  QVariant::fromValue(EmojiCode(0x1f618))},
            {qsl(";-*"), QVariant::fromValue(EmojiCode(0x1f618))},

            {qsl("^_^"), QVariant::fromValue(EmojiCode(0x263a, 0xfe0f))},
            {qsl("^^"),  QVariant::fromValue(EmojiCode(0x263a, 0xfe0f))},
            {qsl("^~^"), QVariant::fromValue(EmojiCode(0x263a, 0xfe0f))},

            {qsl("-_-"), QVariant::fromValue(EmojiCode(0x1f611))},
            {qsl(":-|"), QVariant::fromValue(EmojiCode(0x1f610))},
            {qsl(":|"),  QVariant::fromValue(EmojiCode(0x1f610))},
                                                               
            {qsl(">_<"), QVariant::fromValue(EmojiCode(0x1f623))},
            {qsl("><"),  QVariant::fromValue(EmojiCode(0x1f623))},
            {qsl(">.<"), QVariant::fromValue(EmojiCode(0x1f623))},
                                                               
            {qsl("o_o"), QVariant::fromValue(EmojiCode(0x1f633))},
            {qsl("0_0"), QVariant::fromValue(EmojiCode(0x1f633))},
                                                               
            {qsl("t_t"), QVariant::fromValue(EmojiCode(0x1f62d))},
            {qsl("t-t"), QVariant::fromValue(EmojiCode(0x1f62d))},
            {qsl("tot"), QVariant::fromValue(EmojiCode(0x1f62d))},
            {qsl("t.t"), QVariant::fromValue(EmojiCode(0x1f62d))},

            {qsl("'-_-"), QVariant::fromValue(EmojiCode(0x1f613))},
            {qsl("-_-'"), QVariant::fromValue(EmojiCode(0x1f613))},
#ifndef __APPLE__
            {qsl("’-_-"), QVariant::fromValue(EmojiCode(0x1f613))},
            {qsl("-_-’"), QVariant::fromValue(EmojiCode(0x1f613))},
#endif

            {qsl("<3"),         QVariant::fromValue(EmojiCode(0x2764, 0xfe0f))},
            {qsl("&lt;3"),      QVariant::fromValue(EmojiCode(0x2764, 0xfe0f))},
            {qsl(":like:"),     QVariant::fromValue(EmojiCode(0x1f44d))},
            {qsl("(y)"),        QVariant::fromValue(EmojiCode(0x1f44d))},
            {qsl(":dislike:"),  QVariant::fromValue(EmojiCode(0x1f44e))},
            {qsl("(n)"),        QVariant::fromValue(EmojiCode(0x1f44e))},

            {qsl(":poop:"), QVariant::fromValue(EmojiCode(0x1f4a9))},
            {qsl("<(\")"),  QVariant::fromValue(EmojiCode(0x1f427))},

            {qsl("--"), QVariant::fromValue(QString(0x2014))},
            {qsl("->"), QVariant::fromValue(QString(0x2192))},
            {qsl("<-"), QVariant::fromValue(QString(0x2190))},
            {qsl("<->"), QVariant::fromValue(QString(0x2194))}
        };

        return text2emojiMap;
    }

    TextSymbolReplacer::TextSymbolReplacer(Ui::HistoryTextEdit* _textEdit)
        : textEdit_(_textEdit)
        , replaceAvailable_(ReplaceAvailable::Available)
    {
    }

    TextSymbolReplacer::~TextSymbolReplacer()
    {
    }

    const QVariant& TextSymbolReplacer::getReplacement(const QStringRef& _smile) const
    {
        const auto& replacementMap = getText2ReplacementMap();
        auto trimmedSmile = _smile.trimmed();
        static const QVariant emptyValue;

        if (trimmedSmile.isEmpty() || trimmedSmile.length() - 1 > maxEmojiLength)
            return emptyValue;

        if (const auto emoji = replacementMap.find(trimmedSmile.toString().toLower()); emoji != replacementMap.end())
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
