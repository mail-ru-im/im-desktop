#pragma once

#include "EmojiCode.h"

namespace Emoji
{
    enum class EmojiSizePx
    {
        Invalid,
        Min = 13,

        _13 = Min,
        _16 = 16,
        Auto = 21,
        _22 = 22,
        _27 = 27,
        _32 = 32,
        _40 = 40,
        _44 = 44,
        _48 = 48,
        _64 = 64,
        _80 = 80,
        _96 = 96,


        Max = 96
    };

    void InitializeSubsystem();

    void Cleanup();
    Emoji::EmojiSizePx getEmojiSize() noexcept;

    QImage GetEmoji(const EmojiCode& _code, const EmojiSizePx size = EmojiSizePx::Auto);
    QImage GetEmoji(const EmojiCode& _code, int32_t _sizePx);

    QImage GetEmojiImage(const EmojiCode& _code, int32_t _sizePx); // should not use

    uint32_t readCodepoint(QStringView text, qsizetype& pos);
    EmojiCode getEmoji(QStringView text, qsizetype& pos);

    bool isSupported(QLatin1String _filename);

    // This is meant to be used for "debug" purposes only
    const std::unordered_map<int64_t, const QImage>& GetEmojiCache();

    int32_t GetEmojiSizeForCurrentUiScale();
}
