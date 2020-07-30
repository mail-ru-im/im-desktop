#pragma once

#include "IFileSharingBlockLayout.h"

UI_COMPLEX_MESSAGE_NS_BEGIN

class FileSharingBlock;

class FileSharingPlainBlockLayout final : public IFileSharingBlockLayout
{
public:
    FileSharingPlainBlockLayout();

    virtual ~FileSharingPlainBlockLayout() override;

    QSize setBlockGeometryInternal(const QRect &geometry) override;
    QSize blockSizeForMaxWidth(const int32_t maxWidth) override;

private:
    QRect contentRect_;

    bool isStandalone() const;
    QSize blockSizeForWidth(const int _blockWidth) const;
    const QRect& getContentRect() const override;

    //unused
    const QRect& getFilenameRect() const override;
    QRect getFileSizeRect() const override { return QRect(); }
    QRect getShowInDirLinkRect() const override { return QRect(); }
};

UI_COMPLEX_MESSAGE_NS_END