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
#include "../../../utils/LoadPixmapFromFileTask.h"
#include "../../../utils/log/log.h"
#include "../../../utils/PainterPath.h"
#include "../../../utils/utils.h"
#include "../../../utils/stat_utils.h"
#include "../../../utils/LoadFirstFrameTask.h"
#include "../../contact_list/ContactListModel.h"
#include "../../../cache/stickers/stickers.h"

#include "../../MainPage.h"

#include "../ActionButtonWidget.h"
#include "../FileSizeFormatter.h"
#include "../MessageStyle.h"
#include "../FileSharingInfo.h"

#include "ComplexMessageItem.h"
#include "ComplexMessageUtils.h"
#include "FileSharingImagePreviewBlockLayout.h"
#include "FileSharingPlainBlockLayout.h"
#include "FileSharingUtils.h"

#include "../../sounds/SoundsManager.h"

#include "styles/ThemeParameters.h"
#include "../MessageStyle.h"

#include "FileSharingBlock.h"
#include "QuoteBlock.h"

#ifdef __APPLE__
#include "../../../utils/macos/mac_support.h"
#endif

namespace
{
    constexpr std::chrono::milliseconds previewButtonAnimDelay = std::chrono::seconds(1);

    QSize getGhostSize()
    {
        return Utils::scale_value(QSize(160, 160));
    }

    QPixmap getGhost()
    {
        static const auto ghost = Utils::renderSvg(qsl("://themes/stickers/ghost_sticker"), getGhostSize());
        return ghost;
    }

    auto scaledForStickerSize = [](const auto& image)
    {
        auto image_scaled = image.scaled(QSize(
            Utils::scale_bitmap(Ui::MessageStyle::Preview::getStickerMaxWidth()),
            Utils::scale_bitmap(Ui::MessageStyle::Preview::getStickerMaxHeight())),
            Qt::KeepAspectRatio, Qt::SmoothTransformation);

        Utils::check_pixel_ratio(image_scaled);

        return image_scaled;
    };

    QColor getSelectedColor()
    {
        return Styling::getParameters().getColor(Styling::StyleVariable::TEXT_SOLID_PERMANENT);
    }

    QColor getNameColor(const bool _isSelected)
    {
        return _isSelected ? getSelectedColor() : Ui::MessageStyle::getTextColor();
    }

    QColor getProgressColor(const bool _isSelected, const bool _isOutgoing)
    {
        return _isSelected ? getSelectedColor() : Ui::MessageStyle::Files::getFileSizeColor(_isOutgoing);
    }

    QColor getShowInFolderColor(const bool _isSelected)
    {
        return _isSelected ? getSelectedColor() : Styling::getParameters().getColor(Styling::StyleVariable::PRIMARY_INVERSE);
    }
}

Q_LOGGING_CATEGORY(fileSharingBlock, "fileSharingBlock")

UI_COMPLEX_MESSAGE_NS_BEGIN

FileSharingBlock::FileSharingBlock(ComplexMessageItem* _parent, const QString &link, const core::file_sharing_content_type type)
    : FileSharingBlockBase(_parent, link, type)
    , IsVisible_(false)
    , IsInPreloadDistance_(true)
    , isSmallPreview_(false)
    , PreviewRequestId_(-1)
    , previewButton_(nullptr)
    , plainButton_(nullptr)
{
    parseLink();

    if (!_parent->isHeadless())
        init();
}

