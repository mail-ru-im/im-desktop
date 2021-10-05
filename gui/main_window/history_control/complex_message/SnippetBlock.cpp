#include "stdafx.h"
#include "SnippetBlock.h"

#include "../../../core_dispatcher.h"
#include "controls/TextUnit.h"
#include "MediaUtils.h"
#include "../MessageStyle.h"
#include "styles/ThemeParameters.h"
#include "utils/PainterPath.h"
#include "utils/features.h"
#include "ComplexMessageItem.h"
#include "ComplexMessageUtils.h"
#include "../MessageStatusWidget.h"
#include "MediaControls.h"
#ifndef STRIP_AV_MEDIA
#include "../../mplayer/VideoPlayer.h"
#endif // !STRIP_AV_MEDIA
#include "utils/InterConnector.h"
#include "utils/stat_utils.h"
#include "main_window/MainWindow.h"
#include "../ActionButtonWidget.h"
#include "utils/medialoader.h"
#include "utils/BlurPixmapTask.h"
#include "../FileSizeFormatter.h"
#include "gui_settings.h"
#include "controls/FileSharingIcon.h"
#include "../SnippetCache.h"
#include "controls/TooltipWidget.h"
#include "previewer/toast.h"

#include "../../../common.shared/config/config.h"

namespace
{
    void postOpenGalleryStat(const QString& _aimId)
    {
        Ui::GetDispatcher()->post_stats_to_core(core::stats::stats_event_names::fullmediascr_view, {{"chat_type", Utils::chatTypeByAimId(_aimId)}, {"from", "chat"}, {"media_type", "video"}});
    }

    // takes link and if it is in form XXX/YYY.ZZZ returns YYY.ZZZ, otherwise returns string "File"
    QString fileNameFromLink(QStringView _link)
    {
        if (const auto lastSlashIndex = _link.lastIndexOf(u'/'); lastSlashIndex > 0)
        {
            auto lastPart = _link.mid(lastSlashIndex + 1);
            if (lastPart.contains(u'.') && !lastPart.startsWith(u'.') && !lastPart.endsWith(u'.'))
                return lastPart.toString();
        }

        return QT_TRANSLATE_NOOP("snippet_block", "File");
    }

    QPainterPath calcMediaClipPath(Ui::ComplexMessage::SnippetBlock* _block, const QRect& _contentRect)
    {
        const auto header = _block->getParentComplexMessage()->isHeaderRequired() || _block->isStandalone() && _block->isBubbleRequired();

        int flags = header ? Utils::RenderBubbleFlags::BottomSideRounded : Utils::RenderBubbleFlags::AllRounded;
        if (_block->isSimple() && !_block->isInsideQuote() && !_block->isInsideForward())
        {
            if (!header && _block->getParentComplexMessage()->isChainedToPrevMessage())
                flags |= (_block->isOutgoing() ? Utils::RenderBubbleFlags::RightTopSmall : Utils::RenderBubbleFlags::LeftTopSmall);

            if (_block->getParentComplexMessage()->isChainedToNextMessage())
                flags |= (_block->isOutgoing() ? Utils::RenderBubbleFlags::RightBottomSmall : Utils::RenderBubbleFlags::LeftBottomSmall);
        }

        return Utils::renderMessageBubble(_contentRect, Ui::MessageStyle::Preview::getBorderRadius(_block->isStandalone()), Ui::MessageStyle::getBorderRadiusSmall(), (Utils::RenderBubbleFlags)flags);
    }

    QString removeWWW(const QString& _link)
    {
        constexpr QStringView wwwDot = u"www.";

        if (_link.startsWith(wwwDot))
            return _link.mid(wwwDot.size());

        return _link;
    }

    QString formatLink(const QString& _link)
    {
        QUrl url(_link);
        return removeWWW(url.host()) + url.path();
    }

    QSize horizontalModePreviewSize()
    {
        return Utils::scale_value(QSize(48, 48));
    }

    const QColor& articlePreviewOverlayColor()
    {
        static auto color = QColor(0, 0, 0, 255 * 0.02);
        return color;
    }

    const int titleLineSpacing()
    {
        if constexpr (platform::is_apple())
            return 4;
        else
            return 0;
    }

    const int descriptionLineSpacing()
    {
        if constexpr (platform::is_apple())
            return 3;
        else
            return 0;
    }

    const auto locationTitle()
    {
        return QT_TRANSLATE_NOOP("snippet_block", "Location");
    }

    constexpr auto BLUR_RADIOUS = 150;

    void copyLink(const QString& _link)
    {
        QApplication::clipboard()->setText(_link);
        Utils::showCopiedToast();
    }
}


UI_COMPLEX_MESSAGE_NS_BEGIN

//////////////////////////////////////////////////////////////////////////
// SnippetBlock
//////////////////////////////////////////////////////////////////////////

SnippetBlock::SnippetBlock(ComplexMessageItem* _parent, Data::FStringView _link, const bool _hasLinkInMessage, EstimatedType _estimatedType)
    : GenericBlock(_parent, _link.toFString(), MenuFlagLinkCopyable, false)
    , link_(_link.string().toString())
    , hasLinkInMessage_(_hasLinkInMessage)
    , visible_(false)
    , estimatedType_(_estimatedType)
    , seq_(-1)
{
    Testing::setAccessibleName(this, u"AS HistoryPage messageSnippet " % QString::number(_parent->getId()));

    setMouseTracking(true);
}

SnippetBlock::SnippetBlock(ComplexMessageItem* _parent, const QString& _link, const bool _hasLinkInMessage, EstimatedType _estimatedType)
    : SnippetBlock(_parent, Data::FString(_link), _hasLinkInMessage, _estimatedType) { }

IItemBlockLayout* SnippetBlock::getBlockLayout() const
{
    return nullptr; // SnippetBlock doesn't use a layout
}

Data::FString SnippetBlock::getSelectedText(const bool _isFullSelect, const IItemBlock::TextDestination _dest) const
{
    if (isSelected())
        return getTextForCopy();

    return {};
}

QString SnippetBlock::getTextForCopy() const
{
    return shouldCopySource() ? getSourceText().string() : QString();
}

void SnippetBlock::selectByPos(const QPoint& from, const QPoint& to, bool topToBottom)
{
    if (shouldCopySource())
        GenericBlock::selectByPos(from, to, topToBottom);
}

QRect SnippetBlock::setBlockGeometry(const QRect& _rect)
{
    if (!content_)
        return _rect;

    const auto contentSize = content_->setContentSize(_rect.size());

    contentRect_ = QRect(_rect.topLeft(), contentSize);
    setGeometry(contentRect_);

    content_->onBlockSizeChanged();

    return contentRect_;
}

QRect SnippetBlock::getBlockGeometry() const
{
    return contentRect_;
}

int SnippetBlock::desiredWidth(int _width) const
{
    if (content_)
        return content_->desiredWidth(_width);

    return 0;
}

int SnippetBlock::getMaxWidth() const
{
    if (content_)
        return content_->maxWidth();

    return 0;
}

bool SnippetBlock::clicked(const QPoint& _p)
{
    if (!contentRect_.contains(_p))
        return false;

    return content_ && content_->clicked();
}

bool SnippetBlock::pressed(const QPoint& _p)
{
    if (!contentRect_.contains(_p))
        return false;

    return content_ && content_->pressed();
}

bool SnippetBlock::isBubbleRequired() const
{
    return content_ && content_->isBubbleRequired() && GenericBlock::isBubbleRequired();
}

bool SnippetBlock::isMarginRequired() const
{
    return content_ && content_->isMarginRequired();
}

bool SnippetBlock::isLinkMediaPreview() const
{
    return content_ && content_->mediaType() != MediaType::noMedia;
}

