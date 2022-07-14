#include "stdafx.h"

#include "core_dispatcher.h"
#include "../main_window/history_control/complex_message/FileSharingUtils.h"
#include "../main_window/containers/FileSharingMetaContainer.h"
#include "utils/UrlParser.h"
#include "utils/async/AsyncTask.h"
#include "medialoader.h"

using namespace Utils;

//////////////////////////////////////////////////////////////////////////

FileSharingLoader::FileSharingLoader(const QString& _link, LoadMeta _loadMeta)
    : MediaLoader(_link, _loadMeta)
    , fileSharingId_{ Ui::ComplexMessage::extractIdFromFileSharingUri(_link) }
{
    auto content_type = Ui::ComplexMessage::extractContentTypeFromFileSharingId(fileSharingId_.fileId_);

    isVideo_ = content_type.is_video();
    isGif_ = content_type.is_gif();
    originSize_ = Ui::ComplexMessage::extractSizeFromFileSharingId(fileSharingId_.fileId_);
}

void FileSharingLoader::load(const QString& _downloadPath)
{
    connectSignals();

    downloadPath_ = _downloadPath;

    if (loadMeta_ == LoadMeta::BeforeMedia)
        Logic::GetFileSharingMetaContainer()->requestFileSharingMetaInfo(fileSharingId_, this);
    else
        seq_ = Ui::GetDispatcher()->downloadSharedFile({} , link_, false, _downloadPath, true);
}

void FileSharingLoader::loadPreview()
{
    connectSignals();

    auto loadMeta = (loadMeta_ == LoadMeta::BeforePreview) || (loadMeta_ == LoadMeta::OnlyForVideo && isVideo_);

    if (loadMeta)
        Logic::GetFileSharingMetaContainer()->requestFileSharingMetaInfo(fileSharingId_, this);
    else
        seq_ = Ui::GetDispatcher()->getFileSharingPreviewSize(link_);
}

void FileSharingLoader::cancel()
{
    Ui::GetDispatcher()->abortSharedFileDownloading(link_, seq_);
    seq_ = 0;

    disconnectSignals();
}

void FileSharingLoader::cancelPreview()
{
    Ui::GetDispatcher()->cancelLoaderTask(previewLink_, seq_);
    seq_ = 0;

    disconnectSignals();
}

void FileSharingLoader::processMetaInfo(const Utils::FileSharingId& _fsId, const Data::FileSharingMeta& _meta)
{
    im_assert(_fsId == fileSharingId_);
    if (_fsId != fileSharingId_)
        return;

    duration_ = _meta.duration_;
    gotAudio_ = _meta.gotAudio_;
    fileName_ = _meta.filenameShort_;
    mediaVirusInfected_ = core::antivirus::check::result::unsafe == _meta.antivirusCheck_.result_;

    switch (loadMeta_)
    {
        case LoadMeta::BeforePreview:
        case LoadMeta::OnlyForVideo:
            seq_ = Ui::GetDispatcher()->getFileSharingPreviewSize(link_);
            break;
        case LoadMeta::BeforeMedia:
            seq_ = Ui::GetDispatcher()->downloadSharedFile({}, link_, false, downloadPath_, true);
            break;
        default:
            break;
    }
}

void Utils::FileSharingLoader::processMetaInfoError(const Utils::FileSharingId& _fsId, qint32 /*_errorCode*/)
{
    Q_EMIT error();
}

void FileSharingLoader::onPreviewMetainfo(qint64 _seq, const QString& _miniPreviewUri, const QString& _fullPreviewUri) {
    if (seq_ != _seq)
        return;

    previewLink_ = (_fullPreviewUri.isEmpty() ? _miniPreviewUri : _fullPreviewUri);
    seq_ = Ui::GetDispatcher()->downloadImage(previewLink_, QString(), false, 0, 0, false);
}

void FileSharingLoader::onPreviewDownloaded(qint64 _seq, QString _url, QPixmap _image, QString _local)
{
    if (seq_ != _seq)
        return;

    disconnectSignals();

    seq_ = 0;
    Q_EMIT previewLoaded(_image, originSize_);

}

void FileSharingLoader::onFileDownloaded(int64_t _seq, const Data::FileSharingDownloadResult& _result)
{
    if (seq_ != _seq)
        return;

    seq_ = 0;
    Q_EMIT fileLoaded(_result.filename_);

    disconnectSignals();
}

void FileSharingLoader::onFileError(qint64 _seq)
{
    if (seq_ == _seq)
        Q_EMIT error();
}

