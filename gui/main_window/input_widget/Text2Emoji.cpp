#include "stdafx.h"

#include "gui_settings.h"

#include "Text2Emoji.h"
#include "HistoryTextEdit.h"

namespace
{
    constexpr auto maxEmojiLength = 9;
}

namespace Emoji
{
    const Text2EmojiMap& getText2EmojiMap()
    {
        const static Text2EmojiMap text2emojiMap =
        {
            {qsl(":)"), EmojiCode(0x1f642)},
            {qsl(":-)"), EmojiCode(0x1f642)},
            {qsl(":]"), EmojiCode(0x1f642)},
            {qsl(":)"), EmojiCode(0x1f642)},
            {qsl("(:"), EmojiCode(0x1f642)},

            {qsl("=)"), EmojiCode(0x263a, 0xfe0f)},
            {qsl("(="), EmojiCode(0x263a, 0xfe0f)},

            {qsl(":("), EmojiCode(0x2639, 0xfe0f)},
            {qsl(":-("), EmojiCode(0x2639, 0xfe0f)},
            {qsl("=("), EmojiCode(0x2639, 0xfe0f)},
            {qsl(")="), EmojiCode(0x2639, 0xfe0f)},
            {qsl(":["), EmojiCode(0x2639, 0xfe0f)},

            {qsl(";-p"), EmojiCode(0x1f61c)},
            {qsl(";p"), EmojiCode(0x1f61c)},

            {qsl(":-p"), EmojiCode(0x1f61b)},
            {qsl(":p"), EmojiCode(0x1f61b)},
            {qsl("=p"), EmojiCode(0x1f61b)},

            {qsl(":-o"), EmojiCode(0x1f62e)},
            {qsl(":o"), EmojiCode(0x1f62e)},

            {qsl(":-d"), EmojiCode(0x1f600)},
            {qsl(":d"), EmojiCode(0x1f600)},
            {qsl("=d"), EmojiCode(0x1f600)},

            {qsl(";)"), EmojiCode(0x1f609)},
            {qsl(";-)"), EmojiCode(0x1f609)},

            {qsl("b)"), EmojiCode(0x1f60e)},
            {qsl("b-)"), EmojiCode(0x1f60e)},

            {qsl(">:("), EmojiCode(0x1f620)},
            {qsl(">:-("), EmojiCode(0x1f620)},
            {qsl(">:o"), EmojiCode(0x1f620)},
            {qsl(">:-o"), EmojiCode(0x1f620)},

            {qsl(":\\"), EmojiCode(0x1f615)},
            {qsl(":-\\"), EmojiCode(0x1f615)},
            {qsl(":/"), EmojiCode(0x1f615)},
            {qsl("=/"), EmojiCode(0x1f615)},
            {qsl(":-/"), EmojiCode(0x1f615)},
            {qsl("=\\"), EmojiCode(0x1f615)},
            {qsl("=-\\"), EmojiCode(0x1f615)},

            {qsl(":'("), EmojiCode(0x1f622)},
            {qsl(":'-("), EmojiCode(0x1f622)},
#ifndef __APPLE__
            {qsl(":’("), EmojiCode(0x1f622)},
            {qsl(":’-("), EmojiCode(0x1f622)},
#endif

            {qsl("3:)"), EmojiCode(0x1f608)},
            {qsl("3:-)"), EmojiCode(0x1f608)},

            {qsl("0:)"), EmojiCode(0x1f607)},
            {qsl("o:)"), EmojiCode(0x1f607)},
            {qsl("0:-)"), EmojiCode(0x1f607)},
            {qsl("o:-)"), EmojiCode(0x1f607)},

            {qsl(":*"), EmojiCode(0x1f617)},
            {qsl(":-*"), EmojiCode(0x1f617)},
            {qsl("-3-"), EmojiCode(0x1f617)},

            {qsl(";*"), EmojiCode(0x1f618)},
            {qsl(";-*"), EmojiCode(0x1f618)},

            {qsl("^_^"), EmojiCode(0x263a, 0xfe0f)},
            {qsl("^^"), EmojiCode(0x263a, 0xfe0f)},
            {qsl("^~^"), EmojiCode(0x263a, 0xfe0f)},

            {qsl("-_-"), EmojiCode(0x1f611)},
            {qsl(":-|"), EmojiCode(0x1f610)},
            {qsl(":|"), EmojiCode(0x1f610)},

            {qsl(">_<"), EmojiCode(0x1f623)},
            {qsl("><"), EmojiCode(0x1f623)},
            {qsl(">.<"), EmojiCode(0x1f623)},

            {qsl("o_o"), EmojiCode(0x1f633)},
            {qsl("0_0"), EmojiCode(0x1f633)},

            {qsl("t_t"), EmojiCode(0x1f62d)},
            {qsl("t-t"), EmojiCode(0x1f62d)},
            {qsl("tot"), EmojiCode(0x1f62d)},
            {qsl("t.t"), EmojiCode(0x1f62d)},

            {qsl("'-_-"), EmojiCode(0x1f613)},
            {qsl("-_-'"), EmojiCode(0x1f613)},
#ifndef __APPLE__
            {qsl("’-_-"), EmojiCode(0x1f613)},
            {qsl("-_-’"), EmojiCode(0x1f613)},
#endif

            {qsl("<3"), EmojiCode(0x2764, 0xfe0f) },
            {qsl("&lt;3"), EmojiCode(0x2764, 0xfe0f) },
            {qsl(":like:"), EmojiCode(0x1f44d)},
            {qsl("(y)"), EmojiCode(0x1f44d)},
            {qsl(":dislike:"), EmojiCode(0x1f44e)},
            {qsl("(n)"), EmojiCode(0x1f44e)},

            {qsl(":poop:"), EmojiCode(0x1f4a9)},
            {qsl("<(\")"), EmojiCode(0x1f427)},
            {qsl("--"), doubleDashCode},
        };

        return text2emojiMap;
    }

    TextEmojiReplacer::TextEmojiReplacer(Ui::HistoryTextEdit* _textEdit)
        : textEdit_(_textEdit)
        , replaceAvailable_(ReplaceAvailable::Available)
    {
    }

    TextEmojiReplacer::~TextEmojiReplacer()
    {
    }

    const EmojiCode& TextEmojiReplacer::getEmojiCode(const QStringRef& _smile) const
    {
        const static EmojiCode emptyCode;
        const auto& emojiMap = getText2EmojiMap();

        auto trimmedSmile = _smile.trimmed();

        if (trimmedSmile.isEmpty() || trimmedSmile.length() - 1 > maxEmojiLength)
            return emptyCode;

        if (const auto emoji = emojiMap.find(trimmedSmile.toString().toLower()); emoji != emojiMap.end())
            return emoji->second;

        return emptyCode;
    }

    void TextEmojiReplacer::setAutoreplaceAvailable(const ReplaceAvailable _available)
    {
        replaceAvailable_ = _available;
    }

    bool TextEmojiReplacer::isAutoreplaceAvailable() const
    {
        return replaceAvailable_ == ReplaceAvailable::Available;
    }
}
