#include "stdafx.h"

#include "../../../utils/log/log.h"
#include "../../../utils/utils.h"

#include "../ActionButtonWidget.h"
#include "../MessageStatusWidget.h"
#include "../MessageStyle.h"

#include "ComplexMessageItem.h"
#include "IItemBlock.h"
#include "IItemBlockLayout.h"


#include "ComplexMessageItemLayout.h"
#include "../../../controls/TextUnit.h"

UI_COMPLEX_MESSAGE_NS_BEGIN

ComplexMessageItemLayout::ComplexMessageItemLayout(ComplexMessageItem *parent)
    : QLayout(parent)
    , Item_(parent)
    , WidgetHeight_(-1)
    , bubbleBlockHorPadding_(MessageStyle::getBubbleHorPadding())
{
    assert(Item_);
}

ComplexMessageItemLayout::~ComplexMessageItemLayout() = default;

QRect ComplexMessageItemLayout::evaluateAvatarRect(const QRect& _widgetContentLtr) const
{
    assert(_widgetContentLtr.width() > 0 && _widgetContentLtr.height() >= 0);

    const auto avatarSize = MessageStyle::getAvatarSize();
    return QRect(_widgetContentLtr.topLeft(), QSize(avatarSize, avatarSize));
}

QRect ComplexMessageItemLayout::evaluateBlocksBubbleGeometry(
    const bool isBubbleRequired,
    const QRect& _blocksContentLtr) const
{
    QMargins margins;
    if (isBubbleRequired)
    {
        margins.setLeft(bubbleBlockHorPadding_);
        margins.setRight(bubbleBlockHorPadding_);
        margins.setTop(MessageStyle::getBubbleVerPadding());
        margins.setBottom(MessageStyle::getBubbleVerPadding());
    }
    else if (isHeaderOrSticker())
    {
        margins.setBottom(MessageStyle::getBubbleVerPadding());
    }

    auto bubbleGeometry = _blocksContentLtr + margins;
    const auto validBubbleHeight = std::max(bubbleGeometry.height(), MessageStyle::getMinBubbleHeight());

    bubbleGeometry.setHeight(validBubbleHeight);

    assert(!bubbleGeometry.isEmpty());
    return bubbleGeometry;
}

QRect ComplexMessageItemLayout::evaluateBlocksContainerLtr(const QRect& _avatarRect, const QRect& _widgetContentLtr) const
{
    auto left = 0;

    if (isOutgoing() || !_avatarRect.isValid())
        left = _widgetContentLtr.left();
    else
        left = _avatarRect.right() + 1 + MessageStyle::getAvatarRightMargin();

    auto right = _widgetContentLtr.right();

    assert(right > left);

    auto desired = Item_->desiredWidth();
    if (desired != 0)
    {
        auto cur = right - left;
        auto diff = cur - desired;
        if (diff > 0)
        {
            if (isOutgoing())
                left += diff;
            else
                right -= diff;
        }
    }

    QRect blocksContainerLtr;

    blocksContainerLtr.setLeft(left);
    blocksContainerLtr.setRight(right);
    blocksContainerLtr.setTop(_widgetContentLtr.top());
    blocksContainerLtr.setHeight(_widgetContentLtr.height());

    return blocksContainerLtr;
}

QMargins ComplexMessageItemLayout::evaluateBlocksContentRectMargins() const
{
    auto margins = MessageStyle::getDefaultBlockBubbleMargins();

    assert(Item_);
    const auto &blocks = Item_->Blocks_;

    assert(!blocks.empty());
    if (blocks.empty())
        return margins;

    const auto &firstBlock = blocks.front();
    const auto firstBlockLayout = firstBlock->getBlockLayout();

    assert(firstBlockLayout);
    if (!firstBlockLayout)
        return margins;

    const auto &lastBlock = blocks.back();
    const auto lastBlockLayout = lastBlock->getBlockLayout();

    assert(lastBlockLayout);
    if (!lastBlockLayout)
        return margins;

    const auto &firstBoxModel = firstBlockLayout->getBlockBoxModel();
    margins.setTop(firstBoxModel.getBubbleMargins().top());

    const auto &lastBoxModel = lastBlockLayout->getBlockBoxModel();
    margins.setBottom(lastBoxModel.getBubbleMargins().bottom());

    return margins;
}

