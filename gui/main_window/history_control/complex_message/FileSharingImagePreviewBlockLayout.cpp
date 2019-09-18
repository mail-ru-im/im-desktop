#include "stdafx.h"

#include "../../../utils/log/log.h"
#include "../../../utils/utils.h"

#include "../MessageStyle.h"

#include "ComplexMessageUtils.h"
#include "FileSharingBlock.h"


#include "FileSharingImagePreviewBlockLayout.h"

UI_COMPLEX_MESSAGE_NS_BEGIN

FileSharingImagePreviewBlockLayout::FileSharingImagePreviewBlockLayout() = default;

FileSharingImagePreviewBlockLayout::~FileSharingImagePreviewBlockLayout() = default;

QSize FileSharingImagePreviewBlockLayout::blockSizeForMaxWidth(const int32_t maxWidth)
{
    auto &block = *blockWidget<FileSharingBlock>();

    const auto smallSize = block.getSmallPreviewSize();
    auto previewSize = Utils::scale_value(block.getOriginalPreviewSize());

    if (!block.isSticker() && previewSize.width() <= smallSize.width() && previewSize.height() <= smallSize.height())
        return smallSize;

    const QSize maxImageSize = block.getImageMaxSize();

    const auto maxSizeWidth = std::min(Utils::scale_value(maxWidth), maxImageSize.width());
    const QSize maxSize(maxSizeWidth, maxImageSize.height());

    const auto minSize = getMinSize(block);
    previewSize = limitSize(previewSize, maxSize, minSize);

    const auto shouldScaleUp = previewSize.width() < minSize.width() && previewSize.height() < minSize.height();
    if (shouldScaleUp)
        previewSize = previewSize.scaled(minSize, Qt::KeepAspectRatio);

    return previewSize;
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

const IItemBlockLayout::IBoxModel& FileSharingImagePreviewBlockLayout::getBlockBoxModel() const
{
    static const QMargins margins(
        Utils::scale_value(16),
        Utils::scale_value(12),
        Utils::scale_value(8),
        Utils::scale_value(16));

    static const BoxModel boxModel(
        true,
        margins);

    return boxModel;
}

QSize FileSharingImagePreviewBlockLayout::setBlockGeometryInternal(const QRect &geometry)
{
    auto &block = *blockWidget<FileSharingBlock>();

    PreviewRect_ = QRect(QPoint(), evaluatePreviewSize(block, geometry.width()));
    setCtrlButtonGeometry(block, PreviewRect_);

    auto blockSize = PreviewRect_.size();

    return blockSize;
}

QSize FileSharingImagePreviewBlockLayout::evaluatePreviewSize(const FileSharingBlock &block, const int32_t blockWidth) const
{
    assert(blockWidth > 0);

    const auto smallSize = block.getSmallPreviewSize();
    auto previewSize = Utils::scale_value(block.getOriginalPreviewSize());

    if (!block.isSticker() && previewSize.width() <= smallSize.width() && previewSize.height() <= smallSize.height())
        return smallSize;

    auto maxSizeWidth = blockWidth;
    if (const auto maxWidth = block.getMaxPreviewWidth())
        maxSizeWidth = std::min(maxSizeWidth, maxWidth);
    const QSize maxSize(maxSizeWidth, MessageStyle::Preview::getImageHeightMax());

    const auto minSize = getMinSize(block);
    previewSize = limitSize(previewSize, maxSize, minSize);

    const auto shouldScaleUp = previewSize.width() < minSize.width() && previewSize.height() < minSize.height();
    if (shouldScaleUp)
        previewSize = previewSize.scaled(minSize, Qt::KeepAspectRatio);

    return previewSize;
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

QSize FileSharingImagePreviewBlockLayout::getMinSize(const FileSharingBlock & _block) const
{
    return _block.isSticker() ? QSize() : _block.getMinPreviewSize();
}

UI_COMPLEX_MESSAGE_NS_END
