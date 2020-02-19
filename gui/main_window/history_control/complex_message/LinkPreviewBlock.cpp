#include "stdafx.h"

#include "../../../core_dispatcher.h"
#include "../../../controls/TextUnit.h"

#include "../../../fonts.h"
#include "../../../utils/InterConnector.h"
#include "../../../utils/log/log.h"
#include "../../../utils/profiling/auto_stop_watch.h"
#include "../../../utils/Text.h"
#include "../../../utils/utils.h"
#include "../../../utils/Text2DocConverter.h"
#include "../../../utils/UrlParser.h"
#include "../../../utils/PainterPath.h"
#include "../../../utils/ResizePixmapTask.h"

#include "../../contact_list/RecentsModel.h"

#include "../ActionButtonWidget.h"
#include "../MessageStyle.h"

#include "ComplexMessageItem.h"
#include "FileSharingUtils.h"
#include "LinkPreviewBlockBlankLayout.h"

#include "YoutubeLinkPreviewBlockLayout.h"

#include "LinkPreviewBlock.h"
#include "QuoteBlock.h"
#include "styles/ThemeParameters.h"

namespace
{
    constexpr std::chrono::milliseconds preloadAnimDuration = std::chrono::seconds(4);
}

UI_COMPLEX_MESSAGE_NS_BEGIN

LinkPreviewBlock::LinkPreviewBlock(ComplexMessageItem *parent, const QString &uri, const bool _hasLinkInMessage)
    : GenericBlock(parent, uri, MenuFlagLinkCopyable, false)
    , Title_(nullptr)
    , domain_(nullptr)
    , Uri_(Utils::normalizeLink(QStringRef(&uri)).toString())
    , RequestId_(-1)
    , Layout_(nullptr)
    , PreviewSize_(0, 0)
    , PreloadingTickerAnimation_(nullptr)
    , PreloadingTickerValue_(0)
    , Time_(-1)
    , MetaDownloaded_(false)
    , ImageDownloaded_(false)
    , FaviconDownloaded_(false)
    , MaxPreviewWidth_(0)
    , hasLinkInMessage_(_hasLinkInMessage)
{
    assert(!Uri_.isEmpty());

    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    connections_.reserve(4);
}

LinkPreviewBlock::~LinkPreviewBlock() = default;

IItemBlockLayout* LinkPreviewBlock::getBlockLayout() const
{
    if (!Layout_)
    {
        return nullptr;
    }

    return Layout_->asBlockLayout();
}

QSize LinkPreviewBlock::getFaviconSizeUnscaled() const
{
    if (isInPreloadingState() || FavIcon_.isNull())
    {
        return QSize(0, 0);
    }

    return MessageStyle::Snippet::getFaviconSize();
}

QSize LinkPreviewBlock::getPreviewImageSize() const
{
    if (PreviewImage_.isNull())
    {
        assert(PreviewSize_.width() >= 0);
        assert(PreviewSize_.height() >= 0);

        return PreviewSize_;
    }

    const auto previewImageSize = PreviewImage_.size();

    return previewImageSize;
}

QString LinkPreviewBlock::getSelectedText(const bool, const TextDestination) const
{
    if (!isHasLinkInMessage() && isSelected())
        return getSourceText();

    return QString();
}

QString LinkPreviewBlock::getTextForCopy() const
{
    return (isHasLinkInMessage() ? QString() : getSourceText());
}

bool LinkPreviewBlock::updateFriendly(const QString&/* _aimId*/, const QString&/* _friendly*/)
{
    return false;
}

const QString& LinkPreviewBlock::getSiteName() const
{
    return Meta_.getSiteName();
}

bool LinkPreviewBlock::hasTitle() const
{
    return Title_ != nullptr;
}

bool LinkPreviewBlock::isInPreloadingState() const
{
    return (!Title_ && PreviewImage_.isNull());
}

