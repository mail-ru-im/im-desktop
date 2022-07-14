#include "stdafx.h"

#include "utils/utils.h"
#include "ProfileBlockLayout.h"
#include "ProfileBlock.h"

using namespace Ui::ComplexMessage;

ProfileBlockLayout::ProfileBlockLayout() = default;

ProfileBlockLayout::~ProfileBlockLayout() = default;

QRect ProfileBlockLayout::getContentRect() const
{
    return contentRect_;
}

QSize ProfileBlockLayout::setBlockGeometryInternal(const QRect& _widgetGeometry)
{
    auto &block = *blockWidget<ProfileBlockBase>();
    auto width = _widgetGeometry.width();
    contentRect_ = QRect(_widgetGeometry.topLeft(), QSize(width, block.updateSizeForWidth(width)));

    return contentRect_.size();
}
