#include "stdafx.h"

#include "../../../core_dispatcher.h"
#include "../../../main_window/MainWindow.h"
#include "../../../main_window/contact_list/ContactListModel.h"
#include "../../../previewer/GalleryWidget.h"
#include "../../../utils/InterConnector.h"
#include "../../../utils/LoadMovieFromFileTask.h"
#include "../../../utils/log/log.h"
#include "../../../utils/PainterPath.h"
#include "../../../utils/utils.h"
#include "../../../utils/stat_utils.h"
#include "../../../utils/Text2DocConverter.h"
#include "utils/medialoader.h"
#include "../../../gui_settings.h"
#include "../../../controls/TextUnit.h"

#include "../ActionButtonWidget.h"
#include "../FileSizeFormatter.h"
#include "../MessageStyle.h"

#include "ComplexMessageItem.h"
#include "FileSharingUtils.h"
#include "ImagePreviewBlockLayout.h"
#include "Selection.h"


#include "ImagePreviewBlock.h"

UI_COMPLEX_MESSAGE_NS_BEGIN

ImagePreviewBlock::ImagePreviewBlock(ComplexMessageItem *parent, const QString &imageUri, const QString &imageType)
    : GenericBlock(
          parent,
          imageUri,
          (MenuFlags)(MenuFlagFileCopyable | MenuFlagLinkCopyable | MenuFlagOpenInBrowser),
          true)
    , Layout_(nullptr)
    , ImageUri_(Utils::normalizeLink(QStringRef(&imageUri)).toString())
    , ImageType_(imageType)
    , PreviewDownloadSeq_(-1)
    , FullImageDownloadSeq_(-1)
    , isSmallPreview_(false)
    , IsSelected_(false)
    , Selection_(BlockSelectionType::None)
    , ActionButton_(nullptr)
    , CopyFile_(false)
    , MaxPreviewWidth_(MessageStyle::getPreviewDesiredWidth())
    , IsVisible_(false)
    , IsInPreloadDistance_(true)
    , ref_(new bool(false))
{
    assert(!ImageUri_.isEmpty());
    assert(!ImageType_.isEmpty());

    Layout_ = new ImagePreviewBlockLayout();
    setLayout(Layout_);

    QuoteAnimation_.setSemiTransparent();

    if (!getParentComplexMessage()->isHeadless() && MessageStyle::isShowLinksInImagePreview())
    {
        link_ = TextRendering::MakeTextUnit(imageUri);
        link_->init(MessageStyle::getImagePreviewLinkFont(), MessageStyle::getTextColor(), MessageStyle::getLinkColor(), MessageStyle::getSelectionColor(), MessageStyle::getHighlightColor());
    }
    setMouseTracking(true);
}

ImagePreviewBlock::~ImagePreviewBlock() = default;

void ImagePreviewBlock::selectByPos(const QPoint& from, const QPoint& to, const BlockSelectionType selection)
{
    assert(selection > BlockSelectionType::Min);
    assert(selection < BlockSelectionType::Max);
    assert(Selection_ > BlockSelectionType::Min);
    assert(Selection_ < BlockSelectionType::Max);

    if (MessageStyle::isShowLinksInImagePreview())
    {
        if (!link_)
            return;

        Selection_ = selection;

        const auto selectAll = (selection == BlockSelectionType::Full);
        if (selectAll)
        {
            link_->selectAll();
            return;
        }

        link_->select(mapFromGlobal(from), mapFromGlobal(to));
        update();
    }
    else
    {
        const QRect globalWidgetRect(
            mapToGlobal(rect().topLeft()),
            mapToGlobal(rect().bottomRight()));

        auto selectionArea(globalWidgetRect);
        selectionArea.setTop(from.y());
        selectionArea.setBottom(to.y());
        selectionArea = selectionArea.normalized();

        const auto selectionOverlap = globalWidgetRect.intersected(selectionArea);
        assert(selectionOverlap.height() >= 0);

        const auto widgetHeight = std::max(globalWidgetRect.height(), 1);
        const auto overlappedHeight = selectionOverlap.height();
        const auto overlapRatePercents = ((overlappedHeight * 100) / widgetHeight);
        assert(overlapRatePercents >= 0);

        const auto isSelected = (overlapRatePercents > 45);

        if (isSelected != IsSelected_)
        {
            IsSelected_ = isSelected;

            update();
        }
    }
}

