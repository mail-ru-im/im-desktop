#include "stdafx.h"

#include "../utils/utils.h"

#include "DpiAwareImage.h"

namespace Ui
{

    DpiAwareImage::DpiAwareImage()
    {
    }

    DpiAwareImage::DpiAwareImage(QImage _image)
        : Image_(std::move(_image))
    {
    }

    DpiAwareImage::operator bool() const
    {
        return !isNull();
    }

    void DpiAwareImage::draw(QPainter& _p, const int32_t _x, const int32_t _y) const
    {
        draw(_p, QPoint(_x, _y));
    }

    void DpiAwareImage::draw(QPainter& _p, const QPoint& _coords) const
    {
        assert(!Image_.isNull());

#ifdef __APPLE__
        auto imageRect = Image_.rect();
        imageRect.moveTopLeft(_coords);
        _p.drawImage(_coords, Image_);
#else
        auto imageRect = Utils::scale_bitmap(
            Image_.rect()
        );
        imageRect.moveTopLeft(_coords);
        _p.drawImage(imageRect, Image_);
#endif
    }

    bool DpiAwareImage::isNull() const
    {
        return Image_.isNull();
    }

    int32_t DpiAwareImage::width() const
    {
#ifdef __APPLE__
        return Image_.width() / Image_.devicePixelRatio();
#else
        return Image_.width();
#endif
    }

    int32_t DpiAwareImage::height() const
    {
#ifdef __APPLE__
        return Image_.height() / Image_.devicePixelRatio();
#else
        return Image_.height();
#endif
    }

}