MediaType SnippetBlock::getMediaType() const
{
    if (content_)
        return content_->mediaType();

    switch (estimatedType_)
    {
        case EstimatedType::GIF:
            return MediaType::mediaTypeGif;
        case EstimatedType::Image:
            return MediaType::mediaTypePhoto;
        case EstimatedType::Video:
            return MediaType::mediaTypeVideo;
        case EstimatedType::Geo:
            return MediaType::mediaTypeGeo;

        default:
            break;
    }

    return MediaType::noMedia;
}

IItemBlock::ContentType SnippetBlock::getContentType() const
{
    return ContentType::Link;
}

void SnippetBlock::onVisibleRectChanged(const QRect& _visibleRect)
{
    visible_ = _visibleRect.height() >= (mediaVisiblePlayPercent * contentRect_.height());

    if (content_)
        content_->onVisibilityChanged(visible_);
}

bool SnippetBlock::hasLeadLines() const
{
    return true;
}

void SnippetBlock::highlight(const highlightsV& _hl)
{
    if (content_)
        content_->highlight(_hl);
}

void SnippetBlock::removeHighlight()
{
    if (content_)
        content_->removeHighlight();
}

PinPlaceholderType SnippetBlock::getPinPlaceholder() const
{
    switch (estimatedType_)
    {
        case EstimatedType::Image:
            return PinPlaceholderType::Image;
        case EstimatedType::GIF:
        case EstimatedType::Video:
            return PinPlaceholderType::Video;
        case EstimatedType::Article:
            return PinPlaceholderType::Link;
        default:
            break;
    }

    return PinPlaceholderType::None;
}

void SnippetBlock::requestPinPreview()
{
    if (!config::get().is_on(config::features::snippet_in_chat))
        return;

    auto conn = std::make_shared<QMetaObject::Connection>();
    *conn = connect(GetDispatcher(), &core_dispatcher::linkMetainfoImageDownloaded, this, [this, conn](int64_t _seq, bool _success, const QPixmap& _pixmap)
    {
        if (seq_ == _seq && _success)
            Q_EMIT getParentComplexMessage()->pinPreview(_pixmap);

        disconnect(*conn);
    });
    seq_ = GetDispatcher()->downloadLinkMetainfo(link_, core_dispatcher::LoadPreview::Yes, 0, 0, Utils::msgIdLogStr(getParentComplexMessage()->getId()));
}

QString SnippetBlock::formatRecentsText() const
{
    switch (estimatedType_)
    {
        case EstimatedType::Image:
            return QT_TRANSLATE_NOOP("contact_list", "Photo");
        case EstimatedType::Video:
            return QT_TRANSLATE_NOOP("contact_list", "Video");
        case EstimatedType::GIF:
            return QT_TRANSLATE_NOOP("contact_list", "GIF");
        case EstimatedType::Geo:
            return locationTitle();
        default:
            break;
    }

    return link_;
}

bool SnippetBlock::isBlockVisible() const
{
    return visible_;
}

bool SnippetBlock::managesTime() const
{
    return content_ && content_->managesTime();
}

void SnippetBlock::shiftHorizontally(const int _shift)
{
    GenericBlock::shiftHorizontally(_shift);
    contentRect_.translate(_shift, 0);
}

bool SnippetBlock::needStretchToOthers() const
{
    return content_ && content_->needStretchToOthers();
}

bool SnippetBlock::needStretchByShift() const
{
    return content_ && content_->needStretchByShift();
}

void SnippetBlock::resizeBlock(const QSize& _size)
{
    contentRect_.setSize(_size);

    if (content_)
        content_->onBlockSizeChanged();
}

bool SnippetBlock::canStretchWithSender() const
{
    return content_ && content_->canStretchWithSender();
}

const QString& SnippetBlock::getLink() const
{
    return link_;
}

const QPixmap& SnippetBlock::getPreviewImage() const
{
    if (content_)
        return content_->preview();

    const static auto empty = QPixmap();
    return empty;
}

void SnippetBlock::drawBlock(QPainter& _p, const QRect& _rect, const QColor& _quoteColor)
{
    if (content_)
        content_->draw(_p, QRect(QPoint(0, 0), contentRect_.size()));

    if (isSelected())
        _p.fillRect(rect(), MessageStyle::getSelectionColor());
}

void SnippetBlock::initialize()
{
    GenericBlock::initialize();

    if (link_.isEmpty())
        return;

    if (const auto meta = SnippetCache::instance()->get(link_))
    {
        initWithMeta(*meta);
    }
    else if (config::get().is_on(config::features::snippet_in_chat))
    {
        connect(GetDispatcher(), &core_dispatcher::linkMetainfoMetaDownloaded, this, &SnippetBlock::onLinkMetainfoMetaDownloaded);
        seq_ = GetDispatcher()->downloadLinkMetainfo(link_, core_dispatcher::LoadPreview::No, 0, 0, Utils::msgIdLogStr(getParentComplexMessage()->getId()));
        content_ = createPreloader();

        notifyBlockContentsChanged();
    }
}

void SnippetBlock::mouseMoveEvent(QMouseEvent* _event)
{
    setCursor(rect().contains(_event->pos()) ? Qt::PointingHandCursor : Qt::ArrowCursor);

    if (Features::longPathTooltipsAllowed() && content_ && content_->needsTooltip() && content_->isOverLink(_event->pos()))
    {
        if (!isTooltipActivated())
        {
            auto ttRect = content_->tooltipRect();
            ttRect.setX(0);
            ttRect.setWidth(width());
            auto isFullyVisible = visibleRegion().boundingRect().y() < ttRect.top();
            const auto arrowDir = isFullyVisible ? Tooltip::ArrowDirection::Down : Tooltip::ArrowDirection::Up;
            const auto arrowPos = isFullyVisible ? Tooltip::ArrowPointPos::Top : Tooltip::ArrowPointPos::Bottom;
            showTooltip(link_, QRect(mapToGlobal(ttRect.topLeft()), ttRect.size()), arrowDir, arrowPos);
        }
    }
    else
    {
        hideTooltip();
    }

    GenericBlock::mouseMoveEvent(_event);
}

void SnippetBlock::leaveEvent(QEvent *_event)
{
    hideTooltip();
    GenericBlock::leaveEvent(_event);
}

void SnippetBlock::onMenuCopyLink()
{
    copyLink(link_);
}

void SnippetBlock::onMenuCopyFile()
{
    if (content_)
        content_->onMenuCopyFile();
}

void SnippetBlock::onMenuSaveFileAs()
{
    if (content_)
        content_->onMenuSaveFileAs();
}

void SnippetBlock::onMenuOpenInBrowser()
{
    Utils::openUrl(link_);
}

bool SnippetBlock::drag()
{
    Q_EMIT Utils::InterConnector::instance().clearSelecting();

    const auto preview = content_ ? content_->preview() : QPixmap();
    return Utils::dragUrl(this, preview, link_);
}

IItemBlock::MenuFlags SnippetBlock::getMenuFlags(QPoint p) const
{
    int32_t flags = GenericBlock::getMenuFlags(p);

    if (content_)
        flags |= content_->menuFlags();

    return static_cast<IItemBlock::MenuFlags>(flags);
}

void SnippetBlock::onLinkMetainfoMetaDownloaded(int64_t _seq, bool _success, Data::LinkMetadata _meta)
{
    if (_seq != seq_)
        return;

    if (_success)
    {
        initWithMeta(_meta);
    }
    else
    {
        if (hasLinkInMessage_)
            getParentComplexMessage()->removeBlock(this);
        else
            getParentComplexMessage()->replaceBlockWithSourceText(this);
    }
}

void SnippetBlock::onContentLoaded()
{
    content_ = std::move(loadingContent_);
    if (content_)
        content_->showControls();

    notifyBlockContentsChanged();
    getParentComplexMessage()->updateTimeWidgetUnderlay();
}

void SnippetBlock::updateStyle()
{
    if (content_)
        content_->updateStyle();
}

void SnippetBlock::updateFonts()
{
    updateStyle();
}

