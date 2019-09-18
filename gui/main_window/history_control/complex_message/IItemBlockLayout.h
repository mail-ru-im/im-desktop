#pragma once

#include "../../../namespaces.h"

UI_COMPLEX_MESSAGE_NS_BEGIN

class IItemBlockLayout
{
public:
    class IBoxModel
    {
    public:
        virtual ~IBoxModel() = 0;

        virtual bool hasLeadLines() const = 0;

        virtual QMargins getBubbleMargins() const = 0;

    };

    virtual ~IItemBlockLayout() = 0;

    virtual QSize blockSizeForMaxWidth(const int32_t maxWidth) = 0;

    virtual QSize blockSizeHint() const = 0;

    virtual QRect getBlockGeometry() const = 0;

    virtual const IBoxModel& getBlockBoxModel() const = 0;

    virtual QRect setBlockGeometry(const QRect &ltr) = 0;

    virtual bool onBlockContentsChanged() = 0;

    virtual void shiftHorizontally(const int _shift) = 0;

    virtual void markDirty() {}
};

UI_COMPLEX_MESSAGE_NS_END