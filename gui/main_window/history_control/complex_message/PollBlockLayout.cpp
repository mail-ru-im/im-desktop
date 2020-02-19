#include "stdafx.h"

#include "PollBlock.h"
#include "PollBlockLayout.h"

UI_COMPLEX_MESSAGE_NS_BEGIN

PollBlockLayout::PollBlockLayout()
{

}

PollBlockLayout::~PollBlockLayout()
{

}

QSize PollBlockLayout::setBlockGeometryInternal(const QRect& _geometry)
{
    auto block = blockWidget<PollBlock>();
    return block->setBlockSize(_geometry.size());
}

UI_COMPLEX_MESSAGE_NS_END
