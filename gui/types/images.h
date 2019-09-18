#pragma once

namespace core
{
    class coll_helper;
}

namespace Data
{
    struct Image
    {
        Image();
        Image(
            const quint64 _msgId,
            const QString& _url,
            const bool _is_filesharing,
            const QPixmap& _preview,
            const QSize& _originalFileSize);

        bool isNull() const;

        quint64 msgId_;
        QString url_;

        bool is_filesharing_;

        QPixmap preview_;
        QSize originalImageSize_;
    };

    typedef QList<Image> ImageList;
    typedef std::shared_ptr<ImageList> ImageListPtr;

    ImageListPtr UnserializeImages(const core::coll_helper& _helper);
}

bool operator==(const Data::Image& left, const Data::Image& right);
bool operator!=(const Data::Image& left, const Data::Image& right);