void SnippetBlock::initWithMeta(const Data::LinkMetadata& _meta)
{
    auto content = createContent(_meta);

    if (content->needsPreload())
    {
        connect(content.get(), &SnippetContent::loaded, this, &SnippetBlock::onContentLoaded);
        loadingContent_ = std::move(content);
        if (loadingContent_)
            loadingContent_->hideControls();

        if (!content_)
        {
            content_ = createPreloader();
            notifyBlockContentsChanged();
        }
    }
    else
    {
        content_ = std::move(content);
        notifyBlockContentsChanged();
        getParentComplexMessage()->updateTimeWidgetUnderlay();
    }
}

std::unique_ptr<SnippetContent> SnippetBlock::createContent(const Data::LinkMetadata& _meta)
{
    switch (_meta.getContentType())
    {
        case Data::LinkContentType::Image:
        case Data::LinkContentType::Video:
        case Data::LinkContentType::Gif:
            return std::make_unique<MediaContent>(this, _meta, link_);
        case Data::LinkContentType::File:
            return std::make_unique<FileContent>(this, _meta, link_);
        default:
            if (estimatedType_ == EstimatedType::Geo)
                return std::make_unique<LocationContent>(this, _meta, link_);

            return std::make_unique<ArticleContent>(this, _meta, link_);
    }
}

std::unique_ptr<SnippetContent> SnippetBlock::createPreloader()
{
    switch (estimatedType_)
    {
        case EstimatedType::Image:
        case EstimatedType::Video:
        case EstimatedType::GIF:
            return std::make_unique<MediaPreloader>(this, link_);
        case EstimatedType::Geo:
        case EstimatedType::Article:
            return std::make_unique<ArticlePreloader>(this, link_);
    }

    return nullptr;
}

bool SnippetBlock::shouldCopySource() const
{
    return !hasLinkInMessage_ || isLinkMediaPreview();
}

//////////////////////////////////////////////////////////////////////////
// SnippetContent
//////////////////////////////////////////////////////////////////////////

SnippetContent::SnippetContent(SnippetBlock* _snippetBlock, const Data::LinkMetadata& _meta, const QString& _link)
    : snippetBlock_(_snippetBlock)
    , meta_(_meta)
    , link_(_link)
    , previewSeq_(-1)
    , faviconSeq_(-1)
{
    connect(GetDispatcher(), &core_dispatcher::imageDownloaded, this, &SnippetContent::onImageLoaded);
}

MediaType SnippetContent::mediaType() const
{
    return MediaType::noMedia;
}

int SnippetContent::desiredWidth(int _availableWidth) const
{
    Q_UNUSED(_availableWidth);
    return MessageStyle::Snippet::getMaxWidth();
}

int SnippetContent::maxWidth() const
{
    return MessageStyle::Snippet::getMaxWidth();
}

void SnippetContent::setPreview(const QPixmap& _preview)
{
    preview_ = MediaUtils::scaleMediaBlockPreview(_preview);
}

void SnippetContent::setFavicon(const QPixmap& _favicon)
{
    favicon_ = _favicon.scaled(Utils::scale_bitmap(MessageStyle::Snippet::getFaviconSize()), Qt::KeepAspectRatio, Qt::SmoothTransformation);
}

bool SnippetContent::needStretchToOthers() const
{
    return false;
}

bool SnippetContent::needStretchByShift() const
{
    return false;
}

void SnippetContent::loadPreview()
{
    if (!meta_.getPreviewUri().isEmpty())
        previewSeq_ = Ui::GetDispatcher()->downloadImage(meta_.getPreviewUri(), QString(), false, 0, 0, false);
}

void SnippetContent::loadFavicon()
{
    if (!meta_.getFaviconUri().isEmpty())
        faviconSeq_ = Ui::GetDispatcher()->downloadImage(meta_.getFaviconUri(), QString(), false, 0, 0, false);
}

void SnippetContent::onImageLoaded(qint64 _seq, const QString& _rawUri, const QPixmap& _image)
{
    Q_UNUSED(_rawUri)

    if (_seq == previewSeq_)
    {
        setPreview(_image);
        snippetBlock_->notifyBlockContentsChanged();
    }
    else if (_seq == faviconSeq_)
    {
        setFavicon(_image);
        snippetBlock_->notifyBlockContentsChanged();
    }
}

QRect SnippetContent::contentRect() const
{
    return QRect({0, 0}, snippetBlock_->getBlockGeometry().size());
}

//////////////////////////////////////////////////////////////////////////
// ArticleContent
//////////////////////////////////////////////////////////////////////////

ArticleContent::ArticleContent(SnippetBlock* _snippetBlock, const Data::LinkMetadata& _meta, const QString& _link)
    : SnippetContent(_snippetBlock, _meta, _link)
{
    titleUnit_ = TextRendering::MakeTextUnit(meta_.getTitle(), Data::MentionMap(), TextRendering::LinksVisible::SHOW_LINKS, TextRendering::ProcessLineFeeds::REMOVE_LINE_FEEDS);
    descriptionUnit_ = TextRendering::MakeTextUnit(meta_.getDescription(), Data::MentionMap(), TextRendering::LinksVisible::SHOW_LINKS, TextRendering::ProcessLineFeeds::REMOVE_LINE_FEEDS);
    linkUnit_ = TextRendering::MakeTextUnit(formatLink(link_));

    initTextUnits();

    if (isGifArticle() || isVideoArticle())
        createControls();

    loadPreview();
    loadFavicon();
}

ArticleContent::~ArticleContent() = default;

QSize ArticleContent::setContentSize(const QSize& _available)
{
    const auto availableWidth = std::min(_available.width(), MessageStyle::Snippet::getMaxWidth());
    const auto insideQuote = snippetBlock_->isInsideQuote();

    const auto topPadding = MessageStyle::Snippet::getPreviewTopPadding();
    auto currentY = insideQuote ? 0 : topPadding;
    auto xOffset = 0;

    QSize previewRectSize;
    if (!insideQuote && !meta_.getOriginSize().isEmpty())
    {
        if (isHorizontalMode())
        {
            previewRectSize = horizontalModePreviewSize();
            xOffset = previewRectSize.width() + MessageStyle::Snippet::getHorizontalModeImageRightMargin();
        }
        else
        {
            MediaUtils::MediaBlockParams params;
            params.mediaType =  MediaUtils::MediaType::Image;
            params.mediaSize = meta_.getOriginSize();
            params.availableWidth = availableWidth;

            previewRectSize = MediaUtils::calcMediaBlockSize(params);
            currentY += previewRectSize.height();
        }
    }

    if (!meta_.getTitle().isEmpty())
    {
        currentY += (isHorizontalMode() || insideQuote ? 0 : MessageStyle::Snippet::getTitleTopPadding());
        titleUnit_->setOffsets(xOffset, currentY);
        currentY += titleUnit_->getHeight(availableWidth - xOffset);
    }

    if (!insideQuote && !meta_.getDescription().isEmpty())
    {
        currentY += MessageStyle::Snippet::getDescriptionTopPadding();
        descriptionUnit_->setOffsets(xOffset, currentY);
        currentY += descriptionUnit_->getHeight(availableWidth - xOffset);
    }

    currentY += (isHorizontalMode() ? MessageStyle::Snippet::getHorizontalModeLinkTopPadding() : MessageStyle::Snippet::getLinkTopPadding());
    auto linkXOffset = xOffset;

    if (!favicon_.isNull())
    {
        faviconRect_ = QRect(QPoint(xOffset, currentY), MessageStyle::Snippet::getFaviconSize());
        linkXOffset += faviconRect_.width() + MessageStyle::Snippet::getLinkLeftPadding();
    }

    auto timeWidth = 0;
    if (const auto timeWidget = snippetBlock_->getParentComplexMessage()->getTimeWidget())
        timeWidth = timeWidget->width() + MessageStyle::getTimeLeftSpacing();

    const auto linkHeight = linkUnit_->getHeight(availableWidth - timeWidth - linkXOffset);

    auto linkYOffset = std::max(0, faviconRect_.height() - linkHeight) / 2;

    linkUnit_->setOffsets(linkXOffset, currentY + linkYOffset);
    currentY += std::max(linkHeight, faviconRect_.height());

    const auto maxTextWidth = std::max(std::max(descriptionUnit_->desiredWidth() + xOffset, linkUnit_->desiredWidth() + linkXOffset + timeWidth), titleUnit_->desiredWidth() + xOffset);
    const auto desiredWidth = std::max(previewRectSize.width(), maxTextWidth);
    const auto blockWidth = std::max(std::min(desiredWidth, availableWidth),  MessageStyle::Snippet::getMinWidth());

    if (!insideQuote && !meta_.getOriginSize().isEmpty())
    {
        if (isHorizontalMode())
        {
            previewRect_ = QRect(QPoint(0, topPadding), previewRectSize);
        }
        else
        {
            if (MediaUtils::isNarrowMedia(meta_.getOriginSize()))
                previewRect_ = QRect(0, topPadding, blockWidth, previewRectSize.height());
            else
                previewRect_ = QRect(QPoint((blockWidth - previewRectSize.width()) / 2, topPadding), previewRectSize);
        }
    }

    if (controls_)
        controls_->setRect(previewRect_);

    if (isHorizontalMode())
        currentY = std::max(currentY, previewRect_.height() + topPadding); // preview's height can be bigger than total text's height

    return QSize(blockWidth, currentY);
}

