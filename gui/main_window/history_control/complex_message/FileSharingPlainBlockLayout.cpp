#include "stdafx.h"

#include "../../../fonts.h"
#include "../../../utils/Text.h"
#include "../../../utils/utils.h"

#include "../MessageStyle.h"

#include "FileSharingBlock.h"


#include "FileSharingPlainBlockLayout.h"

UI_COMPLEX_MESSAGE_NS_BEGIN

FileSharingPlainBlockLayout::FileSharingPlainBlockLayout() = default;

FileSharingPlainBlockLayout::~FileSharingPlainBlockLayout() = default;

QSize FileSharingPlainBlockLayout::blockSizeForMaxWidth(const int32_t maxWidth)
{
    return blockSizeForWidth(maxWidth);
}

const QRect& FileSharingPlainBlockLayout::getContentRect() const
{
    return contentRect_;
}

const QRect& FileSharingPlainBlockLayout::getFilenameRect() const
{
    static const QRect r;
    return r;
}

QSize FileSharingPlainBlockLayout::setBlockGeometryInternal(const QRect &geometry)
{
    contentRect_ = QRect(geometry.topLeft(), blockSizeForWidth(geometry.width()));
    return contentRect_.size();
}

bool FileSharingPlainBlockLayout::isStandalone() const
{
    auto &block = *blockWidget<FileSharingBlock>();
    return block.isStandalone();
}

QSize FileSharingPlainBlockLayout::blockSizeForWidth(const int _blockWidth) const
{
    auto width = _blockWidth;

    auto &block = *blockWidget<FileSharingBlock>();

    if (const auto fnW = block.filenameDesiredWidth(); fnW != 0)
    {
        width = MessageStyle::Files::getIconSize().width() + MessageStyle::Files::getFilenameLeftMargin() + fnW;

        if (!isStandalone())
            width += MessageStyle::Files::getCtrlIconLeftMargin() + MessageStyle::Files::getFilenameLeftMargin();

        width = std::max(width, MessageStyle::getFileSharingDesiredWidth());
    }

    width = std::min(width, MessageStyle::getFileSharingMaxWidth());
    width = std::min(width, _blockWidth);

    auto height = MessageStyle::Files::getFileBubbleHeight() + (block.isSenderVisible() ? MessageStyle::Files::getVerMarginWithSender() : 0);
    if (!isStandalone())
        height += 2 * MessageStyle::Files::getVerMargin();

    return QSize(width, height);
}

UI_COMPLEX_MESSAGE_NS_END