FileSharingBlock::FileSharingBlock(ComplexMessageItem* _parent, const HistoryControl::FileSharingInfoSptr& fsInfo, const core::file_sharing_content_type type)
    : FileSharingBlockBase(_parent, fsInfo->GetUri(), type)
    , IsVisible_(false)
    , IsInPreloadDistance_(true)
    , isSmallPreview_(false)
    , PreviewRequestId_(-1)
    , previewButton_(nullptr)
    , plainButton_(nullptr)
{
    if (!_parent->isHeadless())
        init();

    if (fsInfo->IsOutgoing())
    {
        setLocalPath(fsInfo->GetLocalPath());
        const auto &uploadingProcessId = fsInfo->GetUploadingProcessId();
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

QSize FileSharingBlock::getOriginalPreviewSize() const
{
    assert(isPreviewable());
    assert(!OriginalPreviewSize_.isEmpty());

    return OriginalPreviewSize_;
}

QSize FileSharingBlock::getSmallPreviewSize() const
{
    return MessageStyle::Preview::getSmallPreviewSize(isImage() ? MessageStyle::Preview::Option::image : MessageStyle::Preview::Option::video);
}

QSize FileSharingBlock::getMinPreviewSize() const
{
    return MessageStyle::Preview::getMinPreviewSize(isImage() ? MessageStyle::Preview::Option::image : MessageStyle::Preview::Option::video);
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
    return !Preview_.isNull();
}

bool FileSharingBlock::isFilenameElided() const
{
    return nameLabel_ && nameLabel_->isElided();
}

bool FileSharingBlock::isSharingEnabled() const
{
    return !isSticker() && !isFileUploading();
}

void FileSharingBlock::onVisibilityChanged(const bool isVisible)
{
    IsVisible_ = isVisible;

    FileSharingBlockBase::onVisibilityChanged(isVisible);
    if (previewButton_)
        previewButton_->onVisibilityChanged(isVisible);
    else if (plainButton_)
        plainButton_->onVisibilityChanged(isVisible);

    if (PreviewRequestId_ != -1)
        GetDispatcher()->raiseDownloadPriority(getChatAimid(), PreviewRequestId_);

    if (isGifImage() || isVideo())
        onGifImageVisibilityChanged(isVisible);
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

    return false;
}

DialogPlayer* FileSharingBlock::createPlayer()
{
    std::underlying_type_t<Ui::DialogPlayer::Flags> flags = 0;

    if (isSticker())
        flags |= Ui::DialogPlayer::Flags::is_sticker;
    else
        flags |= Ui::DialogPlayer::Flags::enable_dialog_controls;

    if (isGifImage())
        flags |= Ui::DialogPlayer::Flags::is_gif;

    auto player  = new Ui::DialogPlayer(this, flags, Preview_);

    connect(player, &DialogPlayer::openGallery, this, [this]()
    {
        Ui::GetDispatcher()->post_stats_to_core(core::stats::stats_event_names::fullmediascr_view, { { "chat_type", Utils::chatTypeByAimId(getChatAimid()) },{ "from", "chat" },{ "media_type", isGifImage() ? "gif" : "video" } });
        Utils::InterConnector::instance().getMainWindow()->openGallery(getChatAimid(), getLink(), getGalleryId(), videoplayer_.get());
    });

    connect(player, &DialogPlayer::mouseClicked, this, [this]()
    {
        onStartClicked(mapToGlobal(rect().center()));
    });

    Ui::GetSoundsManager();
    bool mute = true;
    int32_t volume = Ui::get_gui_settings()->get_value<int32_t>(setting_mplayer_volume, 100);

    player->setParent(this);

    player->setVolume(volume);
    player->setMute(mute);

    player->setReplay((isGifImage() && isAutoplay()) || isSticker());

    player->updateSize(getFileSharingLayout()->getContentRect());
    player->updateVisibility(IsVisible_);
    player->setGotAudio(gotAudio_);
    player->setFillClient(Utils::isPanoramic(OriginalPreviewSize_));
    player->show();

    if (duration_)
        player->setDuration(duration_);

    if (previewButton_ && previewButton_->isVisible())
        previewButton_->raise();

    return player;
}

bool FileSharingBlock::loadPreviewFromLocalFile()
{
    const auto& localPath = getFileLocalPath();
    const auto fileExists = QFile::exists(localPath);
    if (!fileExists)
    {
        return false;
    }

    OriginalPreviewSize_ = Utils::scale_value(QSize(64, 64));
    QFileInfo fileInfo(localPath);
    setFileSize(fileInfo.size());

    const auto ext = fileInfo.suffix();

    if (Utils::is_image_extension(ext))
    {
        auto task = new Utils::LoadPixmapFromFileTask(localPath, QSize(MessageStyle::Preview::getImageWidthMax(), MessageStyle::Preview::getImageHeightMax()));

        QObject::connect(
            task,
            &Utils::LoadPixmapFromFileTask::loadedSignal,
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
    const auto &localPath = getFileLocalPath();
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
    const auto isGifVideo = isGifImage() || isVideo();
    return (isGifVideo && (!isFileDownloaded() || isFileUploading())) || !isGifVideo;
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

void FileSharingBlock::setPreview(QPixmap _preview)
{
    assert(!_preview.isNull());

    Preview_ = isSticker() ? scaledSticker(_preview) : _preview;

    if (isSticker())
    {
        isSmallPreview_ = false;
    }
    else if (isPreviewable() && !Preview_.isNull())
    {
        const auto small = getSmallPreviewSize();
        const auto orig = getOriginalPreviewSize();
        isSmallPreview_ = orig.width() <= small.width() && orig.height() <= small.height();
    }

    if (isVideo() || isGifImage())
    {
        if (!videoplayer_)
            videoplayer_.reset(createPlayer());

        videoplayer_->setPreview(Preview_);
    }

    if (isSmallPreview())
        getParentComplexMessage()->setCustomBubbleHorPadding(MessageStyle::Preview::getSmallPreviewHorPadding());
}

void FileSharingBlock::updateStyle()
{
    const auto updateLabel = [](const auto& _label, const QColor& _color)
    {
        if (_label)
            _label->setColor(_color);
    };

    updateLabel(nameLabel_,         getNameColor(isSelected()));
    updateLabel(sizeProgressLabel_, getProgressColor(isSelected(), isOutgoing()));
    updateLabel(showInFolderLabel_, getShowInFolderColor(isSelected()));

    update();
}

void FileSharingBlock::updateFonts()
{
    const auto updateLabel = [](const auto& _label, const QFont& _font, const QColor& _color)
    {
        if (_label)
        {
            _label->init(_font, _color);
            _label->evaluateDesiredSize();
        }
    };

    updateLabel(nameLabel_,         MessageStyle::Files::getFilenameFont(),         getNameColor(isSelected()));
    updateLabel(sizeProgressLabel_, MessageStyle::Files::getFileSizeFont(),         getProgressColor(isSelected(), isOutgoing()));
    updateLabel(showInFolderLabel_, MessageStyle::Files::getShowInDirLinkFont(),    getShowInFolderColor(isSelected()));

    notifyBlockContentsChanged();
    update();
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
        onLeftMouseClick(mappedPoint);

    return true;
}

void FileSharingBlock::resetClipping()
{
    if (isPreviewable())
        PreviewClippingPath_ = QPainterPath();
}

void FileSharingBlock::onDistanceToViewportChanged(const QRect& _widgetAbsGeometry, const QRect& _viewportVisibilityAbsRect)
{
    if (!isGifImage() && !isVideo())
        return;

    auto isInPreload = isInPreloadRange(_widgetAbsGeometry, _viewportVisibilityAbsRect);
    if (IsInPreloadDistance_ == isInPreload)
        return;

    IsInPreloadDistance_ = isInPreload;

    qCDebug(fileSharingBlock) << "set distance load " << IsInPreloadDistance_ << ", file: " << getFileLocalPath();
}

void FileSharingBlock::setCtrlButtonGeometry(const QRect &rect)
{
    if (!previewButton_ || rect.isEmpty())
        return;

    const auto bias = previewButton_->getCenterBias();
    const auto fixedRect = rect.translated(-bias);
    previewButton_->setGeometry(fixedRect);
    previewButton_->setVisible(isButtonVisible());

    if (previewButton_->isVisible())
        previewButton_->raise();
}

void FileSharingBlock::drawBlock(QPainter &p, const QRect& _rect, const QColor& quote_color)
{
    const auto& contentRect = getFileSharingLayout()->getContentRect();
    assert(!contentRect.isEmpty());

    if (isPreviewable())
    {
        drawPreviewableBlock(p, contentRect, quote_color);

        if (quote_color.isValid() && !isVideo())
        {
            const auto clip_path = Utils::renderMessageBubble(_rect, MessageStyle::getBorderRadius());
            p.setClipPath(clip_path);
            p.fillRect(_rect, QBrush(quote_color));
        }
    }
    else
    {
        drawPlainFileBlock(p, contentRect, quote_color);
    }
}

void FileSharingBlock::initializeFileSharingBlock()
{
    setCursor(Qt::PointingHandCursor);

    sendGenericMetainfoRequests();
}

void FileSharingBlock::init()
{
    connect(this, &GenericBlock::removeMe, getParentComplexMessage(), &ComplexMessageItem::removeMe);

    if (isPreviewable())
        initPreview();
    else
        initPlainFile();

    if (!getType().is_undefined())
    {
        if (getType().is_video()) /// no green quote animation
            QuoteAnimation_.deactivate();
        else
            QuoteAnimation_.setSemiTransparent();
    }

    setMouseTracking(true);
}

void FileSharingBlock::applyClippingPath(QPainter &p, const QRect &_previewRect)
{
    assert(isPreviewable());
    assert(!_previewRect.isEmpty());

    const auto isPreviewRectChanged = (LastContentRect_ != _previewRect);
    const auto shouldResetClippingPath = (PreviewClippingPath_.isEmpty() || isPreviewRectChanged);
    if (shouldResetClippingPath)
    {
        const auto header = getParentComplexMessage()->isHeaderRequired();
        int flags = header ? Utils::RenderBubbleFlags::BottomSideRounded : Utils::RenderBubbleFlags::AllRounded;
        if (getParentComplexMessage()->getBlockCount() == 1)
        {
            if (!header && getParentComplexMessage()->isChainedToPrevMessage())
                flags |= (isOutgoing() ? Utils::RenderBubbleFlags::RightTopSmall : Utils::RenderBubbleFlags::LeftTopSmall);

            if (getParentComplexMessage()->isChainedToNextMessage())
                flags |= (isOutgoing() ? Utils::RenderBubbleFlags::RightBottomSmall : Utils::RenderBubbleFlags::LeftBottomSmall);
        }

        PreviewClippingPath_ = Utils::renderMessageBubble(
            _previewRect,
            MessageStyle::getBorderRadius(),
            MessageStyle::getBorderRadiusSmall(),
            (Utils::RenderBubbleFlags)flags);

        auto relativePreviewRect = QRect(0, 0, _previewRect.width(), _previewRect.height());
        RelativePreviewClippingPath_ = Utils::renderMessageBubble(
            relativePreviewRect,
            MessageStyle::getBorderRadius(),
            MessageStyle::getBorderRadiusSmall(),
            (Utils::RenderBubbleFlags)flags);

        LastContentRect_ = _previewRect;
    }

    p.setClipPath(PreviewClippingPath_);
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

void FileSharingBlock::drawPlainFileBlock(QPainter &p, const QRect &frameRect, const QColor& quote_color)
{
    assert(isPlainFile());

    if (!isStandalone() && isSelected())
        drawPlainFileFrame(p, frameRect);

    drawPlainFileName(p);
    drawPlainFileProgress(p);
    drawPlainFileShowInDirLink(p);
}

void FileSharingBlock::drawPlainFileFrame(QPainter& _p, const QRect &frameRect)
{
    assert(!isStandalone());

    Utils::PainterSaver ps(_p);
    _p.setRenderHint(QPainter::Antialiasing);

    const auto &bodyBrush = MessageStyle::getBodyBrush(isOutgoing(), isSelected(), getChatAimid());
    _p.setBrush(bodyBrush);
    _p.setPen(Qt::NoPen);

    _p.drawRoundedRect(frameRect, MessageStyle::getBorderRadius(), MessageStyle::getBorderRadius());
}

void FileSharingBlock::drawPlainFileName(QPainter &p)
{
    assert(isPlainFile());
    if (!nameLabel_)
        return;

    nameLabel_->draw(p);
}

void FileSharingBlock::drawPlainFileProgress(QPainter& _p)
{
    assert(isPlainFile());

    if (getFileSize() <= 0)
        return;

    const auto text = getProgressText();
    if (text.isEmpty())
        return;

    if (!sizeProgressLabel_)
    {
        sizeProgressLabel_ = TextRendering::MakeTextUnit(text);
        sizeProgressLabel_->init(MessageStyle::Files::getFileSizeFont(), getProgressColor(isSelected(), isOutgoing()));
        sizeProgressLabel_->setOffsets(getLabelLeftOffset(), getSecondRowVerOffset());
        sizeProgressLabel_->evaluateDesiredSize();
    }
    else
        sizeProgressLabel_->setText(text);

    sizeProgressLabel_->draw(_p, TextRendering::VerPosition::BASELINE);
}

void FileSharingBlock::drawPlainFileShowInDirLink(QPainter &p)
{
    if (isFileDownloading() || isFileUploading() || !isFileDownloaded())
        return;

    if (!showInFolderLabel_)
    {
        showInFolderLabel_ = TextRendering::MakeTextUnit(QT_TRANSLATE_NOOP("chat_page", "Show in folder"));
        showInFolderLabel_->init(MessageStyle::Files::getShowInDirLinkFont(), getShowInFolderColor(isSelected()));
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

    showInFolderLabel_->draw(p, TextRendering::VerPosition::BASELINE);
}

void FileSharingBlock::drawPreview(QPainter &p, const QRect &previewRect, QPainterPath& _path, const QColor& quote_color)
{
    assert(isPreviewable());
    assert(!previewRect.isEmpty());

    if (isPreviewReady())
    {
        const auto drawPixmap = [&p, &previewRect, origSize = OriginalPreviewSize_](const QPixmap& _pixmap)
        {
            if (Utils::isPanoramic(origSize))
            {
                if (origSize.height() > origSize.width())
                {
                    const auto r = previewRect.size().scaled(_pixmap.size(), Qt::KeepAspectRatio);
                    p.drawPixmap(previewRect, _pixmap, QRect(QPoint(), r));
                }
                else
                {
                    const auto r = previewRect.size().scaled(_pixmap.size(), Qt::KeepAspectRatio);
                    const auto x = (_pixmap.width() - r.width()) / 2;
                    p.drawPixmap(previewRect, _pixmap, QRect(QPoint(x, 0), r));
                }
            }
            else
            {
                p.drawPixmap(previewRect, _pixmap);
            }
        };

        if (!videoplayer_)
        {
            drawPixmap(Preview_);

            if (quote_color.isValid())
            {
                p.setBrush(QBrush(quote_color));
                p.drawRoundedRect(previewRect, Utils::scale_value(8), Utils::scale_value(8));
            }
        }
        else
        {
            if (!videoplayer_->isFullScreen())
            {
                if (!videoplayer_->getAttachedPlayer())
                {
                    if (!isSmallPreview())
                        videoplayer_->setClippingPath(_path);

                    if (videoplayer_->state() == QMovie::MovieState::Paused)
                        drawPixmap(videoplayer_->getActiveImage());
                }
                else
                {
                    drawPixmap(videoplayer_->getActiveImage());
                }
            }
        }
    }
    else if (!isSticker())
    {
        const auto brush = MessageStyle::Preview::getImagePlaceholderBrush();
        p.fillRect(previewRect, brush);
    }

    if (isSelected())
        p.fillRect(previewRect, MessageStyle::getSelectionColor());
}

void FileSharingBlock::drawPreviewableBlock(QPainter& _p, const QRect& _previewRect, const QColor& _quoteColor)
{
    if (!isSticker() && !isSmallPreview())
        applyClippingPath(_p, _previewRect);

    auto pRect = _previewRect;
    if (isSmallPreview() && isImage())
    {
        pRect = Utils::scale_value(Preview_.rect());
        pRect.moveCenter(_previewRect.center());
    }
    drawPreview(_p, pRect, RelativePreviewClippingPath_, _quoteColor);
}

void FileSharingBlock::initPlainFile()
{
    auto layout = new FileSharingPlainBlockLayout();
    setBlockLayout(layout);
    setLayout(layout);

    if (!plainButton_)
    {
        plainButton_ = new FileSharingIcon(this);
        plainButton_->raise();
        connect(plainButton_, &FileSharingIcon::clicked, this, &FileSharingBlock::onPlainFileIconClicked);
        connect(getParentComplexMessage(), &ComplexMessageItem::hoveredBlockChanged, this, [this]()
        {
            plainButton_->setHovered(getParentComplexMessage()->getHoveredBlock() == this);
            plainButton_->update();
        });
    }

    nameLabel_ = TextRendering::MakeTextUnit(QT_TRANSLATE_NOOP("contact_list", "File"), Data::MentionMap(), TextRendering::LinksVisible::DONT_SHOW_LINKS, TextRendering::ProcessLineFeeds::REMOVE_LINE_FEEDS);
    nameLabel_->init(MessageStyle::Files::getFilenameFont(), getNameColor(isSelected()));
}

void FileSharingBlock::initPreview()
{
    auto layout = new FileSharingImagePreviewBlockLayout();
    setBlockLayout(layout);
    setLayout(layout);

    if (shouldDisplayProgressAnimation())
    {
        auto resourceSet = &ActionButtonResource::ResourceSet::DownloadMediaFile_;

        if (isGifImage() || isVideo())
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
}

bool FileSharingBlock::isGifOrVideoPlaying() const
{
    assert(isGifImage() || isVideo());

    return (videoplayer_ && (videoplayer_->state() == QMovie::Running));
}

bool FileSharingBlock::drag()
{
    const auto pm = isPreviewable() ? Preview_ : (plainButton_ ? plainButton_->getIcon() : QPixmap());
    if (pm.isNull())
        return false;

    emit Utils::InterConnector::instance().clearSelecting();
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
    if (isGifImage() || isVideo())
    {
        if (!videoplayer_)
            videoplayer_.reset(createPlayer());

        videoplayer_->setMedia(getFileLocalPath());
    }
}

void FileSharingBlock::onLeftMouseClick(const QPoint &_pos)
{
    if (!rect().contains(_pos))
        return;

    if (isSticker())
    {
        emit Utils::InterConnector::instance().stopPttRecord();
        Stickers::showStickersPackByFileId(Id_, Stickers::StatContext::Chat);
        return;
    }

    if (previewButton_ && previewButton_->isWaitingForDelay())
        return;

    if (isImage())
    {
        Ui::GetDispatcher()->post_stats_to_core(core::stats::stats_event_names::fullmediascr_view, { { "chat_type", Utils::chatTypeByAimId(getChatAimid()) }, { "from", "chat" }, { "media_type", "photo" } });
        Utils::InterConnector::instance().getMainWindow()->openGallery(getChatAimid(), getLink(), getGalleryId());
        return;
    }

    if (isVideo() || isGifImage())
    {
        if (isFileDownloading())
            stopDataTransfer();
        else
            startDownloading(true);
    }

    if (isPlainFile())
    {
        if (isFileDownloaded())
        {
            if (plainButton_ && QRect(plainButton_->pos(), plainButton_->size()).contains(_pos))
                showFileInDir(Utils::OpenAt::Launcher);
            else
                showFileInDir(Utils::OpenAt::Folder);
        }
        else
        {
            startDownloading(true, false, true);
        }
    }
}

bool FileSharingBlock::isSticker() const noexcept
{
    return getType().is_sticker();
}

IItemBlock::ContentType FileSharingBlock::getContentType() const
{
    if (isSticker())
        return IItemBlock::ContentType::Sticker;

    return IItemBlock::ContentType::FileSharing;
}

void FileSharingBlock::parseLink()
{
    Id_ = extractIdFromFileSharingUri(getLink());
    assert(!Id_.isEmpty());

    if (isPreviewable())
    {
        OriginalPreviewSize_ = extractSizeFromFileSharingId(Id_);

        if (OriginalPreviewSize_.isEmpty())
            OriginalPreviewSize_ = getSmallPreviewSize();
    }
}

void FileSharingBlock::onStartClicked(const QPoint & _globalPos)
{
    onLeftMouseClick(mapFromGlobal(_globalPos));
}

void FileSharingBlock::onStopClicked(const QPoint &)
{
    stopDataTransfer();
    notifyBlockContentsChanged();
}

void FileSharingBlock::onPlainFileIconClicked()
{
    assert(plainButton_);
    const auto pos = plainButton_->pos() + plainButton_->rect().center();

    if (plainButton_->isAnimating())
        onStopClicked(pos);
    else
        onStartClicked(mapToGlobal(pos));

    MainPage::instance()->setFocusOnInput();
}

void FileSharingBlock::localPreviewLoaded(QPixmap pixmap, const QSize _originalSize)
{
    if (pixmap.isNull())
        return;

    OriginalPreviewSize_ = _originalSize;
    setPreview(pixmap);

    notifyBlockContentsChanged();
}

void FileSharingBlock::localDurationLoaded(qint64 _duration)
{
    duration_ = _duration;
    if (videoplayer_)
        videoplayer_->setDuration(_duration);
}

void FileSharingBlock::localGotAudioLoaded(bool _gotAudio)
{
    gotAudio_ = _gotAudio;
    if (videoplayer_)
        videoplayer_->setGotAudio(_gotAudio);
}

void FileSharingBlock::resizeEvent(QResizeEvent * _event)
{
    FileSharingBlockBase::resizeEvent(_event);

    if (videoplayer_)
        videoplayer_->updateSize(getFileSharingLayout()->getContentRect());

    if (plainButton_)
    {
        if (isStandalone())
            plainButton_->move(QPoint(0, getVerMargin()));
        else
            plainButton_->move(MessageStyle::Files::getCtrlIconLeftMargin(), MessageStyle::Files::getVerMargin());

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
        const auto verOffset = getVerMargin() + isStandalone() ? Utils::scale_value(4) : Utils::scale_value(12);
        nameLabel_->setOffsets(labelLeftOffset, verOffset);
        nameLabel_->elide(getMaxFilenameWidth());
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
}

void FileSharingBlock::requestPreview(const QString &uri)
{
    assert(isPreviewable());
    assert(PreviewRequestId_ == -1);

    PreviewRequestId_ = GetDispatcher()->downloadImage(uri, QString(), false, 0, 0, false);
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
    assert(isPlainFile());

    if (plainButton_)
    {
        if (const auto &filename = getFilename(); !filename.isEmpty())
            plainButton_->setFilename(filename);

        if (!getFileLocalPath().isEmpty())
            plainButton_->setDownloaded();
    }
}

void FileSharingBlock::onDataTransfer(const int64_t _bytesTransferred, const int64_t _bytesTotal)
{
    assert(_bytesTotal > 0);
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
    if (isGifImage() || isVideo())
    {
        if (previewButton_)
            previewButton_->setDefaultProgressText(HistoryControl::formatFileSize(getFileSize()));

        if (isVideo() && videoplayer_)
        {
            videoplayer_->setDuration(duration_);
            videoplayer_->setGotAudio(gotAudio_);
        }

        if (const auto localPath = getFileLocalPath(); !localPath.isEmpty() && QFile::exists(localPath))
        {
            if (!videoplayer_)
                videoplayer_.reset(createPlayer());

            videoplayer_->setMedia(localPath);
        }
        else if (isSticker() || isAutoplay())
        {
            startDownloading(false);
        }
    }
    else if (isPlainFile())
    {
        if (nameLabel_)
        {
            if (const auto& name = getFilename(); !name.isEmpty())
            {
                nameLabel_->setText(name);
                nameLabel_->elide(getMaxFilenameWidth());
            }
        }

        updateFileTypeIcon();
        notifyBlockContentsChanged();
    }
}

void FileSharingBlock::onImageDownloadError(qint64 seq, QString /*rawUri*/)
{
    if (PreviewRequestId_ != seq)
        return;

    PreviewRequestId_ = -1;

    if (isSticker())
        Preview_ = getGhost();

    onDownloadingFailed(seq);
}

FileSharingBlock::MenuFlags FileSharingBlock::getMenuFlags() const
{
    if (isSticker())
        return FileSharingBlock::MenuFlags::MenuFlagNone;

    return FileSharingBlockBase::getMenuFlags();
}

void FileSharingBlock::setSelected(const bool _isSelected)
{
    const auto prevSelected = isSelected();

    FileSharingBlockBase::setSelected(_isSelected);

    if (const auto sel = isSelected(); sel != prevSelected)
        updateStyle();
}

bool FileSharingBlock::isSmallPreview() const
{
    return isSmallPreview_;
}

const QPixmap &FileSharingBlock::getPreview() const
{
    return Preview_;
}

void FileSharingBlock::onImageDownloaded(int64_t seq, QString uri, QPixmap image, QString /*localPath*/)
{
    if (PreviewRequestId_ != seq)
        return;

    PreviewRequestId_ = -1;
    setPreview(image);

    update();
}

void FileSharingBlock::onImageDownloadingProgress(qint64 seq, int64_t bytesTotal, int64_t bytesTransferred, int32_t pctTransferred)
{
    Q_UNUSED(seq);
    Q_UNUSED(bytesTotal);
    Q_UNUSED(bytesTransferred);
}

void FileSharingBlock::onImageMetaDownloaded(int64_t seq, Data::LinkMetadata meta)
{
    assert(isPreviewable());

    Q_UNUSED(seq);
    Q_UNUSED(meta);
}

void FileSharingBlock::onGifImageVisibilityChanged(const bool isVisible)
{
    assert(isGifImage() || isVideo());

    if (videoplayer_ && (!isVisible || Logic::getContactListModel()->selectedContact() == getChatAimid()))
        videoplayer_->updateVisibility(isVisible);
}

void FileSharingBlock::onLocalCopyInfoReady(const bool isCopyExists)
{
    if (isCopyExists)
        update();
}

void FileSharingBlock::onPreviewMetainfoDownloaded(const QString &miniPreviewUri, const QString &fullPreviewUri)
{
    const auto &previewUri = fullPreviewUri.isEmpty() ? miniPreviewUri : fullPreviewUri;
    requestPreview(previewUri);
}

void FileSharingBlock::setQuoteSelection()
{
    GenericBlock::setQuoteSelection();
    emit setQuoteAnimation();
}

int FileSharingBlock::desiredWidth(int _width) const
{
    if (isSticker())
        return MessageStyle::Preview::getStickerMaxWidth();

    if (isPreviewable())
    {
        return getMaxPreviewWidth();
    }

    if (_width != 0)
        return getFileSharingLayout()->blockSizeForMaxWidth(_width).width();

    return MessageStyle::getFileSharingMaxWidth();
}

bool FileSharingBlock::isSizeLimited() const
{
    return !isPreviewable() || isSticker();
}

bool FileSharingBlock::updateFriendly(const QString&/* _aimId*/, const QString&/* _friendly*/)
{
    return false;
}

int FileSharingBlock::filenameDesiredWidth() const
{
    if (!nameLabel_)
        return 0;

    return nameLabel_->desiredWidth();
}

int FileSharingBlock::getMaxFilenameWidth() const
{
    auto w = width() - (plainButton_ ? plainButton_->width() : 0) - MessageStyle::Files::getHorMargin();

    if (!isStandalone())
        w -= MessageStyle::Files::getHorMargin() * 2;

    return w;
}

void FileSharingBlock::requestPinPreview()
{
    auto connMeta = std::make_shared<QMetaObject::Connection>();
    *connMeta = connect(GetDispatcher(),&core_dispatcher::fileSharingPreviewMetainfoDownloaded, this,
        [connMeta, this](qint64 _seq, auto miniPreviewUri, auto fullPreviewUri)
    {
        if (this->getPreviewMetaRequestId() == _seq)
        {
            this->onPreviewMetainfoDownloaded(miniPreviewUri, fullPreviewUri);

            this->clearPreviewMetaRequestId();
            disconnect(*connMeta);
        }
    });

    auto connFile = std::make_shared<QMetaObject::Connection>();
    *connFile = connect(GetDispatcher(), &core_dispatcher::imageDownloaded, this,
        [connFile, this](qint64 _seq, QString, QPixmap _preview, QString)
    {
        if (PreviewRequestId_ == _seq)
        {
            if (!_preview.isNull() && getParentComplexMessage())
                emit getParentComplexMessage()->pinPreview(_preview);

            PreviewRequestId_ = -1;
            disconnect(*connFile);
        }
    });

    requestMetainfo(true);
}

UI_COMPLEX_MESSAGE_NS_END