QRect ComplexMessageItemLayout::evaluateBlockLtr(
    const QRect &blocksContentLtr,
    IItemBlock *block,
    const int32_t blockY,
    const bool isBubbleRequired)
{
    assert(blocksContentLtr.width() > 0);
    assert(block);
    assert(blockY >= 0);

    auto blocksContentWidth = blocksContentLtr.width();

    assert(blocksContentWidth > 0);

    auto blockLeft = blocksContentLtr.left();

    QRect blockLtr(
        blockLeft,
        blockY,
        blocksContentWidth,
        0);

    if (isBubbleRequired && !block->isSizeLimited())
        return blockLtr;

    const auto blockSize = block->blockSizeForMaxWidth(blocksContentWidth);

    if (blockSize.isEmpty())
        return blockLtr;

    if (isOutgoing() && !isBubbleRequired)
    {
        const auto outgoingBlockLeft = blockLtr.right() + 1 - blockSize.width();
        blockLeft = std::max(outgoingBlockLeft, blockLeft);
    }

    assert(blockLeft > 0);
    blockLtr.setLeft(blockLeft);

    if (block->isSizeLimited() && blockSize.width() < blockLtr.width())
        blockLtr.setWidth(blockSize.width());

    assert(blockLtr.width() > 0);
    return blockLtr;
}

QRect ComplexMessageItemLayout::evaluateSenderContentLtr(const QRect& _widgetContentLtr, const QRect& _avatarRect) const
{
    assert(_widgetContentLtr.width() > 0 && _widgetContentLtr.height() >= 0);

    if (Item_->isSenderVisible())
    {
        auto blockOffsetHor = _widgetContentLtr.left() + MessageStyle::getBubbleHorPadding();
        if (_avatarRect.isValid())
            blockOffsetHor += _avatarRect.width() + MessageStyle::getAvatarRightMargin();

        const auto blockOffsetVer = _widgetContentLtr.top() + MessageStyle::getSenderTopPadding();
        Item_->Sender_->setOffsets(blockOffsetHor, blockOffsetVer + MessageStyle::getSenderBaseline());

        const auto botMargin = isHeaderOrSticker()
            ? MessageStyle::getSenderBottomMarginLarge()
            : MessageStyle::getSenderBottomMargin();

        return QRect(
            blockOffsetHor,
            blockOffsetVer,
            Item_->getSenderDesiredWidth(),
            MessageStyle::getSenderHeight() + botMargin
        );
    }

    return QRect();
}

QRect ComplexMessageItemLayout::evaluateWidgetContentLtr(const int32_t widgetWidth) const
{
    assert(widgetWidth > 0);

    const auto isOutgoing = this->isOutgoing();

    auto widgetContentLeftMargin = MessageStyle::getLeftMargin(isOutgoing, widgetWidth);
    if (!isOutgoing)
        widgetContentLeftMargin -= (MessageStyle::getAvatarSize() + MessageStyle::getAvatarRightMargin());

    auto widgetContentRightMargin = MessageStyle::getRightMargin(isOutgoing, widgetWidth);

    assert(widgetContentLeftMargin > 0);
    assert(widgetContentRightMargin > 0);

    auto widgetContentWidth = widgetWidth;
    widgetContentWidth -= widgetContentLeftMargin;
    widgetContentWidth -= widgetContentRightMargin;

    if (Item_->getMaxWidth() > 0)
    {
        int maxWidth = Item_->getMaxWidth();
        if (!isOutgoing && Item_->isChat())
            maxWidth += MessageStyle::getAvatarSize() + MessageStyle::getAvatarRightMargin();

        if (maxWidth < widgetContentWidth)
        {
            if (isOutgoing)
                widgetContentLeftMargin += (widgetContentWidth - maxWidth);

            widgetContentWidth = maxWidth;
        }
    }

    QRect result(
        widgetContentLeftMargin,
        MessageStyle::getTopMargin(Item_->hasTopMargin()),
        widgetContentWidth,
        0);


    return result;
}

