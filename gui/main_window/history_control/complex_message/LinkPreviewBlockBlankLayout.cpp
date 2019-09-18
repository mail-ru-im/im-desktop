#include "stdafx.h"

#include "LinkPreviewBlockBlankLayout.h"

UI_COMPLEX_MESSAGE_NS_BEGIN

LinkPreviewBlockBlankLayout::LinkPreviewBlockBlankLayout() = default;

LinkPreviewBlockBlankLayout::~LinkPreviewBlockBlankLayout() = default;

IItemBlockLayout* LinkPreviewBlockBlankLayout::asBlockLayout()
{
    return this;
}

QLayout* LinkPreviewBlockBlankLayout::asQLayout()
{
    return this;
}

QSize LinkPreviewBlockBlankLayout::getMaxPreviewSize() const
{
    return QSize(0, 0);
}

QRect LinkPreviewBlockBlankLayout::getFaviconImageRect() const
{
    return QRect();
}

QRect LinkPreviewBlockBlankLayout::getPreviewImageRect() const
{
    return QRect();
}

QPoint LinkPreviewBlockBlankLayout::getSiteNamePos() const
{
    return QPoint();
}

QRect LinkPreviewBlockBlankLayout::getSiteNameRect() const
{
    return QRect();
}

QFont LinkPreviewBlockBlankLayout::getTitleFont() const
{
    return QFont();
}

QSize LinkPreviewBlockBlankLayout::setBlockGeometryInternal(const QRect &geometry)
{
    assert(geometry.height() >= 0);

    const QSize blockSize(geometry.width(), 0);

    return blockSize;
}

UI_COMPLEX_MESSAGE_NS_END