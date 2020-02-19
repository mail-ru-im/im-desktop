#pragma once

#include "YoutubeLinkPreviewBlockLayout.h"

UI_COMPLEX_MESSAGE_NS_BEGIN

class GeolocationBlockLayout : public YoutubeLinkPreviewBlockLayout
{
public:
    GeolocationBlockLayout();

protected:
    QRect evaluateFaviconImageRect(const LinkPreviewBlock &block, const QRect &titleGeometry) const override;
    QRect evaluateSiteNameGeometry(const LinkPreviewBlock &block, const QRect &titleGeometry, const QRect &faviconGeometry) const override;
};


UI_COMPLEX_MESSAGE_NS_END
