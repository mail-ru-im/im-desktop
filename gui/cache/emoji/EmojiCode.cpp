#include "stdafx.h"

#include "EmojiCode.h"

#include "wide_int.h"

namespace Emoji
{
    QString migration(const std::string& _str)
    {
        using baseType = std::wide_uint<EmojiCode::maxSize() * sizeof(EmojiCode::codePointType) * 8>;

        baseType a = 0;
        const auto res = std::from_chars(_str.data(), _str.data() + _str.size(), a);
        if (!res.ec)
        {
            EmojiCode::codePointType current = a;
            std::array<EmojiCode::codePointType, EmojiCode::maxSize() + 1> unicode;
            unicode.fill(0);
            size_t idx = 0;
            while (current && idx < EmojiCode::maxSize())
            {
                unicode[idx++] = current;
                current = baseType::_impl::shift_right(a, 32 * int(idx));
            }
            return QString::fromUcs4(unicode.data());
        }
        return QString();
    }
}