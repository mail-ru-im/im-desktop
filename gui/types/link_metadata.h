#pragma once

#include "../namespaces.h"

DATA_NS_BEGIN

enum class LinkContentType
{
    None,
    Article,
    ArticleVideo,
    ArticleGif,
    Image,
    Video,
    Gif,
    File
};

class LinkMetadata final
{
public:
    LinkMetadata();

    LinkMetadata(const QString& _title,
        const QString& _description,
        const QString& _contentType,
        const QString& _previewUri,
        const QString& _faviconUri,
        const QSize& _previewSize,
        const QString& _downloadUri,
        const int64_t _fileSize,
        const QSize& _originSize,
        const QString& _fileName,
        const QString& _fileFormat);

    ~LinkMetadata();

    const QString& getTitle() const;
    const QString& getDescription() const;
    const QString& getDownloadUri() const;
    const QString& getContentTypeStr() const;
    const QString& getPreviewUri() const;
    const QString& getFaviconUri() const;
    const QString& getFileName() const;
    const QString& getFileFormat() const;
    LinkContentType getContentType() const;
    const QSize& getPreviewSize() const;
    const QSize& getOriginSize() const;
    int64_t getFileSize() const;

private:
    QString title_;
    QString description_;
    QString downloadUri_;
    QString contentTypeStr_;
    QString previewUri_;
    QString faviconUri_;
    QString fileName_;
    QString fileFormat_;
    QSize previewSize_;
    int64_t fileSize_;
    QSize originSize_;
    LinkContentType contentType_;
};

static_assert(
    std::is_move_assignable<LinkMetadata>::value,
    "std::is_move_assignable<LinkMetadata>::value");

static_assert(
    std::is_move_constructible<LinkMetadata>::value,
    "std::is_move_constructible<LinkMetadata>::value");

DATA_NS_END

Q_DECLARE_METATYPE(Data::LinkMetadata);
