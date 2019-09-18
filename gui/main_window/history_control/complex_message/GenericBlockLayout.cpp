#include "stdafx.h"

#include "../../../utils/utils.h"

#include "../MessageStyle.h"

#include "GenericBlock.h"
#include "ComplexMessageItem.h"


#include "GenericBlockLayout.h"

UI_COMPLEX_MESSAGE_NS_BEGIN

GenericBlockLayout::BoxModel::BoxModel(const bool hasLeadLines, const QMargins &bubbleMargins)
    : HasLeadLines_(hasLeadLines)
    , BubbleMargins_(bubbleMargins)
{
}

GenericBlockLayout::BoxModel::~BoxModel() = default;

QMargins GenericBlockLayout::BoxModel::getBubbleMargins() const
{
    return BubbleMargins_;
}

bool GenericBlockLayout::BoxModel::hasLeadLines() const
{
    return HasLeadLines_;
}

GenericBlockLayout::GenericBlockLayout() = default;

GenericBlockLayout::~GenericBlockLayout() = default;

void GenericBlockLayout::addItem(QLayoutItem* /*item*/)
{
}

QLayoutItem* GenericBlockLayout::itemAt(int /*index*/) const
{
    return nullptr;
}

QLayoutItem* GenericBlockLayout::takeAt(int /*index*/)
{
    return nullptr;
}

int GenericBlockLayout::count() const
{
    return 0;
}

void GenericBlockLayout::invalidate()
{
    QLayoutItem::invalidate();
}

void GenericBlockLayout::setGeometry(const QRect &r)
{
    QLayout::setGeometry(r);

    const auto isResize = (r.topLeft() == QPoint());
    if (isResize)
        return;

    const QRect internalRect(QPoint(), r.size());
    BlockSize_ = setBlockGeometryInternal(internalRect);
    if (!BlockSize_.isValid())
        return;

    assert(BlockSize_.height() >= 0);
    assert(BlockSize_.width() > 0);

    BlockGeometry_ = QRect(r.topLeft(), BlockSize_);
}

QSize GenericBlockLayout::sizeHint() const
{
    const auto blockHeight = BlockSize_.height();
    assert(blockHeight >= -1);

    const auto height = std::max(
        blockHeight,
        MessageStyle::getMinBubbleHeight());

    return QSize(-1, height);
}

QSize GenericBlockLayout::blockSizeForMaxWidth(const int32_t maxWidth)
{
    assert(maxWidth > 0);

    return QSize();
}

QSize GenericBlockLayout::blockSizeHint() const
{
    return sizeHint();
}

const IItemBlockLayout::IBoxModel& GenericBlockLayout::getBlockBoxModel() const
{
    static BoxModel boxModel(
        false,
        MessageStyle::getDefaultBlockBubbleMargins());

    return boxModel;
}

QRect GenericBlockLayout::getBlockGeometry() const
{
    return BlockGeometry_;
}

bool GenericBlockLayout::onBlockContentsChanged()
{
    const auto sizeBefore = BlockSize_;

    const auto &blockGeometry = getBlockGeometry();
    const auto noGeometry = ((blockGeometry.width() == 0) && (blockGeometry.height() == 0));
    if (noGeometry)
    {
        return false;
    }

    BlockSize_ = setBlockGeometryInternal(blockGeometry);

    const auto isSizeChanged = (sizeBefore != BlockSize_);
    if (isSizeChanged)
    {
        update();
    }

    return isSizeChanged;
}

QRect GenericBlockLayout::setBlockGeometry(const QRect &ltr)
{
    setGeometry(ltr);

    return QRect(
        ltr.left(),
        ltr.top(),
        BlockSize_.width(),
        BlockSize_.height());
}

void GenericBlockLayout::shiftHorizontally(const int _shift)
{
    if (BlockGeometry_.isValid())
        BlockGeometry_.translate(_shift, 0);
}

UI_COMPLEX_MESSAGE_NS_END