#include "stdafx.h"

#include "core_dispatcher.h"
#include "../main_window/history_control/complex_message/FileSharingUtils.h"
#include "utils/UrlParser.h"
#include "medialoader.h"

using namespace Utils;

//////////////////////////////////////////////////////////////////////////

FileSharingLoader::FileSharingLoader(const QString &_link, LoadMeta _loadMeta)
    : MediaLoader(_link, _loadMeta)
{
    const QString id = Ui::ComplexMessage::extractIdFromFileSharingUri(_link);
    auto content_type = Ui::ComplexMessage::extractContentTypeFromFileSharingId(id);

    isVideo_ = content_type.is_video();
    isGif_ = content_type.is_gif();
    originSize_ = Ui::ComplexMessage::extractSizeFromFileSharingId(id);
}

void FileSharingLoader::load(const QString& _downloadPath)
{
    connectSignals();

    downloadPath_ = _downloadPath;

    if (loadMeta_ == LoadMeta::BeforeMedia)
        seq_ = Ui::GetDispatcher()->downloadFileSharingMetainfo(link_);
    else
        seq_ = Ui::GetDispatcher()->downloadSharedFile(QString(), link_, false, _downloadPath, true);
}

void FileSharingLoader::loadPreview()
{
    connectSignals();

    auto loadMeta = (loadMeta_ == LoadMeta::BeforePreview) || (loadMeta_ == LoadMeta::OnlyForVideo && isVideo_);

    if (loadMeta)
        seq_ = Ui::GetDispatcher()->downloadFileSharingMetainfo(link_);
    else
        seq_ = Ui::GetDispatcher()->getFileSharingPreviewSize(link_);
}

void FileSharingLoader::cancel()
{
    Ui::GetDispatcher()->abortSharedFileDownloading(link_);
    seq_ = 0;

    disconnectSignals();
}

void FileSharingLoader::cancelPreview()
{
    Ui::GetDispatcher()->cancelLoaderTask(previewLink_);
    seq_ = 0;

    disconnectSignals();
}

void FileSharingLoader::onPreviewMetainfo(qint64 _seq, const QString &_miniPreviewUri, const QString &_fullPreviewUri) {
    if (seq_ != _seq)
        return;

    previewLink_ = (_fullPreviewUri.isEmpty() ? _miniPreviewUri : _fullPreviewUri);
    seq_ = Ui::GetDispatcher()->downloadImage(previewLink_, QString(), false, 0, 0, false);
}

void FileSharingLoader::onFileMetainfo(qint64 _seq, const Data::FileSharingMeta& _meta)
{
    if (seq_ != _seq)
        return;

    duration_ = _meta.duration_;
    gotAudio_ = _meta.gotAudio_;
    fileName_ = _meta.filenameShort_;

    switch (loadMeta_)
    {
        case LoadMeta::BeforePreview:
        case LoadMeta::OnlyForVideo:
            seq_ = Ui::GetDispatcher()->getFileSharingPreviewSize(link_);
            break;
        case LoadMeta::BeforeMedia:
            seq_ = Ui::GetDispatcher()->downloadSharedFile(QString(), link_, false, downloadPath_, true);
            break;
        default:
            break;
    }
}

void FileSharingLoader::onPreviewDownloaded(qint64 _seq, QString _url, QPixmap _image, QString _local)
{
    if (seq_ != _seq)
        return;

    seq_ = 0;
    emit previewLoaded(_image, originSize_);

    disconnectSignals();
}

void FileSharingLoader::onFileDownloaded(int64_t _seq, const Data::FileSharingDownloadResult& _result)
{
    if (seq_ != _seq)
        return;

    seq_ = 0;
    emit fileLoaded(_result.filename_);

    disconnectSignals();
}

void FileSharingLoader::onFileError(qint64 _seq)
{
    if (seq_ == _seq)
        emit error();
}

void FileSharingLoader::connectSignals()
{
    fileDownloadedConneection_ = connect(Ui::GetDispatcher(), &Ui::core_dispatcher::fileSharingFileDownloaded,
                                         this, &FileSharingLoader::onFileDownloaded);

    previewMetaDownloadedConneection_ = connect(Ui::GetDispatcher(), &Ui::core_dispatcher::fileSharingPreviewMetainfoDownloaded,
                                                this, &FileSharingLoader::onPreviewMetainfo);

    fileMetaDownloadedConneection_ = connect(Ui::GetDispatcher(), &Ui::core_dispatcher::fileSharingFileMetainfoDownloaded,
                                             this, &FileSharingLoader::onFileMetainfo);

    imageDownloadedConneection_ = connect(Ui::GetDispatcher(), &Ui::core_dispatcher::imageDownloaded,
                                          this, &FileSharingLoader::onPreviewDownloaded);

    fileErrorConnection_ = connect(Ui::GetDispatcher(), &Ui::core_dispatcher::fileSharingError,
                                   this, &FileSharingLoader::onFileError);
}