const QPixmap &ImagePreviewBlock::getPreviewImage() const
{
    return Preview_;
}

void ImagePreviewBlock::clearSelection()
{
    if (MessageStyle::isShowLinksInImagePreview())
    {
        if (link_)
            link_->clearSelection();

        Selection_ = BlockSelectionType::None;
    }
    else
    {
        if (!IsSelected_)
        {
            return;
        }

        IsSelected_ = false;

        update();
    }
}

bool ImagePreviewBlock::hasActionButton() const
{
    return ActionButton_;
}

QPoint ImagePreviewBlock::getActionButtonLogicalCenter() const
{
    assert(ActionButton_);

    return ActionButton_->getLogicalCenter();
}

QSize ImagePreviewBlock::getActionButtonSize() const
{
    assert(ActionButton_);

    return ActionButton_->sizeHint();
}

IItemBlockLayout* ImagePreviewBlock::getBlockLayout() const
{
    return Layout_;
}

QSize ImagePreviewBlock::getPreviewSize() const
{
    assert (hasPreview());

    return Preview_.size();
}

QSize ImagePreviewBlock::getSmallPreviewSize() const
{
    return MessageStyle::Preview::getSmallPreviewSize((isGifPreview() || isVideoPreview()) ? MessageStyle::Preview::Option::video : MessageStyle::Preview::Option::image);
}

QSize ImagePreviewBlock::getMinPreviewSize() const
{
    return MessageStyle::Preview::getMinPreviewSize((isGifPreview() || isVideoPreview()) ? MessageStyle::Preview::Option::video : MessageStyle::Preview::Option::image);
}

QString ImagePreviewBlock::getSelectedText(const bool, const TextDestination) const
{
    if (MessageStyle::isShowLinksInImagePreview() && link_)
    {
        assert(!ImageUri_.isEmpty());

        switch (Selection_)
        {
        case BlockSelectionType::Full:
            return ImageUri_;

        case BlockSelectionType::TillEnd:
            return (link_->getSelectedText()/* + TrailingSpaces_*/);

        default:
            break;
        }

        return link_->getSelectedText();
    }
    else
    {
        assert(!ImageUri_.isEmpty());

        if (IsSelected_)
            return getSourceText();
    }
    return QString();
}

void ImagePreviewBlock::hideActionButton()
{
    assert(ActionButton_);

    ActionButton_->setVisible(false);
}

bool ImagePreviewBlock::hasPreview() const
{
    return !Preview_.isNull();
}

bool ImagePreviewBlock::isBubbleRequired() const
{
    return isSmallPreview();
}

bool ImagePreviewBlock::isSelected() const
{
    return IsSelected_;
}

void ImagePreviewBlock::onVisibilityChanged(const bool isVisible)
{
    IsVisible_ = isVisible;

    GenericBlock::onVisibilityChanged(isVisible);
    if (ActionButton_)
        ActionButton_->onVisibilityChanged(isVisible);

    if (isVisible && (PreviewDownloadSeq_ > 0))
    {
        GetDispatcher()->raiseDownloadPriority(getChatAimid(), PreviewDownloadSeq_);
    }

    if (isGifPreview())
    {
        qDebug() << "onGifVisibilityChanged";

        onGifVisibilityChanged(isVisible);
    }
}