void ComplexMessageItemLayout::setGeometry(const QRect &r)
{
    QLayout::setGeometry(r);

    setGeometryInternal(r.width());

    LastGeometry_ = r;
}

void ComplexMessageItemLayout::addItem(QLayoutItem* /*item*/)
{
}

QLayoutItem* ComplexMessageItemLayout::itemAt(int /*index*/) const
{
    return nullptr;
}

QLayoutItem* ComplexMessageItemLayout::takeAt(int /*index*/)
{
    return nullptr;
}

int ComplexMessageItemLayout::count() const
{
    return 0;
}

QSize ComplexMessageItemLayout::sizeHint() const
{
    const auto height = std::max(
        WidgetHeight_,
        MessageStyle::getMinBubbleHeight());

    return QSize(-1, height);
}

void ComplexMessageItemLayout::invalidate()
{
    QLayoutItem::invalidate();
}

bool ComplexMessageItemLayout::isOverAvatar(const QPoint &pos) const
{
    return getAvatarRect().contains(pos);
}

QRect ComplexMessageItemLayout::getAvatarRect() const
{
    return AvatarRect_;
}

const QRect& ComplexMessageItemLayout::getBlocksContentRect() const
{
    return BlocksContentRect_;
}

const QRect& ComplexMessageItemLayout::getBubbleRect() const
{
    return BubbleRect_;
}

QPoint ComplexMessageItemLayout::getShareButtonPos(const IItemBlock &block, const bool isBubbleRequired) const
{
    return block.getShareButtonPos(isBubbleRequired, BubbleRect_);
}

void ComplexMessageItemLayout::onBlockSizeChanged()
{
    if (LastGeometry_.width() <= 0)
        return;

    const auto sizeBefore = sizeHint();

    setGeometryInternal(LastGeometry_.width());

    const auto sizeAfter = sizeHint();

    if (sizeBefore != sizeAfter)
        update();
}

QPoint ComplexMessageItemLayout::getInitialTimePosition() const
{
    return initialTimePosition_;
}

void ComplexMessageItemLayout::setCustomBubbleBlockHorPadding(const int32_t _padding)
{
    bubbleBlockHorPadding_ = _padding;
    onBlockSizeChanged();
}

bool ComplexMessageItemLayout::hasSeparator(const IItemBlock *block) const
{
    const auto &blocks = Item_->Blocks_;

    auto blockIter = std::find(blocks.cbegin(), blocks.cend(), block);

    const auto isFirstBlock = (blockIter == blocks.begin());
    if (isFirstBlock)
        return false;

    auto blockLayout = (*blockIter)->getBlockLayout();
    if (!blockLayout)
        return false;

    auto prevBlockIter = blockIter;
    --prevBlockIter;

    const auto afterLastQuote =
        (*prevBlockIter)->getContentType() == IItemBlock::ContentType::Quote &&
        (*blockIter)->getContentType() != IItemBlock::ContentType::Quote;
    if (afterLastQuote)
        return false;

    const auto prevBlockLayout = (*prevBlockIter)->getBlockLayout();
    if (!prevBlockLayout)
        return false;

    const auto &prevBlockBox = prevBlockLayout->getBlockBoxModel();
    const auto &blockBox = blockLayout->getBlockBoxModel();

    const auto hasSeparator = (blockBox.hasLeadLines() || prevBlockBox.hasLeadLines());
    return hasSeparator;
}

bool ComplexMessageItemLayout::isOutgoing() const
{
    assert(Item_);
    return Item_->isOutgoing();
}

bool ComplexMessageItemLayout::isHeaderOrSticker() const
{
    return Item_->isHeaderRequired() || Item_->isSingleSticker();
}

