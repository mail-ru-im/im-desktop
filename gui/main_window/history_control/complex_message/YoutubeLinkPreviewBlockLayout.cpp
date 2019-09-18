#include "stdafx.h"

#include "../../../utils/utils.h"

#include "../MessageStyle.h"

#include "ComplexMessageUtils.h"
#include "LinkPreviewBlock.h"


#include "YoutubeLinkPreviewBlockLayout.h"

UI_COMPLEX_MESSAGE_NS_BEGIN

YoutubeLinkPreviewBlockLayout::YoutubeLinkPreviewBlockLayout() = default;

YoutubeLinkPreviewBlockLayout::~YoutubeLinkPreviewBlockLayout() = default;

IItemBlockLayout* YoutubeLinkPreviewBlockLayout::asBlockLayout()
{
    return this;
}

QLayout* YoutubeLinkPreviewBlockLayout::asQLayout()
{
    return this;
}

const IItemBlockLayout::IBoxModel& YoutubeLinkPreviewBlockLayout::getBlockBoxModel() const
{
    static const BoxModel boxModel(
        true,
        MessageStyle::getDefaultBlockBubbleMargins());

    return boxModel;
}

QSize YoutubeLinkPreviewBlockLayout::getMaxPreviewSize() const
{
    return QSize(
        MessageStyle::Preview::getImageWidthMax(),
        MessageStyle::Snippet::getLinkPreviewHeightMax());
}

QRect YoutubeLinkPreviewBlockLayout::getFaviconImageRect() const
{
    return FaviconImageRect_;
}

QRect YoutubeLinkPreviewBlockLayout::getPreviewImageRect() const
{
    return PreviewImageRect_;
}

QPoint YoutubeLinkPreviewBlockLayout::getSiteNamePos() const
{
    return SiteNameGeometry_.bottomLeft();
}

QRect YoutubeLinkPreviewBlockLayout::getSiteNameRect() const
{
    return SiteNameGeometry_;
}

QFont YoutubeLinkPreviewBlockLayout::getTitleFont() const
{
    return MessageStyle::Snippet::getYoutubeTitleFont();
}

QRect YoutubeLinkPreviewBlockLayout::getTitleRect() const
{
    return TitleGeometry_;
}

QSize YoutubeLinkPreviewBlockLayout::setBlockGeometryInternal(const QRect &geometry)
{
    assert(geometry.height() >= 0);

    const auto enoughSpace = (geometry.width() > 0);
    if (!enoughSpace)
    {
        return QSize(0, 0);
    }

    auto &block = *blockWidget<LinkPreviewBlock>();

    const auto isPreloader = block.isInPreloadingState();

    auto previewContentWidth = std::min(
        geometry.width(),
        MessageStyle::Preview::getImageWidthMax());

    if (block.getMaxPreviewWidth() != 0)
        previewContentWidth = std::min(previewContentWidth, block.getMaxPreviewWidth());

    const QRect previewContentLtr(
        geometry.topLeft(),
        QSize(previewContentWidth, 0));

    PreviewImageRect_ = evaluatePreviewImageRect(block, previewContentLtr);

    auto titleWidth = std::min(
        geometry.width(),
        MessageStyle::Preview::getImageWidthMax());

    const QRect titleContentLtr(
        geometry.topLeft(),
        QSize(titleWidth, 0));

    const auto titleLtr = evaluateTitleLtr(titleContentLtr, PreviewImageRect_, isPreloader);

    TitleGeometry_ = setTitleGeometry(block, titleLtr);

    FaviconImageRect_ = evaluateFaviconImageRect(block, TitleGeometry_);

    SiteNameGeometry_ = evaluateSiteNameGeometry(block, TitleGeometry_, FaviconImageRect_);

    const auto blockHeight = evaluateWidgetHeight(
        PreviewImageRect_,
        FaviconImageRect_,
        SiteNameGeometry_,
        isPreloader);
    assert(blockHeight >= MessageStyle::getMinBubbleHeight());

    const QSize blockSize(geometry.width(), blockHeight);

    return blockSize;
}

QRect YoutubeLinkPreviewBlockLayout::evaluateFaviconImageRect(const LinkPreviewBlock &block, const QRect &titleGeometry) const
{
    QRect faviconGeometry(
        titleGeometry.left(),
        titleGeometry.bottom() + 1,
        0,
        0);

    const auto hasTextAbove = !titleGeometry.isEmpty();
    if (hasTextAbove)
    {
        faviconGeometry.translate(0, MessageStyle::Snippet::getFaviconTopPadding());
    }

    const auto faviconSize =
        block.isInPreloadingState() ?
        MessageStyle::Snippet::getFaviconPlaceholderSize() :
            block.getFaviconSizeUnscaled();

    faviconGeometry.setSize(faviconSize);

    return Utils::scale_value(faviconGeometry);
}

