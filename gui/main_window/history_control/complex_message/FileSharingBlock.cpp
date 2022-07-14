#include "stdafx.h"

#include "../../../../corelib/enumerations.h"
#include "../../../../common.shared/loader_errors.h"

#include "../../../core_dispatcher.h"

#include "../../../controls/TextUnit.h"
#include "../../../controls/FileSharingIcon.h"
#include "../../../gui_settings.h"
#include "../../../main_window/MainWindow.h"
#include "../../../previewer/GalleryWidget.h"
#include "../../../utils/InterConnector.h"
#include "../../../utils/LoadMovieFromFileTask.h"
#include "../../../utils/LoadMediaPreviewFromFileTask.h"
#include "../../../utils/log/log.h"
#include "../../../utils/PainterPath.h"
#include "../../../utils/utils.h"
#include "../../../utils/stat_utils.h"
#include "../../../utils/LoadFirstFrameTask.h"
#include "../../../utils/BlurPixmapTask.h"
#include "../../../utils/RenderLottieFirstFrameTask.h"
#include "../../../utils/features.h"
#include "../../contact_list/ContactListModel.h"
#include "../../../cache/stickers/stickers.h"

#include "../ActionButtonWidget.h"
#include "../FileSizeFormatter.h"
#include "../MessageStyle.h"
#include "../FileSharingInfo.h"

#include "ComplexMessageItem.h"
#include "ComplexMessageUtils.h"
#include "FileSharingImagePreviewBlockLayout.h"
#include "FileSharingPlainBlockLayout.h"
#include "FileSharingUtils.h"
#include "../MessageStatusWidget.h"

#include "styles/ThemeParameters.h"
#include "../MessageStyle.h"

#include "FileSharingBlock.h"
#include "QuoteBlock.h"

#include "MediaUtils.h"
#include "../../../controls/TooltipWidget.h"
#include "main_window/history_control/FileStatus.h"

#ifndef STRIP_AV_MEDIA
#include "../../sounds/SoundsManager.h"
#include "../../mplayer/VideoPlayer.h"
#endif // !STRIP_AV_MEDIA

#ifdef __APPLE__
#include "../../../utils/macos/mac_support.h"
#endif

namespace
{
    constexpr std::chrono::milliseconds previewButtonAnimDelay = std::chrono::seconds(1);
    constexpr auto stickerSize = core::sticker_size::xlarge;

    auto scaledForStickerSize = [](const auto& image)
    {
        auto image_scaled = image.scaled(QSize(
            Utils::scale_bitmap(Ui::MessageStyle::Preview::getStickerMaxWidth()),
            Utils::scale_bitmap(Ui::MessageStyle::Preview::getStickerMaxHeight())),
            Qt::KeepAspectRatio, Qt::SmoothTransformation);

        Utils::check_pixel_ratio(image_scaled);

        return image_scaled;
    };

    auto getNameColor()
    {
        return Ui::MessageStyle::getTextColorKey();
    }

    auto getProgressColor(const bool _isOutgoing)
    {
        return Ui::MessageStyle::Files::getFileSizeColorKey(_isOutgoing);
    }

    auto getShowInFolderColor()
    {
        return Styling::ThemeColorKey{ Styling::StyleVariable::PRIMARY_INVERSE };
    }

    int fileStatusMaring() noexcept
    {
        return Utils::scale_value(8);
    }

    constexpr auto BLUR_RADIOUS = 150;

    QString notAvailable()
    {
        return QT_TRANSLATE_NOOP("chat_page", "File is not available");
    }
} // namespace

Q_LOGGING_CATEGORY(fileSharingBlock, "fileSharingBlock")

UI_COMPLEX_MESSAGE_NS_BEGIN

FileSharingBlock::FileSharingBlock(ComplexMessageItem* _parent, const QString& _link, const core::file_sharing_content_type _type)
    : FileSharingBlockBase(_parent, _link, _type)
{
    parseLink();

    if (!_parent->isHeadless())
        init();
}

FileSharingBlock::FileSharingBlock(ComplexMessageItem* _parent, const HistoryControl::FileSharingInfoSptr& _fsInfo, const core::file_sharing_content_type _type)
    : FileSharingBlockBase(_parent, _fsInfo->GetUri(), _type)
    , antivirusCheckEnabled_{ Features::isAntivirusCheckEnabled() }
{
    if (!_parent->isHeadless())
        init();

    if (_fsInfo->IsOutgoing())
    {
        setLocalPath(_fsInfo->GetLocalPath());
        const auto& uploadingProcessId = _fsInfo->GetUploadingProcessId();
        if (!uploadingProcessId.isEmpty())
        {
            if (isPreviewable())
                loadPreviewFromLocalFile();
            else
                needInitFromLocal_ = true;

            setLoadedFromLocal(true);
            setUploadId(uploadingProcessId);
            onDataTransferStarted();
            return;
        }
    }

    parseLink();
}

FileSharingBlock::~FileSharingBlock() = default;

QSize FileSharingBlock::getCtrlButtonSize() const
{
    if (previewButton_)
        return previewButton_->sizeHint();

    return QSize();
}

QSize FileSharingBlock::originSizeScaled() const
{
    im_assert(isPreviewable());
    im_assert(!OriginalPreviewSize_.isEmpty());

    return Utils::scale_value(OriginalPreviewSize_);
}

QPixmap FileSharingBlock::scaledSticker(const QPixmap& _s)
{
    return scaledForStickerSize(_s);
}

QImage FileSharingBlock::scaledSticker(const QImage& _s)
{
    return scaledForStickerSize(_s);
}

bool FileSharingBlock::isPreviewReady() const
{
    return !Meta_.preview_.isNull();
}

bool FileSharingBlock::isFilenameElided() const
{
    return nameLabel_ && nameLabel_->isElided();
}

bool FileSharingBlock::isSharingEnabled() const
{
    if (isSticker() || isFileUploading())
        return false;

    return isDownloadAllowed();
}

void FileSharingBlock::onVisibleRectChanged(const QRect& _visibleRect)
{
    FileSharingBlockBase::onVisibleRectChanged(_visibleRect);

    if (isSticker())
    {
        IsVisible_ = _visibleRect.height() > 0;
    }
    else
    {
        const auto h = getFileSharingLayout()->getContentRect().height();
        IsVisible_ = _visibleRect.height() >= (h * mediaVisiblePlayPercent);
    }

    if (previewButton_)
        previewButton_->onVisibilityChanged(IsVisible_);
    else if (plainButton_)
        plainButton_->onVisibilityChanged(IsVisible_);

    if (PreviewRequestId_ != -1)
        GetDispatcher()->raiseDownloadPriority(getChatAimid(), PreviewRequestId_);

    if (isPlayable() && !getParentComplexMessage()->isSelected())
        onGifImageVisibilityChanged(IsVisible_);
}

void FileSharingBlock::onSelectionStateChanged(bool isSelected)
{
    if (isPlayable())
        onGifImageVisibilityChanged(!isSelected);
}

bool FileSharingBlock::isInPreloadRange(const QRect& _widgetAbsGeometry, const QRect& _viewportVisibilityAbsRect)
{
    auto intersected = _viewportVisibilityAbsRect.intersected(_widgetAbsGeometry);

    if (intersected.height() != 0)
        return true;

    return std::min(std::abs(_viewportVisibilityAbsRect.y() - _widgetAbsGeometry.y())
                    , std::abs(_viewportVisibilityAbsRect.bottom() - _widgetAbsGeometry.bottom())) < 1000;
}

