#pragma once

#include "../namespaces.h"

DATA_NS_BEGIN

class LinkMetadata final
{
public:
    LinkMetadata();

    LinkMetadata(
        const QString &title,
        const QString &description,
        const QString &siteName,
        const QString &contentType,
        const QSize &previewSize,
        const QString &downloadUri,
        const int64_t fileSize,
        const QSize &originSize);

    ~LinkMetadata();

    const QString& getTitle() const;

    const QString& getDescription() const;

    const QString& getDownloadUri() const;

    const QString& getSiteName() const;

    const QString& getContentType() const;

    const QSize& getPreviewSize() const;

    const QSize& getOriginSize() const;

    int64_t getFileSize() const;

private:
    QString Title_;

    QString Description_;

    QString DownloadUri_;

    QString SiteName_;

    QString ContentType_;

    QSize PreviewSize_;

    int64_t FileSize_;

    QSize originSize_;
};

static_assert(
    std::is_move_assignable<LinkMetadata>::value,
    "std::is_move_assignable<LinkMetadata>::value");

static_assert(
    std::is_move_constructible<LinkMetadata>::value,
    "std::is_move_constructible<LinkMetadata>::value");

DATA_NS_END

Q_DECLARE_METATYPE(Data::LinkMetadata);