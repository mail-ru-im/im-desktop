#include "stdafx.h"

#include "EmojiDb.h"
#include "EmojiIndexDataNew.h"
#include "Emoji.h"
#include "EmojiCategory.h"

#include "utils/StringComparator.h"

#include <qjsondocument.h>
#include <qjsonobject.h>
#include <qfile.h>

#if defined(__APPLE__)
#include "macos/Emoji_mac.h"
#endif

namespace
{
    using namespace Emoji;

    EmojiRecordVec EmojiIndexByOrder_;

    using EmojiIndex = std::unordered_map<EmojiCode, size_t, EmojiCodeHasher>;
    EmojiIndex emojiIndexByFullCodepoint_;
    EmojiIndex emojiIndexByBaseCodepoint_;

    using EmojiFlags = std::map<QString, EmojiCode, Utils::StringComparatorInsensitive>;
    EmojiFlags flagEmojis_;

    using EmojiIndexByCategory = std::map<QLatin1String, EmojiRecordVec>;
    EmojiIndexByCategory EmojiIndexByCategory_;

    QVector<QLatin1String> EmojiCategories_ ;

    const QVector<QLatin1String> EmojiCategoriesOrder_ = { peopleCategory(), natureCategory(), foodCategory(), activityCategory(), travelCategory(),
                                                     objectsCategory(), symbolsCategory(), flagsCategory(), modifierCategory(), regionalCategory()};

#if defined(__APPLE__)
    constexpr inline bool canViewEmojiOneOnMac() noexcept { return true; }
    constexpr inline bool canSendEmojiOneOnMac() noexcept { return canViewEmojiOneOnMac() && false; }
#endif
}

namespace Emoji
{
    EmojiRecord::EmojiRecord(QLatin1String _category, QLatin1String _fileName, QLatin1String _shortName, const int _index, EmojiCode _baseCodePoints, EmojiCode _fullCodePoints)
        : Category_(_category)
        , FileName_(_fileName)
        , ShortName_(_shortName)
        , Index_(_index)
        , baseCodePoints_(std::move(_baseCodePoints))
        , fullCodePoints(std::move(_fullCodePoints))
    {
        im_assert(Category_.size() > 0);
        im_assert(FileName_.size() > 0);
        im_assert(!baseCodePoints_.isNull());
        im_assert(!fullCodePoints.isNull());
    }

    bool EmojiRecord::isValid() const noexcept
    {
        return Index_ >= 0;
    }

    const EmojiRecord& EmojiRecord::invalid()
    {
        const static auto v = EmojiRecord(QLatin1String(), QLatin1String(), QLatin1String(), -1, EmojiCode(0));
        return v;
    }

    EmojiRecord::EmojiRecord(QLatin1String _category, QLatin1String _fileName, QLatin1String _shortName, const int _index, EmojiCode _baseCodePoints)
        : EmojiRecord(std::move(_category), std::move(_fileName), std::move(_shortName), _index, _baseCodePoints, _baseCodePoints)
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

