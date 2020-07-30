#include "stdafx.h"

#include "link_metadata.h"

namespace
{
    Data::LinkContentType contentTypeFromStr(const QString& _contentTypeStr)
    {
        if (_contentTypeStr == u"article")
            return Data::LinkContentType::Article;
        else if (_contentTypeStr == u"article-video")
            return Data::LinkContentType::ArticleVideo;
        else if (_contentTypeStr == u"article-gif")
            return Data::LinkContentType::ArticleGif;
        else if (_contentTypeStr == u"image")
            return Data::LinkContentType::Image;
        else if (_contentTypeStr == u"video")
            return Data::LinkContentType::Video;
        else if (_contentTypeStr == u"gif")
            return Data::LinkContentType::Gif;
        else if (_contentTypeStr == u"file")
            return Data::LinkContentType::File;
        else
            return Data::LinkContentType::None;
    }
}

DATA_NS_BEGIN

LinkMetadata::LinkMetadata()
    : fileSize_(-1)
    , contentType_(LinkContentType::None)
{
}

LinkMetadata::LinkMetadata(const QString &_title,
    const QString &_description,
    const QString &_siteName,
    const QString &_contentType,
    const QString& _previewUri,
    const QString& _faviconUri,
    const QSize &_previewSize,
    const QString &_downloadUri,
    const int64_t _fileSize,
    const QSize &_originSize,
    const QString& _fileName)
    : title_(_title)
    , description_(_description)
    , downloadUri_(_downloadUri)
    , siteName_(_siteName)
    , contentTypeStr_(_contentType)
    , previewUri_(_previewUri)
    , faviconUri_(_faviconUri)
    , fileName_(_fileName)
    , previewSize_(_previewSize)
    , fileSize_(_fileSize)
    , originSize_(_originSize)
{
    assert(previewSize_.width() >= 0);
    assert(previewSize_.height() >= 0);
    assert(fileSize_ >= -1);

    contentType_ = contentTypeFromStr(contentTypeStr_);
}

LinkMetadata::~LinkMetadata()
{
}

const QString& LinkMetadata::getTitle() const
{
    return title_;
}

const QString& LinkMetadata::getDescription() const
{
    return description_;
}

const QString& LinkMetadata::getDownloadUri() const
{
    return downloadUri_;
}

const QString& LinkMetadata::getSiteName() const
{
    return siteName_;
}

const QString& LinkMetadata::getContentTypeStr() const
{
    return contentTypeStr_;
}

const QString& LinkMetadata::getPreviewUri() const
{
    return previewUri_;
}

const QString& LinkMetadata::getFaviconUri() const
{
    return faviconUri_;
}

const QString& LinkMetadata::getFileName() const
{
    return fileName_;
}

LinkContentType LinkMetadata::getContentType() const
{
    return contentType_;
}

const QSize& LinkMetadata::getPreviewSize() const
{
    return previewSize_;
}

const QSize& LinkMetadata::getOriginSize() const
{
    return originSize_;
}

int64_t LinkMetadata::getFileSize() const
{
    return fileSize_;
}

DATA_NS_END
