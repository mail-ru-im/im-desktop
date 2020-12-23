#include "stdafx.h"
#include "FileToSend.h"

#include "utils/memory_utils.h"
#include "utils/utils.h"
#include "utils/features.h"

namespace Ui
{
    FileToSend::FileToSend(QString _path, bool _isGif)
        : file_(std::move(_path))
        , isGif_(_isGif)
    {
    }

    FileToSend::FileToSend(QFileInfo _info)
        : file_(std::move(_info))
        , isGif_(false)
    {
    }

    FileToSend::FileToSend(const QPixmap& _pixmap)
        : pixmap_(_pixmap)
        , isGif_(false)
    {
    }

    int64_t FileToSend::getSize() const noexcept
    {
        return isFile() ? file_.size() : Utils::getMemoryFootprint(pixmap_);
    }

    bool FileToSend::canConvertToWebp() const
    {
        if (isClipboardImage())
            return true;

        return file_.size() <= Features::maxFileSizeForWebpConvert() && Utils::canCovertToWebp(file_.absoluteFilePath());
    }

    QByteArray FileToSend::loadImageData(const FileToSend& _fileToSend, Format _format)
    {
        const auto isWebp = _format == Format::webp;
        if (_fileToSend.isClipboardImage())
            return isWebp ? Utils::imageToData(_fileToSend.getPixmap(), "webp", 100) : Utils::imageToData(_fileToSend.getPixmap(), "png");

        QImage image;
        QSize size;
        Utils::loadImageScaled(_fileToSend.getFileInfo().absoluteFilePath(), {}, image, size);
        if (!image.isNull())
            return isWebp ? Utils::imageToData(image, "webp", 100) : Utils::imageToData(image, "png");
        Q_UNREACHABLE();
        return {};
    }

    bool FileToSend::operator==(const FileToSend& _other) const
    {
        if (isClipboardImage() || _other.isClipboardImage())
            return false;

        return file_ == _other.file_;
    }
}
