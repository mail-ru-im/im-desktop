#pragma once

#include "GenericBlock.h"
#include "GenericBlockLayout.h"


UI_COMPLEX_MESSAGE_NS_BEGIN

class ProfileBlockLayout: public GenericBlockLayout
{
public:
    ProfileBlockLayout();

    virtual ~ProfileBlockLayout() override;

    QRect getContentRect() const;
    QSize blockSizeForMaxWidth(const int32_t maxWidth) override;

protected:
    virtual QSize setBlockGeometryInternal(const QRect &widgetGeometry) override;

protected:
    QRect contentRect_;
};


UI_COMPLEX_MESSAGE_NS_END
