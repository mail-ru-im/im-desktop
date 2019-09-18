#include "stdafx.h"

#include "EmojiDb.h"
#include "EmojiIndexDataNew.h"
#include "Emoji.h"
#include "EmojiCategory.h"

#include <qjsondocument.h>
#include <qjsonobject.h>
#include <qfile.h>

#if defined(__APPLE__)
#include "macos/Emoji_mac.h"
#endif

namespace
{
    using namespace Emoji;

    EmojiRecordSptrVec EmojiIndexByOrder_;

    using EmojiIndex = std::unordered_map<EmojiCode, EmojiRecordSptr, EmojiCodeHasher>;
    EmojiIndex emojiIndexByFullCodepoint_;
    EmojiIndex emojiIndexByBaseCodepoint_;

    using EmojiIndexByCategory = std::map<QString, EmojiRecordSptrVec>;
    EmojiIndexByCategory EmojiIndexByCategory_;

    QVector<QString> EmojiCategories_ ;

    const QVector<QString> EmojiCategoriesOrder_ = { peopleCategory(), natureCategory(), foodCategory(), activityCategory(), travelCategory(),
                                                     objectsCategory(), symbolsCategory(), flagsCategory(), modifierCategory(), regionalCategory()};

#if defined(__APPLE__)
    constexpr inline bool canViewEmojiOneOnMac() noexcept { return true; }
    constexpr inline bool canSendEmojiOneOnMac() noexcept { return canViewEmojiOneOnMac() && false; }
#endif
}

namespace Emoji
{
    EmojiRecord::EmojiRecord(QString _category, QString _fileName, const int _index, EmojiCode _baseCodePoints, EmojiCode _fullCodePoints)
        : Category_(std::move(_category))
        , FileName_(std::move(_fileName))
        , Index_(_index)
        , baseCodePoints_(std::move(_baseCodePoints))
        , fullCodePoints(std::move(_fullCodePoints))
    {
        assert(!Category_.isEmpty());
        assert(!FileName_.isEmpty());
        assert(Index_ >= 0);
        assert(!baseCodePoints_.isNull());
        assert(!fullCodePoints.isNull());
    }

    EmojiRecord::EmojiRecord(QString _category, QString _fileName, const int _index, EmojiCode _baseCodePoints)
        : EmojiRecord(std::move(_category), std::move(_fileName), _index, _baseCodePoints, _baseCodePoints)
    {
    }

    static constexpr bool skipEmoji(const EmojiCode& code) noexcept
    {
        if (code.size() == 1)
        {
            const auto main = code.codepointAt(0);
            if (main <= '9' && main >= '0')
                return true;
            if (main == '#' || main == '*')
                return true;
        }
        return false;
    }

    namespace
    {
        enum class CodePointType
        {
            Full,
            Base
        };
    }

    static EmojiIndex makeIndex(const EmojiRecordSptrVec& vector, CodePointType type)
    {
#if defined(__APPLE__)
        static_assert ((canSendEmojiOneOnMac() && canViewEmojiOneOnMac())
                       || (!canSendEmojiOneOnMac() && canViewEmojiOneOnMac())
                       || (!canSendEmojiOneOnMac() && !canViewEmojiOneOnMac()), "invariant fail");
#endif
        EmojiIndex index;
        for (const auto& record : vector)
        {
            if (skipEmoji(record->baseCodePoints_))
                continue;

#if defined(__APPLE__)
            if constexpr (useNativeEmoji())
            {
                if (mac::isSkipEmojiFullCode(record->fullCodePoints))
                    continue;

                if (!mac::supportEmoji(record->fullCodePoints) && !canViewEmojiOneOnMac())
                    continue;
            }
#endif
            index.emplace(type == CodePointType::Full ? record->fullCodePoints : record->baseCodePoints_, record);
        }
        return index;
    }

    static bool containsSkinTone(const EmojiCode& code) noexcept // c++20: make constexpr
    {
        constexpr static EmojiCode::codePointType skinTones[] = { 0x1f3fc, 0x1f3fb, 0x1f3fd, 0x1f3fe, 0x1f3ff };
        return std::any_of(std::begin(skinTones), std::end(skinTones), [&code](auto tone) { return code.contains(tone); });
    }