bool ImagePreviewBlock::isInPreloadRange(const QRect& _widgetAbsGeometry, const QRect& _viewportVisibilityAbsRect)
{
    auto intersected = _viewportVisibilityAbsRect.intersected(_widgetAbsGeometry);

    if (intersected.height() != 0)
        return true;

    return std::min(abs(_viewportVisibilityAbsRect.y() - _widgetAbsGeometry.y())
                    , abs(_viewportVisibilityAbsRect.bottom() - _widgetAbsGeometry.bottom())) < 1000;
}

void ImagePreviewBlock::stopDownloading()
{
    stopDownloadingAnimation();

    GetDispatcher()->cancelLoaderTask(ImageUri_);
    FullImageDownloadSeq_ = -1;

    notifyBlockContentsChanged();
}

bool ImagePreviewBlock::isAutoPlay() const
{
    return get_gui_settings()->get_value<bool>(settings_autoplay_gif, true);
}

bool ImagePreviewBlock::isSmallPreview() const
{
    return isSmallPreview_;
}

void ImagePreviewBlock::cancelRequests()
{
    if (!GetDispatcher())
        return;

    GetDispatcher()->cancelLoaderTask(ImageUri_);
    GetDispatcher()->cancelLoaderTask(DownloadUri_);
}

void ImagePreviewBlock::updateStyle()
{
    if (link_)
    {
        link_->init(MessageStyle::getImagePreviewLinkFont(), MessageStyle::getTextColor(), MessageStyle::getLinkColor(), MessageStyle::getSelectionColor(), MessageStyle::getHighlightColor());
        link_->evaluateDesiredSize();
        update();
    }
}

void ImagePreviewBlock::updateFonts()
{
    updateStyle();
}

DialogPlayer *ImagePreviewBlock::createPlayer()
{
    std::underlying_type_t<Ui::DialogPlayer::Flags> flags = Ui::DialogPlayer::Flags::enable_dialog_controls | Ui::DialogPlayer::Flags::is_gif;

    auto player  = new Ui::DialogPlayer(this, flags, Preview_);

    connect(player, &DialogPlayer::openGallery, this, [this]()
    {
        Ui::GetDispatcher()->post_stats_to_core(core::stats::stats_event_names::fullmediascr_view, { { "chat_type", Utils::chatTypeByAimId(getChatAimid()) },{ "from", "chat" },{ "media_type", "gif" } });
        Utils::InterConnector::instance().getMainWindow()->openGallery(getChatAimid(), DownloadUri_, getGalleryId(), videoPlayer_.get());
    });

    bool mute = true;
    int32_t volume = Ui::get_gui_settings()->get_value<int32_t>(setting_mplayer_volume, 100);

    player->setParent(this);

    player->setVolume(volume);
    player->setMute(mute);

    player->setReplay(Ui::get_gui_settings()->get_value(settings_autoplay_gif, true));
    player->setFillClient(Utils::isPanoramic(originSize_));

    player->updateSize(Layout_->getPreviewRect());
    player->updateVisibility(IsVisible_);
    player->show();

    return player;
}

void ImagePreviewBlock::onDistanceToViewportChanged(const QRect& _widgetAbsGeometry, const QRect& _viewportVisibilityAbsRect)
{
    auto isInPreload = isInPreloadRange(_widgetAbsGeometry, _viewportVisibilityAbsRect);
    if (IsInPreloadDistance_ == isInPreload)
    {
        return;
    }
    IsInPreloadDistance_ = isInPreload;

    GenericBlock::onDistanceToViewportChanged(_widgetAbsGeometry, _viewportVisibilityAbsRect);
}

void ImagePreviewBlock::setMaxPreviewWidth(int width)
{
    MaxPreviewWidth_ = width;
}

int ImagePreviewBlock::getMaxPreviewWidth() const
{
    return MaxPreviewWidth_;
}

