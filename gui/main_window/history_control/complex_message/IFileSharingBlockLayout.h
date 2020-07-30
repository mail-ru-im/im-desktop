#pragma once

#include "../../../namespaces.h"

#include "GenericBlockLayout.h"

UI_COMPLEX_MESSAGE_NS_BEGIN

class IFileSharingBlockLayout : public GenericBlockLayout
{
public:
    IFileSharingBlockLayout();

    virtual ~IFileSharingBlockLayout() override;

    virtual const QRect& getContentRect() const = 0;

    virtual const QRect& getFilenameRect() const = 0;

    virtual QRect getFileSizeRect() const = 0;

    virtual QRect getShowInDirLinkRect() const = 0;

    virtual bool isForQuote() const { return false; }

};

UI_COMPLEX_MESSAGE_NS_END