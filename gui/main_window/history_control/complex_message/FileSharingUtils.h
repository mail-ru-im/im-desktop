#pragma once

#include "../../../namespaces.h"
#include "../../../../corelib/enumerations.h"

namespace Utils
{
    struct FileSharingId
    {
        QString fileId;
        std::optional<QString> sourceId;
    private:
        friend bool operator==(const FileSharingId& _lhs, const FileSharingId& _rhs)
        {
            return _lhs.fileId == _rhs.fileId && _lhs.sourceId == _rhs.sourceId;
        }
        friend bool operator!=(const FileSharingId& _lhs, const FileSharingId& _rhs)
        {
            return !operator==(_lhs, _rhs);
        }
    };

    struct FileSharingIdHasher
    {
        std::size_t operator()(const FileSharingId& _fsId) const noexcept
        {
            const auto h1 = qHash(_fsId.fileId);
            if (!_fsId.sourceId)
                return h1;
            const auto h2 = qHash(*_fsId.sourceId);
            return ((h1 << 16) | (h1 >> 16)) ^ h2;
        }
    };
}

UI_COMPLEX_MESSAGE_NS_BEGIN

core::file_sharing_content_type extractContentTypeFromFileSharingId(QStringView _id);
bool isLottieFileSharingId(QStringView _id);

int32_t extractDurationFromFileSharingId(QStringView id);

QSize extractSizeFromFileSharingId(QStringView id);

Utils::FileSharingId extractIdFromFileSharingUri(QStringView uri);

inline core::file_sharing_content_type getFileSharingContentType(QStringView _uri)
{
    return extractContentTypeFromFileSharingId(extractIdFromFileSharingUri(_uri).fileId);
}

UI_COMPLEX_MESSAGE_NS_END