void ImagePreviewBlock::showActionButton(const QRect &pos)
{
    assert(hasActionButton());

    if (!pos.isEmpty())
    {
        assert(pos.size() == ActionButton_->sizeHint());
        ActionButton_->setGeometry(pos);
    }

    const auto isActionButtonVisible = (isGifPreview() && !isFullImageDownloaded()) || !isGifPreview();
    if (isActionButtonVisible)
    {
        ActionButton_->setVisible(true);
        ActionButton_->raise();
    }
}

void ImagePreviewBlock::drawBlock(QPainter &p, const QRect& _rect, const QColor& quote_color)
{
    const bool isSingleBlock = isSingle();

    const auto& imageRect = Layout_->getPreviewRect();
    const auto& blockRect = Layout_->getBlockRect();
    const auto clipRect = (isSingleBlock ? blockRect : imageRect);

    if (!isSmallPreview())
    {
        const auto updateClippingPath = (PreviewClippingPathRect_.isEmpty() || PreviewClippingPathRect_ != clipRect);
        if (updateClippingPath)
        {
            PreviewClippingPathRect_ = clipRect;
            PreviewClippingPath_ = evaluateClippingPath(clipRect);

            auto relativePreviewRect = QRect(0, 0, clipRect.width(), clipRect.height());
            RelativePreviewClippingPath_ = evaluateClippingPath(relativePreviewRect);
        }

        p.setClipPath(PreviewClippingPath_);
    }

    if (!hasPreview())
    {
        drawEmptyBubble(p, imageRect);
    }
    else
    {
        auto previewRect = imageRect;
        if (isSmallPreview() && !isGifPreview() && !isVideoPreview())
        {
            previewRect = Utils::scale_value(Preview_.rect());
            previewRect.moveCenter(imageRect.center());
        }

        const auto drawPixmap = [&p, &previewRect, origSize = originSize_](const QPixmap& _pixmap)
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

        if (!videoPlayer_)
        {
            drawPixmap(Preview_);
        }
        else
        {
            if (!videoPlayer_->isFullScreen())
            {
                if (!videoPlayer_->getAttachedPlayer())
                {
                    if (!isSmallPreview())
                        videoPlayer_->setClippingPath(RelativePreviewClippingPath_);

                    if (videoPlayer_->state() == QMovie::MovieState::Paused)
                        drawPixmap(videoPlayer_->getActiveImage());
                }
                else
                {
                    drawPixmap(videoPlayer_->getActiveImage());
                }
            }
        }

    }

    if (IsSelected_)
        p.fillRect(imageRect, MessageStyle::getSelectionColor());

    if (quote_color.isValid() && !videoPlayer_)
        p.fillRect(imageRect, QBrush(quote_color));

    if (link_)
        link_->draw(p);

    GenericBlock::drawBlock(p, _rect, quote_color);
}

void ImagePreviewBlock::drawEmptyBubble(QPainter &p, const QRect &bubbleRect)
{
    p.fillRect(bubbleRect, MessageStyle::Preview::getImagePlaceholderBrush());
}

void ImagePreviewBlock::initialize()
{
    GenericBlock::initialize();

    connectSignals();

    assert(PreviewDownloadSeq_ == -1);
    PreviewDownloadSeq_ = Ui::GetDispatcher()->downloadImage(
        ImageUri_,
        QString(),
        true,
        MessageStyle::Preview::getImageWidthMax(),
        0,
        true);
}

void ImagePreviewBlock::mouseMoveEvent(QMouseEvent *event)
{
    event->ignore();

    const auto mousePos = event->pos();

    if (isOverImage(mousePos))
    {
        setCursor(Qt::PointingHandCursor);

        return GenericBlock::mouseMoveEvent(event);
    }

    setCursor(Qt::ArrowCursor);

    return GenericBlock::mouseMoveEvent(event);
}

