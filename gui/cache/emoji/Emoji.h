#ifndef EMOJI_H
#define EMOJI_H

#pragma once

#include "stdafx.h"
#include <unordered_map>
#include "EmojiCode.h"

namespace Emoji
{
    enum class EmojiSizePx
    {
        Invalid,
        Min = 16,

        _16 = Min,
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

    QImage GetEmoji(const EmojiCode& _code, const EmojiSizePx size = EmojiSizePx::Auto);
    QImage GetEmoji(const EmojiCode& _code, int32_t _sizePx);

    uint32_t readCodepoint(const QStringRef& text, int& pos);
    EmojiCode getEmoji(const QStringRef& text, int& pos);

    bool isSupported(const QString& _filename);

    // This is meant to be used for "debug" purposes only
    const std::unordered_map<int64_t, const QImage>& GetEmojiCache();

    int32_t GetEmojiSizeForCurrentUiScale();
}
#endif // EMOJI_H
