#include "GeolocationBlockLayout.h"

UI_COMPLEX_MESSAGE_NS_BEGIN

GeolocationBlockLayout::GeolocationBlockLayout()
{

}

QRect GeolocationBlockLayout::evaluateFaviconImageRect(const Ui::ComplexMessage::LinkPreviewBlock& block, const QRect& titleGeometry) const
{
    return QRect();
}

QRect GeolocationBlockLayout::evaluateSiteNameGeometry(const Ui::ComplexMessage::LinkPreviewBlock& block, const QRect& titleGeometry, const QRect& faviconGeometry) const
{
    return QRect();
}

UI_COMPLEX_MESSAGE_NS_END
