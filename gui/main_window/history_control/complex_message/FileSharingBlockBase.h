#pragma once

#include "../../../namespaces.h"

#include "GenericBlock.h"
#include "types/filesharing_meta.h"
#include "types/filesharing_download_result.h"

namespace Ui
{
    enum class MediaType;
}

CORE_NS_BEGIN
struct file_sharing_content_type;
CORE_NS_END

UI_COMPLEX_MESSAGE_NS_BEGIN

class IFileSharingBlockLayout;

class FileSharingBlockBase : public GenericBlock
{
    Q_OBJECT

public:
    FileSharingBlockBase(
        ComplexMessageItem *_parent,
        const QString &_link,
        const core::file_sharing_content_type& _type);

    virtual ~FileSharingBlockBase() override;

    virtual void clearSelection() override;

    virtual QString formatRecentsText() const final override;

    Ui::MediaType getMediaType() const final override;

    virtual IItemBlockLayout* getBlockLayout() const override;

    QString getProgressText() const;

    virtual QString getSourceText() const override;

    virtual QString getPlaceholderText() const override;

    virtual QString getSelectedText(const bool _isFullSelect = false, const TextDestination _dest = TextDestination::selection) const override;

    virtual void initialize() final override;

    virtual bool isBubbleRequired() const override;

    virtual bool isMarginRequired() const override;

    virtual bool isSelected() const override;

    virtual bool isAllSelected() const override { return isSelected(); }

    virtual ContentType getContentType() const override { return ContentType::FileSharing; }

    IItemBlock::MenuFlags getMenuFlags(QPoint p) const override;

    PinPlaceholderType getPinPlaceholder() const override;

    const QString& getFileLocalPath() const;

    int64_t getLastModified() const;

    const QString& getFilename() const;

    int64_t getFileSize() const;
    int64_t getBytesTransferred() const;

    const QString& getLink() const override;

    const QString& getDirectUri() const;

    bool isFileDownloaded() const;

    bool isFileDownloading() const;

    bool isFileUploading() const;

    bool isGifImage() const;

    bool isImage() const;

    bool isPtt() const;

    bool isPlainFile() const;

    bool isPreviewable() const override;

    bool isVideo() const;

    Data::FilesPlaceholderMap getFilePlaceholders() override;

Q_SIGNALS:
    void uploaded(QPrivateSignal) const;

protected:

    IFileSharingBlockLayout* getFileSharingLayout() const;

    QString getFileSharingId() const;

    const core::file_sharing_content_type& getType() const;

    virtual void initializeFileSharingBlock() = 0;

    virtual void onDataTransferStarted() = 0;

    virtual void onDataTransferStopped() = 0;

    virtual void onDownloaded() = 0;

    virtual void onDownloadedAction() = 0;

    virtual void onDataTransfer(const int64_t bytesTransferred, const int64_t bytesTotal) = 0;

    virtual void onDownloadingFailed(const int64_t requestId) = 0;

    virtual void onLocalCopyInfoReady(const bool isCopyExists) = 0;

    virtual void onMenuCopyLink() final override;

    virtual void onMenuCopyFile() final override;

    virtual void onMenuSaveFileAs() final override;

    virtual void onMenuOpenInBrowser() final override;

    void showFileInDir(const Utils::OpenAt _inFolder) override;

    virtual void onMetainfoDownloaded() = 0;

    virtual void onPreviewMetainfoDownloaded(const QString &miniPreviewUri, const QString &fullPreviewUri) = 0;

    void requestMetainfo(const bool isPreview);

    void setBlockLayout(IFileSharingBlockLayout *_layout);

    void setSelected(const bool _isSelected) override;

    void startDownloading(const bool _sendStats, const bool _forceRequestMetainfo = false, bool _highPriority = false);

    void stopDataTransfer();

    int64_t getPreviewMetaRequestId() const;
    void clearPreviewMetaRequestId();

    virtual void cancelRequests() override;

    void setFileName(const QString& _filename);

    void setFileSize(int64_t _size);

    void setLocalPath(const QString& _localPath);

    void setLoadedFromLocal(bool _value);

    void setUploadId(const QString& _id);

    bool isInited() const;

private:
    void connectSignals();

    bool isDownload(const Data::FileSharingDownloadResult& _result) const;

    QString getPlaceholderFormatText() const;

    int64_t BytesTransferred_;

    bool CopyFile_;

    int64_t DownloadRequestId_;

    QString FileLocalPath_;

    int64_t LastModified_;

    int64_t FileMetaRequestId_;

    int64_t fileDirectLinkRequestId_;

    QString Filename_;

    int64_t FileSizeBytes_;

    bool IsSelected_;

    IFileSharingBlockLayout *Layout_;

    QString Link_;

    int64_t PreviewMetaRequestId_;

    QString SaveAs_;

    std::unique_ptr<core::file_sharing_content_type> Type_;

    QString directUri_;

    bool inited_;

    bool loadedFromLocal_;

    QString uploadId_;

protected:
    bool savedByUser_;
    bool recognize_;
    bool gotAudio_;
    int32_t duration_;

private Q_SLOTS:
    void onFileDownloaded(qint64 seq, const Data::FileSharingDownloadResult& _result);

    void onFileDownloading(qint64 seq, QString rawUri, qint64 bytesTransferred, qint64 bytesTotal);

    void onFileMetainfoDownloaded(qint64 seq, const Data::FileSharingMeta& _meta);

    void onFileSharingError(qint64 seq, QString rawUri, qint32 errorCode);

    void onPreviewMetainfoDownloadedSlot(qint64 seq, QString miniPreviewUri, QString fullPreviewUri);

    void fileSharingUploadingProgress(const QString& _uploadingId, qint64 _bytesTransferred);

    void fileSharingUploadingResult(const QString& _seq, bool _success, const QString& localPath, const QString& _uri, int _contentType, qint64 _size, qint64 _lastModified, bool _isFileTooBig);
};

UI_COMPLEX_MESSAGE_NS_END