QRect ComplexMessageItemLayout::setBlocksGeometry(
    const bool _isBubbleRequired,
    const QRect& _messageRect,
    const QRect& _senderRect)
{
    const auto topY = _senderRect.isValid() ? _senderRect.bottom() + 1 : _messageRect.top();
    auto blocksHeight = 0;
    auto blocksWidth = 0;
    auto blocksLeft = isOutgoing() ? _messageRect.left() + _messageRect.width() : _messageRect.left();

    auto& blocks = Item_->Blocks_;
    for (auto block : blocks)
    {
        assert(block);

        const auto blockSeparatorHeight = hasSeparator(block) ? MessageStyle::getBlocksSeparatorVertMargins(): 0;
        const auto blockY = topY + blocksHeight + blockSeparatorHeight;

        auto blockLtr = evaluateBlockLtr(_messageRect, block, blockY, _isBubbleRequired);

        auto widthAdjust = 0;
        if (block->getQuote().isForward_)
        {
            blockLtr.adjust(-MessageStyle::Quote::getFwdLeftOverflow(), 0, MessageStyle::Quote::getFwdLeftOverflow(), 0);
            widthAdjust = 2 * MessageStyle::Quote::getFwdLeftOverflow();
        }

        const auto blockGeometry = block->setBlockGeometry(blockLtr);

        const auto blockHeight = blockGeometry.height();
        assert(blocksHeight >= 0);

        blocksHeight += blockHeight + blockSeparatorHeight;

        if (block->getContentType() == IItemBlock::ContentType::Quote && blocks.back() != block)
            blocksHeight += (block->getQuote().isLastQuote_ ? MessageStyle::Quote::getSpacingAfterLastQuote() : MessageStyle::Quote::getQuoteSpacing());

        blocksWidth = std::max(blocksWidth, blockGeometry.width() - widthAdjust);

        if (isOutgoing())
            blocksLeft = std::min(blocksLeft, blockGeometry.left());
        else
            blocksLeft = std::max(blocksLeft, blockGeometry.left());
    }

    if (Item_->TimeWidget_)
        blocksWidth = std::max(blocksWidth, Item_->TimeWidget_->width() - MessageStyle::getTimeLeftSpacing() / 2);

    if (_isBubbleRequired && !blocks.empty())
    {
        if (isOutgoing() && blocksWidth < _messageRect.width())
        {
            const auto shift = _messageRect.width() - blocksWidth;
            for (auto block : blocks)
                block->shiftHorizontally(shift);
            blocksLeft += shift;
        }

        const auto lastBlock = blocks.back();
        const auto lastBlockType = lastBlock->getContentType();

        if (isOutgoing() && lastBlockType == IItemBlock::ContentType::Text)
        {
            if (Item_->TimeWidget_ && !Item_->TimeWidget_->isUnderlayVisible())
            {
                const auto lastBlockWidth = lastBlock->getBlockLayout()->getBlockGeometry().width();
                const auto lastBlockHeight = lastBlock->getBlockLayout()->getBlockGeometry().height();
                if (_messageRect.width() - lastBlockWidth > MessageStyle::getTimeLeftSpacing()
                    && _messageRect.width() - lastBlockWidth < Item_->TimeWidget_->width())
                {
                    if (lastBlockHeight < Item_->TimeWidget_->height() + Item_->TimeWidget_->getVerMargin() + MessageStyle::getShiftedTimeTopMargin())
                        blocksHeight += MessageStyle::getShiftedTimeTopMargin() + Item_->TimeWidget_->height();
                }
            }
        }

        if (lastBlockType != IItemBlock::ContentType::Text && Item_->TimeWidget_ && !Item_->TimeWidget_->isUnderlayVisible())
        {
            const auto tw = MessageStyle::getTimeLeftSpacing() + Item_->TimeWidget_->width();
            const auto th = MessageStyle::getShiftedTimeTopMargin() + Item_->TimeWidget_->height();

            if (lastBlock->isNeedCheckTimeShift())
            {
                blocksHeight += th;
            }
            else
            {
                if (const auto layout = lastBlock->getBlockLayout())
                {
                    QRect lastBlockGeometry = layout->getBlockGeometry();
                    if (blocks.size() > 1)
                    {
                        const bool enoughWidth = blocksWidth - lastBlockGeometry.width() > tw;
                        const bool rightAligned = enoughWidth && (blocksLeft + blocksWidth) - (lastBlockGeometry.right() + 1) < tw;
                        if (!enoughWidth || rightAligned)
                            blocksHeight += th;
                    }
                    else
                    {
                        if (lastBlockType != IItemBlock::ContentType::FileSharing)
                        {
                            const bool enoughHeight = blocksHeight - lastBlockGeometry.height() > MessageStyle::getShiftedTimeTopMargin();
                            if (!enoughHeight)
                                blocksHeight += th;
                        }

                    }
                }
            }
        }
    }

    assert(blocksWidth >= 0);
    return QRect(blocksLeft, topY, blocksWidth, blocksHeight);
}