bool FileSharingBlock::isAutoplay()
{
    if (isGifImage())
        return get_gui_settings()->get_value<bool>(settings_autoplay_gif, true);
    else if (isVideo())
        return get_gui_settings()->get_value<bool>(settings_autoplay_video, true);
    else if (isLottie())
        return true;

    return false;
}

#ifndef STRIP_AV_MEDIA
std::unique_ptr<Ui::DialogPlayer> FileSharingBlock::createPlayer()
{
    std::underlying_type_t<Ui::DialogPlayer::Flags> flags = 0;

    if (isSticker())
        flags |= Ui::DialogPlayer::Flags::is_sticker;
    else
        flags |= Ui::DialogPlayer::Flags::dialog_mode;

    if (isGifImage())
        flags |= Ui::DialogPlayer::Flags::is_gif;

    if (isLottie())
        flags |= Ui::DialogPlayer::Flags::lottie;

    if (isInsideQuote())
        flags |= Ui::DialogPlayer::Flags::compact_mode;

    auto player = std::make_unique<Ui::DialogPlayer>(this, flags, Meta_.preview_);

    connect(player.get(), &DialogPlayer::openGallery, this, [this]()
    {
        Ui::GetDispatcher()->post_stats_to_core(core::stats::stats_event_names::fullmediascr_view, { { "chat_type", Utils::chatTypeByAimId(getChatAimid()) },{ "from", "chat" },{ "media_type", isGifImage() ? "gif" : "video" } });
        auto data = Utils::GalleryData(getChatAimid(), getLink(), getGalleryId(), videoplayer_.get());
        if (auto msg = getParentComplexMessage())
            msg->fillGalleryData(data);

        Utils::InterConnector::instance().openGallery(data);
    });

    connect(player.get(), &DialogPlayer::mouseClicked, this, [this]()
    {
        onStartClicked(mapToGlobal(rect().center()));
    });

    const bool hasSound = !isGifImage() && !isSticker();

    if (hasSound)
        Ui::GetSoundsManager();

    bool mute = true;
    int32_t volume = Ui::get_gui_settings()->get_value<int32_t>(setting_mplayer_volume, 100);

    player->setParent(this);

    player->setVolume(volume);
    player->setMute(mute);

    player->setReplay((isGifImage() && isAutoplay()) || isSticker());

    player->updateSize(getFileSharingLayout()->getContentRect());
    player->updateVisibility(IsVisible_);

    if (hasSound)
        player->setGotAudio(Meta_.gotAudio_);

    player->show();

    if (Meta_.duration_ && !isInsideQuote())
        player->setDuration(Meta_.duration_);

    if (previewButton_ && previewButton_->isVisible())
        previewButton_->raise();

    Utils::InterConnector::instance().disableInMultiselect(player.get(), getChatAimid());
    return player;
}
#endif // !STRIP_AV_MEDIA

bool FileSharingBlock::loadPreviewFromLocalFile()
{
    const auto& localPath = getFileLocalPath();
    if (!QFileInfo::exists(localPath))
        return false;

    OriginalPreviewSize_ = Utils::scale_value(QSize(64, 64));
    QFileInfo fileInfo(localPath);
    setFileSize(fileInfo.size());

    const auto ext = fileInfo.suffix();

    if (Utils::is_image_extension(ext))
    {
        QImageReader reader(localPath);
        if (reader.supportsOption(QImageIOHandler::Size))
        {
            OriginalPreviewSize_ = reader.size();
            if (reader.transformation() == QImageIOHandler::TransformationRotate270 ||
                reader.transformation() == QImageIOHandler::TransformationRotate90)
                OriginalPreviewSize_.transpose();
        }

        auto task = new Utils::LoadMediaPreviewFromFileTask(localPath);

        QObject::connect(
            task,
            &Utils::LoadMediaPreviewFromFileTask::loaded,
            this,
            &FileSharingBlock::localPreviewLoaded);

        QThreadPool::globalInstance()->start(task);
    }
    else if (Utils::is_video_extension(ext))
    {
        auto task = new Utils::LoadFirstFrameTask(localPath);

        QObject::connect(
            task,
            &Utils::LoadFirstFrameTask::loadedSignal,
            this,
            &FileSharingBlock::localPreviewLoaded);

        QObject::connect(
            task,
            &Utils::LoadFirstFrameTask::duration,
            this,
            &FileSharingBlock::localDurationLoaded);

        QObject::connect(
            task,
            &Utils::LoadFirstFrameTask::gotAudio,
            this,
            &FileSharingBlock::localGotAudioLoaded);

        QThreadPool::globalInstance()->start(task);
    }

    return true;
}

bool FileSharingBlock::getLocalFileMetainfo()
{
    const auto& localPath = getFileLocalPath();
    QFileInfo info(localPath);

    if (!info.exists())
        return false;

    setFileName(info.fileName());
    setFileSize(info.size());
    onMetainfoDownloaded();

    return true;
}

int FileSharingBlock::getLabelLeftOffset()
{
    return
        (isStandalone() ? 0 : MessageStyle::Files::getHorMargin()) +
        (plainButton_ ? plainButton_->width() : 0) +
        MessageStyle::Files::getFilenameLeftMargin();
}

int FileSharingBlock::getSecondRowVerOffset()
{
    return MessageStyle::Files::getSecondRowVerOffset() + (isStandalone() ? 0 : MessageStyle::Files::getVerMargin());
}

int FileSharingBlock::getVerMargin() const
{
    return isSenderVisible() ? MessageStyle::Files::getVerMarginWithSender() : 0;
}

bool FileSharingBlock::isButtonVisible() const
{
    return !isPlayable() || (!isFileDownloaded() || isFileUploading());
}

void FileSharingBlock::buttonStartAnimation()
{
    if (previewButton_)
        previewButton_->startAnimation(previewButtonAnimDelay.count());
    else if (plainButton_)
        plainButton_->startAnimation();
}

void FileSharingBlock::buttonStopAnimation()
{
    if (previewButton_)
    {
        previewButton_->stopAnimation();
    }
    else if (plainButton_)
    {
        plainButton_->stopAnimation();

        if (isFileDownloaded())
            plainButton_->setDownloaded();
        else
            plainButton_->reset();
    }
}

void FileSharingBlock::setPreview(QPixmap _preview, QPixmap _background)
{
    if (_preview.isNull())
        return;

    if (!_background.isNull())
        Background_ = _background;

    //temp fix for exif issue on server
    if ((_preview.width() > _preview.height()) != (OriginalPreviewSize_.width() > OriginalPreviewSize_.height()))
    {
        OriginalPreviewSize_.transpose();
        if (auto parent = getParentComplexMessage())
            parent->updateSize();
    }

    Meta_.preview_ = isSticker() ? scaledSticker(_preview) : MediaUtils::scaleMediaBlockPreview(_preview);
    if (!isSticker())
    {
        if (Background_.isNull() && (isBubbleRequired() || MediaUtils::isNarrowMedia(originSizeScaled()) || MediaUtils::isWideMedia(originSizeScaled())))
        {
            seq_ = Meta_.preview_.cacheKey();

            auto task = new Utils::BlurPixmapTask(Meta_.preview_, BLUR_RADIOUS);
            connect(task, &Utils::BlurPixmapTask::blurred, this, &FileSharingBlock::prepareBackground);
            QThreadPool::globalInstance()->start(task);
        }
    }

#ifndef STRIP_AV_MEDIA
    if (isPlayable())
    {
        if (!videoplayer_)
            videoplayer_ = createPlayer();

        videoplayer_->setPreview(Meta_.preview_);
    }
#endif // !STRIP_AV_MEDIA

    update();
    getParentComplexMessage()->update();
}