QRect YoutubeLinkPreviewBlockLayout::evaluatePreviewImageRect(const LinkPreviewBlock &block, const QRect &previewContentLtr) const
{
    assert(previewContentLtr.isEmpty());

    auto previewImageSize = block.isInPreloadingState()
        ? MessageStyle::Snippet::getImagePreloaderSize()
        : Utils::scale_value(block.getPreviewImageSize());

    if (previewImageSize.isEmpty())
        return QRect(previewContentLtr.topLeft(), QSize());

    const QSize maxSize(
        previewContentLtr.width(),
        MessageStyle::Snippet::getLinkPreviewHeightMax());

    previewImageSize = limitSize(previewImageSize, maxSize);

    const QRect previewImageRect( previewContentLtr.topLeft(), previewImageSize);
    assert(!previewImageRect.isEmpty());

    return previewImageRect;
}

QRect YoutubeLinkPreviewBlockLayout::evaluateSiteNameGeometry(const LinkPreviewBlock &block, const QRect &titleGeometry, const QRect &faviconGeometry) const
{
    auto siteNameY = (titleGeometry.bottom() + 1);

    const auto hasTextAbove = !titleGeometry.isEmpty();
    if (hasTextAbove)
    {
        siteNameY += MessageStyle::Snippet::getSiteNameTopPadding();
    }

    auto siteNameX = (faviconGeometry.right() + 1);

    const auto hasFavicon = !faviconGeometry.isEmpty();
    if (hasFavicon)
    {
        siteNameX += MessageStyle::Snippet::getSiteNameLeftPadding();
    }

    QSize siteNameSize(0, 0);

    const auto &siteName = block.getSiteName();
    if (!siteName.isEmpty())
    {
        QFontMetrics m(block.getSiteNameFont());

        siteNameSize = m.tightBoundingRect(siteName).size();

        if (hasTextAbove)
        {
            siteNameY -= siteNameSize.height();
        }
    }
    else if (block.isInPreloadingState())
    {
        assert(!faviconGeometry.isEmpty());
        siteNameSize = MessageStyle::Snippet::getSiteNamePlaceholderSize();
        siteNameY = faviconGeometry.top();
    }

    const QPoint leftTop(siteNameX, siteNameY);

    return QRect(leftTop, siteNameSize);
}

QRect YoutubeLinkPreviewBlockLayout::evaluateTitleLtr(const QRect &previewContentLtr, const QRect &previewImageRect, const bool isPlaceholder)
{
    assert(previewContentLtr.width() > 0);
    assert(previewContentLtr.height() == 0);

    if (previewImageRect.isEmpty())
        return previewContentLtr;

    QRect titleLtr(
        previewContentLtr.left(),
        previewImageRect.bottom() + 1,
        previewContentLtr.width(),
        0);

    const auto topOffset = (isPlaceholder ? MessageStyle::Snippet::getTitlePlaceholderTopOffset() : MessageStyle::Snippet::getTitleTopOffset());
    titleLtr.translate(0, topOffset);

    return titleLtr;
}

int32_t YoutubeLinkPreviewBlockLayout::evaluateWidgetHeight(
    const QRect &previewImageGeometry,
    const QRect &faviconGeometry,
    const QRect &siteNameGeometry,
    const bool isPlaceholder)
{
    auto bottom = (previewImageGeometry.bottom() + 1);
    bottom = std::max(bottom, faviconGeometry.bottom());

    auto siteNameBottom = (siteNameGeometry.bottom() + 1);

    const auto hasSiteName = !siteNameGeometry.isEmpty();
    const auto applyBaselineFix = (hasSiteName && !isPlaceholder);
    if (applyBaselineFix)
    {
        // an approximation of the gap between font baseline and font bottom line
        siteNameBottom += (siteNameGeometry.height() / 3);
    }

    bottom = std::max(bottom, siteNameBottom);

    auto top = previewImageGeometry.top();

    auto contentHeight = (bottom - top);
    assert(contentHeight >= 0);

    const auto height = std::max(MessageStyle::getMinBubbleHeight(), contentHeight);

    return height;
}

QRect YoutubeLinkPreviewBlockLayout::setTitleGeometry(LinkPreviewBlock &block, const QRect &titleLtr)
{
    assert(titleLtr.isEmpty());

    if (block.isInPreloadingState())
    {
        QRect preloaderGeometry(
            titleLtr.topLeft(),
            MessageStyle::Snippet::getTitlePlaceholderSize());

        return preloaderGeometry;
    }

    if (!block.hasTitle())
    {
        return titleLtr;
    }

    const auto textWidth = titleLtr.width();
    assert(textWidth > 0);

    int titleHeight = block.getTitleHeight(textWidth);

    const auto textX = titleLtr.left();
    assert(textX >= 0);

    const auto textY = titleLtr.top();
    assert(textY >= 0);

    const QRect titleGeometry(
        textX,
        textY,
        textWidth,
        titleHeight);

    block.setTitleOffsets(0, titleGeometry.y());

    return titleGeometry;
}

UI_COMPLEX_MESSAGE_NS_END
