#include "stdafx.h"

#include "../../../../corelib/enumerations.h"

#include "FileSharingUtils.h"

#include "../../../utils/utils.h"
#include "../../../utils/UrlParser.h"

UI_COMPLEX_MESSAGE_NS_BEGIN

namespace
{
    using ReverseIndexMap = std::vector<int32_t>;

    int32_t decodeDuration(const QStringRef &str);

    int32_t decodeSize(const QChar qch0, const QChar qch1);

    const ReverseIndexMap& getReverseIndexMap();
}

core::file_sharing_content_type extractContentTypeFromFileSharingId(const QStringRef& _id)
{
    core::file_sharing_content_type type;

    if (_id.isEmpty())
        return type;

    const auto id0 = _id.at(0).toLatin1();

    if (const auto is_gif = (id0 >= '4') && (id0 <= '5'); is_gif)
    {
        type.type_ = core::file_sharing_base_content_type::gif;
        if (id0 == '5')
            type.subtype_ = core::file_sharing_sub_content_type::sticker;

        return type;
    }

    if (const auto is_ppt = (id0 == 'I') || (id0 == 'J'); is_ppt)
    {
        type.type_ = core::file_sharing_base_content_type::ptt;

        return type;
    }

    if (const auto is_image = (id0 >= '0') && (id0 <= '7'); is_image)
    {
        type.type_ = core::file_sharing_base_content_type::image;
        if (id0 == '2')
            type.subtype_ = core::file_sharing_sub_content_type::sticker;

        return type;
    }

    if (const auto is_video = ((id0 >= '8') && (id0 <= '9')) || ((id0 >= 'A') && (id0 <= 'F')); is_video)
    {
        type.type_ = core::file_sharing_base_content_type::video;
        if (id0 == 'D')
            type.subtype_ = core::file_sharing_sub_content_type::sticker;

        return type;
    }
    return type;
}

int32_t extractDurationFromFileSharingId(const QStringRef &id)
{
    const auto isValidId = (id.length() >= 5);
    if (!isValidId)
    {
        assert(!"invalid file sharing id");
        return -1;
    }
    if (const auto type = extractContentTypeFromFileSharingId(id); !type.is_ptt())
        return -1;

    return decodeDuration(id.mid(1, 4));
}

QSize extractSizeFromFileSharingId(const QStringRef &id)
{
    const auto isValidId = (id.length() > 5);
    if (!isValidId)
    {
        assert(!"invalid id");
        return QSize();
    }

    const auto width = decodeSize(id.at(1), id.at(2));
    assert(width >= 0);

    const auto height = decodeSize(id.at(3), id.at(4));
    assert(height >= 0);

    return QSize(width, height);
}

QString extractIdFromFileSharingUri(const QStringRef &uri)
{
    assert(!uri.isEmpty());
    if (uri.isEmpty())
    {
        return QString();
    }

    const auto normalizedUri = Utils::normalizeLink(uri);

    Utils::UrlParser parser;
    parser.process(normalizedUri);
    return parser.getFilesharingId();
}

namespace
{
    constexpr auto ASCII_MAX = 128;

    constexpr auto INDEX_DIVISOR = 62;

    int32_t decodeDuration(const QStringRef &str)
    {
        const static auto arr = qsl("0123456789abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ");
        int32_t result = 0;
        auto i = 0;
        for (const auto qch : str)
        {
            auto value = arr.indexOf(qch);
            const auto exp = arr.length() * (str.length() - i - 1);
            if (exp > 0)
                value *= exp;

            result += value;
            ++i;
        }

        return result;
    }

    int32_t decodeSize(const QChar qch0, const QChar qch1)
    {
        const auto ch0 = (size_t)qch0.toLatin1();
        const auto ch1 = (size_t)qch1.toLatin1();

        const auto &map = getReverseIndexMap();

        if (ch0 >= map.size())
        {
            assert(!"invalid first character");
            return 0;
        }

        if (ch1 >= map.size())
        {
            assert(!"invalid first character");
            return 0;
        }

        const auto index0 = map[ch0];
        assert(index0 >= 0);

        const auto index1 = map[ch1];
        assert(index1 >= 0);

        auto size = (index0 * INDEX_DIVISOR);
        size += index1;

        assert(size > 0);
        return size;
    }

    const ReverseIndexMap& getReverseIndexMap()
    {
        static ReverseIndexMap map;

        if (!map.empty())
            return map;

        map.resize(ASCII_MAX);

        auto index = 0;

        const auto fillMap =
            [&index]
            (const char from, const char to)
            {
                for (auto ch = from; ch <= to; ++ch, ++index)
                {
                    map[ch] = index;
                }
            };

        fillMap('0', '9');
        fillMap('a', 'z');
        fillMap('A', 'Z');

        return map;
    }
}

UI_COMPLEX_MESSAGE_NS_END
