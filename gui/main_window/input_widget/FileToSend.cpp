#include "stdafx.h"
#include "FileToSend.h"

#include "utils/memory_utils.h"

namespace Ui
{
    FileToSend::FileToSend(const QString& _path)
        : file_(_path)
    {
    }

    FileToSend::FileToSend(QFileInfo _info)
        : file_(std::move(_info))
    {
    }

    FileToSend::FileToSend(const QPixmap& _pixmap)
        : pixmap_(_pixmap)
    {
    }

    int64_t FileToSend::getSize() const noexcept
    {
        return isFile() ? file_.size() : Utils::getMemoryFootprint(pixmap_);
    }

    bool FileToSend::operator==(const FileToSend& _other) const
    {
        if (isClipboardImage() || _other.isClipboardImage())
            return false;

        return file_ == _other.file_;
    }
}