void ArticleContent::draw(QPainter& _p, const QRect& _rect)
{
    Q_UNUSED(_rect)

    Utils::PainterSaver saver(_p);
    _p.setRenderHint(QPainter::Antialiasing);

    const auto insideQuote = snippetBlock_->isInsideQuote();

    if (!preview_.isNull() && !insideQuote)
    {
        Utils::PainterSaver previewSaver(_p);
        _p.setClipPath(Utils::renderMessageBubble(previewRect_, Ui::MessageStyle::Preview::getInternalBorderRadius()));

        if (isHorizontalMode())
            _p.drawPixmap(previewRect_, preview_);
        else
            MediaUtils::drawMediaInRect(_p, previewRect_, preview_, meta_.getOriginSize());
    }

    if (!favicon_.isNull())
        _p.drawPixmap(faviconRect_, favicon_);

    titleUnit_->draw(_p);

    if (!insideQuote)
        descriptionUnit_->draw(_p);

    linkUnit_->draw(_p);
}

void ArticleContent::onBlockSizeChanged()
{
    if (controls_)
        controls_->setRect(previewRect_);
}

bool ArticleContent::clicked()
{
    Utils::openUrl(link_);
    return true;
}

bool ArticleContent::pressed()
{
    return true;
}

bool ArticleContent::isBubbleRequired() const
{
    return true;
}

bool ArticleContent::isMarginRequired() const
{
    return true;
}

void ArticleContent::setPreview(const QPixmap& _preview)
{
    if (isHorizontalMode())
        preview_ = _preview.scaled(Utils::scale_bitmap(horizontalModePreviewSize()), Qt::KeepAspectRatio, Qt::SmoothTransformation);
    else
        SnippetContent::setPreview(_preview);

    // draw overlay
    QPainter p(&preview_);
    p.fillRect(QRect(QPoint(0, 0), preview_.size()), articlePreviewOverlayColor());
}

void ArticleContent::highlight(const highlightsV& _hl)
{
    titleUnit_->setHighlighted(_hl);
    descriptionUnit_->setHighlighted(_hl);
    linkUnit_->setHighlighted(_hl);
}

void ArticleContent::removeHighlight()
{
    titleUnit_->setHighlighted(false);
    descriptionUnit_->setHighlighted(false);
    linkUnit_->setHighlighted(false);
}

void ArticleContent::updateStyle()
{
    initTextUnits();

    snippetBlock_->notifyBlockContentsChanged();
}

IItemBlock::MenuFlags ArticleContent::menuFlags() const
{
    return IItemBlock::MenuFlagNone;
}

void ArticleContent::showControls()
{
    if (controls_)
        controls_->show();
}

void ArticleContent::hideControls()
{
    if (controls_)
        controls_->hide();
}

bool ArticleContent::isOverLink(const QPoint& _p) const
{
    return linkUnit_ && linkUnit_->contains(_p);
}

bool ArticleContent::needsTooltip() const
{
    return linkUnit_ && linkUnit_->isElided();
}

QRect ArticleContent::tooltipRect() const
{
    return linkUnit_ ? QRect(linkUnit_->offsets(), linkUnit_->cachedSize()) : QRect();
}

bool ArticleContent::isGifArticle() const
{
    return meta_.getContentType() == Data::LinkContentType::ArticleGif;
}

bool ArticleContent::isVideoArticle() const
{
    return meta_.getContentType() == Data::LinkContentType::ArticleVideo;
}

void ArticleContent::createControls()
{
    std::set<MediaControls::ControlType> controls;
    controls.insert(MediaControls::Play);

    if (isGifArticle())
        controls.insert(MediaControls::GifLabel);
    else
        controls.insert(MediaControls::Duration);

    uint32_t options = MediaControls::ShowPlayOnPreview;

    if (snippetBlock_->isInsideQuote())
        options |= MediaControls::CompactMode;

    controls_ = new MediaControls(controls, options, snippetBlock_);

    connect(controls_, &MediaControls::clicked, this, &ArticleContent::clicked);

    controls_->setState(MediaControls::State::Preview);
    controls_->show();
    controls_->raise();
}

void ArticleContent::initTextUnits()
{
    const auto titleLines = (snippetBlock_->isInsideQuote() || isHorizontalMode() ? 1 : 3);

    titleUnit_->init(MessageStyle::Snippet::getTitleFont(), MessageStyle::Snippet::getTitleColor(), MessageStyle::Snippet::getTitleColor(),
                     MessageStyle::getSelectionColor(), MessageStyle::getHighlightColor(), TextRendering::HorAligment::LEFT, titleLines, TextRendering::LineBreakType::PREFER_SPACES);

    titleUnit_->setLineSpacing(titleLineSpacing());

    descriptionUnit_->init(MessageStyle::Snippet::getDescriptionFont(), MessageStyle::Snippet::getDescriptionColor(), MessageStyle::Snippet::getDescriptionColor(),
                           QColor(), MessageStyle::getHighlightColor(), TextRendering::HorAligment::LEFT, 5, TextRendering::LineBreakType::PREFER_SPACES);

    descriptionUnit_->setLineSpacing(descriptionLineSpacing());

    const auto outgoing = snippetBlock_->isOutgoing();
    linkUnit_->init(MessageStyle::Snippet::getLinkFont(), MessageStyle::Snippet::getLinkColor(outgoing), MessageStyle::Snippet::getLinkColor(outgoing),
                    QColor(), MessageStyle::getHighlightColor(), TextRendering::HorAligment::LEFT, 1, TextRendering::LineBreakType::PREFER_SPACES);

    titleUnit_->setHighlightedTextColor(MessageStyle::getHighlightTextColor());
    descriptionUnit_->setHighlightedTextColor(MessageStyle::getHighlightTextColor());
    linkUnit_->setHighlightedTextColor(MessageStyle::getHighlightTextColor());
}

// horizontal mode is a mode when preview is drawn on the left of the text units,
// it is used when preview is not superwide or narrow and both its dimensions are less than 144 px
bool ArticleContent::isHorizontalMode() const
{
    static const auto smallPreviewThreshold = Utils::scale_bitmap_with_value(144);

    const auto previewSize = meta_.getPreviewSize();
    const auto wideOrNarrow = MediaUtils::isWideMedia(previewSize) || MediaUtils::isNarrowMedia(previewSize);
    const auto lessThanThreshold = previewSize.width() < smallPreviewThreshold && previewSize.height() < smallPreviewThreshold;
    const auto isPlayableArticle = isGifArticle() || isVideoArticle();

    return lessThanThreshold && !wideOrNarrow && !isPlayableArticle;
}