void LinkPreviewBlock::onVisibilityChanged(const bool isVisible)
{
    GenericBlock::onVisibilityChanged(isVisible);

    if (isVisible && (RequestId_ > 0))
    {
        GetDispatcher()->raiseDownloadPriority(getChatAimid(), RequestId_);
    }

    if (isVisible)
    {
        const auto isPaused = (
            PreloadingTickerAnimation_ &&
            (PreloadingTickerAnimation_->state() == QAbstractAnimation::Paused));
        if (isPaused)
        {
            PreloadingTickerAnimation_->setPaused(false);
        }
    }
    else
    {
        const auto isRunnning = (
            PreloadingTickerAnimation_ &&
            (PreloadingTickerAnimation_->state() == QAbstractAnimation::Running));
        if (isRunnning)
        {
            PreloadingTickerAnimation_->setPaused(true);
        }
    }
}

void LinkPreviewBlock::onDistanceToViewportChanged(const QRect& _widgetAbsGeometry, const QRect& _viewportVisibilityAbsRect)
{

}

void LinkPreviewBlock::selectByPos(const QPoint& from, const QPoint& to, bool topToBottom)
{
    if (isHasLinkInMessage())
        return;

    GenericBlock::selectByPos(from, to, topToBottom);
}

void LinkPreviewBlock::setMaxPreviewWidth(int width)
{
    MaxPreviewWidth_ = width;
}

int LinkPreviewBlock::getMaxPreviewWidth() const
{
    return MaxPreviewWidth_;
}

void LinkPreviewBlock::mouseMoveEvent(QMouseEvent* _event)
{
    _event->ignore();

    const auto cursor = isOverSiteLink(_event->pos()) ? Qt::PointingHandCursor : Qt::ArrowCursor;
    setCursor(cursor);

    GenericBlock::mouseMoveEvent(_event);
}

void LinkPreviewBlock::showEvent(QShowEvent *event)
{
    GenericBlock::showEvent(event);

    // text controls creation postponing

    const auto hasTextControls = hasTitle();
    if (hasTextControls)
        return;

    const auto hasText = (!Meta_.getTitle().isEmpty());
    if (!hasText)
        return;

    const auto &blockGeometry = Layout_->asBlockLayout()->getBlockGeometry();
    assert(blockGeometry.width() > 0);

    createTextControls(blockGeometry);
}

void LinkPreviewBlock::onMenuCopyLink()
{
    assert(!Uri_.isEmpty());

    QApplication::clipboard()->setText(Uri_);
}

bool LinkPreviewBlock::drag()
{
    emit Utils::InterConnector::instance().clearSelecting();
    return Utils::dragUrl(this, PreviewImage_, Uri_);
}

void LinkPreviewBlock::drawBlock(QPainter &p, const QRect& _rect, const QColor& _quoteColor)
{
    if (isInPreloadingState())
    {
        drawPreloader(p);
    }
    else
    {
        drawPreview(p);

        drawFavicon(p);

        drawSiteName(p);
    }

    if (Title_)
        Title_->draw(p);

    GenericBlock::drawBlock(p, _rect, _quoteColor);
}

