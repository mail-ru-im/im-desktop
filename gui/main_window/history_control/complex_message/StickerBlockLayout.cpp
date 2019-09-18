#include "stdafx.h"

#include "../MessageStyle.h"

#include "../../../utils/utils.h"

#include "StickerBlock.h"

#include "StickerBlockLayout.h"



UI_COMPLEX_MESSAGE_NS_BEGIN

StickerBlockLayout::StickerBlockLayout() = default;

StickerBlockLayout::~StickerBlockLayout() = default;

QSize StickerBlockLayout::blockSizeForMaxWidth(const int32_t maxWidth)
{
    assert(maxWidth > 0);

    auto &block = *blockWidget<StickerBlock>();

    return block.LastSize_;
}

const IItemBlockLayout::IBoxModel& StickerBlockLayout::getBlockBoxModel() const
{
    static const QMargins bubbleMargins(
        0,
        Utils::scale_value(4),
        0,
        Utils::scale_value(8)
    );

    static const BoxModel boxModel(
        false,
        bubbleMargins);

    return boxModel;

}

QRect StickerBlockLayout::getBlockGeometry() const
{
    auto &block = *blockWidget<StickerBlock>();
    return block.Geometry_;
}

QSize StickerBlockLayout::setBlockGeometryInternal(const QRect &widgetGeometry)
{
    QLayout::setGeometry(widgetGeometry);

    auto &block = *blockWidget<StickerBlock>();
    auto r = block.setBlockGeometry(widgetGeometry);

    return QSize(r.width(), r.height());
}

UI_COMPLEX_MESSAGE_NS_END