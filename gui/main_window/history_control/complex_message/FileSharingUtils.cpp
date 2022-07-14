#include "stdafx.h"

#include "FileSharingUtils.h"

#include "../../../utils/utils.h"
#include "../../../utils/UrlParser.h"

UI_COMPLEX_MESSAGE_NS_BEGIN

namespace
{
    using ReverseIndexMap = std::vector<int32_t>;

    int32_t decodeDuration(QStringView _str);

    int32_t decodeSize(QChar _qch0, QChar _qch1);

    const ReverseIndexMap& getReverseIndexMap();
} // namespace

core::file_sharing_content_type extractContentTypeFromFileSharingId(QStringView _id)
{
    if (_id.isEmpty())
        return {};

    const auto id0 = _id.at(0).toLatin1();
    core::file_sharing_content_type type;

    const auto isOneOf = [id0](std::string_view _values)
    {
        return _values.find(id0) != std::string_view::npos;
    };

    if (isOneOf("45")) // gif
    {
        type.type_ = core::file_sharing_base_content_type::gif;
        if (id0 == '5')
            type.subtype_ = core::file_sharing_sub_content_type::sticker;
    }
    else if (isOneOf("IJ")) // ptt
    {
        type.type_ = core::file_sharing_base_content_type::ptt;
    }
    else if (isOneOf("L")) // lottie sticker
    {
        type.type_ = core::file_sharing_base_content_type::lottie;
        type.subtype_ = core::file_sharing_sub_content_type::sticker;
    }
    else if (isOneOf("01234567")) // image
    {
        type.type_ = core::file_sharing_base_content_type::image;
        if (id0 == '2')
            type.subtype_ = core::file_sharing_sub_content_type::sticker;
    }
    else if (isOneOf("89ABCDEF")) // video
    {
        type.type_ = core::file_sharing_base_content_type::video;
        if (id0 == 'D')
            type.subtype_ = core::file_sharing_sub_content_type::sticker;
    }
    return type;
}

bool isLottieFileSharingId(QStringView _id)
{
    return extractContentTypeFromFileSharingId(_id).is_lottie();
}

int32_t extractDurationFromFileSharingId(QStringView _id)
{
    const auto isValidId = (_id.size() >= 5);
    if (!isValidId)
    {
        im_assert(!"invalid file sharing id");
        return -1;
    }
    if (const auto type = extractContentTypeFromFileSharingId(_id); !type.is_ptt())
        return -1;

    return decodeDuration(_id.mid(1, 4));
}

QSize extractSizeFromFileSharingId(QStringView _id)
{
    const auto isValidId = (_id.size() > 5);
    if (!isValidId)
    {
        im_assert(!"invalid id");
        return QSize();
    }

    const auto width = decodeSize(_id.at(1), _id.at(2));
    im_assert(width >= 0);

    const auto height = decodeSize(_id.at(3), _id.at(4));
    im_assert(height >= 0);

    return QSize(width, height);
}

Utils::FileSharingId extractIdFromFileSharingUri(QStringView _uri)
{
    if (_uri.isEmpty())
        return {};

    const auto normalizedUri = Utils::normalizeLink(_uri);
    Utils::UrlParser parser;
    parser.process(normalizedUri);
    return parser.getFilesharingId();
}

namespace
{
    constexpr auto ASCII_MAX = 128;

    constexpr auto INDEX_DIVISOR = 62;

    int32_t decodeDuration(QStringView _str)
    {
        static constexpr auto arr = QStringView(u"0123456789abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ");
        int32_t result = 0;
        auto i = 0;
        for (auto qch : _str)
        {
            auto value = arr.indexOf(qch);
            const auto exp = arr.size() * (_str.size() - i - 1);
            if (exp > 0)
                value *= exp;

            result += value;
            ++i;
        }

        return result;
    }

    int32_t decodeSize(QChar _qch0, QChar _qch1)
    {
        const auto ch0 = (size_t)_qch0.toLatin1();
        const auto ch1 = (size_t)_qch1.toLatin1();

        const auto& map = getReverseIndexMap();

        if (ch0 >= map.size())
        {
            im_assert(!"invalid first character");
            return 0;
        }

        if (ch1 >= map.size())
        {
            im_assert(!"invalid first character");
            return 0;
        }

        const auto index0 = map[ch0];
        im_assert(index0 >= 0);

        const auto index1 = map[ch1];
        im_assert(index1 >= 0);

        auto size = (index0 * INDEX_DIVISOR);
        size += index1;

        im_assert(size > 0);
        return size;
    }

    const ReverseIndexMap& getReverseIndexMap()
    {
        static ReverseIndexMap map;

        if (!map.empty())
            return map;

        map.resize(ASCII_MAX);

        auto index = 0;

        const auto fillMap = [&index](const char _from, const char _to)
        {
            for (auto ch = _from; ch <= _to; ++ch, ++index)
                map[ch] = index;
        };

        fillMap('0', '9');
        fillMap('a', 'z');
        fillMap('A', 'Z');

        return map;
    }
} // namespace

UI_COMPLEX_MESSAGE_NS_END