void LinkPreviewBlock::onLinkMetainfoMetaDownloaded(int64_t seq, bool success, Data::LinkMetadata meta)
{
    assert(seq > 0);

    const auto skipSeq = (seq != RequestId_);
    if (skipSeq)
    {
        return;
    }

    Meta_ = std::move(meta);

    // 1. for the better performance disconnect signals if possible and stop the animation

    assert(!MetaDownloaded_);
    MetaDownloaded_ = true;
    updateRequestId();

    if (PreloadingTickerAnimation_)
        PreloadingTickerAnimation_->stop();

    // 2. on error replace the current block with the text one and exit

    if (!success)
    {
        if (isHasLinkInMessage())
            getParentComplexMessage()->removeBlock(this);
        else
            getParentComplexMessage()->replaceBlockWithSourceText(this);

        return;
    }
    else if (getId() != -1)
    {
        if (getId() >= Logic::getRecentsModel()->getLastMsgId(getChatAimid()))
            getParentComplexMessage()->needUpdateRecentsText();
    }

    // 3. if succeed then extract valuable data from the metainfo

    ContentType_ = Meta_.getContentTypeStr();

    const auto &previewSize = Meta_.getPreviewSize();
    assert(previewSize.width() >= 0);
    assert(previewSize.height() >= 0);

    PreviewSize_ = scalePreviewSize(previewSize);

    // 4. make a proper layout for the content type

    assert(Layout_);
    const auto existingGeometry = Layout_->asBlockLayout()->getBlockGeometry();

    Layout_ = createLayout();
    setLayout(Layout_->asQLayout());

    // 5. postpone text processing if geometry is not ready

    if (existingGeometry.isEmpty())
        return;

    createTextControls(existingGeometry);
}

void LinkPreviewBlock::onLinkMetainfoFaviconDownloaded(int64_t seq, bool success, QPixmap image)
{
    assert(seq > 0);
    if (seq != RequestId_)
        return;

    assert(!FaviconDownloaded_);
    FaviconDownloaded_ = true;
    updateRequestId();

    if (!success || image.isNull())
        return;

    FavIcon_ = image;
    Utils::check_pixel_ratio(FavIcon_);

    notifyBlockContentsChanged();
}

void LinkPreviewBlock::onLinkMetainfoImageDownloaded(int64_t seq, bool success, QPixmap image)
{
    assert(seq > 0);

    const auto skipSeq = (seq != RequestId_);
    if (skipSeq)
        return;

    assert(!ImageDownloaded_);
    ImageDownloaded_ = true;
    updateRequestId();

    if (!success)
        return;

    assert(!image.isNull());

    scalePreview(image);
}

void LinkPreviewBlock::createTextControls(const QRect &blockGeometry)
{
    if (const auto title = getTitle(); !Title_ && !title.isEmpty())
        Title_ = TextRendering::MakeTextUnit(title, {}, TextRendering::LinksVisible::DONT_SHOW_LINKS, TextRendering::ProcessLineFeeds::REMOVE_LINE_FEEDS);
    if (const auto domain = Meta_.getSiteName(); !domain_ && !domain.isEmpty())
        domain_ = TextRendering::MakeTextUnit(domain, {}, TextRendering::LinksVisible::DONT_SHOW_LINKS, TextRendering::ProcessLineFeeds::REMOVE_LINE_FEEDS);
    updateStyle();
}

int LinkPreviewBlock::getPreloadingTickerValue() const
{
    assert(PreloadingTickerValue_ >= 0);
    assert(PreloadingTickerValue_ <= 100);

    return PreloadingTickerValue_;
}

void LinkPreviewBlock::setPreloadingTickerValue(const int32_t _val)
{
    assert(_val >= 0);
    assert(_val <= 100);

    PreloadingTickerValue_ = _val;

    update();
}

void LinkPreviewBlock::updateStyle()
{
    bool needUpdate = false;
    if (Title_)
    {
        Title_->init(Layout_->getTitleFont(), MessageStyle::getTextColor(), MessageStyle::getLinkColor(), MessageStyle::getSelectionColor(), MessageStyle::getHighlightColor());
        Title_->evaluateDesiredSize();
        needUpdate = true;
    }

    if (domain_)
    {
        domain_->init(MessageStyle::Snippet::getSiteNameFont(), MessageStyle::Snippet::getSiteNameColor(), QColor(), QColor(), MessageStyle::getHighlightColor());
        domain_->evaluateDesiredSize();
        needUpdate = true;
    }

    if (needUpdate)
    {
        notifyBlockContentsChanged();
        update();
    }
}

void LinkPreviewBlock::updateFonts()
{
    updateStyle();
}

