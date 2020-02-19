#pragma once

#include "../../utils/utils.h"
#include "../../cache/emoji/EmojiCode.h"

namespace Ui
{
    class HistoryTextEdit;
}

namespace Emoji
{
    using Text2ReplacementMap = std::unordered_map<QString, QVariant, Utils::QStringHasher>;

    const Text2ReplacementMap& getText2ReplacementMap();

    enum class ReplaceAvailable
    {
        Unavailable = 0,
        Available = 1,
    };


    class TextSymbolReplacer final
    {
    public:
        TextSymbolReplacer(Ui::HistoryTextEdit *_textEdit);
        ~TextSymbolReplacer();

        const QVariant& getReplacement(const QStringRef& _smile) const;
        void setAutoreplaceAvailable(const ReplaceAvailable _available = ReplaceAvailable::Available);
        bool isAutoreplaceAvailable() const;
    private:
        Ui::HistoryTextEdit *textEdit_;
        ReplaceAvailable replaceAvailable_;
    };
}
