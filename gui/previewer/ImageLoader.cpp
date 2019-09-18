#include "stdafx.h"

#include "../core_dispatcher.h"
#include "../gui_settings.h"
#include "../utils/utils.h"
#include "../utils/LoadPixmapFromFileTask.h"
#include "../utils/LoadFirstFrameTask.h"
#include "../utils/gui_coll_helper.h"

#include "ImageLoader.h"

Previewer::ImageLoader::ImageLoader(const QString& _aimId, const Data::Image& _image)
    : aimId_(_aimId)
    , image_(_image)
    , seq_(0)
    , state_(State::ReadyToLoad)
{
}

Previewer::ImageLoader::ImageLoader(const QString& _aimId, const Data::Image& _image, const QString& _localPath)
    : aimId_(_aimId)
    , image_(_image)
    , seq_(0)
    , state_(State::Success)
    , localFileName_(_localPath)
{
    QFileInfo fileInfo(_localPath);
    if (!fileInfo.exists())
    {
        load();
    }
    else
    {
        if (!_image.preview_.isNull() && _image.originalImageSize_.isValid() &&
            (_image.originalImageSize_.width() > _image.preview_.width() || _image.originalImageSize_.height() > _image.preview_.height()))
        {
            QSize imageSize = _image.originalImageSize_;

            const QSize maxImageSize = Utils::getMaxImageSize();

            if (imageSize.width() > maxImageSize.width() || imageSize.height() > maxImageSize.height())
                imageSize.scale(maxImageSize, Qt::KeepAspectRatio);

            pixmap_ = _image.preview_.scaled(imageSize, Qt::KeepAspectRatio);
        }
        else
        {
            pixmap_ = _image.preview_;
        }

        const auto ext = fileInfo.suffix();
        if (!Utils::is_video_extension(ext))
        {
            auto task = new Utils::LoadPixmapFromFileTask(_localPath, Utils::getMaxImageSize());

            QObject::connect(task, &Utils::LoadPixmapFromFileTask::loadedSignal, this, &ImageLoader::localPreviewLoaded);

            QThreadPool::globalInstance()->start(task);
        }
        else
        {
            auto task = new Utils::LoadFirstFrameTask(_localPath);
            QObject::connect(task, &Utils::LoadFirstFrameTask::loadedSignal, this, &ImageLoader::localPreviewLoaded);
            QThreadPool::globalInstance()->start(task);
        }
    }
}

Previewer::ImageLoader::~ImageLoader()
{

}

void Previewer::ImageLoader::localPreviewLoaded(QPixmap _pixmap, const QSize _originalSize)
{
    pixmap_ = std::move(_pixmap);

    emit imageLoaded();
}

void Previewer::ImageLoader::load()
{
    assert(getState() != State::Loading);

    if (image_.is_filesharing_)
    {
//        connect(Ui::GetDispatcher(), &Ui::core_dispatcher::fileSharingFileDownloaded,
//            this, &ImageLoader::onSharedFileDownloaded);

        connect(Ui::GetDispatcher(), &Ui::core_dispatcher::fileSharingError,
            this, &ImageLoader::onSharedFileDownloadError);
    }
    else
    {
        connect(Ui::GetDispatcher(), &Ui::core_dispatcher::imageDownloaded,
            this, &ImageLoader::onImageDownloaded);

        connect(Ui::GetDispatcher(), &Ui::core_dispatcher::imageMetaDownloaded,
            this, &ImageLoader::onImageMetaDownloaded);

        connect(Ui::GetDispatcher(), &Ui::core_dispatcher::imageDownloadError,
            this, &ImageLoader::onImageDownloadError);
    }

    start();
}

void Previewer::ImageLoader::cancelLoading() const
{
    if (getState() != State::Loading)
        return;

    Ui::gui_coll_helper helper(Ui::GetDispatcher()->create_collection(), true);
    helper.set_value_as_qstring("contact", aimId_);
    helper.set_value_as_qstring("url", image_.url_);
    helper.set_value_as_int64("process_seq", seq_);
    Ui::GetDispatcher()->post_message_to_core("files/download/abort", helper.get());
}

Previewer::ImageLoader::State Previewer::ImageLoader::getState() const
{
    return state_;
}

QString Previewer::ImageLoader::getFileName()
{
    if (image_.is_filesharing_)
    {
        QEventLoop wait;
        connect(this, &Previewer::ImageLoader::metainfoLoaded, &wait, &QEventLoop::quit);

        const auto seq = Ui::GetDispatcher()->downloadFileSharingMetainfo(image_.url_);

        QString fileName;
        connect(Ui::GetDispatcher(), &Ui::core_dispatcher::fileSharingFileMetainfoDownloaded, this,
            [this, seq, &fileName](qint64 s, const Data::FileSharingMeta& _meta)
            {
                if (seq == s)
                {
                    fileName = _meta.filenameShort_;
                    emit metainfoLoaded();
                }
            });

        wait.exec();

        return fileName;
    }

    QUrl urlParser(image_.url_);
    return urlParser.fileName();
}

const QPixmap& Previewer::ImageLoader::getPixmap() const
{
    return pixmap_;
}

const QSize& Previewer::ImageLoader::getOriginalImageSize() const
{
    return image_.originalImageSize_;
}

int64_t Previewer::ImageLoader::getMsgId() const
{
    return image_.msgId_;
}

QString Previewer::ImageLoader::getUrl() const
{
    return image_.url_;
}

const QString& Previewer::ImageLoader::getLocalFileName() const
{
    return localFileName_;
}

void Previewer::ImageLoader::start()
{
    setState(State::Loading);

    if (image_.is_filesharing_)
    {
        seq_ = Ui::GetDispatcher()->downloadSharedFile(aimId_, image_.url_, false, QString(), true);
    }
    else
    {
        seq_ = Ui::GetDispatcher()->downloadImage(image_.url_, QString(), true, 0, 0, true);
    }
}

void Previewer::ImageLoader::setState(State _newState)
{
    state_ = _newState;
}

void Previewer::ImageLoader::onCancelLoading()
{
    cancelLoading();
}

void Previewer::ImageLoader::onTryAgain()
{
    start();
}

//void Previewer::ImageLoader::onSharedFileDownloaded(qint64 _seq, QString /*_url*/, QString _local)
//{
//    if (seq_ == _seq)
//    {
//        setState(State::Success);
//        localFileName_ = _local;
//        emit imageLoaded();
//    }
//}

void Previewer::ImageLoader::onSharedFileDownloadError(qint64 _seq, QString /*_url*/, qint32 /*_errorCode*/)
{
    if (seq_ == _seq)
    {
        setState(State::Error);
        emit imageLoadingError();
    }
}

void Previewer::ImageLoader::onImageMetaDownloaded(qint64 _seq, const Data::LinkMetadata &_meta)
{
    if (seq_ == _seq)
    {
        const auto needContent = _meta.getContentType() == ql1s("image");
        seq_ = Ui::GetDispatcher()->downloadImage(_meta.getDownloadUri(), QString(), false, 0, 0, true, false, needContent);
    }
}

void Previewer::ImageLoader::onImageDownloaded(qint64 _seq, QString _url, QPixmap _image, QString _local)
{
    if (seq_ == _seq)
    {
        setState(State::Success);
        pixmap_ = std::move(_image);
        localFileName_ = _local;
        emit imageLoaded();
    }
}

void Previewer::ImageLoader::onImageDownloadError(qint64 _seq, QString /*_url*/)
{
    if (seq_ == _seq)
    {
        setState(State::Error);
        emit imageLoadingError();
    }
}
