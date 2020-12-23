#include "stdafx.h"

#include "StickerData.h"
#include "utils/utils.h"

#ifndef STRIP_AV_MEDIA
#include "main_window/mplayer/LottieHandle.h"
#endif // !STRIP_AV_MEDIA

namespace Ui::Stickers
{
    void StickerData::loadFromFile(const QString& _path)
    {
        if (_path.isEmpty())
        {
            assert(false);
            return;
        }

        if (Q_UNLIKELY(!QCoreApplication::instance()))
            return;

        QPixmap pm;
        QSize originalSize;

        if (Utils::loadPixmapScaled(_path, QSize(), pm, originalSize) && !!QCoreApplication::instance())
        {
            setPixmap(std::move(pm));
        }
#ifndef STRIP_AV_MEDIA
        else if (LottieHandleCache::instance().getHandle(_path).isValid())
        {
            data_ = _path;
        }
#endif
    }

    void StickerData::setPixmap(QPixmap _pixmap)
    {
        assert(!_pixmap.isNull());

        Utils::check_pixel_ratio(_pixmap);

        if (Q_UNLIKELY(!QCoreApplication::instance()))
            return;
        data_ = std::move(_pixmap);
    }

    const QPixmap& StickerData::getPixmap() const noexcept
    {
        assert(isPixmap() || !isValid());
        if (auto dataPointer = std::get_if<QPixmap>(&data_))
        {
            return *dataPointer;
        }
        else
        {
            static const QPixmap nullPixmap;
            return nullPixmap;
        }
    }

    const QString& StickerData::getLottiePath() const noexcept
    {
        assert(isLottie() || !isValid());
        if (auto dataPointer = std::get_if<QString>(&data_))
        {
            return *dataPointer;
        }
        else
        {
            static const QString emptyString;
            return emptyString;
        }
    }

    const StickerData& StickerData::invalid()
    {
        static const StickerData icon;
        return icon;
    }

    StickerData::StickerData(const StickerData& other)
        : data_(canSafelyConstructFrom(other) ? other.data_ : std::monostate{})
    {

    }

    StickerData::StickerData(StickerData&& other) noexcept
        : data_(canSafelyConstructFrom(other) ? std::move(other.data_) : std::monostate{})
    {
        other.data_ = {};
    }

    StickerData& StickerData::operator=(StickerData&& other) noexcept
    {
        data_ = std::move(other.data_);
        other.data_ = {};
        return *this;
    }

    bool StickerData::canSafelyConstructFrom(const StickerData& _stickerData)
    {
        // QPixmap has deleted move constructor in Qt5
        return QCoreApplication::instance() || !_stickerData.isPixmap();
    }

    StickerData StickerData::makeLottieData(const QString& _path)
    {
        StickerData data;
        data.data_ = _path;
        return data;
    }
}
