#include "stdafx.h"

#include "../../../../corelib/enumerations.h"
#include "../../../../common.shared/loader_errors.h"

#include "../../../core_dispatcher.h"
#include "../../../gui_settings.h"
#include "../../../utils/log/log.h"
#include "../../../utils/utils.h"
#include "previewer/toast.h"
#include "utils/medialoader.h"

#include "../FileSizeFormatter.h"
#include "../MessageStyle.h"


#include "../../mediatype.h"

#include "IFileSharingBlockLayout.h"
#include "FileSharingUtils.h"

#include "FileSharingBlockBase.h"
#include "ComplexMessageItem.h"
#include "IItemBlock.h"

namespace
{
    constexpr std::chrono::milliseconds getDataTransferTimeout() noexcept
    {
        return std::chrono::milliseconds(300);
    }
}

UI_COMPLEX_MESSAGE_NS_BEGIN

FileSharingBlockBase::FileSharingBlockBase(
    ComplexMessageItem *_parent,
    const QString& _link,
    const core::file_sharing_content_type& _type)
    : GenericBlock(
        _parent,
        _link,
        MenuFlags::MenuFlagFileCopyable,
        true)
    , BytesTransferred_(-1)
    , CopyFile_(false)
    , DownloadRequestId_(-1)
    , LastModified_(-1)
    , FileMetaRequestId_(-1)
    , fileDirectLinkRequestId_(-1)
    , FileSizeBytes_(-1)
    , IsSelected_(false)
    , Layout_(nullptr)
    , Link_(Utils::normalizeLink(_link).toString())
    , PreviewMetaRequestId_(-1)
    , Type_(std::make_unique<core::file_sharing_content_type>(_type))
    , inited_(false)
    , loadedFromLocal_(false)
    , dataTransferTimeout_(nullptr)
    , savedByUser_(false)
    , recognize_(false)
    , gotAudio_(false)
    , duration_(0)
{
    Testing::setAccessibleName(this, u"AS HistoryPage messageFileSharing " % QString::number(_parent->getId()));
}

FileSharingBlockBase::~FileSharingBlockBase() = default;

void FileSharingBlockBase::clearSelection()
{
    if (!IsSelected_)
        return;

    setSelected(false);
}

QString FileSharingBlockBase::formatRecentsText() const
{
    using namespace core;

    const auto& type = getType();
    if (type.is_sticker())
        return QT_TRANSLATE_NOOP("contact_list", "Sticker");

    switch (type.type_)
    {
        case file_sharing_base_content_type::gif:
            return QT_TRANSLATE_NOOP("contact_list", "GIF");

        case file_sharing_base_content_type::image:
            return QT_TRANSLATE_NOOP("contact_list", "Photo");

        case file_sharing_base_content_type::video:
            return QT_TRANSLATE_NOOP("contact_list", "Video");

        case file_sharing_base_content_type::ptt:
            return QT_TRANSLATE_NOOP("contact_list", "Voice message");

        default:
            ;
    }

    return QT_TRANSLATE_NOOP("contact_list", "File");
}

Ui::MediaType FileSharingBlockBase::getMediaType() const
{
    using namespace core;

    const auto& type = getType();
    if (type.is_sticker())
        return Ui::MediaType::mediaTypeSticker;

    switch (type.type_)
    {
    case file_sharing_base_content_type::gif:
        return Ui::MediaType::mediaTypeFsGif;

    case file_sharing_base_content_type::image:
        return Ui::MediaType::mediaTypeFsPhoto;

    case file_sharing_base_content_type::video:
        return Ui::MediaType::mediaTypeFsVideo;

    case file_sharing_base_content_type::ptt:
        return Ui::MediaType::mediaTypePtt;

    default:
        ;
    }

    return Ui::MediaType::mediaTypeFileSharing;
}

IItemBlockLayout* FileSharingBlockBase::getBlockLayout() const
{
    assert(Layout_);
    return Layout_;
}