void FileSharingBlock::updateFonts()
{
    const auto updateLabel = [](const auto& _label, const QFont& _font, const Styling::ThemeColorKey& _color)
    {
        if (_label)
        {
            _label->init({ _font, _color });
            _label->evaluateDesiredSize();
        }
    };

    updateLabel(nameLabel_,         MessageStyle::Files::getFilenameFont(),         getNameColor());
    updateLabel(sizeProgressLabel_, MessageStyle::Files::getFileSizeFont(),         getProgressColor(isOutgoing()));
    updateLabel(showInFolderLabel_, MessageStyle::Files::getShowInDirLinkFont(),    getShowInFolderColor());

    notifyBlockContentsChanged();
    update();
}

bool FileSharingBlock::isProgressVisible() const
{
    const auto height = getFileSharingLayout()->getContentRect().height();
    return !isInsideQuote() && height >= MessageStyle::Preview::showDownloadProgressThreshold();
}

void FileSharingBlock::onSticker(qint32 _error, const Utils::FileSharingId& _fsId)
{
    if (_error != 0 || _fsId.fileId_.isEmpty() || _fsId != getFileSharingId())
        return;

    im_assert(isSticker());

    const auto& data = Stickers::getStickerData(getFileSharingId(), stickerSize);
    if (data.isPixmap())
    {
        setPreview(data.getPixmap());
        update();
    }
#ifndef STRIP_AV_MEDIA
    else if (data.isLottie())
    {
        const auto& path = data.getLottiePath();
        if (path.isEmpty())
            return;

        if (Features::isAnimatedStickersInChatAllowed())
        {
            setLocalPath(path);
            if (!videoplayer_)
                videoplayer_ = createPlayer();
            videoplayer_->setMedia(path);

            if (Meta_.preview_.isNull())
            {
                auto task = new Utils::RenderLottieFirstFrameTask(path, getFileSharingLayout()->getContentRect().size());
                connect(task, &Utils::RenderLottieFirstFrameTask::result, this, [this](const QImage& _result)
                {
                    if (!_result.isNull())
                    {
                        if (videoplayer_)
                            videoplayer_->setPreview(_result);
                        Meta_.preview_ = QPixmap::fromImage(scaledSticker(_result));
                    }
                });
                QThreadPool::globalInstance()->start(task);
            }
        }
        else
        {
            getParentComplexMessage()->replaceBlockWithSourceText(this, ReplaceReason::NoMeta);
        }
    }
#endif // !STRIP_AV_MEDIA
}

QRect FileSharingBlock::getFileStatusRect() const
{
    if (needShowStatus(status_))
    {
        const auto x = width() - getFileStatusIconSizeWithCircle().width();
        const auto y = isStandalone() ? 0 : fileStatusMaring();
        return { { x, y }, getFileStatusIconSizeWithCircle() };
    }
    return {};
}

QRect FileSharingBlock::getPlainButtonRect() const
{
    const auto pos = isStandalone()
        ? QPoint(0, getVerMargin())
        : QPoint(MessageStyle::Files::getCtrlIconLeftMargin(), MessageStyle::Files::getVerMargin());
    return { pos, FileSharingIcon::getIconSize() };
}

void FileSharingBlock::elidePlainFileName()
{
    if (nameLabel_)
        nameLabel_->getHeight(getMaxFilenameWidth());
}

void FileSharingBlock::setPlainFileName(const QString& _name)
{
    if (nameLabel_ && !_name.isEmpty())
    {
        nameLabel_->setText(_name);
        elidePlainFileName();
    }
}

void FileSharingBlock::updatePlainButtonBlockReason(bool _forcePlaceholder)
{
    if (!plainButton_)
        return;

    auto reason = FileSharingIcon::BlockReason::NoBlock;
    if (_forcePlaceholder && status_.isTrustRequired())
        reason = FileSharingIcon::BlockReason::TrustCheck;
    else if (status_.isVirusInfectedType())
        reason = FileSharingIcon::BlockReason::Antivirus;

    plainButton_->setBlocked(reason);
}

bool FileSharingBlock::isPlaceholder() const
{
    return getFileSharingId().fileId_ == filesharingPlaceholder();
}

QSize FileSharingBlock::getImageMaxSize()
{
    if (isSticker())
        return QSize(MessageStyle::Preview::getStickerMaxWidth(), MessageStyle::Preview::getStickerMaxHeight());

    return QSize(MessageStyle::Preview::getImageWidthMax(), MessageStyle::Preview::getImageHeightMax());
}

bool FileSharingBlock::clicked(const QPoint& _p)
{
    QPoint mappedPoint = mapFromParent(_p, getFileSharingLayout()->getBlockGeometry());

    if (plainButton_ && QRect(plainButton_->pos(), plainButton_->size()).contains(mappedPoint))
        return true;

    if (showInFolderLabel_ && showInFolderLabel_->contains(mappedPoint))
        showFileInDir(Utils::OpenAt::Folder);
    else
        return onLeftMouseClick(mappedPoint);

    return true;
}

void FileSharingBlock::resetClipping()
{
    if (isPreviewable())
        PreviewClippingPath_ = QPainterPath();
}

void FileSharingBlock::onDistanceToViewportChanged(const QRect& _widgetAbsGeometry, const QRect& _viewportVisibilityAbsRect)
{
    if (!isPlayable())
        return;

    auto isInPreload = isInPreloadRange(_widgetAbsGeometry, _viewportVisibilityAbsRect);
    if (IsInPreloadDistance_ == isInPreload)
        return;

    IsInPreloadDistance_ = isInPreload;

    qCDebug(fileSharingBlock) << "set distance load " << IsInPreloadDistance_ << ", file: " << getFileLocalPath();
}

void FileSharingBlock::setCtrlButtonGeometry(const QRect& _rect)
{
    if (!previewButton_ || _rect.isEmpty())
        return;

    const auto bias = previewButton_->getCenterBias();
    const auto fixedRect = _rect.translated(-bias);
    previewButton_->setGeometry(fixedRect);
    previewButton_->setVisible(isButtonVisible());

    if (previewButton_->isVisible())
        previewButton_->raise();
}

void FileSharingBlock::drawBlock(QPainter& _p, const QRect& _rect, const QColor& quote_color)
{
    const auto& contentRect = getFileSharingLayout()->getContentRect();
    im_assert(!contentRect.isEmpty());

    if (isPreviewable())
    {
        drawPreviewableBlock(_p, contentRect, quote_color);

        if (quote_color.isValid() && !isVideo())
        {
            const auto clip_path = Utils::renderMessageBubble(_rect, MessageStyle::Preview::getBorderRadius(isStandalone()));
            _p.setClipPath(clip_path);
            _p.fillRect(_rect, QBrush(quote_color));
        }
    }
    else
    {
        drawPlainFileBlock(_p, contentRect, quote_color);
    }
}

