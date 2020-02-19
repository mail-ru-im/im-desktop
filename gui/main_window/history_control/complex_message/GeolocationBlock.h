#pragma once

#include "LinkPreviewBlock.h"

UI_COMPLEX_MESSAGE_NS_BEGIN

class GeolocationBlock : public LinkPreviewBlock
{
public:
    GeolocationBlock(ComplexMessageItem* _parent, const QString& _link);

    MediaType getMediaType() const override;

    QString formatRecentsText() const override;

protected:

    void drawFavicon(QPainter &p) override;

    void drawSiteName(QPainter &p) override;

    std::unique_ptr<YoutubeLinkPreviewBlockLayout> createLayout() override;

    QString getTitle() const override;
};

UI_COMPLEX_MESSAGE_NS_END