void ImagePreviewBlock::onMenuCopyFile()
{
    CopyFile_ = true;

    if (isFullImageDownloading())
    {
        return;
    }

    QUrl urlParser(ImageUri_);

    auto filename = urlParser.fileName();
    if (filename.isEmpty())
    {
        static const QRegularExpression re(
            qsl("[{}-]"),
            QRegularExpression::UseUnicodePropertiesOption | QRegularExpression::OptimizeOnFirstUsageOption
        );

        filename = QUuid::createUuid().toString();
        filename.remove(re);
    }

    const QString dstPath = QDir::temp().absolutePath() % ql1c('/') % filename;
    downloadFullImage(dstPath);
}

void ImagePreviewBlock::onMenuCopyLink()
{
    assert(!ImageUri_.isEmpty());

    QApplication::clipboard()->setText(ImageUri_);
}

void ImagePreviewBlock::onMenuSaveFileAs()
{
    assert(!ImageUri_.isEmpty());

    QUrl urlParser(ImageUri_);

    std::weak_ptr<bool> wr_ref = ref_;

    Utils::saveAs(urlParser.fileName(), [this, wr_ref](const QString& file, const QString& dir_result)
    {
        auto ref = wr_ref.lock();
        if (!ref)
            return;

        const auto addTrailingSlash = !dir_result.endsWith(ql1c('\\')) && !dir_result.endsWith(ql1c('/'));
        const auto slash = ql1s(addTrailingSlash ? "/" : "");
        const QString dir = dir_result % slash % file;

        auto saver = new Utils::FileSaver(this);
        saver->save([this, dir](bool _success)
        {
            if (_success)
                showFileSavedToast(dir);
            else
                showErrorToast();
        }, (DownloadUri_.isEmpty() ? ImageUri_ : DownloadUri_), dir);
    });
}

void ImagePreviewBlock::onMenuOpenInBrowser()
{
    QDesktopServices::openUrl(ImageUri_);
}

bool ImagePreviewBlock::drag()
{
    emit Utils::InterConnector::instance().clearSelecting();
    return Utils::dragUrl(this, Preview_, DownloadUri_);
}

void ImagePreviewBlock::connectSignals()
{
    const auto connType = Qt::UniqueConnection;
    connect(GetDispatcher(), &core_dispatcher::imageDownloaded,         this, &ImagePreviewBlock::onImageDownloaded, connType);
    connect(GetDispatcher(), &core_dispatcher::imageDownloadingProgress,this, &ImagePreviewBlock::onImageDownloadingProgress, connType);
    connect(GetDispatcher(), &core_dispatcher::imageMetaDownloaded,     this, &ImagePreviewBlock::onImageMetaDownloaded, connType);
    connect(GetDispatcher(), &core_dispatcher::imageDownloadError,      this, &ImagePreviewBlock::onImageDownloadError, connType);
}

void ImagePreviewBlock::downloadFullImage(const QString &destination)
{
    assert(FullImageDownloadSeq_ == -1);

    const auto &imageUri = (DownloadUri_.isEmpty() ? ImageUri_ : DownloadUri_);
    assert(!imageUri.isEmpty());

    const bool needContent = metaContentType_ == ql1s("image");
    FullImageDownloadSeq_ = Ui::GetDispatcher()->downloadImage(imageUri, destination, false, 0, 0, true, false, needContent);

    if (!shouldDisplayProgressAnimation())
    {
        return;
    }

    if (ActionButton_)
    {
        ActionButton_->startAnimation(1000);
    }

    notifyBlockContentsChanged();
}

QPainterPath ImagePreviewBlock::evaluateClippingPath(const QRect &imageRect) const
{
    assert(!imageRect.isEmpty());

    const auto header = getParentComplexMessage()->isHeaderRequired();

    int flags = header ? Utils::RenderBubbleFlags::BottomSideRounded : Utils::RenderBubbleFlags::AllRounded;
    if (isSingle())
    {
        if (!header && getParentComplexMessage()->isChainedToPrevMessage())
            flags |= (isOutgoing() ? Utils::RenderBubbleFlags::RightTopSmall : Utils::RenderBubbleFlags::LeftTopSmall);

        if (getParentComplexMessage()->isChainedToNextMessage())
            flags |= (isOutgoing() ? Utils::RenderBubbleFlags::RightBottomSmall : Utils::RenderBubbleFlags::LeftBottomSmall);
    }

    return Utils::renderMessageBubble(imageRect, Ui::MessageStyle::getBorderRadius(), MessageStyle::getBorderRadiusSmall(), (Utils::RenderBubbleFlags)flags);
}

