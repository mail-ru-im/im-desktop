#pragma once

#include "GenericBlock.h"
#include "GenericBlockLayout.h"

UI_COMPLEX_MESSAGE_NS_BEGIN

class PollBlockLayout : public GenericBlockLayout
{
public:
    PollBlockLayout();    
    ~PollBlockLayout() override;

protected:
    QSize setBlockGeometryInternal(const QRect& _geometry) override;

};

UI_COMPLEX_MESSAGE_NS_END