    static EmojiIndex makeIndex(const EmojiRecordVec& vector, CodePointType type)
    {
#if defined(__APPLE__)
        static_assert ((canSendEmojiOneOnMac() && canViewEmojiOneOnMac())
                       || (!canSendEmojiOneOnMac() && canViewEmojiOneOnMac())
                       || (!canSendEmojiOneOnMac() && !canViewEmojiOneOnMac()), "invariant fail");
#endif
        EmojiIndex index;
        for (size_t i = 0; i < vector.size(); ++i)
        {
            const auto& record = vector[i];
            if (skipEmoji(record.baseCodePoints_))
                continue;

#if defined(__APPLE__)
            if constexpr (useNativeEmoji())
            {
                if (!mac::supportEmoji(record.fullCodePoints) && !canViewEmojiOneOnMac())
                    continue;
            }
#endif
            index.emplace(type == CodePointType::Full ? record.fullCodePoints : record.baseCodePoints_, i);
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
            constexpr static EmojiCode::codePointType genderNeutral = 0x1f9d1;
            const auto containsWithGender = std::any_of(std::begin(genders), std::end(genders),
                                                        [&index, &code](auto gender) { return index.find(EmojiCode::addCodePoint(code, gender)) != index.end(); });
            const auto containsGenderNeutral = code.contains(genderNeutral);

            return containsWithGender || containsGenderNeutral;
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
        im_assert(!EmojiIndexByOrder_.empty());

        emojiIndexByFullCodepoint_ = makeIndex(EmojiIndexByOrder_, CodePointType::Full);

        emojiIndexByBaseCodepoint_ = makeIndex(EmojiIndexByOrder_, CodePointType::Base);

        for (const auto& record : EmojiIndexByOrder_)
        {
            if (skipEmoji(record.baseCodePoints_))
                continue;

            if (containsSkinTone(record.baseCodePoints_)) // don't add skin tones to picker
                continue;

            if (containsWithGender(emojiIndexByBaseCodepoint_, record.baseCodePoints_))
                continue;

            if (isHairType(record.baseCodePoints_))
                continue;

            if (record.Category_.size() == 0)
                continue;

#if defined(__APPLE__)
            if constexpr (useNativeEmoji())
            {
                if (!canSendEmojiOneOnMac() && !mac::supportEmoji(record.fullCodePoints) && !isSupported(record.FileName_))
                    continue;
            }
#else
            if (!isSupported(record.FileName_))
                continue;
#endif

            const auto& category = record.Category_;
            im_assert(category.size() > 0);

            auto& records = EmojiIndexByCategory_[category];
            records.push_back(record);

            if (record.ShortName_.size() != 0)
                flagEmojis_[record.ShortName_] = record.fullCodePoints;
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
        im_assert(!_code.isNull());
        im_assert(!emojiIndexByFullCodepoint_.empty() && !emojiIndexByBaseCodepoint_.empty());
        return std::as_const(emojiIndexByFullCodepoint_).find(_code) != std::as_const(emojiIndexByFullCodepoint_).end()
            || std::as_const(emojiIndexByBaseCodepoint_).find(_code) != std::as_const(emojiIndexByBaseCodepoint_).end();
    }

    const EmojiRecord& GetEmojiInfoByCodepoint(const EmojiCode& _code)
    {
        im_assert(!_code.isNull());
        im_assert(!emojiIndexByFullCodepoint_.empty() && !emojiIndexByBaseCodepoint_.empty());

        if (const auto iter = std::as_const(emojiIndexByFullCodepoint_).find(_code); iter != std::as_const(emojiIndexByFullCodepoint_).end())
            return EmojiIndexByOrder_[iter->second];

        if (const auto iter = std::as_const(emojiIndexByBaseCodepoint_).find(_code); iter != std::as_const(emojiIndexByBaseCodepoint_).end())
            return EmojiIndexByOrder_[iter->second];

        return EmojiRecord::invalid();
    }

    const QVector<QLatin1String>& GetEmojiCategories()
    {
        im_assert(!EmojiCategories_.isEmpty());

        return EmojiCategories_;
    }

    const EmojiRecordVec& GetEmojiInfoByCategory(QLatin1String _category)
    {
        im_assert(_category.size() > 0);

        if (const auto iter = std::as_const(EmojiIndexByCategory_).find(_category); iter != std::as_const(EmojiIndexByCategory_).end())
            return iter->second;

        static const EmojiRecordVec empty;
        return empty;
    }

    const EmojiCode& GetEmojiInfoByCountryName(const QString& _country)
    {
        if (const auto iter = std::as_const(flagEmojis_).find(_country); iter != std::as_const(flagEmojis_).end())
            return iter->second;

        static const EmojiCode empty;
        return empty;
    }
}