void ImagePreviewBlock::initializeActionButton()
{
    assert(!Preview_.isNull());
    assert(!ImageType_.isEmpty());

    if (ActionButton_)
    {
        return;
    }

    const auto &resourceSet =
        [&]
        {
            if (isGifPreview())
            {
                return ActionButtonResource::ResourceSet::DownloadVideo_;
            }

            if (isVideoPreview())
            {
                return ActionButtonResource::ResourceSet::Play_;
            }

            return ActionButtonResource::ResourceSet::DownloadMediaFile_;
        }();

    __TRACE(
        "gif_preview",
        "initialized action button\n"
        "    uri=<" << ImageUri_ << ">\n"
        "    image_type=<" << ImageType_ << ">\n"
        "    is_gif=<" << logutils::yn(isGifPreview()) << ">\n"
        "    is_video=<" << logutils::yn(isVideoPreview()) << ">");

    ActionButton_ = new ActionButtonWidget(resourceSet, this);
    connect(ActionButton_, &ActionButtonWidget::startClickedSignal, this, &ImagePreviewBlock::onActionButtonClicked);
    connect(ActionButton_, &ActionButtonWidget::stopClickedSignal, this, &ImagePreviewBlock::onActionButtonClicked);
    connect(ActionButton_, &ActionButtonWidget::internallyChangedSignal, this, &ImagePreviewBlock::notifyBlockContentsChanged);
}

bool ImagePreviewBlock::isFullImageDownloaded() const
{
    return !FullImageLocalPath_.isEmpty();
}

bool ImagePreviewBlock::isFullImageDownloading() const
{
    assert(FullImageDownloadSeq_ >= -1);

    return (FullImageDownloadSeq_ > 0);
}

bool ImagePreviewBlock::isGifPreview() const
{
    if (ImageType_.isEmpty())
    {
        return false;
    }

    return ImageType_ == ql1s("gif");
}

bool ImagePreviewBlock::isOverImage(const QPoint &pos) const
{
    if (!hasPreview())
        return false;

    const auto &previewRect = Layout_->getPreviewRect();
    return previewRect.contains(pos);
}

bool ImagePreviewBlock::isVideoPreview() const
{
    if (ImageType_.isEmpty())
    {
        return false;
    }

    return Utils::is_video_extension(ImageType_);
}

void ImagePreviewBlock::onFullImageDownloaded(QPixmap image, const QString &localPath)
{
    assert(!localPath.isEmpty());

    FullImageLocalPath_ = localPath;

    stopDownloadingAnimation();

    if (!QFile::exists(localPath))
    {
        assert(!"file is missing");
        return;
    }

    if (CopyFile_)
    {
        CopyFile_ = false;
        Utils::copyFileToClipboard(localPath);
        showFileCopiedToast();
    }

    if (isGifPreview())
    {
        videoPlayer_.reset(createPlayer());
        videoPlayer_->setMedia(localPath);

        hideActionButton();
    }
}

void ImagePreviewBlock::onGifLeftMouseClick()
{
    assert(isGifPreview());

    if (!isFullImageDownloaded())
    {
        downloadFullImage(QString());
        return;
    }
}

void ImagePreviewBlock::onGifVisibilityChanged(const bool isVisible)
{
    assert(isGifPreview());

    if (videoPlayer_)
        videoPlayer_->updateVisibility(isVisible);
}