QString FileSharingBlockBase::getProgressText() const
{
    assert(FileSizeBytes_ >= -1);
    assert(BytesTransferred_ >= -1);

    const auto isInvalidProgressData = ((FileSizeBytes_ <= -1) || (BytesTransferred_ < -1));
    if (isInvalidProgressData)
    {
        return QString();
    }

    return ::HistoryControl::formatProgressText(FileSizeBytes_, BytesTransferred_);
}

QString FileSharingBlockBase::getSourceText() const
{
    if (Link_.isEmpty())
        return Link_;

    const auto mediaType = getPlaceholderFormatText();
    const auto id = getFileSharingId();
    return u'[' % mediaType % u": " % id % u']';
}

QString FileSharingBlockBase::getPlaceholderText() const
{
    return u'[' % QT_TRANSLATE_NOOP("placeholders", "Failed to download file or media") % u']';
}

QString FileSharingBlockBase::getSelectedText(const bool, const TextDestination) const
{
    if (IsSelected_)
        return getSourceText();

    return QString();
}

void FileSharingBlockBase::initialize()
{
    connectSignals();

    initializeFileSharingBlock();

    inited_ = true;
}

bool FileSharingBlockBase::isBubbleRequired() const
{
    return isStandalone() && (!isPreviewable() || isSmallPreview());
}

bool FileSharingBlockBase::isMarginRequired() const
{
    return !isStandalone() || !isPreviewable();
}

bool FileSharingBlockBase::isSelected() const
{
    return IsSelected_;
}

const QString& FileSharingBlockBase::getFileLocalPath() const
{
    return FileLocalPath_;
}

int64_t FileSharingBlockBase::getLastModified() const
{
    return LastModified_;
}

const QString& FileSharingBlockBase::getFilename() const
{
    return Filename_;
}

IFileSharingBlockLayout* FileSharingBlockBase::getFileSharingLayout() const
{
    assert(Layout_);
    return Layout_;
}

QString FileSharingBlockBase::getFileSharingId() const
{
    return extractIdFromFileSharingUri(getLink());
}

int64_t FileSharingBlockBase::getFileSize() const
{
    assert(FileSizeBytes_ >= -1);

    return FileSizeBytes_;
}

int64_t FileSharingBlockBase::getBytesTransferred() const
{
    return BytesTransferred_;
}

const QString& FileSharingBlockBase::getLink() const
{
    return Link_;
}

const core::file_sharing_content_type& FileSharingBlockBase::getType() const
{
    return *Type_;
}

const QString& FileSharingBlockBase::getDirectUri() const
{
    return directUri_;
}

bool FileSharingBlockBase::isFileDownloaded() const
{
    return !FileLocalPath_.isEmpty();
}

bool FileSharingBlockBase::isFileDownloading() const
{
    assert(DownloadRequestId_ >= -1);

    return DownloadRequestId_ > 0;
}

bool FileSharingBlockBase::isFileUploading() const
{
    return !uploadId_.isEmpty();
}

bool FileSharingBlockBase::isGifImage() const
{
    return getType().is_gif();
}

bool FileSharingBlockBase::isLottie() const
{
    return getType().is_lottie();
}

bool FileSharingBlockBase::isImage() const
{
    return getType().is_image();
}

bool FileSharingBlockBase::isPtt() const
{
    return  getType().is_ptt();
}

bool FileSharingBlockBase::isVideo() const
{
    return getType().is_video();
}

Data::FilesPlaceholderMap FileSharingBlockBase::getFilePlaceholders()
{
    Data::FilesPlaceholderMap files;
    if (const auto& link = getLink(); !link.isEmpty())
        files.insert({ link, getSourceText() });
    return files;
}

bool FileSharingBlockBase::isPlainFile() const
{
    return !isPreviewable();
}