//////////////////////////////////////////////////////////////////////////
// MediaContent
//////////////////////////////////////////////////////////////////////////

MediaContent::MediaContent(SnippetBlock* _snippetBlock, const Data::LinkMetadata& _meta, const QString& _link)
    : SnippetContent(_snippetBlock, _meta, _link),
      fileLoaded_(false)
{
    if (isPlayable())
        initPlayableMedia();
    else if (!_snippetBlock->isInsideQuote())
        createControls();

    loadPreview();
    loadFavicon();
}

MediaContent::~MediaContent() = default;

QSize MediaContent::setContentSize(const QSize& _available)
{
    return calcContentSize(_available.width());
}

void MediaContent::draw(QPainter& _p, const QRect& _rect)
{
    Utils::PainterSaver saver(_p);

    _p.setClipPath(clipPath_);

    if (!background_.isNull())
    {
        auto bRect = background_.rect();
        if (bRect.width() > bRect.height())
        {
            const auto diff = bRect.width() - _rect.width();
            bRect.setX(diff / 2);
            bRect.setWidth(_rect.width());
        }
        else
        {
            const auto diff = bRect.height() - _rect.height();
            bRect.setY(diff / 2);
            bRect.setHeight(_rect.height());
        }

        _p.drawPixmap(_rect, background_, bRect);
    }

    if (!preview_.isNull()
#ifndef STRIP_AV_MEDIA
        && !player_
#endif // !STRIP_AV_MEDIA
    )
    {
        MediaUtils::drawMediaInRect(_p, _rect, preview_, originSizeScaled(), background_.isNull() ? MediaUtils::BackgroundMode::Auto : MediaUtils::BackgroundMode::NoBackground);
    }

}

void MediaContent::onBlockSizeChanged()
{
    updateButtonGeometry();

    clipPath_ = calcMediaClipPath(snippetBlock_, contentRect());

#ifndef STRIP_AV_MEDIA
    if (player_)
    {
        player_->updateSize(contentRect());
        player_->setClippingPath(clipPath_);
    }
#endif // !STRIP_AV_MEDIA

    if (controls_)
        controls_->setRect(contentRect());

    if (!background_.isNull())
    {
        const auto& r = contentRect();
        if (r.width() >= r.height())
            background_ = background_.scaledToHeight(r.height());
        else
            background_ = background_.scaledToWidth(r.width());
    }
}

bool MediaContent::clicked()
{
    if (!fileLoaded_ && isPlayable())
    {
        if (!loader_)
            loadMedia();
        else
            stopLoading();
    }
    else if (fileLoaded_ || !isPlayable())
    {
#ifndef STRIP_AV_MEDIA
        postOpenGalleryStat(snippetBlock_->getChatAimid());
        auto data = Utils::GalleryData(snippetBlock_->getChatAimid(), snippetBlock_->getLink(), snippetBlock_->getGalleryId(), player_);
        if (auto msg = snippetBlock_->getParentComplexMessage())
            msg->fillGalleryData(data);

        Utils::InterConnector::instance().openGallery(data);
#endif // !STRIP_AV_MEDIA
    }

    return true;
}

bool MediaContent::pressed()
{
    return true;
}

bool MediaContent::isBubbleRequired() const
{
    const auto blockSize = contentRect().size();
    const auto smallerThanBlock = MediaUtils::isMediaSmallerThanBlock(blockSize, originSizeScaled());
    const auto smallerAndNotWideOrNarrow = smallerThanBlock && !(MediaUtils::isNarrowMedia(originSizeScaled()) || MediaUtils::isWideMedia(originSizeScaled()));

    return smallerAndNotWideOrNarrow || MediaUtils::isSmallMedia(originSizeScaled()) || !snippetBlock_->isStandalone();
}

bool MediaContent::isMarginRequired() const
{
    return !snippetBlock_->isStandalone() && !MediaUtils::isSmallMedia(originSizeScaled());
}

MediaType MediaContent::mediaType() const
{
    if (isVideo())
        return MediaType::mediaTypeVideo;
    else if (isGif())
        return MediaType::mediaTypeGif;
    else
        return MediaType::mediaTypePhoto;
}

int MediaContent::desiredWidth(int _availableWidth) const
{
    return calcContentSize(_availableWidth).width();
}

int MediaContent::maxWidth() const
{
    return MediaUtils::mediaBlockMaxWidth(originSizeScaled());
}

void MediaContent::onVisibilityChanged(const bool _visible)
{
#ifndef STRIP_AV_MEDIA
    if (player_)
        player_->updateVisibility(_visible);
#endif // !STRIP_AV_MEDIA
}

void MediaContent::setPreview(const QPixmap& _preview)
{
    SnippetContent::setPreview(_preview);

    background_ = QPixmap();
    if (isBubbleRequired() || MediaUtils::isNarrowMedia(originSizeScaled()) || MediaUtils::isWideMedia(originSizeScaled()))
    {
        backgroundSeq_ = _preview.cacheKey();

        auto task = new Utils::BlurPixmapTask(_preview, BLUR_RADIOUS);
        connect(task, &Utils::BlurPixmapTask::blurred, this, &MediaContent::prepareBackground);
        QThreadPool::globalInstance()->start(task);
    }

    Q_EMIT loaded();

#ifndef STRIP_AV_MEDIA
    if (player_)
        player_->setPreview(_preview);
#endif // !STRIP_AV_MEDIA
}

void MediaContent::setFavicon(const QPixmap& _favicon)
{
    SnippetContent::setFavicon(_favicon);

#ifndef STRIP_AV_MEDIA
    if (player_)
        player_->setFavIcon(_favicon);
#endif // !STRIP_AV_MEDIA
}

void MediaContent::onMenuCopyFile()
{
    auto saver = new Utils::FileSaver(this);
    saver->save([](bool _success, const QString& _savedPath)
    {
        if (_success)
        {
            Utils::copyFileToClipboard(_savedPath);
            Utils::showCopiedToast();
        }
        else
        {
            GenericBlock::showErrorToast();
        }
    }, meta_.getDownloadUri(), QString()/*download path*/, false/*export as png*/);
}

void MediaContent::onMenuSaveFileAs()
{
    QUrl urlParser(meta_.getDownloadUri());

    QString fileName = urlParser.fileName();
    const QString suffix = QFileInfo(fileName).suffix();
    if (suffix.isEmpty())
    {
        fileName += u'.';
        if (isGif())
            fileName += ql1s("gif");
        else
            fileName += meta_.getFileFormat();
    }

    Utils::saveAs(fileName, [this, guard = QPointer(this)](const QString& _file, const QString& _dirResult, bool _exportAsPng)
    {
        if (!guard)
            return;

        const auto destinationFile = QFileInfo(_dirResult, _file).absoluteFilePath();
        auto saver = new Utils::FileSaver(this);
        saver->save([destinationFile](bool _success, const QString& _savedPath)
        {
            Q_UNUSED(_savedPath)

            if (_success)
                GenericBlock::showFileSavedToast(destinationFile);
            else
                GenericBlock::showErrorToast();
        }, meta_.getDownloadUri(), destinationFile, _exportAsPng);
    });
}

IItemBlock::MenuFlags MediaContent::menuFlags() const
{
    return static_cast<IItemBlock::MenuFlags>(IItemBlock::MenuFlagCopyable | IItemBlock::MenuFlagFileCopyable | IItemBlock::MenuFlagOpenInBrowser);
}

bool MediaContent::managesTime() const
{
    return false;
}

void MediaContent::showControls()
{
    if (controls_)
        controls_->show();

#ifndef STRIP_AV_MEDIA
    if (player_)
        player_->show();
#endif // !STRIP_AV_MEDIA

    if (button_ && !fileLoaded_)
        button_->show();
}

