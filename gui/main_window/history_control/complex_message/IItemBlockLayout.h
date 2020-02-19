#pragma once

#include "../../../namespaces.h"

UI_COMPLEX_MESSAGE_NS_BEGIN

class IItemBlockLayout
{
public:

    virtual ~IItemBlockLayout() = 0;

    virtual QSize blockSizeForMaxWidth(const int32_t maxWidth) = 0;

    virtual QSize blockSizeHint() const = 0;

    virtual QRect getBlockGeometry() const = 0;

    virtual QRect setBlockGeometry(const QRect &ltr) = 0;

    virtual bool onBlockContentsChanged() = 0;

    virtual void shiftHorizontally(const int _shift) = 0;

    virtual void markDirty() {}

    virtual void resizeBlock(const QSize& _size) = 0;

    virtual void onBlockSizeChanged(const QSize& _size) {}
};

UI_COMPLEX_MESSAGE_NS_END
