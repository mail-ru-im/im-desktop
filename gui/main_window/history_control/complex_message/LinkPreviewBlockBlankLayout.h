#pragma once

#include "GenericBlockLayout.h"
#include "ILinkPreviewBlockLayout.h"

UI_COMPLEX_MESSAGE_NS_BEGIN

class LinkPreviewBlockBlankLayout final
    : public GenericBlockLayout
    , public ILinkPreviewBlockLayout
{
public:
    LinkPreviewBlockBlankLayout();

    virtual ~LinkPreviewBlockBlankLayout() override;

    virtual IItemBlockLayout* asBlockLayout() override;

    virtual QLayout* asQLayout() override;

    virtual QSize getMaxPreviewSize() const override;

    virtual QRect getFaviconImageRect() const override;

    virtual QRect getPreviewImageRect() const override;

    virtual QPoint getSiteNamePos() const override;

    virtual QRect getSiteNameRect() const override;

    virtual QFont getTitleFont() const override;

protected:
    virtual QSize setBlockGeometryInternal(const QRect &geometry) override;

};

UI_COMPLEX_MESSAGE_NS_END