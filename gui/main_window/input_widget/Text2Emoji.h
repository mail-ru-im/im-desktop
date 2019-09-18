#pragma once

#include "../../utils/utils.h"
#include "../../cache/emoji/EmojiCode.h"

namespace Ui
{
    class HistoryTextEdit;
}

namespace Emoji
{
    using Text2EmojiMap = std::unordered_map<QString, EmojiCode, Utils::QStringHasher>;

    const Text2EmojiMap& getText2EmojiMap();

    constexpr auto doubleDashCode = EmojiCode(0xdeadbeef);

    enum class ReplaceAvailable
    {
        Unavailable = 0,
        Available = 1,
    };


    class TextEmojiReplacer final
    {
    public:
        TextEmojiReplacer(Ui::HistoryTextEdit *_textEdit);
        ~TextEmojiReplacer();

        const EmojiCode& getEmojiCode(const QStringRef& _smile) const;
        void setAutoreplaceAvailable(const ReplaceAvailable _available = ReplaceAvailable::Available);
        bool isAutoreplaceAvailable() const;
    private:
        Ui::HistoryTextEdit *textEdit_;
        ReplaceAvailable replaceAvailable_;
    };
}