bool FileSharingBlockBase::isPreviewable() const
{
    return isImage() || isPlayable();
}

bool FileSharingBlockBase::isPlayable() const
{
    return isVideo() || isGifImage() || isLottie();
}

void FileSharingBlockBase::onMenuCopyLink()
{
    QApplication::clipboard()->setText(getLink());
}

void FileSharingBlockBase::onMenuOpenInBrowser()
{
    QDesktopServices::openUrl(getLink());
}

void FileSharingBlockBase::showFileInDir(const Utils::OpenAt _inFolder)
{
    if (const QFileInfo f(getFileLocalPath()); !f.exists() || f.size() != getFileSize() || f.lastModified().toTime_t() != getLastModified())
    {
        if (isPlainFile())
            return startDownloading(true, false, true);
        else
            return onMenuSaveFileAs();
    }

    Utils::openFileOrFolder(getFileLocalPath(), _inFolder);
}

void FileSharingBlockBase::onMenuCopyFile()
{
    if (QFile::exists(getFileLocalPath()))
    {
        Utils::copyFileToClipboard(getFileLocalPath());
        showFileCopiedToast();
        return;
    }

    CopyFile_ = true;

    if (isFileDownloading())
    {
        return;
    }

    QUrl urlParser(getLink());

    auto filename = urlParser.fileName();
    if (filename.isEmpty())
    {
        static const QRegularExpression re(
            qsl("[{}-]"),
            QRegularExpression::UseUnicodePropertiesOption);

        filename = QUuid::createUuid().toString();
        filename.remove(re);
    }

    startDownloading(true);
}

void FileSharingBlockBase::onMenuSaveFileAs()
{
    if (Filename_.isEmpty())
    {
        assert(!"no filename");
        return;
    }

    QUrl urlParser(Filename_);

    Utils::saveAs(urlParser.fileName(), [weak_this = QPointer(this)](const QString& file, const QString& dir_result, bool _exportAsPng)
    {
        if (!weak_this)
            return;

        const auto addTrailingSlash = (!dir_result.endsWith(u'\\') && !dir_result.endsWith(u'/'));
        const auto slash = addTrailingSlash ? QStringView(u"/") : QStringView();
        const QString dir = dir_result % slash % file;

        auto saver = new Utils::FileSaver(weak_this);
        saver->save([dir](bool _success, const QString& /*_savedPath*/)
        {
            if (_success)
                showFileSavedToast(dir);
            else
                showErrorToast();
        }, weak_this->getLink(), dir, _exportAsPng);
    });
}

void FileSharingBlockBase::requestMetainfo(const bool _isPreview)
{
    if (loadedFromLocal_)
        return;

    const auto& link = getLink();
    qint64 requestId;
    if (_isPreview)
    {
        auto origMaxSize = -1;
        if (const auto id = extractIdFromFileSharingUri(link); !id.isEmpty())
        {
            if (const auto originalSize = extractSizeFromFileSharingId(id); !originalSize.isEmpty())
                origMaxSize = Utils::scale_bitmap(std::max(originalSize.width(), originalSize.height()));
        }

        requestId = GetDispatcher()->getFileSharingPreviewSize(link, origMaxSize);
    }
    else
    {
        requestId = GetDispatcher()->downloadFileSharingMetainfo(link);
    }

    assert(requestId > 0);

    if (_isPreview)
    {
        assert(PreviewMetaRequestId_ == -1);
        PreviewMetaRequestId_ = requestId;

        __TRACE(
            "prefetch",
            "initiated file sharing preview metadata downloading\n"
            "    contact=<" << getSenderAimid() << ">\n"
            "    fsid=<" << getFileSharingId() << ">\n"
            "    request_id=<" << requestId << ">");

        return;
    }

    assert(FileMetaRequestId_ == -1);
    FileMetaRequestId_ = requestId;
}

