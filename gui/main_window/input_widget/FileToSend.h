#pragma once

namespace Ui
{
    class FileToSend
    {
    public:
        FileToSend() = default;
        FileToSend(QString _path, bool _isGif = false);
        FileToSend(QFileInfo _info);
        FileToSend(const QPixmap& _pixmap);

        bool isClipboardImage() const noexcept { return !pixmap_.isNull(); }
        bool isFile() const noexcept { return !isClipboardImage(); }
        bool canConvertToWebp() const;
        bool isGifFile() const noexcept { return isGif_; }

        int64_t getSize() const noexcept;
        const QFileInfo& getFileInfo() const noexcept { return file_; }
        const QPixmap& getPixmap() const noexcept { return pixmap_; }

        bool operator==(const FileToSend& _other) const;

        enum class Format
        {
            webp,
            png
        };

        static QByteArray loadImageData(const FileToSend& _fileToSend, Format _format); // precondition: canConvertToWebp is true

    private:
        QFileInfo file_;
        QPixmap pixmap_;
        bool isGif_;
    };

    using FilesToSend = std::vector<FileToSend>;
}
