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

QSize ProfileBlockLayout::blockSizeForMaxWidth(const int32_t maxWidth)
{
    auto &block = *blockWidget<ProfileBlockBase>();
    auto width = std::min(maxWidth, block.desiredWidth());

    return QSize(width, block.getHeightForWidth(width));
}

QSize ProfileBlockLayout::setBlockGeometryInternal(const QRect& widgetGeometry)
{
    auto &block = *blockWidget<ProfileBlockBase>();

    auto width = widgetGeometry.width();
    contentRect_ = QRect(widgetGeometry.topLeft(), QSize(width, block.getHeightForWidth(width)));

    return contentRect_.size();
}