void FileSharingBlockBase::setBlockLayout(IFileSharingBlockLayout* _layout)
{
    assert(_layout);

    if (Layout_)
        delete Layout_;

    Layout_ = _layout;
}

void FileSharingBlockBase::setSelected(const bool _isSelected)
{
    if (IsSelected_ == _isSelected)
        return;

    IsSelected_ = _isSelected;

    update();
}

void FileSharingBlockBase::startDownloading(const bool _sendStats, const bool _forceRequestMetainfo, bool _highPriority)
{
    assert(!isFileDownloading());

    if (_sendStats)
        GetDispatcher()->post_stats_to_core(core::stats::stats_event_names::filesharing_download_file);

    DownloadRequestId_ = GetDispatcher()->downloadSharedFile(
        getChatAimid(),
        getLink(),
        _forceRequestMetainfo,
        SaveAs_, //QString(), // don't loose user's 'save as' path
        _highPriority);

    startDataTransferTimeoutTimer();

    onDataTransferStarted();
}

void FileSharingBlockBase::stopDataTransfer()
{
    if (isFileDownloading())
    {
        GetDispatcher()->abortSharedFileDownloading(Link_, DownloadRequestId_);
    }
    else if (isFileUploading())
    {
        GetDispatcher()->abortSharedFileUploading(getChatAimid(), FileLocalPath_, uploadId_);
        Q_EMIT removeMe();
    }

    stopDataTransferTimeoutTimer();

    DownloadRequestId_ = -1;

    uploadId_.clear();

    BytesTransferred_ = -1;

    onDataTransferStopped();
}

int64_t FileSharingBlockBase::getPreviewMetaRequestId() const
{
    return PreviewMetaRequestId_;
}

void FileSharingBlockBase::clearPreviewMetaRequestId()
{
    PreviewMetaRequestId_ = -1;
}

void FileSharingBlockBase::cancelRequests()
{
    if (!GetDispatcher() || !inited_)
        return;

    if (isFileDownloading())
    {
        GetDispatcher()->abortSharedFileDownloading(Link_, DownloadRequestId_);
        GetDispatcher()->cancelLoaderTask(Link_, DownloadRequestId_);
    }
}

void FileSharingBlockBase::setFileName(const QString& _filename)
{
    Filename_ = _filename;
}

void FileSharingBlockBase::setFileSize(int64_t _size)
{
    FileSizeBytes_ = _size;
}

void FileSharingBlockBase::setLocalPath(const QString& _localPath)
{
    FileLocalPath_ = _localPath;
    const QFileInfo f(_localPath);
    FileSizeBytes_ = f.size();
    LastModified_ = f.lastModified().toTime_t();
}

void FileSharingBlockBase::setLoadedFromLocal(bool _value)
{
    loadedFromLocal_ = _value;
}

void FileSharingBlockBase::setUploadId(const QString& _id)
{
    uploadId_ = _id;
}

bool FileSharingBlockBase::isInited() const
{
    return inited_;
}

void FileSharingBlockBase::connectSignals()
{
    if (!isInited())
    {
        const auto& type = getType();
        if (!type.is_sticker() || type.is_gif())
        {
            connect(GetDispatcher(), &core_dispatcher::fileSharingFileDownloaded, this, &FileSharingBlockBase::onFileDownloaded);
            connect(GetDispatcher(), &core_dispatcher::fileSharingFileDownloading, this, &FileSharingBlockBase::onFileDownloading);
            connect(GetDispatcher(), &core_dispatcher::fileSharingError, this, &FileSharingBlockBase::onFileSharingError);
            connect(GetDispatcher(), &core_dispatcher::fileSharingFileMetainfoDownloaded, this, &FileSharingBlockBase::onFileMetainfoDownloaded);
            connect(GetDispatcher(), &core_dispatcher::fileSharingUploadingResult, this, &FileSharingBlockBase::fileSharingUploadingResult);
            connect(GetDispatcher(), &core_dispatcher::fileSharingUploadingProgress, this, &FileSharingBlockBase::fileSharingUploadingProgress);

            if (isPreviewable())
                connect(GetDispatcher(), &core_dispatcher::fileSharingPreviewMetainfoDownloaded, this, &FileSharingBlockBase::onPreviewMetainfoDownloadedSlot);
        }
    }
}

