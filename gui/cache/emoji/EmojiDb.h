#ifndef EMOJI_DB
#define EMOJI_DB

#pragma once

#include "EmojiCode.h"

namespace Emoji
{
    struct EmojiCodeHasher
    {
        std::size_t operator()(const EmojiCode& k) const noexcept
        {
            return k.hash();
        }
    };

#if defined(__APPLE__)
    constexpr inline bool useNativeEmoji() noexcept { return true; }
#endif

    struct EmojiRecord
    {
        EmojiRecord(QString _category, QString _fileName, const int _index, EmojiCode _baseCodePoints);
        EmojiRecord(QString _category, QString _fileName, const int _index, EmojiCode _baseCodePoints, EmojiCode _fullCodePoints);

        const QString Category_;
        const QString FileName_;

        const int Index_;

        const EmojiCode baseCodePoints_;
        const EmojiCode fullCodePoints;
    };

    using EmojiRecordSptr = std::shared_ptr<EmojiRecord>;

    using EmojiRecordSptrVec = std::vector<EmojiRecordSptr>;

    void InitEmojiDb();

    bool isEmoji(const EmojiCode& _code);

    const EmojiRecordSptr& GetEmojiInfoByCodepoint(const EmojiCode& _code);

    const QVector<QString>& GetEmojiCategories();

    const EmojiRecordSptrVec& GetEmojiInfoByCategory(const QString& _category);
}

#endif // EMOJI_DB
