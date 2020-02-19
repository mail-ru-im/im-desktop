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

    // EmojiRecord don't store fileName string
    struct EmojiRecord
    {
        EmojiRecord(QLatin1String _category, QLatin1String _fileName, QLatin1String _shortName, const int _index, EmojiCode _baseCodePoints);
        EmojiRecord(QLatin1String _category, QLatin1String _fileName, QLatin1String _shortName, const int _index, EmojiCode _baseCodePoints, EmojiCode _fullCodePoints);

        EmojiRecord(const EmojiRecord&) = default;
        EmojiRecord(EmojiRecord&&) = default;
        EmojiRecord& operator=(const EmojiRecord&) = default;
        EmojiRecord& operator=(EmojiRecord&&) = default;

        QLatin1String Category_;
        QLatin1String FileName_;
        QLatin1String ShortName_;

        int Index_;

        EmojiCode baseCodePoints_;
        EmojiCode fullCodePoints;

        bool isValid() const noexcept;

        static const EmojiRecord& invalid();
    };

    using EmojiRecordVec = std::vector<EmojiRecord>;

    void InitEmojiDb();

    bool isEmoji(const EmojiCode& _code);

    const EmojiRecord& GetEmojiInfoByCodepoint(const EmojiCode& _code);

    const QVector<QLatin1String>& GetEmojiCategories();

    const EmojiRecordVec& GetEmojiInfoByCategory(QLatin1String _category);

    const EmojiCode& GetEmojiInfoByCountryName(const QString& _country);
}

#endif // EMOJI_DB
