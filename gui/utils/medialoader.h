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

    MediaLoader(const QString& _link, LoadMeta _loadMeta = LoadMeta::BeforeMedia) : link_(_link), loadMeta_(_loadMeta) {}

    virtual void load(const QString& _downloadPath = QString()) = 0;
    virtual void loadPreview() = 0;

    virtual void cancel() = 0;
    virtual void cancelPreview() = 0;

    virtual bool isLoading() { return seq_ != 0; }

    virtual bool isVideo() { return isVideo_; }
    virtual bool isGif() { return isGif_; }
    virtual QSize originalSize() { return originSize_; }
    virtual int64_t duration() { return duration_; }
    virtual bool gotAudio() { return gotAudio_; }
    virtual QString fileName() { return fileName_; }

signals:
    void previewLoaded(const QPixmap& _preview, const QSize& _originSize);
    void fileLoaded(const QString& _path);
    void error();

protected:
    int64_t seq_ = 0;
    QString link_;
    bool isVideo_;
    bool isGif_;
    QSize originSize_;
    int32_t duration_;
    bool gotAudio_;
    LoadMeta loadMeta_;
    QString downloadPath_;
    QString fileName_;
};

//////////////////////////////////////////////////////////////////////////

class FileSharingLoader : public MediaLoader
{
    Q_OBJECT
public:
    FileSharingLoader(const QString& _link, LoadMeta _loadMeta = LoadMeta::BeforeMedia);
    void load(const QString& _downloadPath = QString());
    void loadPreview();

    void cancel();
    void cancelPreview();

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
    QMetaObject::Connection fileDownloadedConneection_;
    QMetaObject::Connection fileMetaDownloadedConneection_;
    QMetaObject::Connection previewMetaDownloadedConneection_;
    QMetaObject::Connection imageDownloadedConneection_;
    QMetaObject::Connection fileErrorConnection_;
};

//////////////////////////////////////////////////////////////////////////

class CommonMediaLoader : public MediaLoader
{
    Q_OBJECT
public:
    CommonMediaLoader(const QString& _link, LoadMeta _loadMeta = LoadMeta::BeforeMedia);
    void load(const QString& _downloadPath = QString());
    void loadPreview();

    void cancel();
    void cancelPreview();

private Q_SLOTS:
    void onImageDownloaded(qint64 _seq, const QString& _url, const QPixmap& _image, const QString& _path);
    void onImageMetaDownloaded(qint64 _seq, const Data::LinkMetadata &_meta);
    void onImageError(qint64 _seq);

private:
    void connectSignals();
    void disconnectSignals();

    QString downloadLink_;
    QMetaObject::Connection imageDownloadedConneection_;
    QMetaObject::Connection metaDownloadedConneection_;
    QMetaObject::Connection imageErrorConneection_;
};


class FileSaver : public QObject
{
    Q_OBJECT
public:

    using Callback = std::function<void(bool)>;

    FileSaver(QObject* _parent = nullptr) : QObject(_parent) {}

    void save(Callback _callback, const QString& _link, const QString& _path);

private Q_SLOTS:
    void onSaved(const QString& _path);
    void onError();

private:
    std::unique_ptr<MediaLoader> loader_;
    Callback callback_;
};


}