    static bool containsWithGender(const EmojiIndex& index, const EmojiCode& code)
    {
        if (code.size() < EmojiCode::maxSize())
        {
            constexpr static EmojiCode::codePointType genders[] = { 0x2640, 0x2642 };
            return std::any_of(std::begin(genders), std::end(genders), [&index, &code](auto gender) { return index.find(EmojiCode::addCodePoint(code, gender)) != index.end(); });
        }
        return false;
    }

    static bool isHairType(const EmojiCode& code) noexcept // c++20: make constexpr
    {
        if (code.size() > 1)
            return false;

        constexpr static EmojiCode::codePointType hairTypes[] = { 0x1f9b0, 0x1f9b1, 0x1f9b2, 0x1f9b3 };
        return std::any_of(std::begin(hairTypes), std::end(hairTypes), [&code](auto tone) { return code.contains(tone); });
    }

    void InitEmojiDb()
    {
        EmojiIndexByOrder_ = getEmojiVector();
#if defined(__APPLE__)
        mac::setEmojiVector(EmojiIndexByOrder_);
#endif
        assert(!EmojiIndexByOrder_.empty());

        emojiIndexByFullCodepoint_ = makeIndex(EmojiIndexByOrder_, CodePointType::Full);

        emojiIndexByBaseCodepoint_ = makeIndex(EmojiIndexByOrder_, CodePointType::Base);

        for (const auto& record : EmojiIndexByOrder_)
        {
            if (skipEmoji(record->baseCodePoints_))
                continue;

            if (containsSkinTone(record->baseCodePoints_)) // don't add skin tones to picker
                continue;

            if (containsWithGender(emojiIndexByBaseCodepoint_, record->baseCodePoints_))
                continue;

            if (isHairType(record->baseCodePoints_))
                continue;

            if (record->Category_.isEmpty())
                continue;

#if defined(__APPLE__)
            if constexpr (useNativeEmoji())
            {
                if (mac::isSkipEmojiFullCode(record->fullCodePoints))
                    continue;

                if (!canSendEmojiOneOnMac() && !mac::supportEmoji(record->fullCodePoints) && !isSupported(record->FileName_))
                    continue;
            }
#else
            if (!isSupported(record->FileName_))
                continue;
#endif

            const auto& category = record->Category_;
            assert(!category.isEmpty());

            auto& records = EmojiIndexByCategory_[category];
            records.push_back(record);
        }

        for (auto& category : EmojiCategoriesOrder_)
        {
            auto& records = EmojiIndexByCategory_[category];
            if (records.size() > 0)
                EmojiCategories_.push_back(category);
        }
    }

    bool isEmoji(const EmojiCode& _code)
    {
        assert(!_code.isNull());
        assert(!emojiIndexByFullCodepoint_.empty() && !emojiIndexByBaseCodepoint_.empty());
        return std::as_const(emojiIndexByFullCodepoint_).find(_code) != std::as_const(emojiIndexByFullCodepoint_).end()
            || std::as_const(emojiIndexByBaseCodepoint_).find(_code) != std::as_const(emojiIndexByBaseCodepoint_).end();
    }

    const EmojiRecordSptr& GetEmojiInfoByCodepoint(const EmojiCode& _code)
    {
        assert(!_code.isNull());
        assert(!emojiIndexByFullCodepoint_.empty() && !emojiIndexByBaseCodepoint_.empty());

        if (const auto iter = std::as_const(emojiIndexByFullCodepoint_).find(_code); iter != std::as_const(emojiIndexByFullCodepoint_).end())
            return iter->second;

        if (const auto iter = std::as_const(emojiIndexByBaseCodepoint_).find(_code); iter != std::as_const(emojiIndexByBaseCodepoint_).end())
            return iter->second;

        static const EmojiRecordSptr EmptyEmoji;
        return EmptyEmoji;
    }

    const QVector<QString>& GetEmojiCategories()
    {
        assert(!EmojiCategories_.isEmpty());

        return EmojiCategories_;
    }

    const EmojiRecordSptrVec& GetEmojiInfoByCategory(const QString& _category)
    {
        assert(!_category.isEmpty());

        if (const auto iter = std::as_const(EmojiIndexByCategory_).find(_category); iter != std::as_const(EmojiIndexByCategory_).end())
            return iter->second;

        static const EmojiRecordSptrVec empty;
        return empty;
    }
}
