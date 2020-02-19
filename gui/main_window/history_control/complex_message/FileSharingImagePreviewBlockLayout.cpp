#include "stdafx.h"

#include "../../../utils/log/log.h"
#include "../../../utils/utils.h"

#include "../MessageStyle.h"

#include "ComplexMessageUtils.h"
#include "FileSharingBlock.h"
#include "MediaUtils.h"


#include "FileSharingImagePreviewBlockLayout.h"

UI_COMPLEX_MESSAGE_NS_BEGIN

FileSharingImagePreviewBlockLayout::FileSharingImagePreviewBlockLayout() = default;

FileSharingImagePreviewBlockLayout::~FileSharingImagePreviewBlockLayout() = default;

QSize FileSharingImagePreviewBlockLayout::blockSizeForMaxWidth(const int32_t maxWidth)
{
    return calcBlockSize(maxWidth);
}

const QRect& FileSharingImagePreviewBlockLayout::getContentRect() const
{
    return PreviewRect_;
}

const QRect& FileSharingImagePreviewBlockLayout::getFilenameRect() const
{
    static QRect empty;
    return empty;
}

QRect FileSharingImagePreviewBlockLayout::getFileSizeRect() const
{
    return QRect();
}

QRect FileSharingImagePreviewBlockLayout::getShowInDirLinkRect() const
{
    return QRect();
}

void FileSharingImagePreviewBlockLayout::onBlockSizeChanged(const QSize& _size)
{
    auto &block = *blockWidget<FileSharingBlock>();
    PreviewRect_.setSize(_size);
    setCtrlButtonGeometry(block, PreviewRect_);
}

QSize FileSharingImagePreviewBlockLayout::setBlockGeometryInternal(const QRect &geometry)
{
    auto block = blockWidget<FileSharingBlock>();

    const auto blockSize = calcBlockSize(geometry.width());

    PreviewRect_ = QRect(QPoint(), blockSize);
    setCtrlButtonGeometry(*block, PreviewRect_);

    return blockSize;
}

void FileSharingImagePreviewBlockLayout::setCtrlButtonGeometry(FileSharingBlock &block, const QRect &previewRect)
{
    if (previewRect.isEmpty())
        return;

    const auto buttonSize = block.getCtrlButtonSize();

    if (buttonSize.isEmpty())
        return;

    QRect buttonRect(QPoint(), buttonSize);

    buttonRect.moveCenter(previewRect.center());

    block.setCtrlButtonGeometry(buttonRect);
}

QSize FileSharingImagePreviewBlockLayout::calcBlockSize(int _availableWidth)
{
    auto &block = *blockWidget<FileSharingBlock>();

    MediaUtils::MediaBlockParams params;
    params.availableWidth = _availableWidth;
    params.isInsideQuote = block.isInsideQuote();
    params.mediaSize = block.originSizeScaled();
    params.mediaType = block.mediaType();
    params.minBlockWidth = block.minControlsWidth();


    return MediaUtils::calcMediaBlockSize(params);
}

UI_COMPLEX_MESSAGE_NS_END