void LinkPreviewBlock::connectSignals(const bool _isConnected)
{
    for (const auto& c : connections_)
        disconnect(c);

    connections_.clear();

    if (_isConnected)
    {
        connections_.assign({
            connect(GetDispatcher(), &core_dispatcher::linkMetainfoMetaDownloaded,    this, &LinkPreviewBlock::onLinkMetainfoMetaDownloaded),
            connect(GetDispatcher(), &core_dispatcher::linkMetainfoImageDownloaded,   this, &LinkPreviewBlock::onLinkMetainfoImageDownloaded),
            connect(GetDispatcher(), &core_dispatcher::linkMetainfoFaviconDownloaded, this, &LinkPreviewBlock::onLinkMetainfoFaviconDownloaded)
        });
    }
}

void LinkPreviewBlock::drawFavicon(QPainter &p)
{
    if (FavIcon_.isNull())
        return;

    const auto &imageRect = Layout_->getFaviconImageRect();

    p.drawPixmap(imageRect, FavIcon_);
}

void LinkPreviewBlock::drawPreloader(QPainter &p)
{
    assert(isInPreloadingState());
    assert(Layout_);

    const auto &preloaderBrush = MessageStyle::Snippet::getPreloaderBrush();
    p.setBrush(preloaderBrush);

    const auto width = getBlockLayout() ? getBlockLayout()->getBlockGeometry().width() : 0;
    const auto brushX = ((width * getPreloadingTickerValue()) / 50);
    p.setBrushOrigin(brushX, 0);

    const auto &imageRect = Layout_->getPreviewImageRect();
    p.drawRect(imageRect);

    const auto &faviconRect = Layout_->getFaviconImageRect();
    p.drawRect(faviconRect);

    const auto &siteNameRect = Layout_->getSiteNameRect();
    p.drawRect(siteNameRect);

    const auto &titleRect = Layout_->getTitleRect();
    p.drawRect(titleRect);
}

QPainterPath LinkPreviewBlock::evaluateClippingPath(const QRect &imageRect) const
{
    assert(!imageRect.isEmpty());

    return Utils::renderMessageBubble(imageRect, Ui::MessageStyle::getBorderRadius());
}


void LinkPreviewBlock::drawPreview(QPainter &p)
{
    const auto &imageRect = Layout_->getPreviewImageRect();

    if (MessageStyle::isBlocksGridEnabled())
    {
        Utils::PainterSaver ps(p);

        p.setPen(Styling::getParameters().getColor(Styling::StyleVariable::PRIMARY));
        p.drawRect(imageRect);
    }

    if (PreviewImage_.isNull())
    {
        return;
    }

    const auto updateClippingPath = (previewClippingPathRect_ != imageRect);

    if (updateClippingPath)
    {
        previewClippingPathRect_ = imageRect;
        previewClippingPath_ = evaluateClippingPath(imageRect);
    }

    Utils::PainterSaver ps(p);

    p.setClipPath(previewClippingPath_);
    p.drawPixmap(imageRect, PreviewImage_);
}

void LinkPreviewBlock::drawSiteName(QPainter &p)
{
    if (domain_)
    {
        domain_->setOffsets(Layout_->getSiteNamePos());
        domain_->draw(p, TextRendering::VerPosition::BASELINE);
    }
}

std::unique_ptr<YoutubeLinkPreviewBlockLayout> LinkPreviewBlock::createLayout()
{
    return std::make_unique<YoutubeLinkPreviewBlockLayout>();
}

QString LinkPreviewBlock::getTitle() const
{
    return Meta_.getTitle();
}