void ImagePreviewBlock::onLeftMouseClick(const QPoint &_pos)
{
    if (isGifPreview())
    {
        onGifLeftMouseClick();
    }
    else
    {
        Ui::GetDispatcher()->post_stats_to_core(core::stats::stats_event_names::fullmediascr_view, { { "chat_type", Utils::chatTypeByAimId(getChatAimid()) },{ "from", "chat" },{ "media_type", "photo" } });
        Utils::InterConnector::instance().getMainWindow()->openGallery(getChatAimid(), ImageUri_, getGalleryId());
    }
}

void ImagePreviewBlock::onPreviewImageDownloaded(QPixmap image, const QString &localPath)
{
    assert(!image.isNull());
    assert(Preview_.isNull());

    PreviewLocalPath_ = localPath;

    Preview_ = image;
    Utils::check_pixel_ratio(Preview_);

    if (isSingle() && hasPreview())
    {
        const auto small = getSmallPreviewSize();
        isSmallPreview_ =
            Preview_.width() / Utils::scale_bitmap_ratio() <= small.width() &&
            Preview_.height() / Utils::scale_bitmap_ratio() <= small.height();
    }

    if (isSmallPreview())
        getParentComplexMessage()->setCustomBubbleHorPadding(MessageStyle::Preview::getSmallPreviewHorPadding());

    initializeActionButton();

    notifyBlockContentsChanged();

    preloadFullImageIfNeeded();
}

void ImagePreviewBlock::preloadFullImageIfNeeded()
{
    if (!isGifPreview())
    {
        return;
    }

    if (isFullImageDownloading())
    {
        return;
    }

    downloadFullImage(QString());
}

bool ImagePreviewBlock::shouldDisplayProgressAnimation() const
{
    return true;
}

bool ImagePreviewBlock::isSingle() const
{
    return getParentComplexMessage()->isSimple();
}

void ImagePreviewBlock::stopDownloadingAnimation()
{
    if (isGifPreview() || isVideoPreview())
    {
        assert(ActionButton_);
        ActionButton_->stopAnimation();
    }
    else
    {
        delete ActionButton_;
        ActionButton_ = nullptr;
    }
}

void ImagePreviewBlock::onGifFrameUpdated(int /*frameNumber*/)
{
    assert(videoPlayer_);

    //const auto frame = Player_->currentPixmap();
   // Preview_ = frame;

    //update();
}

void ImagePreviewBlock::onImageDownloadError(qint64 seq, QString rawUri)
{
    const auto isPreviewSeq = (seq == PreviewDownloadSeq_);
    if (isPreviewSeq)
    {

        getParentComplexMessage()->replaceBlockWithSourceText(this);

        return;
    }

    const auto isFullImageSeq = (seq == FullImageDownloadSeq_);
    if (isFullImageSeq)
    {
        FullImageDownloadSeq_ = -1;

        update();

        stopDownloadingAnimation();
    }
}

void ImagePreviewBlock::onImageDownloaded(int64_t seq, QString, QPixmap image, QString localPath)
{
    const auto isPreviewSeq = (seq == PreviewDownloadSeq_);
    if (isPreviewSeq)
    {
        assert(!image.isNull());

        onPreviewImageDownloaded(image, localPath);

        PreviewDownloadSeq_ = -1;

        return;
    }

    const auto isFullImageSeq = (seq == FullImageDownloadSeq_);
    if (isFullImageSeq)
    {
        assert(!localPath.isEmpty());

        onFullImageDownloaded(image, localPath);

        FullImageDownloadSeq_ = -1;

        update();
    }
}

void ImagePreviewBlock::onImageDownloadingProgress(qint64 seq, int64_t bytesTotal, int64_t bytesTransferred, int32_t pctTransferred)
{
    assert(seq > 0);
    assert(bytesTotal > 0);
    assert(bytesTransferred >= 0);
    assert(pctTransferred >= 0);
    assert(pctTransferred <= 100);

    if (seq != FullImageDownloadSeq_)
    {
        return;
    }

    if (!ActionButton_)
    {
        return;
    }

    const auto normalizedPercents = ((double)pctTransferred / 100.0);

    ActionButton_->setProgress(normalizedPercents);

    const auto progressText = HistoryControl::formatProgressText(bytesTotal, bytesTransferred);
    assert(!progressText.isEmpty());

    ActionButton_->setProgressText(progressText);

    notifyBlockContentsChanged();
}

