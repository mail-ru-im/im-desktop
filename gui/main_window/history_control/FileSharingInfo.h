#pragma once

namespace core
{
    class coll_helper;
    struct file_sharing_content_type;
}

namespace HistoryControl
{
    class FileSharingInfo
    {
    public:
        FileSharingInfo(const core::coll_helper &info);

        const QString& GetLocalPath() const;

        const QString& GetUploadingProcessId() const;

        QSize GetSize() const;

        const QString& GetUri() const;

        bool IsOutgoing() const;

        bool HasLocalPath() const;

        bool HasSize() const;

        bool HasUri() const;

        void SetUri(const QString &uri);

        QString ToLogString() const;

        core::file_sharing_content_type& getContentType() const;

        void setContentType(const core::file_sharing_content_type& type);

        const std::optional<qint64>& duration() const noexcept;

    private:
        bool IsOutgoing_;

        QString LocalPath_;

        std::unique_ptr<QSize> Size_;

        QString Uri_;

        QString UploadingProcessId_;

        std::unique_ptr<core::file_sharing_content_type> ContentType_;

        std::optional<qint64> duration_;
    };

    typedef std::shared_ptr<FileSharingInfo> FileSharingInfoSptr;
}