void MediaContent::hideControls()
{
    if (controls_)
        controls_->hide();

#ifndef STRIP_AV_MEDIA
    if (player_)
        player_->hide();
#endif // !STRIP_AV_MEDIA

    if (button_)
        button_->hide();
}

bool MediaContent::needStretchToOthers() const
{
    return (!snippetBlock_->isInsideQuote() || MediaUtils::isNarrowMedia(originSizeScaled()));
}

bool MediaContent::needStretchByShift() const
{
    return !snippetBlock_->isInsideQuote() && !MediaUtils::isNarrowMedia(originSizeScaled());
}

bool MediaContent::canStretchWithSender() const
{
    return false;
}

void MediaContent::onProgress(int64_t _bytesTotal, int64_t _bytesTransferred)
{
    if (button_)
    {
        if (const auto text = HistoryControl::formatProgressText(_bytesTotal, _bytesTransferred); !text.isEmpty())
        {
            const auto progress = ((double)_bytesTransferred / (double)_bytesTotal);
            button_->setProgress(progress);

            if (isProgressVisible())
                button_->setProgressText(text);

            updateButtonGeometry();
        }
    }
}

void MediaContent::onLoaded(const QString& _path)
{
    if (isPlayable())
    {
#ifndef STRIP_AV_MEDIA
        if (player_)
            player_->setMedia(_path);
#endif // !STRIP_AV_MEDIA

        if (button_)
            button_->hide();

        fileLoaded_ = true;
    }
}

void MediaContent::onError()
{
    if (button_)
        button_->stopAnimation();

    if (auto loader = loader_.release())
        loader->deleteLater();
}

void MediaContent::onFilePath(int64_t _seq, const QString& _path)
{
    if (seq_ == _seq && !_path.isEmpty())
        onLoaded(_path);
}

void MediaContent::prepareBackground(const QPixmap& _result, qint64 _srcCacheKey)
{
    if (_srcCacheKey != backgroundSeq_ || _result.isNull())
        return;

    background_ = _result;
    {
        auto color = Styling::getParameters().getColor(Styling::StyleVariable::CHAT_SECONDARY_MEDIA, 0.15);
        QPainter p(&background_);
        p.fillRect(background_.rect(), color);
    }


    const auto& r = contentRect();
    if (r.width() >= r.height())
        background_ = background_.scaledToHeight(r.height());
    else
        background_ = background_.scaledToWidth(r.width());
}

QSize MediaContent::calcContentSize(int _availableWidth) const
{
    MediaUtils::MediaBlockParams params;

    if (isVideo())
        params.mediaType =  MediaUtils::MediaType::Video;
    else if (isGif())
        params.mediaType =  MediaUtils::MediaType::Gif;
    else
        params.mediaType =  MediaUtils::MediaType::Image;

    params.mediaSize = originSizeScaled();
    params.availableWidth = _availableWidth;
    params.isInsideQuote = snippetBlock_->isInsideQuote();
    params.minBlockWidth = minControlsWidth();

    return MediaUtils::calcMediaBlockSize(params);
}

int MediaContent::minControlsWidth() const
{
    if (snippetBlock_->isInsideQuote())
        return 0;

    int controlsWidth = 0;
#ifndef STRIP_AV_MEDIA
    if (player_)
        controlsWidth += player_->bottomLeftControlsWidth();
#endif // !STRIP_AV_MEDIA

    if (controls_)
        controlsWidth += controls_->bottomLeftControlsWidth();

    const auto timeWidget = snippetBlock_->getParentComplexMessage()->getTimeWidget();
    if (timeWidget && snippetBlock_->isStandalone())
        controlsWidth += timeWidget->width() + timeWidget->getHorMargin();

    return controlsWidth + MessageStyle::getTimeLeftSpacing();
}

QSize MediaContent::originSizeScaled() const
{
    return Utils::scale_value(meta_.getOriginSize());
}

void MediaContent::updateButtonGeometry()
{
    if (!button_)
        return;

    const auto contentSize = contentRect().size();
    const auto buttonSize = button_->sizeHint();
    const auto buttonRect = QRect((contentSize.width() - buttonSize.width()) / 2, (contentSize.height() - buttonSize.height()) / 2, buttonSize.width(), buttonSize.height());

    button_->setGeometry(buttonRect.translated(-button_->getCenterBias()));
}

#ifndef STRIP_AV_MEDIA
DialogPlayer* MediaContent::createPlayer()
{
    std::underlying_type_t<Ui::DialogPlayer::Flags> flags = 0;

    flags |= Ui::DialogPlayer::Flags::dialog_mode;

    if (isGif())
        flags |= Ui::DialogPlayer::Flags::is_gif;

    if (snippetBlock_->isInsideQuote())
        flags |= Ui::DialogPlayer::Flags::compact_mode;

    auto player = new Ui::DialogPlayer(snippetBlock_, flags, preview_);

    connect(player, &DialogPlayer::openGallery, this, [this]()
    {
        postOpenGalleryStat(snippetBlock_->getChatAimid());
        auto data = Utils::GalleryData(snippetBlock_->getChatAimid(), snippetBlock_->getLink(), snippetBlock_->getGalleryId());
        if (auto msg = snippetBlock_->getParentComplexMessage())
            msg->fillGalleryData(data);
        Utils::InterConnector::instance().openGallery(data);
    });

    connect(player, &DialogPlayer::mouseClicked, this, &MediaContent::clicked);
    connect(player, &DialogPlayer::copyLink, this, [this]() { copyLink(link_); });

    player->setParent(snippetBlock_);

    int32_t volume = Ui::get_gui_settings()->get_value<int32_t>(setting_mplayer_volume, 100);

    player->setVolume(volume);
    player->setMute(true);

    if (!isGif())
        player->setGotAudio(true);

    player->setReplay(isGif() && isAutoPlay());

    QUrl url(link_);
    player->setSiteName(removeWWW(url.host()));

    player->updateSize(contentRect());
    player->setClippingPath(clipPath_);
    player->updateVisibility(snippetBlock_->isBlockVisible());
    player->show();

    return player;
}
#endif // !STRIP_AV_MEDIA

void MediaContent::createControls()
{
    std::set<MediaControls::ControlType> controls;
    controls.insert(MediaControls::CopyLink);

    controls_ = new MediaControls(controls, 0, snippetBlock_);

    connect(controls_, &MediaControls::clicked, this, &MediaContent::clicked);
    connect(controls_, &MediaControls::copyLink, this, [this]() { copyLink(link_); });

    QUrl url(link_);
    controls_->setSiteName(removeWWW(url.host()));
    controls_->show();
    controls_->raise();
}

void MediaContent::initPlayableMedia()
{
#ifndef STRIP_AV_MEDIA
    player_ = createPlayer();
#endif // !STRIP_AV_MEDIA

    button_ = new ActionButtonWidget(ActionButtonResource::ResourceSet::DownloadVideo_, snippetBlock_);
    button_->setAttribute(Qt::WA_TransparentForMouseEvents);
    button_->show();
    button_->raise();

    connect(GetDispatcher(), &core_dispatcher::externalFilePath, this, &MediaContent::onFilePath);

    if (isAutoPlay())
        loadMedia();
    else
        seq_ = GetDispatcher()->getExternalFilePath(link_);
}

void MediaContent::loadMedia()
{
    loader_ = std::make_unique<Utils::CommonMediaLoader>(link_);
    connect(loader_.get(), &Utils::MediaLoader::fileLoaded, this, &MediaContent::onLoaded);
    connect(loader_.get(), &Utils::MediaLoader::progress, this, &MediaContent::onProgress);
    connect(loader_.get(), &Utils::MediaLoader::error, this, &MediaContent::onError);

    loader_->load();

    if (button_)
        button_->startAnimation();
}

