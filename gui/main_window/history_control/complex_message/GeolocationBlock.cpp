#include "stdafx.h"

#include "GeolocationBlock.h"
#include "GeolocationBlockLayout.h"

UI_COMPLEX_MESSAGE_NS_BEGIN

GeolocationBlock::GeolocationBlock(ComplexMessageItem* _parent, const QString& _link)
    : LinkPreviewBlock(_parent, _link, false)
{

}

MediaType GeolocationBlock::getMediaType() const
{
    return Ui::MediaType::mediaTypeGeo;
}

QString GeolocationBlock::formatRecentsText() const
{
    return getTitle();
}

void GeolocationBlock::drawFavicon(QPainter &p)
{
    Q_UNUSED(p)
}

void GeolocationBlock::drawSiteName(QPainter &p)
{
    Q_UNUSED(p)
}

std::unique_ptr<YoutubeLinkPreviewBlockLayout> GeolocationBlock::createLayout()
{
    return std::make_unique<GeolocationBlockLayout>();
}

QString GeolocationBlock::getTitle() const
{
    return QT_TRANSLATE_NOOP("geolocation_block", "Location");
}

UI_COMPLEX_MESSAGE_NS_END