bool FileSharingBlockBase::isDownload(const Data::FileSharingDownloadResult& _result) const
{
    return !_result.requestedUrl_.isEmpty();
}

QString FileSharingBlockBase::getPlaceholderFormatText() const
{
    using namespace core;

    const auto& primalType = getType();

    if( primalType.is_sticker())
        return qsl("Sticker");

    switch (primalType.type_)
    {
        case file_sharing_base_content_type::gif:
            return qsl("GIF");

        case file_sharing_base_content_type::image:
            return qsl("Photo");

        case file_sharing_base_content_type::video:
            return qsl("Video");

        case file_sharing_base_content_type::ptt:
            return qsl("Voice");

        default:
        {
            if (const auto &filename = getFilename(); !filename.isEmpty())
                return FileSharing::getPlaceholderForType(FileSharing::getFSType(filename));
        }
    }
    return qsl("File");
}

void FileSharingBlockBase::startDataTransferTimeoutTimer()
{
    if (!dataTransferTimeout_)
    {
        dataTransferTimeout_ = new QTimer(this);
        dataTransferTimeout_->setSingleShot(true);
        dataTransferTimeout_->setInterval(getDataTransferTimeout());

        connect(dataTransferTimeout_, &QTimer::timeout, this, [this]()
        {
            // emulate 1% of transferred data for the spinner
            onDataTransfer(1, 100);
        });
    }

    dataTransferTimeout_->start();
}

void FileSharingBlockBase::stopDataTransferTimeoutTimer()
{
    if (dataTransferTimeout_)
        dataTransferTimeout_->stop();
}

void FileSharingBlockBase::onFileDownloaded(qint64 _seq, const Data::FileSharingDownloadResult& _result)
{
    if (_seq != DownloadRequestId_ && getLink() != _result.requestedUrl_)
        return;

    assert(!_result.filename_.isEmpty());

    DownloadRequestId_ = -1;

    BytesTransferred_ = -1;

    SaveAs_.clear();

    FileLocalPath_ = _result.filename_;

    const QFileInfo f(_result.filename_);
    FileSizeBytes_ = f.size();
    LastModified_ = f.lastModified().toTime_t();

    savedByUser_ = _result.savedByUser_;

    onDownloaded();

    if (CopyFile_)
    {
        CopyFile_ = false;
        Utils::copyFileToClipboard(_result.filename_);
        showFileCopiedToast();

        return;
    }

    onDownloadedAction();

    if (isDownload(_result) && isPlainFile() && !isPtt())
        Utils::showDownloadToast(_result);
}

void FileSharingBlockBase::onFileDownloading(qint64 _seq, QString /*_rawUri*/, qint64 _bytesTransferred, qint64 _bytesTotal)
{
    assert(_bytesTotal > 0);

    if (_seq != DownloadRequestId_)
        return;

    stopDataTransferTimeoutTimer();

    BytesTransferred_ = _bytesTransferred;

    onDataTransfer(_bytesTransferred, _bytesTotal);
}

void FileSharingBlockBase::onFileMetainfoDownloaded(qint64 _seq, const Data::FileSharingMeta& _meta)
{
    if (FileMetaRequestId_ != _seq)
        return;

    FileMetaRequestId_ = -1;
    FileSizeBytes_ = _meta.size_;

    FileLocalPath_ = _meta.localPath_;
    Filename_ = _meta.filenameShort_;
    directUri_ = _meta.downloadUri_;
    savedByUser_ = _meta.savedByUser_;
    duration_ = _meta.duration_;
    gotAudio_ = _meta.gotAudio_;
    recognize_ = _meta.recognize_;
    LastModified_ = _meta.lastModified_;

    onMetainfoDownloaded();

    notifyBlockContentsChanged();
}