void FileSharingBlock::initializeFileSharingBlock()
{
    setCursor(Qt::PointingHandCursor);

    if (isSticker())
    {
        if (isGifImage())
            sendGenericMetainfoRequests();
        else
            connect(&Stickers::getCache(), &Stickers::Cache::stickerUpdated, this, &FileSharingBlock::onSticker);

        if (const auto& data = Stickers::getStickerData(getFileSharingId(), stickerSize); data.isValid())
        {
            if (data.isPixmap())
            {
                Meta_.preview_ = scaledSticker(data.getPixmap());
            }
            else if (data.isLottie() && !data.getLottiePath().isEmpty())
            {
                onSticker(0, getFileSharingId());
                connect(&Utils::InterConnector::instance(), &Utils::InterConnector::omicronUpdated, this, [this]()
                {
                    if (!Features::isAnimatedStickersInChatAllowed())
                        getParentComplexMessage()->replaceBlockWithSourceText(this, ReplaceReason::NoMeta);
                });
            }
        }
    }
    else
    {
        sendGenericMetainfoRequests();

        connect(&Utils::InterConnector::instance(), &Utils::InterConnector::omicronUpdated, this, &FileSharingBlock::onConfigChanged);
        connect(GetDispatcher(), &core_dispatcher::externalUrlConfigUpdated, this, &FileSharingBlock::onConfigChanged);
    }
}

void FileSharingBlock::init()
{
    connect(&Utils::InterConnector::instance(), &Utils::InterConnector::multiselectChanged, this, &FileSharingBlock::multiselectChanged);
    connect(this, &GenericBlock::removeMe, getParentComplexMessage(), &ComplexMessageItem::removeMe);

    if (isPreviewable())
        initPreview();
    else
        initPlainFile();

    if (const auto type = getType(); !type.is_undefined())
    {
        if (type.is_video()) /// no green quote animation
            QuoteAnimation_->deactivate();
        else
            QuoteAnimation_->setSemiTransparent();
    }

    setMouseTracking(true);
}

void FileSharingBlock::applyClippingPath(QPainter& _p, const QRect& _previewRect)
{
    im_assert(isPreviewable());
    im_assert(!_previewRect.isEmpty());

    const auto isPreviewRectChanged = (LastContentRect_ != _previewRect);
    const auto shouldResetClippingPath = (PreviewClippingPath_.isEmpty() || isPreviewRectChanged);
    if (shouldResetClippingPath)
    {
        const auto header = getParentComplexMessage()->isHeaderRequired() || (isStandalone() && isBubbleRequired());
        int flags = header ? Utils::RenderBubbleFlags::BottomSideRounded : Utils::RenderBubbleFlags::AllRounded;
        if (getParentComplexMessage()->getBlockCount() == 1 && !isInsideQuote() && !isInsideForward())
        {
            if (!header && getParentComplexMessage()->isChainedToPrevMessage())
                flags |= (isOutgoing() ? Utils::RenderBubbleFlags::RightTopSmall : Utils::RenderBubbleFlags::LeftTopSmall);

            if (getParentComplexMessage()->isChainedToNextMessage())
                flags |= (isOutgoing() ? Utils::RenderBubbleFlags::RightBottomSmall : Utils::RenderBubbleFlags::LeftBottomSmall);
        }

        PreviewClippingPath_ = Utils::renderMessageBubble(
            _previewRect,
            MessageStyle::Preview::getBorderRadius(isStandalone()),
            MessageStyle::getBorderRadiusSmall(),
            (Utils::RenderBubbleFlags)flags);

        LastContentRect_ = _previewRect;
    }

    _p.setClipPath(PreviewClippingPath_);
}

void FileSharingBlock::connectImageSignals()
{
    if (!isInited())
    {
        connect(GetDispatcher(), &core_dispatcher::imageDownloaded, this, &FileSharingBlock::onImageDownloaded);
        connect(GetDispatcher(), &core_dispatcher::imageDownloadingProgress, this, &FileSharingBlock::onImageDownloadingProgress);
        connect(GetDispatcher(), &core_dispatcher::imageMetaDownloaded, this, &FileSharingBlock::onImageMetaDownloaded);
        connect(GetDispatcher(), &core_dispatcher::imageDownloadError, this, &FileSharingBlock::onImageDownloadError);
    }
}

void FileSharingBlock::drawPlainFileBlock(QPainter& _p, const QRect& frameRect, const QColor& _quoteColor)
{
    im_assert(isPlainFile());

    if (!isStandalone() && isSelected())
        drawPlainFileFrame(_p, frameRect);

    drawPlainFileName(_p);
    drawPlainFileProgress(_p);
    drawPlainFileStatus(_p);

    if ((!isBlockedFileSharing() || !Utils::InterConnector::instance().isMultiselect(getChatAimid())) && isAllowedByAntivirus())
        drawPlainFileShowInDirLink(_p);
}

void FileSharingBlock::drawPlainFileFrame(QPainter& _p, const QRect& _frameRect)
{
    im_assert(!isStandalone());

    Utils::PainterSaver ps(_p);
    _p.setRenderHint(QPainter::Antialiasing);

    _p.setBrush(MessageStyle::getBodyBrush(isOutgoing(), getChatAimid()));
    _p.setPen(Qt::NoPen);

    _p.drawRoundedRect(_frameRect, MessageStyle::getBorderRadius(), MessageStyle::getBorderRadius());
}

void FileSharingBlock::drawPlainFileName(QPainter& _p)
{
    im_assert(isPlainFile());
    if (nameLabel_)
        nameLabel_->draw(_p);
}

void FileSharingBlock::drawPlainFileProgress(QPainter& _p)
{
    im_assert(isPlainFile());

    if (getFileSize() <= 0 && !isPlaceholder())
        return;

    const auto text = getProgressText();
    if (text.isEmpty())
        return;

    if (!sizeProgressLabel_)
    {
        sizeProgressLabel_ = TextRendering::MakeTextUnit(text);
        sizeProgressLabel_->init({ MessageStyle::Files::getFileSizeFont(), getProgressColor(isOutgoing()) });
        sizeProgressLabel_->setOffsets(getLabelLeftOffset(), getSecondRowVerOffset());
        sizeProgressLabel_->evaluateDesiredSize();
    }
    else if (sizeProgressLabel_->getText() != text)
    {
        sizeProgressLabel_->setText(text);
    }

    sizeProgressLabel_->draw(_p, TextRendering::VerPosition::BASELINE);
}

void FileSharingBlock::drawPlainFileShowInDirLink(QPainter& _p)
{
    if (isFileDownloading() || isFileUploading() || !isFileDownloaded())
        return;

    if (!showInFolderLabel_)
    {
        showInFolderLabel_ = TextRendering::MakeTextUnit(QT_TRANSLATE_NOOP("chat_page", "Show in folder"));
        showInFolderLabel_->init({ MessageStyle::Files::getShowInDirLinkFont(), getShowInFolderColor() });
        showInFolderLabel_->evaluateDesiredSize();
    }

    int offsetHor = 0;
    if (sizeProgressLabel_)
        offsetHor = sizeProgressLabel_->horOffset() + sizeProgressLabel_->cachedSize().width() + Utils::scale_value(6);
    else
        offsetHor = getLabelLeftOffset();

    if (offsetHor != showInFolderLabel_->horOffset())
    {
        showInFolderLabel_->setOffsets(offsetHor, getSecondRowVerOffset());
        showInFolderLabel_->elide(width() - showInFolderLabel_->horOffset() - MessageStyle::Files::getHorMargin());
    }

    showInFolderLabel_->draw(_p, TextRendering::VerPosition::BASELINE);
}

