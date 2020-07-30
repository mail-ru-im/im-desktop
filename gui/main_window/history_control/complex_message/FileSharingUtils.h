#pragma once

#include "../../../namespaces.h"

CORE_NS_BEGIN
struct file_sharing_content_type;
CORE_NS_END

UI_COMPLEX_MESSAGE_NS_BEGIN

core::file_sharing_content_type extractContentTypeFromFileSharingId(const QStringRef& _id);
inline core::file_sharing_content_type extractContentTypeFromFileSharingId(const QString& _id)
{
    return extractContentTypeFromFileSharingId(QStringRef(&_id));
}

int32_t extractDurationFromFileSharingId(const QStringRef &id);
inline int32_t extractDurationFromFileSharingId(const QString &id)
{
    return extractDurationFromFileSharingId(QStringRef(&id));
}

QSize extractSizeFromFileSharingId(const QStringRef &id);
inline QSize extractSizeFromFileSharingId(const QString &id)
{
    return extractSizeFromFileSharingId(QStringRef(&id));
}

QString extractIdFromFileSharingUri(const QStringRef &uri);
inline QString extractIdFromFileSharingUri(const QString &uri)
{
    return extractIdFromFileSharingUri(QStringRef(&uri));
}

inline core::file_sharing_content_type getFileSharingContentType(const QStringRef& _uri)
{
    return extractContentTypeFromFileSharingId(extractIdFromFileSharingUri(_uri));
}
inline core::file_sharing_content_type getFileSharingContentType(const QString& _uri)
{
    return getFileSharingContentType(QStringRef(&_uri));
}

UI_COMPLEX_MESSAGE_NS_END