void LinkPreviewBlock::initialize()
{
    GenericBlock::initialize();

    Layout_ = createLayout();
    setLayout(Layout_->asQLayout());

    connectSignals(true);

    assert(RequestId_ == -1);
    RequestId_ = GetDispatcher()->downloadLinkMetainfo(
        Uri_,
        core_dispatcher::LoadPreview::Yes,
        0,
        0,
        Utils::msgIdLogStr(getParentComplexMessage()->getId()));

    assert(!PreloadingTickerAnimation_);
    PreloadingTickerAnimation_ = new QPropertyAnimation(this, QByteArrayLiteral("PreloadingTicker"), this);

    PreloadingTickerAnimation_->setDuration(preloadAnimDuration.count());
    PreloadingTickerAnimation_->setLoopCount(-1);
    PreloadingTickerAnimation_->setStartValue(0);
    PreloadingTickerAnimation_->setEndValue(100);

    PreloadingTickerAnimation_->start();
}

bool LinkPreviewBlock::isOverSiteLink(const QPoint& p) const
{
    return rect().contains(p);
}

bool LinkPreviewBlock::isTitleClickable() const
{
    return (Title_ && PreviewImage_.isNull());
}

void LinkPreviewBlock::setPreviewImage(const QPixmap& _image)
{
    PreviewImage_ = _image;

    Utils::check_pixel_ratio(PreviewImage_);

    notifyBlockContentsChanged();
}

const QPixmap& LinkPreviewBlock::getPreviewImage() const
{
    return PreviewImage_;
}

void LinkPreviewBlock::scalePreview(QPixmap &image)
{
    assert(PreviewImage_.isNull());
    assert(!image.isNull());

    const auto previewSize = image.size();
    const auto scaledPreviewSize = scalePreviewSize(previewSize);

    const auto shouldScalePreview = (previewSize != scaledPreviewSize);
    if (shouldScalePreview)
    {
        auto task = new Utils::ResizePixmapTask(image, scaledPreviewSize);

        const auto succeed = QObject::connect(task, &Utils::ResizePixmapTask::resizedSignal, this, &LinkPreviewBlock::setPreviewImage);
        assert(succeed);

        QThreadPool::globalInstance()->start(task);
    }
    else
    {
        setPreviewImage(image);
    }
}

QSize LinkPreviewBlock::scalePreviewSize(const QSize &size) const
{
    assert(Layout_);

    const auto maxSize = Layout_->getMaxPreviewSize();
    assert(maxSize.width() > 0);

    if (size.width() > maxSize.width() || size.height() > maxSize.height())
    {
        auto result = size;
        result.scale(maxSize, Qt::KeepAspectRatio);
        return Utils::scale_bitmap(result);
    }

    return size;
}

void LinkPreviewBlock::updateRequestId()
{
    const auto pendingDataExists = (!MetaDownloaded_ || !ImageDownloaded_ || !FaviconDownloaded_);
    if (pendingDataExists)
        return;

    RequestId_ = -1;

    connectSignals(false);
}

void LinkPreviewBlock::setQuoteSelection()
{
    GenericBlock::setQuoteSelection();
    emit setQuoteAnimation();
}

bool LinkPreviewBlock::isHasLinkInMessage() const
{
    return hasLinkInMessage_;
}

int LinkPreviewBlock::getMaxWidth() const
{
    return MessageStyle::getSnippetMaxWidth();
}

QPoint LinkPreviewBlock::getShareButtonPos(const bool _isBubbleRequired, const QRect& _bubbleRect) const
{
    if (PreviewImage_.isNull())
        return GenericBlock::getShareButtonPos(_isBubbleRequired, _bubbleRect);

    const auto &blockGeometry = Layout_->asBlockLayout()->getBlockGeometry();

    const auto &imageRect = Layout_->getPreviewImageRect();

    auto buttonX = blockGeometry.left();

    if (isOutgoing())
        buttonX -= getParentComplexMessage()->getSharingAdditionalWidth();
    else
        buttonX += imageRect.right() + 1 + MessageStyle::getSharingButtonMargin();

    const auto buttonY = blockGeometry.top() + imageRect.top();

    return QPoint(buttonX, buttonY);
}