void FileSharingBlock::drawPlainFileStatus(QPainter& _p) const
{
    const auto r = getFileStatusRect();
    if (r.isEmpty())
        return;

    Utils::PainterSaver ps(_p);
    _p.setRenderHints(QPainter::Antialiasing | QPainter::SmoothPixmapTransform);
    _p.setPen(Qt::NoPen);
    _p.setBrush(MessageStyle::getFileStatusIconBackground(status_.type(), isOutgoing()));

    _p.drawEllipse(r);

    const auto x = r.left() + (r.width() - getFileStatusIconSize().width()) / 2;
    const auto y = r.top() + (r.height() - getFileStatusIconSize().height()) / 2;
    _p.drawPixmap(x, y, getFileStatusIcon(status_.type(), MessageStyle::getFileStatusIconColor(status_.type(), isOutgoing())));
}

void FileSharingBlock::drawPreview(QPainter& _p, const QRect& _previewRect, const QColor& _quoteColor)
{
    im_assert(isPreviewable());
    im_assert(!_previewRect.isEmpty());

    if (isPreviewReady())
    {
        if (!Background_.isNull())
        {
            const auto& r = rect();
            auto bRect = Background_.rect();
            if (bRect.width() > bRect.height())
            {
                const auto diff = bRect.width() - r.width();
                bRect.setX(diff / 2);
                bRect.setWidth(r.width());
            }
            else
            {
                const auto diff = bRect.height() - r.height();
                bRect.setY(diff / 2);
                bRect.setHeight(r.height());
            }

            if ((getParentComplexMessage() && getParentComplexMessage()->maxEffectiveBlockWidth() > effectiveBlockWidth()) || MediaUtils::isNarrowMedia(originSizeScaled()) || MediaUtils::isWideMedia(originSizeScaled()))
                _p.drawPixmap(r, Background_, bRect);
        }

        const auto backgroundMode = (isSticker() || !Background_.isNull()) ? MediaUtils::BackgroundMode::NoBackground : MediaUtils::BackgroundMode::Auto;
#ifndef STRIP_AV_MEDIA
        if (!videoplayer_)
#endif // !STRIP_AV_MEDIA
        {
            MediaUtils::drawMediaInRect(_p, _previewRect, Meta_.preview_, originSizeScaled(), backgroundMode);

            if (_quoteColor.isValid())
            {
                _p.setBrush(QBrush(_quoteColor));
                _p.drawRoundedRect(_previewRect, Utils::scale_value(8), Utils::scale_value(8));
            }
        }
#ifndef STRIP_AV_MEDIA
        else
        {
            if (!videoplayer_->isFullScreen())
            {
                if (!videoplayer_->getAttachedPlayer())
                {
                    videoplayer_->setClippingPath(PreviewClippingPath_);

                    if (videoplayer_->state() == QMovie::MovieState::Paused)
                        MediaUtils::drawMediaInRect(_p, _previewRect, videoplayer_->getActiveImage(), originSizeScaled(), backgroundMode);
                }
                else
                {
                    MediaUtils::drawMediaInRect(_p, _previewRect, videoplayer_->getActiveImage(), originSizeScaled(), backgroundMode);
                }
            }
        }
#endif // !STRIP_AV_MEDIA
    }
    else if (!isSticker())
    {
        const auto brush = MessageStyle::Preview::getImagePlaceholderBrush();
        _p.fillRect(_previewRect, brush);
    }
}

void FileSharingBlock::drawPreviewableBlock(QPainter& _p, const QRect& _previewRect, const QColor& _quoteColor)
{
    if (!isSticker())
        applyClippingPath(_p, _previewRect);

    drawPreview(_p, _previewRect, _quoteColor);
}

void FileSharingBlock::initPlainFile()
{
    im_assert(previewButton_ == nullptr);
    auto layout = new FileSharingPlainBlockLayout();
    setBlockLayout(layout);
    setLayout(layout);

    status_.setTrustRequired(Logic::getContactListModel()->isTrustRequired(getChatAimid()) || Meta_.trustRequired_);

    if (!plainButton_)
    {
        plainButton_ = new FileSharingIcon(isOutgoing(), this);
        Utils::InterConnector::instance().disableInMultiselect(plainButton_, getChatAimid());
        updatePlainButtonBlockReason(isPlaceholder());
        plainButton_->raise();
        connect(plainButton_, &FileSharingIcon::clicked, this, &FileSharingBlock::onPlainFileIconClicked);
        connect(getParentComplexMessage(), &ComplexMessageItem::hoveredBlockChanged, this, [this]()
        {
            if (plainButton_)
                plainButton_->setHovered(getParentComplexMessage()->getHoveredBlock() == this);
        });
    }

    const auto text = isPlaceholder() ? notAvailable() : formatRecentsText();
    nameLabel_ = TextRendering::MakeTextUnit(text, {}, TextRendering::LinksVisible::DONT_SHOW_LINKS, TextRendering::ProcessLineFeeds::REMOVE_LINE_FEEDS);
    TextRendering::TextUnit::InitializeParameters params{ MessageStyle::Files::getFilenameFont(), getNameColor() };
    params.maxLinesCount_ = 1;
    nameLabel_->init(params);
}

void FileSharingBlock::initPreview()
{
    im_assert(plainButton_ == nullptr);
    auto layout = new FileSharingImagePreviewBlockLayout();
    setBlockLayout(layout);
    setLayout(layout);

    if (shouldDisplayProgressAnimation())
    {
        auto resourceSet = &ActionButtonResource::ResourceSet::DownloadMediaFile_;

        if (isPlayableType())
            resourceSet = &ActionButtonResource::ResourceSet::DownloadVideo_;

        initPreviewButton(*resourceSet);
    }
}

void FileSharingBlock::initPreviewButton(const ActionButtonResource::ResourceSet& _resourceSet)
{
    if (previewButton_)
        return;

    previewButton_ = new ActionButtonWidget(_resourceSet, this);

    connect(previewButton_, &ActionButtonWidget::startClickedSignal, this, &FileSharingBlock::onStartClicked);
    connect(previewButton_, &ActionButtonWidget::stopClickedSignal, this, &FileSharingBlock::onStopClicked);
    connect(previewButton_, &ActionButtonWidget::dragSignal, this, &FileSharingBlock::drag);
    connect(previewButton_, &ActionButtonWidget::internallyChangedSignal, this, &FileSharingBlock::notifyBlockContentsChanged);

    Utils::InterConnector::instance().disableInMultiselect(previewButton_, getChatAimid());
}

bool FileSharingBlock::isGifOrVideoPlaying() const
{
    im_assert(isPlayable());
#ifndef STRIP_AV_MEDIA
    return (videoplayer_ && (videoplayer_->state() == QMovie::Running));
#else
    return false;
#endif // !STRIP_AV_MEDIA
}

bool FileSharingBlock::drag()
{
    const auto pm = isPreviewable() ? Meta_.preview_ : (plainButton_ ? plainButton_->getIcon() : QPixmap());
    if (pm.isNull())
        return false;

    Q_EMIT Utils::InterConnector::instance().clearSelecting();
    return Utils::dragUrl(this, pm, getLink());
}

void FileSharingBlock::onDownloadingFailed(const int64_t requestId)
{
    buttonStopAnimation();
    update();
}

void FileSharingBlock::onDataTransferStarted()
{
    buttonStartAnimation();
    notifyBlockContentsChanged();
}

void FileSharingBlock::onDataTransferStopped()
{
    buttonStopAnimation();
    notifyBlockContentsChanged();
}

void FileSharingBlock::onDownloaded()
{
    buttonStopAnimation();
    notifyBlockContentsChanged();
}

void FileSharingBlock::onDownloadedAction()
{
#ifndef STRIP_AV_MEDIA
    if (isPlayable())
    {
        if (!videoplayer_)
            videoplayer_ = createPlayer();

        videoplayer_->setMedia(getFileLocalPath());
    }
#endif // !STRIP_AV_MEDIA
}

