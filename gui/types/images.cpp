#include "stdafx.h"

#include "../../corelib/collection_helper.h"

#include "images.h"

Data::Image::Image()
    : msgId_(0)
    , is_filesharing_(false)
{
}

Data::Image::Image(
    const quint64 _msgId,
    const QString& _url,
    const bool _is_filesharing,
    const QPixmap& _preview,
    const QSize& _originalFileSize)
    : msgId_(_msgId)
    , url_(_url)
    , is_filesharing_(_is_filesharing)
    , preview_(_preview)
    , originalImageSize_(_originalFileSize)
{
}

bool Data::Image::isNull() const
{
    return msgId_ == 0 || url_.isEmpty();
}

Data::ImageListPtr Data::UnserializeImages(const core::coll_helper& _helper)
{
    auto images = std::make_shared<ImageList>();

    const auto array = _helper.get_value_as_array("images");
    const int size = array->size();
    images->reserve(size);
    for (core::iarray::size_type i = 0; i < size; ++i)
    {
        core::coll_helper value(array->get_at(i)->get_as_collection(), false);
        const auto msgid = value.get_value_as_int64("msgid");
        const auto is_filesharing = value.get_value_as_bool("is_filesharing");

        images->push_back(Image(msgid, QString::fromUtf8(value.get_value_as_string("url")), is_filesharing, QPixmap(), QSize()));
    }

    return images;
}

bool operator==(const Data::Image& left, const Data::Image& right)
{
    if (left.is_filesharing_ != right.is_filesharing_)
        return false;

    if (left.msgId_ != right.msgId_)
        return false;

    return left.url_ == right.url_;
}

bool operator!=(const Data::Image& left, const Data::Image& right)
{
    return !(left == right);
}
