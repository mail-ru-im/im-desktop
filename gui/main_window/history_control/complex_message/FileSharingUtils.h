#pragma once

#include "../../../namespaces.h"
#include "../../../../corelib/enumerations.h"

namespace Utils
{
    struct FileSharingId
    {
        QString fileId_;
        std::optional<QString> sourceId_;
    private:
        friend bool operator==(const FileSharingId& _lhs, const FileSharingId& _rhs)
        {
            return _lhs.fileId_ == _rhs.fileId_ && _lhs.sourceId_ == _rhs.sourceId_;
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
            const auto h1 = qHash(_fsId.fileId_);
            if (!_fsId.sourceId_)
                return h1;
            const auto h2 = qHash(*_fsId.sourceId_);
            return ((h1 << 16) | (h1 >> 16)) ^ h2;
        }
    };
}

UI_COMPLEX_MESSAGE_NS_BEGIN

core::file_sharing_content_type extractContentTypeFromFileSharingId(QStringView _id);
bool isLottieFileSharingId(QStringView _id);

int32_t extractDurationFromFileSharingId(QStringView _id);

QSize extractSizeFromFileSharingId(QStringView _id);

Utils::FileSharingId extractIdFromFileSharingUri(QStringView _uri);

inline core::file_sharing_content_type getFileSharingContentType(QStringView _uri)
{
    return extractContentTypeFromFileSharingId(extractIdFromFileSharingUri(_uri).fileId_);
}

UI_COMPLEX_MESSAGE_NS_END