void MediaContent::stopLoading()
{
    if (loader_)
        loader_->cancel();

    if (button_)
        button_->stopAnimation();

    loader_.reset();
}

bool MediaContent::isPlayable() const
{
    return isGif() || isVideo();
}

bool MediaContent::isAutoPlay() const
{
    if (isGif())
        return get_gui_settings()->get_value<bool>(settings_autoplay_gif, true);
    else if (isVideo())
        return get_gui_settings()->get_value<bool>(settings_autoplay_video, true);

    return false;
}

bool MediaContent::isVideo() const
{
    return meta_.getContentType() == Data::LinkContentType::Video;
}

bool MediaContent::isGif() const
{
    return meta_.getContentType() == Data::LinkContentType::Gif;
}

bool MediaContent::isProgressVisible() const
{
    const auto height = contentRect().height();
    return !snippetBlock_->isInsideQuote() && height >= MessageStyle::Preview::showDownloadProgressThreshold();
}

//////////////////////////////////////////////////////////////////////////
// FileContent
//////////////////////////////////////////////////////////////////////////

FileContent::FileContent(SnippetBlock* _snippetBlock, const Data::LinkMetadata& _meta, const QString& _link)
    : SnippetContent(_snippetBlock, _meta, _link)
{
    auto fileName = _meta.getFileName();

    if (fileName.isEmpty())
        fileName = fileNameFromLink(link_);

    fileNameUnit_ = TextRendering::MakeTextUnit(fileName);
    linkUnit_ = TextRendering::MakeTextUnit(formatLink(link_));

    initTextUnits();
    loadFavicon();

    preview_ = FileSharingIcon::getIcon(fileName);
}

FileContent::~FileContent() = default;

QSize FileContent::setContentSize(const QSize& _available)
{
    const auto availableWidth = std::min(_available.width(), MessageStyle::Snippet::getMaxWidth());
    auto currentY = 0;
    auto xOffset = 0;

    if (!snippetBlock_->isInsideQuote())
        xOffset = MessageStyle::Files::getIconSize().width() + MessageStyle::Files::getFilenameLeftMargin();

    fileNameUnit_->setOffsets(xOffset, currentY);
    currentY += fileNameUnit_->getHeight(availableWidth - xOffset);

    currentY += MessageStyle::Files::getLinkTopMargin();
    auto linkXOffset = xOffset;
    if (!favicon_.isNull())
    {
        faviconRect_ = QRect(QPoint(xOffset, currentY), MessageStyle::Snippet::getFaviconSize());
        linkXOffset = xOffset + faviconRect_.width() + MessageStyle::Snippet::getLinkLeftPadding();
    }

    auto timeWidth = 0;
    if (const auto timeWidget = snippetBlock_->getParentComplexMessage()->getTimeWidget())
        timeWidth = timeWidget->width() + MessageStyle::getTimeLeftSpacing();

    const auto linkHeight = linkUnit_->getHeight(availableWidth - timeWidth - linkXOffset);

    auto linkYOffset = std::max(0, faviconRect_.height() - linkHeight) / 2;

    linkUnit_->setOffsets(linkXOffset, currentY + linkYOffset);
    currentY += std::max(linkHeight, faviconRect_.height());

    const auto desiredWidth = std::min(availableWidth, std::max(fileNameUnit_->desiredWidth() + xOffset, linkUnit_->desiredWidth() + linkXOffset + timeWidth));

    return QSize(desiredWidth, std::max(currentY, MessageStyle::Files::getIconSize().height()));
}


void FileContent::draw(QPainter& _p, const QRect& _rect)
{
    Utils::PainterSaver saver(_p);

    fileNameUnit_->draw(_p);
    linkUnit_->draw(_p);

    if (!favicon_.isNull())
        _p.drawPixmap(faviconRect_, favicon_);

    if (!snippetBlock_->isInsideQuote())
    {
        const auto iconRect = QRect(QPoint(0, 0), MessageStyle::Files::getIconSize());
        _p.drawPixmap(iconRect, preview_);
    }
}

bool FileContent::clicked()
{
    Utils::openUrl(link_);
    return true;
}

bool FileContent::pressed()
{
    return true;
}

bool FileContent::isBubbleRequired() const
{
    return true;
}

bool FileContent::isMarginRequired() const
{
    return true;
}

void FileContent::highlight(const highlightsV& _hl)
{
    fileNameUnit_->setHighlighted(_hl);
    linkUnit_->setHighlighted(_hl);
}

void FileContent::removeHighlight()
{
    fileNameUnit_->setHighlighted(false);
    linkUnit_->setHighlighted(false);
}

void FileContent::updateStyle()
{
    initTextUnits();

    snippetBlock_->notifyBlockContentsChanged();
}

IItemBlock::MenuFlags FileContent::menuFlags() const
{
    return IItemBlock::MenuFlagNone;
}

bool FileContent::isOverLink(const QPoint& _p) const
{
    return linkUnit_ && linkUnit_->contains(_p);
}

bool FileContent::needsTooltip() const
{
    return linkUnit_ && linkUnit_->isElided();
}

QRect FileContent::tooltipRect() const
{
    return linkUnit_ ? QRect(linkUnit_->offsets(), linkUnit_->cachedSize()) : QRect();
}

void FileContent::initTextUnits()
{
    fileNameUnit_->init(MessageStyle::Snippet::getTitleFont(), MessageStyle::Snippet::getTitleColor(), MessageStyle::Snippet::getTitleColor(),
                     MessageStyle::getSelectionColor(), MessageStyle::getHighlightColor(), TextRendering::HorAligment::LEFT, 1, TextRendering::LineBreakType::PREFER_SPACES);

    const auto outgoing = snippetBlock_->isOutgoing();
    linkUnit_->init(MessageStyle::Snippet::getLinkFont(), MessageStyle::Snippet::getLinkColor(outgoing), MessageStyle::Snippet::getLinkColor(outgoing),
                    QColor(), MessageStyle::getHighlightColor(), TextRendering::HorAligment::LEFT, 1, TextRendering::LineBreakType::PREFER_SPACES);

    fileNameUnit_->setHighlightedTextColor(MessageStyle::getHighlightTextColor());
    linkUnit_->setHighlightedTextColor(MessageStyle::getHighlightTextColor());
}

//////////////////////////////////////////////////////////////////////////
// LocationContent
//////////////////////////////////////////////////////////////////////////

LocationContent::LocationContent(SnippetBlock* _snippetBlock, const Data::LinkMetadata& _meta, const QString& _link)
    : SnippetContent(_snippetBlock, _meta, _link)
{
    titleUnit_ = TextRendering::MakeTextUnit(locationTitle(), Data::MentionMap(),
                                             TextRendering::LinksVisible::SHOW_LINKS, TextRendering::ProcessLineFeeds::REMOVE_LINE_FEEDS);

    initTextUnits();
    loadPreview();
}

LocationContent::~LocationContent()
{

}

QSize LocationContent::setContentSize(const QSize& _available)
{
    const auto availableWidth = std::min(_available.width(), MessageStyle::Snippet::getMaxWidth());
    const auto topPadding = MessageStyle::Snippet::getPreviewTopPadding();

    auto currentY = topPadding;

    QSize previewRectSize;
    if (!meta_.getOriginSize().isEmpty())
    {
        MediaUtils::MediaBlockParams params;
        params.mediaType =  MediaUtils::MediaType::Image;
        params.mediaSize = meta_.getOriginSize();
        params.availableWidth = availableWidth;

        previewRectSize = calcPreviewSize(availableWidth);
        currentY += previewRectSize.height();
    }

    auto timeWidth = 0;
    const auto timeWidget = snippetBlock_->getParentComplexMessage()->getTimeWidget();
    if (timeWidget && isTimeInTitle())
        timeWidth = timeWidget->width() + MessageStyle::getTimeLeftSpacing();

    currentY += MessageStyle::Snippet::getTitleTopPadding();
    titleUnit_->setOffsets(0, currentY);
    currentY += titleUnit_->getHeight(availableWidth - timeWidth);

    const auto textWidth = titleUnit_->desiredWidth() + timeWidth;
    const auto desiredWidth = std::max(previewRectSize.width(), textWidth);
    const auto blockWidth = std::min(desiredWidth, availableWidth);

    previewRect_ = QRect(QPoint((blockWidth - previewRectSize.width()) / 2, topPadding), previewRectSize);

    return QSize(blockWidth, currentY);
}

