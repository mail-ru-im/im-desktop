#pragma once

#include "GenericBlockLayout.h"

UI_COMPLEX_MESSAGE_NS_BEGIN

class StickerBlockLayout final : public GenericBlockLayout
{
public:
    StickerBlockLayout();

    virtual ~StickerBlockLayout() override;

    virtual QSize blockSizeForMaxWidth(const int32_t maxWidth) override;

    const IItemBlockLayout::IBoxModel& getBlockBoxModel() const override;

    virtual QRect getBlockGeometry() const override;

protected:
    virtual QSize setBlockGeometryInternal(const QRect &widgetGeometry) override;

};

UI_COMPLEX_MESSAGE_NS_END