bool FileSharingBlock::onLeftMouseClick(const QPoint& _pos)
{
    if (!rect().contains(_pos) || Utils::InterConnector::instance().isMultiselect(getChatAimid()))
        return false;

    if (isSticker())
    {
        Q_EMIT Utils::InterConnector::instance().stopPttRecord();
        Stickers::showStickersPackByFileId(getFileSharingId(), Stickers::StatContext::Chat);
        return true;
    }

    if (previewButton_ && previewButton_->isWaitingForDelay())
        return false;

    if (isPlainFile())
    {
        if (!isAllowedByAntivirus() || (!isFileTrustAllowed() && !isFileDownloaded()))
        {
            showFileStatusToast(status_.type());
            return true;
        }
        if (isFileDownloaded())
        {
            if (plainButton_ && QRect(plainButton_->pos(), plainButton_->size()).contains(_pos))
                showFileInDir(Utils::OpenAt::Launcher);
            else
                showFileInDir(Utils::OpenAt::Folder);
        }
        else
        {
            startDownloading(true, Priority::High);
        }
        return true;
    }

    if (isImage())
    {
        Ui::GetDispatcher()->post_stats_to_core(core::stats::stats_event_names::fullmediascr_view, { { "chat_type", Utils::chatTypeByAimId(getChatAimid()) }, { "from", "chat" }, { "media_type", "photo" } });
        auto data = Utils::GalleryData(getChatAimid(), getLink(), getGalleryId(), nullptr, Meta_.preview_, OriginalPreviewSize_);
        if (auto msg = getParentComplexMessage())
            msg->fillGalleryData(data);

        Utils::InterConnector::instance().openGallery(data);
        return true;
    }

    if (isPlayable())
    {
        if (isFileDownloading())
            stopDataTransfer();
        else
            startDownloading(true);

        return true;
    }

    return false;
}

bool FileSharingBlock::isSticker() const noexcept
{
    return getType().is_sticker();
}

MediaUtils::MediaType FileSharingBlock::mediaType() const
{
    if (isSticker())
        return MediaUtils::MediaType::Sticker;
    else if (isVideo())
        return MediaUtils::MediaType::Video;
    else if (isGifImage())
        return MediaUtils::MediaType::Gif;
    else
        return MediaUtils::MediaType::Image;
}

bool FileSharingBlock::needStretchToOthers() const
{
    if (isPreviewable())
    {
        if (!isInsideQuote() || MediaUtils::isNarrowMedia(originSizeScaled()))
            return true;
    }

    return false;
}

bool FileSharingBlock::needStretchByShift() const
{
    return isSticker();
}

bool FileSharingBlock::hasLeadLines() const
{
    return isPreviewable();
}

bool FileSharingBlock::isBubbleRequired() const
{
    return !isStandalone() || !isPreviewable();
}

bool FileSharingBlock::canStretchWithSender() const
{
    return false;
}

int FileSharingBlock::minControlsWidth() const
{
    if (isInsideQuote())
        return 0;
#ifndef STRIP_AV_MEDIA
    auto controlsWidth = videoplayer_ ? videoplayer_->bottomLeftControlsWidth() : 0;
#else
    auto controlsWidth = 0;
#endif // !STRIP_AV_MEDIA
    const auto timeWidget = getParentComplexMessage()->getTimeWidget();
    if (timeWidget && isStandalone())
        controlsWidth += timeWidget->width() + timeWidget->getHorMargin();

    return controlsWidth + MessageStyle::getTimeLeftSpacing();
}

void FileSharingBlock::updateWith(IItemBlock* _other)
{
    auto otherFsBlock = dynamic_cast<FileSharingBlock*>(_other);
    if (otherFsBlock)
    {
        setPreview(otherFsBlock->getPreview(), otherFsBlock->getBackground());
        OriginalPreviewSize_ = otherFsBlock->OriginalPreviewSize_;
        Meta_.mergeWith(otherFsBlock->Meta_);
    }
}

int FileSharingBlock::effectiveBlockWidth() const
{
    if (isPreviewable() && isPreviewReady())
        return MediaUtils::getTargetSize(getFileSharingLayout()->getContentRect(), Meta_.preview_, originSizeScaled()).width();

    return 0;
}

QString FileSharingBlock::getProgressText() const
{
    const auto blockedSelected =
        isBlockedFileSharing() &&
        isPlainFile() &&
        Utils::InterConnector::instance().isMultiselect(getChatAimid());

    if (isPlaceholder() || blockedSelected)
        return QT_TRANSLATE_NOOP("chat_page", "Access limited");

    return FileSharingBlockBase::getProgressText();
}

void FileSharingBlock::markTrustRequired()
{
    const auto oldStatus = status_.type();
    status_.setTrustRequired(true);
    if (oldStatus != status_.type() && isPlainFile())
    {
        setPlainFileName(getFilename());
        update();
    }
}

IItemBlock::ContentType FileSharingBlock::getContentType() const
{
    if (isSticker())
        return IItemBlock::ContentType::Sticker;

    return IItemBlock::ContentType::FileSharing;
}

void FileSharingBlock::parseLink()
{
    im_assert(!getFileSharingId().fileId_.isEmpty());

    if (isPlaceholder())
        status_.setTrustRequired(true);

    if (isPreviewable())
    {
        OriginalPreviewSize_ = extractSizeFromFileSharingId(getFileSharingId().fileId_);

        if (OriginalPreviewSize_.isEmpty())
            OriginalPreviewSize_ = Utils::scale_value(QSize(64, 64));
    }
}

void FileSharingBlock::onAntivirusCheckResult(const Utils::FileSharingId& _fileHash, core::antivirus::check::result _result)
{
    if (_fileHash == getFileSharingId())
    {
        Meta_.antivirusCheck_.result_ = _result;
        if (core::antivirus::check::result::unchecked != _result)
        {
            disconnect(Ui::GetDispatcher(), &Ui::core_dispatcher::antivirusCheckResult, this, &FileSharingBlock::onAntivirusCheckResult);
            onMetainfoDownloaded();
        }
    }
}

void FileSharingBlock::onStartClicked(const QPoint& _globalPos)
{
    onLeftMouseClick(mapFromGlobal(_globalPos));
}

void FileSharingBlock::onStopClicked(const QPoint&)
{
    stopDataTransfer();
    notifyBlockContentsChanged();
}

void FileSharingBlock::onPlainFileIconClicked()
{
    im_assert(plainButton_);
    const auto pos = plainButton_->pos() + plainButton_->rect().center();

    if (isFileUploading() || isFileDownloading())
        onStopClicked(pos);
    else
        onStartClicked(mapToGlobal(pos));

    Q_EMIT Utils::InterConnector::instance().setFocusOnInput(getChatAimid());
}

void FileSharingBlock::localPreviewLoaded(QPixmap _pixmap, const QSize _originalSize)
{
    if (_pixmap.isNull())
        return;

    OriginalPreviewSize_ = _originalSize;

    notifyBlockContentsChanged();

    setPreview(_pixmap);
}

void FileSharingBlock::localDurationLoaded(qint64 _duration)
{
    Meta_.duration_ = _duration;
#ifndef STRIP_AV_MEDIA
    if (videoplayer_)
        videoplayer_->setDuration(_duration);
#endif // !STRIP_AV_MEDIA
}

