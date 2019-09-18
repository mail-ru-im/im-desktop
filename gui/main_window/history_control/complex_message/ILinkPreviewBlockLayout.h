#pragma once

#include "../../../namespaces.h"

UI_COMPLEX_MESSAGE_NS_BEGIN

class IItemBlockLayout;

class ILinkPreviewBlockLayout
{
public:
    virtual ~ILinkPreviewBlockLayout();

    virtual IItemBlockLayout* asBlockLayout() = 0;

    virtual QLayout* asQLayout() = 0;

    virtual QSize getMaxPreviewSize() const = 0;

    virtual QRect getFaviconImageRect() const = 0;

    virtual QRect getPreviewImageRect() const = 0;

    virtual QPoint getSiteNamePos() const = 0;

    virtual QRect getSiteNameRect() const = 0;

    virtual QFont getTitleFont() const = 0;

    virtual QRect getTitleRect() const = 0;

};

UI_COMPLEX_MESSAGE_NS_END