#pragma once

#include "../../../namespaces.h"

#include "IFileSharingBlockLayout.h"

UI_COMPLEX_MESSAGE_NS_BEGIN

class FileSharingBlock;

class FileSharingImagePreviewBlockLayout final : public IFileSharingBlockLayout
{
public:
    FileSharingImagePreviewBlockLayout();

    virtual ~FileSharingImagePreviewBlockLayout() override;

    virtual QSize blockSizeForMaxWidth(const int32_t maxWidth) override;

    virtual const QRect& getContentRect() const override;

    virtual QSize setBlockGeometryInternal(const QRect &geometry) override;

    virtual const QRect& getFilenameRect() const override;

    virtual QRect getFileSizeRect() const override;

    virtual QRect getShowInDirLinkRect() const override;

    void onBlockSizeChanged(const QSize& _size) override;

private:
    QRect PreviewRect_;

    void setCtrlButtonGeometry(FileSharingBlock &block, const QRect &previewRect);   

    QSize calcBlockSize(int _availableWidth);
};

UI_COMPLEX_MESSAGE_NS_END
