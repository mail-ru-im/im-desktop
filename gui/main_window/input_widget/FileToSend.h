#pragma once

namespace Ui
{
    class FileToSend
    {
    public:
        FileToSend() = default;
        FileToSend(const QString& _path);
        FileToSend(QFileInfo _info);
        FileToSend(const QPixmap& _pixmap);

        bool isClipboardImage() const noexcept { return !pixmap_.isNull(); }
        bool isFile() const noexcept { return !isClipboardImage(); }

        int64_t getSize() const noexcept;
        const QFileInfo& getFileInfo() const noexcept { return file_; }
        const QPixmap& getPixmap() const noexcept { return pixmap_; }

        bool operator==(const FileToSend& _other) const;

    private:
        QFileInfo file_;
        QPixmap pixmap_;
    };

    using FilesToSend = std::vector<FileToSend>;
}