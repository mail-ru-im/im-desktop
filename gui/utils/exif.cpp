#include "stdafx.h"

#include "exif.h"

UTILS_EXIF_NS_BEGIN

namespace
{
    struct ExifTagInfo
    {
        ExifTagInfo();

        ExifTagInfo(const char *begin, const size_t length, const bool isBigEndian);

        bool isEmpty() const;

        const char *begin_;

        const size_t length_;

        const bool isBigEndian_;
    };

    struct Interoperability
    {
        Interoperability();

        Interoperability(const int32_t tag, const int32_t type, const int32_t count, const int32_t valueOrOffset);

        bool isEmpty() const;

        const int32_t tag_;

        const int32_t type_;

        const int32_t count_;

        const int32_t valueOrOffset_;
    };

    ExifTagInfo findExifData(const char *buf, const size_t bufSize);

    uint16_t peekUnsignedShort(const char *buf, const bool isBigEndian);

    ExifOrientation searchForOrientationTag(const ExifTagInfo &exifInfo);
}

void applyExifOrientation(const ExifOrientation orientation, InOut QPixmap &pixmap)
{
    assert(!pixmap.isNull());

    const auto isOrientationValid = (
        (orientation > ExifOrientation::Min) &&
        (orientation < ExifOrientation::Max));
    if (!isOrientationValid)
    {
        assert(!"invalid orientation value");
        return;
    }

    if (orientation == ExifOrientation::Normal)
    {
        return;
    }

    QMatrix modelMatrix;

    switch(orientation)
    {
        case ExifOrientation::FlipX:
            modelMatrix.scale(-1, 1);
            break;

        case ExifOrientation::FlipY:
            modelMatrix.scale(1, -1);
            break;

        case ExifOrientation::Rotate180C:
            modelMatrix.rotate(180);
            break;

        case ExifOrientation::Rotate90A:
            modelMatrix.rotate(-90);
            break;

        case ExifOrientation::Rotate90AFlipX:
            modelMatrix.rotate(-90);
            modelMatrix.scale(-1, 1);
            break;

        case ExifOrientation::Rotate90C:
            modelMatrix.rotate(90);
            break;

        case ExifOrientation::Rotate90CFlipX:
            modelMatrix.rotate(90);
            modelMatrix.scale(-1, 1);
            break;

        default:
            assert(!"IP is not expected to be here");
            return;
    }

    pixmap = pixmap.transformed(modelMatrix);
}

ExifOrientation getExifOrientation(const char *buf, const size_t bufSize)
{
    assert(buf);
    assert(bufSize > 0);

    const auto exifInfo = findExifData(buf, bufSize);
    if (exifInfo.isEmpty())
    {
        return ExifOrientation::Normal;
    }

    return searchForOrientationTag(exifInfo);
}

namespace
{
    ExifTagInfo::ExifTagInfo()
        : begin_(nullptr)
        , length_(0)
        , isBigEndian_(false)
    {
    }

    ExifTagInfo::ExifTagInfo(const char *begin, const size_t length, const bool isBigEndian)
        : begin_(begin)
        , length_(length)
        , isBigEndian_(isBigEndian)
    {
        assert(begin_);
        assert(length > 0);
    }

    bool ExifTagInfo::isEmpty() const
    {
        return !begin_;
    }

