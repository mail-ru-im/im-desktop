#pragma once

#include "types/message.h"
#include "types/link_metadata.h"
#include "types/filesharing_meta.h"
#include "types/filesharing_download_result.h"

namespace Utils
{

class MediaLoader : public QObject
{
    Q_OBJECT
public:

    enum class LoadMeta
    {
        No,
        BeforePreview,
        OnlyForVideo,
        BeforeMedia
    };

    explicit MediaLoader(const QString& _link, LoadMeta _loadMeta = LoadMeta::BeforeMedia)
        : link_(_link)
        , seq_(0)
        , duration_(0)
        , loadMeta_(_loadMeta)
        , gotAudio_(false)
        , isVideo_(false)
        , isGif_(false)
    {}
    virtual ~MediaLoader() {}
    virtual void load(const QString& _downloadPath = QString()) = 0;
    virtual void loadPreview() = 0;

    virtual void cancel() = 0;
    virtual void cancelPreview() = 0;

    virtual bool isLoading() const { return seq_ != 0; }

    virtual bool isVideo() const { return isVideo_; }
    virtual bool isGif() const { return isGif_; }
    virtual const QSize& originalSize() const { return originSize_; }
    virtual int64_t duration() const{ return duration_; }
    virtual bool gotAudio() const { return gotAudio_; }
    virtual const QString& fileName() const { return fileName_; }
    virtual const QString& downloadUri() const { return downloadPath_; }
    virtual const QString& fileFormat() const { return fileFormat_; }

Q_SIGNALS:
    void previewLoaded(const QPixmap& _preview, const QSize& _originSize);
    void fileLoaded(const QString& _path);
    void progress(int64_t _bytesTotal, int64_t _bytesTransferred);
    void error();

protected:
    QString link_;
    QSize originSize_;
    QString downloadPath_;
    QString fileName_;
    QString fileFormat_;
    int64_t seq_;
    int32_t duration_;
    LoadMeta loadMeta_;
    bool gotAudio_;
    bool isVideo_;
    bool isGif_;
};

//////////////////////////////////////////////////////////////////////////

class FileSharingLoader : public MediaLoader
{
    Q_OBJECT
public:
    FileSharingLoader(const QString& _link, LoadMeta _loadMeta = LoadMeta::BeforeMedia);
    void load(const QString& _downloadPath = QString()) override;
    void loadPreview() override;

    void cancel() override;
    void cancelPreview() override;

private Q_SLOTS:
    void onPreviewMetainfo(qint64 _seq, const QString &_miniPreviewUri, const QString &_fullPreviewUri);
    void onFileMetainfo(qint64 _seq, const Data::FileSharingMeta& _meta);
    void onPreviewDownloaded(qint64 _seq, QString _url, QPixmap _image, QString _local);
    void onFileDownloaded(int64_t _seq, const Data::FileSharingDownloadResult& _result);
    void onFileError(qint64 _seq);

private:
    void connectSignals();
    void disconnectSignals();

private:
    QString previewLink_;
    QMetaObject::Connection fileDownloadedConnection_;
    QMetaObject::Connection fileMetaDownloadedConnection_;
    QMetaObject::Connection previewMetaDownloadedConnection_;
    QMetaObject::Connection imageDownloadedConnection_;
    QMetaObject::Connection fileErrorConnection_;
};

//////////////////////////////////////////////////////////////////////////

class CommonMediaLoader : public MediaLoader
{
    Q_OBJECT
public:
    CommonMediaLoader(const QString& _link, LoadMeta _loadMeta = LoadMeta::BeforeMedia);
    void load(const QString& _downloadPath = QString()) override;
    void loadPreview() override;

    void cancel() override;
    void cancelPreview() override;

    const QString& downloadUri() const override { return downloadLink_; }

private Q_SLOTS:
    void onImageDownloaded(qint64 _seq, const QString& _url, const QPixmap& _image, const QString& _path, bool _is_preview);
    void onImageMetaDownloaded(qint64 _seq, const Data::LinkMetadata &_meta);
    void onImageError(qint64 _seq);
    void onProgress(qint64 _seq, int64_t _bytesTotal, int64_t _bytesTransferred, int32_t _pctTransferred);

private:
    void connectSignals();
    void disconnectSignals();

    QString downloadLink_;
    QMetaObject::Connection imageDownloadedConneection_;
    QMetaObject::Connection metaDownloadedConneection_;
    QMetaObject::Connection imageErrorConneection_;
    QMetaObject::Connection progressConneection_;
};


class FileSaver : public QObject
{
    Q_OBJECT
public:

    using Callback = std::function<void(bool, const QString&)>;

    FileSaver(QObject* _parent = nullptr) : QObject(_parent) {}

    void save(Callback _callback, const QString& _link, const QString& _path, bool _exportAsPng = false);

private Q_SLOTS:
    void onSaved(const QString& _path);
    void onError();

private:
    std::unique_ptr<MediaLoader> loader_;
    Callback callback_;
    bool exportAsPng_ = false;
};


}