void LocationContent::draw(QPainter& _p, const QRect& _rect)
{
    Utils::PainterSaver saver(_p);
    _p.setRenderHint(QPainter::Antialiasing);

    titleUnit_->draw(_p);

    if (!preview_.isNull())
    {
        Utils::PainterSaver previewSaver(_p);
        _p.setClipPath(Utils::renderMessageBubble(previewRect_, Ui::MessageStyle::Preview::getInternalBorderRadius()));

        MediaUtils::drawMediaInRect(_p, previewRect_, preview_, meta_.getOriginSize());
    }
}

bool LocationContent::clicked()
{
    Utils::openUrl(link_);
    return true;
}

bool LocationContent::pressed()
{
    return true;
}

bool LocationContent::isBubbleRequired() const
{
    return true;
}

bool LocationContent::isMarginRequired() const
{
    return true;
}

MediaType LocationContent::mediaType() const
{
    return MediaType::mediaTypeGeo;
}

int LocationContent::desiredWidth(int _availableWidth) const
{
    auto timeWidth = 0;
    const auto timeWidget = snippetBlock_->getParentComplexMessage()->getTimeWidget();
    if (timeWidget && isTimeInTitle())
        timeWidth = timeWidget->width() + MessageStyle::getTimeLeftSpacing();

    const auto previewWidth = calcPreviewSize(_availableWidth).width();

    return std::max(previewWidth, timeWidth + titleUnit_->desiredWidth());
}

void LocationContent::updateStyle()
{
    initTextUnits();
}

QSize LocationContent::calcPreviewSize(int _availableWidth) const
{
    MediaUtils::MediaBlockParams params;

    params.mediaType =  MediaUtils::MediaType::Image;
    params.mediaSize = originSizeScaled();
    params.availableWidth = _availableWidth;

    return MediaUtils::calcMediaBlockSize(params);
}

QSize LocationContent::originSizeScaled() const
{
    return Utils::scale_value(meta_.getOriginSize());
}

bool LocationContent::isTimeInTitle() const
{
    return !snippetBlock_->isInsideQuote() && !snippetBlock_->isInsideForward();
}

void LocationContent::initTextUnits()
{
    titleUnit_->init(MessageStyle::Snippet::getTitleFont(), MessageStyle::Snippet::getTitleColor(), MessageStyle::Snippet::getTitleColor(),
                     MessageStyle::getSelectionColor(), MessageStyle::getHighlightColor(), TextRendering::HorAligment::LEFT, 1, TextRendering::LineBreakType::PREFER_SPACES);
}

//////////////////////////////////////////////////////////////////////////
// ArticlePreloader
//////////////////////////////////////////////////////////////////////////

constexpr std::chrono::milliseconds animationDuration = std::chrono::seconds(2);

ArticlePreloader::ArticlePreloader(SnippetBlock* _snippetBlock, const QString& _link)
    : SnippetContent(_snippetBlock, Data::LinkMetadata(), _link)
    , animation_(new QVariantAnimation(this))
{
    startAnimation();
}

QSize ArticlePreloader::setContentSize(const QSize& _available)
{
    const auto availableWidth = std::min(_available.width(), MessageStyle::Snippet::getMaxWidth());

    static const auto commonMargin = Utils::scale_value(8);
    static const auto titleHeight = Utils::scale_value(20);
    static const auto linkHeight = Utils::scale_value(10);
    static const auto previewHeight = Utils::scale_value(180);
    static const auto faviconWidth = Utils::scale_value(12);

    const auto titleWidth = std::min(availableWidth , Utils::scale_value(270));

    previewRect_ = QRect(0, 0, availableWidth, previewHeight);
    titleRect_ = QRect(0, previewRect_.bottom() + commonMargin, titleWidth, titleHeight);
    faviconRect_ = QRect(0, titleRect_.bottom() + commonMargin, faviconWidth, linkHeight);

    const auto linkWidth = std::min(Utils::scale_value(62), availableWidth - faviconWidth - commonMargin);

    linkRect_ = QRect(faviconRect_.right() + commonMargin, titleRect_.bottom() + commonMargin, linkWidth, linkHeight);

    return QSize(availableWidth, linkRect_.bottom() + commonMargin);
}

void ArticlePreloader::draw(QPainter& _p, const QRect& _rect)
{
    const auto &preloaderBrush = MessageStyle::Snippet::getPreloaderBrush();
    _p.setBrush(preloaderBrush);

    _p.setBrushOrigin(_rect.width() * animation_->currentValue().toDouble(), 0);
    _p.drawRect(previewRect_);
    _p.drawRect(titleRect_);
    _p.drawRect(faviconRect_);
    _p.drawRect(linkRect_);
}

bool ArticlePreloader::clicked()
{
    Utils::openUrl(link_);
    return true;
}

bool ArticlePreloader::pressed()
{
    return true;
}

bool ArticlePreloader::isBubbleRequired() const
{
    return true;
}

bool ArticlePreloader::isMarginRequired() const
{
    return true;
}

void ArticlePreloader::onVisibilityChanged(const bool _visible)
{
    if (_visible && animation_->state() == QAbstractAnimation::Paused)
        animation_->resume();
    else if (animation_->state() == QAbstractAnimation::Running)
        animation_->pause();
}

IItemBlock::MenuFlags ArticlePreloader::menuFlags() const
{
    return IItemBlock::MenuFlagNone;
}

bool ArticlePreloader::isOverLink(const QPoint& _p) const
{
    return linkRect_.contains(_p);
}

bool ArticlePreloader::needsTooltip() const
{
    return true;
}

QRect ArticlePreloader::tooltipRect() const
{
    return linkRect_;
}

void ArticlePreloader::startAnimation()
{
    animation_->setStartValue(0.0);
    animation_->setEndValue(1.0);
    animation_->setDuration(animationDuration.count());
    animation_->setLoopCount(-1);
    animation_->disconnect(snippetBlock_);
    connect(animation_, &QVariantAnimation::valueChanged, snippetBlock_, qOverload<>(&SnippetBlock::update));
    animation_->start();
}

//////////////////////////////////////////////////////////////////////////
// MediaPreloader
//////////////////////////////////////////////////////////////////////////

MediaPreloader::MediaPreloader(SnippetBlock* _snippetBlock, const QString& _link)
    : SnippetContent(_snippetBlock, Data::LinkMetadata(), _link)
{

}

QSize MediaPreloader::setContentSize(const QSize& _available)
{
    auto placeholderSize = MessageStyle::Preview::getImagePlaceholderSize();
    placeholderSize.setWidth(std::min(placeholderSize.width(), _available.width()));
    return placeholderSize;
}

void MediaPreloader::draw(QPainter& _p, const QRect& _rect)
{
    _p.setClipPath(clipPath_);
    _p.fillRect(_rect, MessageStyle::Preview::getImagePlaceholderBrush());
}

void MediaPreloader::onBlockSizeChanged()
{
    clipPath_ = calcMediaClipPath(snippetBlock_, contentRect());
}

bool MediaPreloader::clicked()
{
    Utils::openUrl(link_);
    return true;
}

bool MediaPreloader::pressed()
{
    return true;
}

int MediaPreloader::desiredWidth(int _availableWidth) const
{
    Q_UNUSED(_availableWidth)

    return MessageStyle::Preview::getImagePlaceholderSize().width();
}

UI_COMPLEX_MESSAGE_NS_END