int LinkPreviewBlock::desiredWidth(int) const
{
    return MessageStyle::getSnippetMaxWidth();
}

int LinkPreviewBlock::getTitleHeight(int _titleWidth)
{
    return Title_ ? Title_->getHeight(_titleWidth) : 0;
}

void LinkPreviewBlock::setTitleOffsets(int _x, int _y)
{
    if (Title_)
        Title_->setOffsets(_x, _y);
}

void LinkPreviewBlock::requestPinPreview()
{
    struct TempRequestData
    {
        int64_t reqId_ = -1;
        bool getImage_ = false;
        bool getMeta_  = false;
        QMetaObject::Connection connImage_;
        QMetaObject::Connection connMeta_;

        void checkPending()
        {
            if (getImage_ && getMeta_)
            {
                reqId_ = -1;
                disconnect(connImage_);
                disconnect(connMeta_);
            }
        }
    };

    auto reqData = std::make_shared<TempRequestData>();

    reqData->connImage_ = connect(GetDispatcher(), &core_dispatcher::linkMetainfoImageDownloaded, this,
        [reqData, this](int64_t _seq, bool _success, QPixmap _image)
    {
        if (reqData->reqId_ == _seq)
        {
            if (_success && !_image.isNull() && getParentComplexMessage())
                emit getParentComplexMessage()->pinPreview(_image);

            reqData->getImage_ = true;
            reqData->checkPending();
        }
    });

    reqData->connMeta_ = connect(GetDispatcher(), &core_dispatcher::linkMetainfoMetaDownloaded, this,
        [reqData, this](int64_t _seq, bool _success, const Data::LinkMetadata& _meta)
    {
        if (reqData->reqId_ == _seq)
        {
            if (_success && !_meta.getTitle().isEmpty() && getParentComplexMessage())
            {
                Meta_ = std::move(_meta);
                emit getParentComplexMessage()->needUpdateRecentsText();
            }

            reqData->getMeta_ = true;
            reqData->checkPending();
        }
    });

    reqData->reqId_ = GetDispatcher()->downloadLinkMetainfo(Uri_, core_dispatcher::LoadPreview::Yes, 0, 0, Utils::msgIdLogStr(getParentComplexMessage()->getId()));
}

QString LinkPreviewBlock::formatRecentsText() const
{
    if (getParentComplexMessage() && getParentComplexMessage()->getBlockCount() == 1)
    {
        if (auto title = Meta_.getTitle(); !title.isEmpty())
            return title;
    }

    return GenericBlock::formatRecentsText();
}

bool LinkPreviewBlock::pressed(const QPoint& _p)
{
    const auto mappedPoint = mapFromParent(_p, Layout_->asBlockLayout()->getBlockGeometry());

    if (isOverSiteLink(mappedPoint))
        return true;

    return false;
}

bool LinkPreviewBlock::clicked(const QPoint& _p)
{
    QPoint mappedPoint = mapFromParent(_p, Layout_->asBlockLayout()->getBlockGeometry());

    if (!isOverSiteLink(mappedPoint))
        return true;

    assert(!Uri_.isEmpty());

    Utils::UrlParser parser;
    parser.process(QStringRef(&Uri_));

    if (parser.hasUrl())
        Utils::openUrl(QString::fromStdString(parser.getUrl().url_));

    return true;
}

void LinkPreviewBlock::cancelRequests()
{
    if (!GetDispatcher())
        return;

    GetDispatcher()->cancelLoaderTask(Uri_);
}

void LinkPreviewBlock::highlight(const highlightsV& _hl)
{
    if (Title_)
        Title_->setHighlighted(_hl);
    if (domain_)
        domain_->setHighlighted(_hl);
}

void LinkPreviewBlock::removeHighlight()
{
    if (Title_)
        Title_->setHighlighted(false);
    if (domain_)
        domain_->setHighlighted(false);
}

UI_COMPLEX_MESSAGE_NS_END
