#include "stdafx.h"

#include "BlurPixmapTask.h"
#include "utils/blur/stackblur.h"

namespace Utils
{
    BlurPixmapTask::BlurPixmapTask(const QPixmap& _source, const unsigned int _radius, const int _coreCount)
        : source_(_source)
        , radius_(_radius)
        , coreCount_(_coreCount)
    {
        assert(!source_.isNull());
        assert(radius_ >= minRadius());
        assert(radius_ <= maxRadius()); // algorithm requirement
    }

    void BlurPixmapTask::run()
    {
        const auto key = source_.cacheKey();
        if (radius_ < minRadius() || radius_ > maxRadius() || source_.isNull())
        {
            emit blurred(source_, key, QPrivateSignal());
            return;
        }

        emit blurred(blurPixmap(), key, QPrivateSignal());
    }

    QPixmap BlurPixmapTask::blurPixmap() const
    {
        auto img = source_.toImage().convertToFormat(QImage::Format_ARGB32);
        stackblur(img.bits(), source_.width(), source_.height(), radius_, coreCount_);
        return QPixmap::fromImage(std::move(img));
    }
}