    ExifTagInfo findExifData(const char *buf, const size_t bufSize)
    {
        assert(buf);
        assert(bufSize > 0);

        const auto EXIF_TAG_LENGTH = 6u; // 'e', 'x', 'i', 'f', '\0', '\0'

        const auto HEADER_LENGTH = (4u + EXIF_TAG_LENGTH); // 0xff, 0xe1, length, length
        if (bufSize <= HEADER_LENGTH)
        {
            return ExifTagInfo();
        }

        const auto scanMax = (bufSize - HEADER_LENGTH);

        const auto MIN_BUF_LENGTH = 512u;
        if (scanMax < MIN_BUF_LENGTH)
        {
            return ExifTagInfo();
        }

        auto index = 0u;
        for (; index <= scanMax; ++index)
        {
            const auto ifdBegin = (buf + index);
            auto cursor = ifdBegin;

            const auto header = *(uint16_t*)cursor;
            const auto isHeader = (header == 0xE1FF);
            if (!isHeader)
            {
                continue;
            }

            cursor += sizeof(uint16_t);

            const auto exifSize = *(uint16_t*)cursor;
            const auto isSizeEnough = (exifSize > EXIF_TAG_LENGTH);
            if (!isSizeEnough)
            {
                continue;
            }

            const auto exifBlockEnd = (index + exifSize);
            const auto isValidSize = (exifBlockEnd < bufSize);
            if (!isValidSize)
            {
                continue;
            }

            cursor += sizeof(uint16_t);

            const auto magicStrBegin = cursor;
            const auto isExifTag = (::strncmp(magicStrBegin, "Exif\0\0", EXIF_TAG_LENGTH) == 0);
            if (!isExifTag)
            {
                continue;
            }

            cursor += EXIF_TAG_LENGTH;

            const char end0 = *cursor;
            ++cursor;

            const char end1 = *cursor;
            ++cursor;

            const auto isLittleEndian = ((end0 == 'I') && (end1 == 'I')); // Intel
            const auto isBigEndian = ((end0 == 'M') && (end1 == 'M')); // Motorola
            const auto unknownEndianness = (!isLittleEndian && !isBigEndian);
            if (unknownEndianness)
            {
                continue;
            }

            cursor += sizeof(uint16_t); // 42

            const auto exifData = cursor;
            return ExifTagInfo(exifData, exifSize, isBigEndian);
        }

        return ExifTagInfo();
    }

    uint16_t peekUnsignedShort(const char *buf, const bool isBigEndian)
    {
        if (isBigEndian)
        {
            return qFromBigEndian(*(quint16*)buf);
        }

        return *(quint16*)buf;
    }

    ExifOrientation searchForOrientationTag(const ExifTagInfo &exifInfo)
    {
        assert(exifInfo.begin_);
        assert(exifInfo.length_ > 0);

        auto bytesLeft = exifInfo.length_;
        const char *cursor = exifInfo.begin_;

        if (bytesLeft <= sizeof(uint32_t))
        {
            return ExifOrientation::Normal;
        }

        cursor += sizeof(uint32_t);
        bytesLeft -= sizeof(uint32_t);

        for (;;)
        {
            assert(bytesLeft <= exifInfo.length_);

            if (bytesLeft <= sizeof(uint16_t))
            {
                return ExifOrientation::Normal;
            }

            const auto fieldsNum = peekUnsignedShort(cursor, exifInfo.isBigEndian_);

            const auto isValidFieldsNum = (fieldsNum > 0);
            if (!isValidFieldsNum)
            {
                return ExifOrientation::Normal;
            }

            cursor += sizeof(uint16_t);
            bytesLeft -= sizeof(uint16_t);

            std::vector<Interoperability> fields;
            fields.reserve(fieldsNum);

            for (auto index = 0u; index < fieldsNum; ++index)
            {
                const auto FIELD_LENGTH = 12;

                if (FIELD_LENGTH >= bytesLeft)
                {
                    return ExifOrientation::Normal;
                }

                const auto tagCode = peekUnsignedShort(cursor, exifInfo.isBigEndian_);
                const auto ORIENTATION_TAG_CODE = 0x0112;
                if (tagCode != ORIENTATION_TAG_CODE)
                {
                    cursor += FIELD_LENGTH;
                    bytesLeft -= FIELD_LENGTH;

                    continue;
                }

                cursor += sizeof(uint16_t); // tag
                cursor += sizeof(uint16_t); // type
                cursor += sizeof(uint32_t); // count

                const auto orientation = (ExifOrientation)peekUnsignedShort(cursor, exifInfo.isBigEndian_);

                const auto isOrientationValid = (
                    (orientation > ExifOrientation::Min) &&
                    (orientation < ExifOrientation::Max));
                if (isOrientationValid)
                {
                    return orientation;
                }

                assert(!"nonstandard orientation?");
                return ExifOrientation::Normal;
            }
        }

        return ExifOrientation::Normal;
    }
}

UTILS_EXIF_NS_END