#pragma once

#include "../../../namespaces.h"

CORE_NS_BEGIN
struct file_sharing_content_type;
CORE_NS_END

UI_COMPLEX_MESSAGE_NS_BEGIN

core::file_sharing_content_type extractContentTypeFromFileSharingId(QStringView _id);
bool isLottieFileSharingId(QStringView _id);

int32_t extractDurationFromFileSharingId(QStringView id);

QSize extractSizeFromFileSharingId(QStringView id);

QString extractIdFromFileSharingUri(QStringView uri);

inline core::file_sharing_content_type getFileSharingContentType(QStringView _uri)
{
    return extractContentTypeFromFileSharingId(extractIdFromFileSharingUri(_uri));
}

UI_COMPLEX_MESSAGE_NS_END