void FileSharingLoader::connectSignals()
{
    connect(Ui::GetDispatcher(), &Ui::core_dispatcher::fileSharingFileDownloaded, this, &FileSharingLoader::onFileDownloaded, Qt::UniqueConnection);
    connect(Ui::GetDispatcher(), &Ui::core_dispatcher::fileSharingPreviewMetainfoDownloaded, this, &FileSharingLoader::onPreviewMetainfo, Qt::UniqueConnection);
    connect(Ui::GetDispatcher(), &Ui::core_dispatcher::imageDownloaded, this, &FileSharingLoader::onPreviewDownloaded, Qt::UniqueConnection);
    connect(Ui::GetDispatcher(), &Ui::core_dispatcher::fileSharingError, this, &FileSharingLoader::onFileError, Qt::UniqueConnection);
}

void FileSharingLoader::disconnectSignals()
{
    Ui::GetDispatcher()->disconnect(this);
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
    Ui::GetDispatcher()->cancelLoaderTask(downloadLink_.isEmpty() ? link_ : downloadLink_, seq_);
    seq_ = 0;

    disconnectSignals();
}

void CommonMediaLoader::cancelPreview()
{
    Ui::GetDispatcher()->cancelLoaderTask(link_, seq_);
    seq_ = 0;

    disconnectSignals();
}

void CommonMediaLoader::onImageDownloaded(qint64 _seq, const QString& _url, const QPixmap& _image, const QString& _path, bool _is_preview)
{
    if (seq_ != _seq)
        return;

    seq_ = 0;

    disconnectSignals();

    if (_is_preview)
        Q_EMIT previewLoaded(_image, originSize_);
    else
        Q_EMIT fileLoaded(_path);

}

void CommonMediaLoader::onImageMetaDownloaded(qint64 _seq, const Data::LinkMetadata &_meta)
{
    if (seq_ != _seq)
        return;

    isVideo_ = _meta.getContentTypeStr() == u"video";
    isGif_ = _meta.getContentTypeStr() == u"gif";
    originSize_ = _meta.getOriginSize();
    downloadLink_ = _meta.getDownloadUri();
    fileFormat_ = _meta.getFileFormat();
}

void CommonMediaLoader::onImageError(qint64 _seq)
{
    if (seq_ == _seq)
    {
        disconnectSignals();
        Q_EMIT error();
    }
}

void CommonMediaLoader::onProgress(qint64 _seq, int64_t _bytesTotal, int64_t _bytesTransferred, int32_t _pctTransferred)
{
    Q_UNUSED(_pctTransferred)

    if (seq_ == _seq)
        Q_EMIT progress(_bytesTotal, _bytesTransferred);
}

void CommonMediaLoader::connectSignals()
{
    connect(Ui::GetDispatcher(), &Ui::core_dispatcher::imageDownloaded, this, &CommonMediaLoader::onImageDownloaded, Qt::UniqueConnection);
    connect(Ui::GetDispatcher(), &Ui::core_dispatcher::imageMetaDownloaded, this, &CommonMediaLoader::onImageMetaDownloaded, Qt::UniqueConnection);
    connect(Ui::GetDispatcher(), &Ui::core_dispatcher::imageDownloadError, this, &CommonMediaLoader::onImageError, Qt::UniqueConnection);
    connect(Ui::GetDispatcher(), &Ui::core_dispatcher::imageDownloadingProgress, this, &CommonMediaLoader::onProgress, Qt::UniqueConnection);
}

void CommonMediaLoader::disconnectSignals()
{
    Ui::GetDispatcher()->disconnect(this);
}

void FileSaver::save(FileSaver::Callback _callback, const QString& _link, const QString& _path, bool _exportAsPng)
{
    callback_ = std::move(_callback);
    exportAsPng_ = _exportAsPng;

    Utils::UrlParser parser;
    parser.process(_link);

    if (parser.hasUrl() && parser.isFileSharing())
        loader_ = std::make_unique<Utils::FileSharingLoader>(_link);
    else
        loader_= std::make_unique<Utils::CommonMediaLoader>(_link);

    connect(loader_.get(), &MediaLoader::fileLoaded, this, &FileSaver::onSaved);
    connect(loader_.get(), &MediaLoader::error, this, &FileSaver::onError);

    loader_->load(_path);
}

void FileSaver::onSaved(const QString& _path)
{
    if (!exportAsPng_)
    {
        callback_(true, _path);
        deleteLater();
    }
    else
    {
        Async::runAsync([pThis = QPointer(this), _path]()
        {
            if (!pThis)
                return;
            QImage image;
            QSize size;
            Utils::loadImageScaled(_path, {}, image, size);
            const bool res = image.save(_path, "png");
            Async::runInMain([pThis, _path, res]()
            {
                if (!pThis)
                    return;
                if (res)
                {
                    pThis->callback_(true, _path);
                    pThis->deleteLater();
                }
                else
                {
                    pThis->onError();
                }
            });
        });
    }
}

void FileSaver::onError()
{
    callback_(false, QString());
    deleteLater();
}