void ComplexMessageItemLayout::setGeometryInternal(const int32_t widgetWidth)
{
    assert(widgetWidth > 0);

    auto widgetContentLtr = evaluateWidgetContentLtr(widgetWidth);

    const auto enoughSpace = (widgetContentLtr.width() > 0);
    if (!enoughSpace)
        return;

    AvatarRect_ = (Item_->isChat() && !isOutgoing()) ? evaluateAvatarRect(widgetContentLtr) : QRect(); // vertical pos invalid at this point

    const auto senderRect = evaluateSenderContentLtr(widgetContentLtr, AvatarRect_); // including bottom margin
    auto messageContentRect = evaluateBlocksContainerLtr(AvatarRect_, widgetContentLtr);

    const auto isBubbleRequired = Item_->isBubbleRequired();
    if (isBubbleRequired)
        messageContentRect.adjust(
            bubbleBlockHorPadding_,
            MessageStyle::getBubbleVerPadding(),
            -bubbleBlockHorPadding_,
            0
        );

    const auto blocksGeometry = setBlocksGeometry(isBubbleRequired, messageContentRect, senderRect);

    BlocksContentRect_ = blocksGeometry;

    messageContentRect.setHeight(blocksGeometry.height() + senderRect.height());

    if (Item_->isSingleSticker())
    {
        messageContentRect.setWidth(std::min(messageContentRect.width(), MessageStyle::getSenderStickerWidth()));
    }
    else
    {
        if (isOutgoing())
        {
            messageContentRect.setLeft(blocksGeometry.left());
        }
        else
        {
            if (!Item_->isHeaderRequired() && Item_->isSenderVisible())
            {
                if (senderRect.width() < blocksGeometry.width() || Item_->isSmallPreview())
                    messageContentRect.setWidth(blocksGeometry.width());
                else if (senderRect.width() >= blocksGeometry.width() && senderRect.width() <= messageContentRect.width())
                    messageContentRect.setWidth(senderRect.width());
            }
            else
                messageContentRect.setWidth(blocksGeometry.width());
        }
    }

    if (Item_->isSenderVisible())
    {
        const auto availableWidth = messageContentRect.width() - (isHeaderOrSticker() ? 2 * MessageStyle::getBubbleHorPadding() : 0);
        Item_->Sender_->getHeight(availableWidth);
    }

    auto bubbleRect = evaluateBlocksBubbleGeometry(isBubbleRequired, messageContentRect);

    assert(bubbleRect.height() >= MessageStyle::getMinBubbleHeight());

    setTimePosition(isBubbleRequired ? bubbleRect : blocksGeometry);

    BubbleRect_ = bubbleRect;

    if (AvatarRect_.isValid())  // set valid vertical pos
    {
        if (Item_->isSingleSticker())
            AvatarRect_.moveBottom(blocksGeometry.bottom());
        else
            AvatarRect_.moveBottom(BubbleRect_.bottom());
    }

    WidgetHeight_ = (bubbleRect.bottom() + 1) + Item_->bottomOffset();

    assert(WidgetHeight_ >= 0);
}

void ComplexMessageItemLayout::setTimePosition(const QRect& _availableGeometry)
{
    auto timeWidget = Item_->TimeWidget_;

    if (!Item_->TimeWidget_)
        return;

    if (!_availableGeometry.isValid() || !timeWidget || !timeWidget->size().isValid())
        return;

    const auto x = _availableGeometry.right() + 1 - timeWidget->width() - timeWidget->getHorMargin();
    const auto y = _availableGeometry.bottom() + 1 - timeWidget->height() - timeWidget->getVerMargin();

    timeWidget->move(x, y);
    initialTimePosition_ = timeWidget->pos();
}

UI_COMPLEX_MESSAGE_NS_END