void FileSharingLoader::disconnectSignals()
{
    disconnect(fileDownloadedConneection_);
    disconnect(previewMetaDownloadedConneection_);
    disconnect(fileMetaDownloadedConneection_);
    disconnect(imageDownloadedConneection_);
    disconnect(fileErrorConnection_);
}

//////////////////////////////////////////////////////////////////////////

CommonMediaLoader::CommonMediaLoader(const QString &_link, LoadMeta _loadMeta)
    : MediaLoader(_link, _loadMeta)
{
}

void CommonMediaLoader::load(const QString& _downloadPath)
{
    connectSignals();

    seq_ = Ui::GetDispatcher()->downloadImage(downloadLink_.isEmpty() ? link_ : downloadLink_, _downloadPath, false, 0, 0, true, false, false);
}

void CommonMediaLoader::loadPreview()
{
    connectSignals();

    seq_ = Ui::GetDispatcher()->downloadImage(link_, QString(), true, 0, 0, true);
}

void CommonMediaLoader::cancel()
{
    Ui::GetDispatcher()->cancelLoaderTask(downloadLink_);
    seq_ = 0;

    disconnectSignals();
}

void CommonMediaLoader::cancelPreview()
{
    Ui::GetDispatcher()->cancelLoaderTask(link_);
    seq_ = 0;

    disconnectSignals();
}

void CommonMediaLoader::onImageDownloaded(qint64 _seq, const QString& _url, const QPixmap& _image, const QString& _path)
{
    if (seq_ != _seq)
        return;

    seq_ = 0;

    if (!_image.isNull())
        emit previewLoaded(_image, originSize_);
    else
        emit fileLoaded(_path);

    disconnectSignals();
}

void CommonMediaLoader::onImageMetaDownloaded(qint64 _seq, const Data::LinkMetadata &_meta)
{
    if (seq_ != _seq)
        return;

    isVideo_ = _meta.getContentType() == ql1s("video");
    isGif_ = _meta.getContentType() == ql1s("gif");
    originSize_ = _meta.getOriginSize();
    downloadLink_ = _meta.getDownloadUri();
}

void CommonMediaLoader::onImageError(qint64 _seq)
{
    if (seq_ == _seq)
    {
        emit error();
        disconnectSignals();
    }
}

void CommonMediaLoader::connectSignals()
{
    imageDownloadedConneection_ = connect(Ui::GetDispatcher(), &Ui::core_dispatcher::imageDownloaded,
                                          this, &CommonMediaLoader::onImageDownloaded);

    metaDownloadedConneection_ = connect(Ui::GetDispatcher(), &Ui::core_dispatcher::imageMetaDownloaded,
                                         this, &CommonMediaLoader::onImageMetaDownloaded);

    imageErrorConneection_ = connect(Ui::GetDispatcher(), &Ui::core_dispatcher::imageDownloadError,
                                     this, &CommonMediaLoader::onImageError);
}

void CommonMediaLoader::disconnectSignals()
{
    disconnect(imageDownloadedConneection_);
    disconnect(metaDownloadedConneection_);
    disconnect(imageErrorConneection_);
}

void FileSaver::save(FileSaver::Callback _callback, const QString& _link, const QString& _path)
{
    callback_ = std::move(_callback);

    Utils::UrlParser parser;
    parser.process(QStringRef(&_link));

    if (parser.hasUrl() && parser.getUrl().is_filesharing())
        loader_ = std::make_unique<Utils::FileSharingLoader>(_link);
    else
        loader_= std::make_unique<Utils::CommonMediaLoader>(_link);

    connect(loader_.get(), &MediaLoader::fileLoaded, this, &FileSaver::onSaved);
    connect(loader_.get(), &MediaLoader::error, this, &FileSaver::onError);

    loader_->load(_path);
}

void FileSaver::onSaved(const QString& _path)
{
    Q_UNUSED(_path)
    callback_(true);

    deleteLater();
}

void FileSaver::onError()
{
    callback_(false);

    deleteLater();
}
