#pragma once

#include "GenericBlockLayout.h"

UI_COMPLEX_MESSAGE_NS_BEGIN

class TextBlock;

class TextBlockLayout final : public GenericBlockLayout
{
public:
    TextBlockLayout();

    virtual ~TextBlockLayout() override;

    void markDirty() override;

protected:
    virtual QSize setBlockGeometryInternal(const QRect &widgetGeometry) override;

private:
    QRect evaluateContentLtr(const QRect &widgetGeometry) const;

    QSize setTextControlGeometry(const QRect &contentLtr);

    int prevWidth_ = 0;
    QSize cachedSize_;
};

UI_COMPLEX_MESSAGE_NS_END