void ImagePreviewBlock::onImageMetaDownloaded(int64_t seq, Data::LinkMetadata meta)
{
    const auto isPreviewSeq = (seq == PreviewDownloadSeq_);
    if (!isPreviewSeq)
    {
        return;
    }

    DownloadUri_ = meta.getDownloadUri();
    originSize_ = meta.getOriginSize();
    metaContentType_ = meta.getContentType();
}

void ImagePreviewBlock::onActionButtonClicked(const QPoint& _globalPos)
{
    onLeftMouseClick(mapFromGlobal(_globalPos));
}

Ui::MediaType ImagePreviewBlock::getMediaType() const
{
    if (isVideoPreview())
        return Ui::MediaType::mediaTypeVideo;
    else if (isGifPreview())
        return Ui::MediaType::mediaTypeGif;

    return Ui::MediaType::mediaTypePhoto;
}

PinPlaceholderType ImagePreviewBlock::getPinPlaceholder() const
{
    return (isVideoPreview() || isGifPreview())
        ? PinPlaceholderType::Video
        : PinPlaceholderType::Image;
}

void ImagePreviewBlock::resizeEvent(QResizeEvent* _event)
{
    if (videoPlayer_)
    {
        const auto &imageRect = Layout_->getPreviewRect();

        videoPlayer_->updateSize(imageRect);
    }
    GenericBlock::resizeEvent(_event);
}

void ImagePreviewBlock::setQuoteSelection()
{
    emit setQuoteAnimation();
    GenericBlock::setQuoteSelection();
}

int ImagePreviewBlock::desiredWidth(int) const
{
    if (const auto maxSize = Layout_->getMaxPreviewSize(); maxSize.isValid())
        return maxSize.width();

    return getMaxPreviewWidth();
}

void ImagePreviewBlock::requestPinPreview()
{
    auto conn = std::make_shared<QMetaObject::Connection>();
    *conn = connect(GetDispatcher(), &core_dispatcher::linkMetainfoImageDownloaded, this,
        [conn, this](int64_t _seq, bool _success, QPixmap _image)
    {
        if (PreviewDownloadSeq_ == _seq)
        {
            if (_success && !_image.isNull() && getParentComplexMessage())
                emit getParentComplexMessage()->pinPreview(_image);

            PreviewDownloadSeq_ = -1;
            disconnect(*conn);
        }
    });

    PreviewDownloadSeq_ = GetDispatcher()->downloadLinkMetainfo(getChatAimid(), ImageUri_, 0, 0);
}

QString ImagePreviewBlock::formatRecentsText() const
{
    if (isVideoPreview())
        return QT_TRANSLATE_NOOP("contact_list", "Video");
    else if (isGifPreview())
        return QT_TRANSLATE_NOOP("contact_list", "GIF");

    return QT_TRANSLATE_NOOP("contact_list", "Photo");
}

bool ImagePreviewBlock::updateFriendly(const QString&/* _aimId*/, const QString&/* _friendly*/)
{
    return false;
}

bool ImagePreviewBlock::clicked(const QPoint& _p)
{
    QPoint mappedPoint = mapFromParent(_p, Layout_->getBlockGeometry());

    if (link_ && MessageStyle::isShowLinksInImagePreview())
        link_->clicked(mappedPoint);

    if (!isOverImage(mappedPoint))
        return true;

    onLeftMouseClick(mappedPoint);

    return true;
}

void ImagePreviewBlock::resetClipping()
{
    PreviewClippingPathRect_ = QRect();
}

UI_COMPLEX_MESSAGE_NS_END