void FileSharingBlock::localGotAudioLoaded(bool _gotAudio)
{
    Meta_.gotAudio_ = _gotAudio;
#ifndef STRIP_AV_MEDIA
    if (videoplayer_)
        videoplayer_->setGotAudio(_gotAudio);
#endif // !STRIP_AV_MEDIA
}

void FileSharingBlock::multiselectChanged()
{
    if (!getParentComplexMessage()->isMultiselectEnabled())
        return;

    if (isPlainFile())
    {
        const auto multiselectInCurrentChat = Utils::InterConnector::instance().isMultiselect(getChatAimid());
        updatePlainButtonBlockReason(isPlaceholder() || multiselectInCurrentChat);

        if (isBlockedFileSharing())
            setPlainFileName(multiselectInCurrentChat ? notAvailable() : getFilename());
    }
    else if (isPlayable())
    {
        onGifImageVisibilityChanged(IsVisible_);
    }
}

void FileSharingBlock::prepareBackground(const QPixmap& _result, qint64 _srcCacheKey)
{
    if (_srcCacheKey != seq_ || _result.isNull())
        return;

    Background_ = _result;
    {
        auto color = Styling::getParameters().getColor(Styling::StyleVariable::CHAT_SECONDARY_MEDIA, 0.15);
        QPainter p(&Background_);
        p.fillRect(Background_.rect(), color);
    }

    const auto& r = rect();
    if (r.width() >= r.height())
        Background_ = Background_.scaledToHeight(r.height());
    else
        Background_ = Background_.scaledToWidth(r.width());

    update();
}

void FileSharingBlock::onConfigChanged()
{
    const auto antivirusEnabled = Features::isAntivirusCheckEnabled();
    if (std::exchange(antivirusCheckEnabled_, antivirusEnabled) != antivirusEnabled)
    {
        onMetainfoDownloaded();
        notifyBlockContentsChanged();
    }
}

void FileSharingBlock::resizeEvent(QResizeEvent* _event)
{
    FileSharingBlockBase::resizeEvent(_event);
#ifndef STRIP_AV_MEDIA
    if (videoplayer_)
        videoplayer_->updateSize(getFileSharingLayout()->getContentRect());
#endif // !STRIP_AV_MEDIA

    if (plainButton_)
    {
        plainButton_->move(getPlainButtonRect().topLeft());
        plainButton_->setVisible(isButtonVisible());

        if (plainButton_->isVisible())
            plainButton_->raise();
    }

    const auto labelLeftOffset = getLabelLeftOffset();

    if (needInitFromLocal_)
    {
        needInitFromLocal_ = false;
        getLocalFileMetainfo();
    }

    if (nameLabel_)
    {
        const auto verOffset = getVerMargin() + (isStandalone() ? Utils::scale_value(4) : Utils::scale_value(12));
        nameLabel_->setOffsets(labelLeftOffset, verOffset);
        elidePlainFileName();
    }

    const auto secondRowVerOffset = getSecondRowVerOffset();
    if (sizeProgressLabel_)
        sizeProgressLabel_->setOffsets(labelLeftOffset, secondRowVerOffset);

    if (showInFolderLabel_)
    {
        auto offsetHor = 0;
        if (sizeProgressLabel_)
            offsetHor = sizeProgressLabel_->horOffset() + sizeProgressLabel_->cachedSize().width() + Utils::scale_value(6);
        else
            offsetHor = labelLeftOffset;

        showInFolderLabel_->setOffsets(offsetHor, secondRowVerOffset);
        showInFolderLabel_->elide(width() - showInFolderLabel_->horOffset() - MessageStyle::Files::getHorMargin());
    }

    if (!Background_.isNull())
    {
        const auto& r = rect();
        if (r.width() >= r.height())
            Background_ = Background_.scaledToHeight(r.height());
        else
            Background_ = Background_.scaledToWidth(r.width());
    }
}

void FileSharingBlock::requestPreview(const QString& _uri)
{
    im_assert(isPreviewable());
    im_assert(PreviewRequestId_ == -1);

    PreviewRequestId_ = GetDispatcher()->downloadImage(_uri, QString(), false, 0, 0, false);
}

void FileSharingBlock::sendGenericMetainfoRequests()
{
    if (isPreviewable())
    {
        connectImageSignals();

        requestMetainfo(true);
    }

    requestMetainfo(false);
}

void FileSharingBlock::onMenuOpenFolder()
{
    showFileInDir(Utils::OpenAt::Folder);
}

bool FileSharingBlock::shouldDisplayProgressAnimation() const
{
    return !isSticker();
}

void FileSharingBlock::updateFileTypeIcon()
{
    im_assert(isPlainFile());

    if (plainButton_)
    {
        if (const auto& filename = getFilename(); !filename.isEmpty())
            plainButton_->setFilename(filename);

        if (!getFileLocalPath().isEmpty())
            plainButton_->setDownloaded();
    }
}

void FileSharingBlock::onDataTransfer(const int64_t _bytesTransferred, const int64_t _bytesTotal, bool _showBytes)
{
    im_assert(_bytesTotal > 0);
    if (_bytesTotal <= 0)
        return;

    if (!shouldDisplayProgressAnimation())
        return;

    if (previewButton_ && isPreviewable())
    {
        if (const auto text = HistoryControl::formatProgressText(_bytesTotal, _bytesTransferred); !text.isEmpty())
        {
            const auto progress = ((double)_bytesTransferred / (double)_bytesTotal);
            previewButton_->setProgress(progress);

            if (isProgressVisible() && _showBytes)
                previewButton_->setProgressText(text);
        }
    }
    else if (plainButton_)
    {
        plainButton_->setBytes(_bytesTotal, _bytesTransferred);
    }

    update();
    notifyBlockContentsChanged();
}

void FileSharingBlock::onMetainfoDownloaded()
{
    antivirusCheckEnabled_ = Features::isAntivirusCheckEnabled();
    const auto trustRequired = Meta_.trustRequired_ || Logic::getContactListModel()->isTrustRequired(getChatAimid());
    const auto& antivirusCheck = Meta_.antivirusCheck_;
    status_.updateData(antivirusCheck, trustRequired, antivirusCheckEnabled_);
    if (antivirusCheckEnabled_)
    {
        if (status_.isAntivirusInProgress())
        {
            if (!getLink().isEmpty())
            {
                Ui::GetDispatcher()->subscribeFileSharingAntivirus(getFileSharingId());
                connect(Ui::GetDispatcher(), &Ui::core_dispatcher::antivirusCheckResult, this, &FileSharingBlock::onAntivirusCheckResult, Qt::UniqueConnection);
            }
        }
        else if (previewButton_ && core::antivirus::check::result::unsafe == antivirusCheck.result_)
        {
            // Collapse preview block to plain file block when photo/video unsafe
#ifndef STRIP_AV_MEDIA
            videoplayer_.reset();
#endif
            previewButton_->deleteLater();
            previewButton_ = nullptr;
            initPlainFile();
        }
        updatePlainButtonBlockReason(false);
    }
    else
    {
        // Expand preview again when antivirus check disabled
        if (isPreviewable() && plainButton_)
        {
            plainButton_->deleteLater();
            plainButton_ = nullptr;
            initPreview();
        }
    }

    if (isPlayable())
    {
        if (previewButton_ && isProgressVisible())
            previewButton_->setDefaultProgressText(HistoryControl::formatFileSize(getFileSize()));

#ifndef STRIP_AV_MEDIA
        if (isVideo() && videoplayer_)
        {
            videoplayer_->setDuration(Meta_.duration_);
            videoplayer_->setGotAudio(Meta_.gotAudio_);
        }

        if (const auto localPath = getFileLocalPath(); !localPath.isEmpty() && QFile::exists(localPath))
        {
            if (!videoplayer_)
                videoplayer_ = createPlayer();

            videoplayer_->setMedia(localPath);
        }
        else
#endif // !STRIP_AV_MEDIA
        if ((isSticker() || isAutoplay()) && isDownloadAllowed())
        {
            startDownloading(false);
        }
    }
    else if (isPlainFile())
    {
        setPlainFileName(getFilename());
        updateFileTypeIcon();
        updatePlainButtonBlockReason(false);
        notifyBlockContentsChanged();
    }
}