void FileSharingBlockBase::onFileSharingError(qint64 _seq, QString /*_rawUri*/, qint32 _errorCode)
{
    assert(_seq > 0);

    const auto isDownloadRequestFailed = (DownloadRequestId_ == _seq);
    if (isDownloadRequestFailed)
    {
        DownloadRequestId_ = -1;

        if (!SaveAs_.isEmpty())
        {
            showErrorToast();
            SaveAs_.clear();
        }

        onDownloadingFailed(_seq);
    }
    const auto isFileMetainfoRequestFailed = (FileMetaRequestId_ == _seq);
    const auto isPreviewMetainfoRequestFailed = (PreviewMetaRequestId_ == _seq);
    if (isFileMetainfoRequestFailed || isPreviewMetainfoRequestFailed)
    {
        FileMetaRequestId_ = -1;
        clearPreviewMetaRequestId();

        if (isFileMetainfoRequestFailed)
            getParentComplexMessage()->replaceBlockWithSourceText(this, ReplaceReason::NoMeta);
    }

    if (static_cast<loader_errors>(_errorCode) == loader_errors::move_file)
        onDownloaded();//stop animation
}

void FileSharingBlockBase::onPreviewMetainfoDownloadedSlot(qint64 _seq, QString _miniPreviewUri, QString _fullPreviewUri)
{
    if (_seq != PreviewMetaRequestId_)
        return;

    assert(isPreviewable());
    assert(!_miniPreviewUri.isEmpty());
    assert(!_fullPreviewUri.isEmpty());

    clearPreviewMetaRequestId();

    onPreviewMetainfoDownloaded(_miniPreviewUri, _fullPreviewUri);
}

void FileSharingBlockBase::fileSharingUploadingProgress(const QString& _uploadingId, qint64 _bytesTransferred)
{
    if (_uploadingId != uploadId_)
        return;

    stopDataTransferTimeoutTimer();

    BytesTransferred_ = _bytesTransferred;

    onDataTransfer(_bytesTransferred, FileSizeBytes_);
}

void FileSharingBlockBase::fileSharingUploadingResult(const QString& _seq, bool /*_success*/, const QString& localPath, const QString& _uri, int /*_contentType*/, qint64 /*_size*/, qint64 /*_lastModified*/, bool _isFileTooBig)
{
    if (_seq != uploadId_)
        return;

    if (_isFileTooBig)
    {
        Q_EMIT removeMe();
        return;
    }

    loadedFromLocal_ = false;
    Link_ = _uri;
    setFileName(QFileInfo(localPath).fileName());
    setSourceText(_uri);
    uploadId_.clear();

    Data::FileSharingDownloadResult result;
    result.rawUri_ = _uri;
    result.filename_ = localPath;

    onFileDownloaded(DownloadRequestId_, result);

    Q_EMIT uploaded(QPrivateSignal());
}

IItemBlock::MenuFlags FileSharingBlockBase::getMenuFlags(QPoint _p) const
{
    int flags = GenericBlock::getMenuFlags(_p);
    if (savedByUser_ && QFileInfo::exists(FileLocalPath_))
        flags |= IItemBlock::MenuFlags::MenuFlagOpenFolder;

    return IItemBlock::MenuFlags(flags);
}

PinPlaceholderType FileSharingBlockBase::getPinPlaceholder() const
{
    if (getType().is_sticker())
        return PinPlaceholderType::Sticker;

    if (isPreviewable())
        return isImage() ? PinPlaceholderType::FilesharingImage : PinPlaceholderType::FilesharingVideo;

    return PinPlaceholderType::Filesharing;
}

UI_COMPLEX_MESSAGE_NS_END
