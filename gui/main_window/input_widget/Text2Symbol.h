#pragma once

#include "../../utils/utils.h"
#include "../../cache/emoji/EmojiCode.h"

namespace Emoji
{
    enum class ReplaceAvailable
    {
        Unavailable,
        Available,
    };


    class TextSymbolReplacer final
    {
    public:
        TextSymbolReplacer() = default;
        ~TextSymbolReplacer() = default;

        const QVariant& getReplacement(QStringView _smile) const;
        void setAutoreplaceAvailable(const ReplaceAvailable _available = ReplaceAvailable::Available);
        bool isAutoreplaceAvailable() const;

    private:
        ReplaceAvailable replaceAvailable_ = ReplaceAvailable::Available;
    };
}