void FileSharingBlock::onImageDownloadError(qint64 _seq, QString /*_rawUri*/)
{
    if (PreviewRequestId_ != _seq)
        return;

    PreviewRequestId_ = -1;

    onDownloadingFailed(_seq);
}

FileSharingBlock::MenuFlags FileSharingBlock::getMenuFlags(QPoint _p) const
{
    if (isSticker())
        return FileSharingBlock::MenuFlags::MenuFlagNone;

    return FileSharingBlockBase::getMenuFlags(_p);
}

bool FileSharingBlock::isSmallPreview() const
{
    if (const auto size = originSizeScaled(); size.isValid() && !isSticker())
        return MediaUtils::isSmallMedia(size);

    return false;
}

void FileSharingBlock::mouseMoveEvent(QMouseEvent* _event)
{
    const auto overName = Features::longPathTooltipsAllowed() && nameLabel_ && nameLabel_->contains(_event->pos()) && nameLabel_->isElided();
    const auto overStatus = getFileStatusRect().contains(_event->pos());

    if (overName || overStatus)
    {
        if (!isTooltipActivated())
        {
            const auto ttRect = overName
                ? QRect(0, nameLabel_->offsets().y(), width(), nameLabel_->cachedSize().height())
                : getFileStatusRect();

            const auto isFullyVisible = visibleRegion().boundingRect().y() < ttRect.top();
            const auto arrowDir = isFullyVisible ? Tooltip::ArrowDirection::Down : Tooltip::ArrowDirection::Up;
            const auto arrowPos = isFullyVisible ? Tooltip::ArrowPointPos::Top : Tooltip::ArrowPointPos::Bottom;
            const auto text = overName ? nameLabel_->getSourceText().string() : getFileStatusText(status_.type());
            showTooltip(text, QRect(mapToGlobal(ttRect.topLeft()), ttRect.size()), arrowDir, arrowPos);
        }
    }
    else
    {
        hideTooltip();
    }
    FileSharingBlockBase::mouseMoveEvent(_event);
}

void FileSharingBlock::leaveEvent(QEvent* _event)
{
    hideTooltip();
    GenericBlock::leaveEvent(_event);
}

const QPixmap& FileSharingBlock::getPreview() const
{
    return Meta_.preview_;
}

const QPixmap& FileSharingBlock::getBackground() const
{
    return Background_;
}

void FileSharingBlock::onImageDownloaded(int64_t _seq, QString /*_uri*/, QPixmap _image)
{
    if (PreviewRequestId_ != _seq)
        return;
    if (isVirusInfected())
        return;

    PreviewRequestId_ = -1;
    setPreview(_image);

    update();
}

void FileSharingBlock::onImageDownloadingProgress(qint64 /*seq*/, int64_t /*bytesTotal*/, int64_t /*bytesTransferred*/, int32_t /*pctTransferred*/)
{
}

void FileSharingBlock::onImageMetaDownloaded(int64_t /*_seq*/, Data::LinkMetadata /*_meta*/)
{
    im_assert(isPreviewable());
}

void FileSharingBlock::onGifImageVisibilityChanged(const bool _isVisible)
{
    im_assert(isPlayable());

#ifndef STRIP_AV_MEDIA
    if (!videoplayer_)
        return;

    const auto canUpdateVisibility =
        !_isVisible ||
        getParentComplexMessage()->isThreadFeedMessage() ||
        Utils::InterConnector::instance().isHistoryPageVisible(getChatAimid());

    if (canUpdateVisibility)
        videoplayer_->updateVisibility(_isVisible);
#endif // !STRIP_AV_MEDIA
}

void FileSharingBlock::onLocalCopyInfoReady(const bool _isCopyExists)
{
    if (_isCopyExists)
        update();
}

void FileSharingBlock::onPreviewMetainfoDownloaded()
{
    if (!Meta_.preview_.isNull())
    {
        setPreview(Meta_.preview_);
    }
    else
    {
        const auto& previewUri = Meta_.fullPreviewUri_.isEmpty() ? Meta_.miniPreviewUri_ : Meta_.fullPreviewUri_;
        requestPreview(previewUri);
    }
}

void FileSharingBlock::setQuoteSelection()
{
    GenericBlock::setQuoteSelection();
    Q_EMIT setQuoteAnimation();
}

int FileSharingBlock::desiredWidth(int _width) const
{
    if (isSticker())
        return MessageStyle::Preview::getStickerMaxWidth();

    if (_width != 0)
        return getFileSharingLayout()->blockSizeForMaxWidth(_width).width();

    return MessageStyle::getFileSharingMaxWidth();
}

int FileSharingBlock::getMaxWidth() const
{
    if (isPreviewable())
        return MediaUtils::mediaBlockMaxWidth(originSizeScaled());
    else
        return GenericBlock::getMaxWidth();
}

bool FileSharingBlock::isSizeLimited() const
{
    return !isPreviewable() || isSticker();
}

int FileSharingBlock::filenameDesiredWidth() const
{
    if (!nameLabel_)
        return 0;

    return nameLabel_->desiredWidth();
}

int FileSharingBlock::getMaxFilenameWidth() const
{
    const auto statusWidth = needShowStatus(status_)
        ? getFileStatusIconSizeWithCircle().width() + fileStatusMaring()
        : 0;

    auto w = width() - (plainButton_ ? plainButton_->width() : 0) - MessageStyle::Files::getHorMargin();
    if (!isStandalone())
        w -= MessageStyle::Files::getHorMargin() * (statusWidth > 0 ? 1 : 2);

    return w - statusWidth;
}

void FileSharingBlock::requestPinPreview()
{
    auto connMeta = std::make_shared<QMetaObject::Connection>();
    *connMeta = connect(GetDispatcher(),&core_dispatcher::fileSharingPreviewMetainfoDownloaded, this,
        [connMeta, this](qint64 _seq, auto _miniPreviewUri, auto _fullPreviewUri)
    {
        if (getPreviewMetaRequestId() == _seq)
        {
            Meta_.miniPreviewUri_ = _miniPreviewUri;
            Meta_.fullPreviewUri_ = _fullPreviewUri;
            onPreviewMetainfoDownloaded();

            clearPreviewMetaRequestId();
            disconnect(*connMeta);
        }
    });

    auto connFile = std::make_shared<QMetaObject::Connection>();
    *connFile = connect(GetDispatcher(), &core_dispatcher::imageDownloaded, this,
        [connFile, this](qint64 _seq, QString, QPixmap _preview)
    {
        if (PreviewRequestId_ == _seq)
        {
            if (!_preview.isNull() && getParentComplexMessage())
                Q_EMIT getParentComplexMessage()->pinPreview(_preview);

            PreviewRequestId_ = -1;
            disconnect(*connFile);
        }
    });

    requestMetainfo(true);
}

UI_COMPLEX_MESSAGE_NS_END
