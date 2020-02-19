#pragma once

#include "GenericBlockLayout.h"

UI_COMPLEX_MESSAGE_NS_BEGIN

class QuoteBlockLayout final : public GenericBlockLayout
{
public:
    QuoteBlockLayout();

    virtual ~QuoteBlockLayout() override;

    QRect getBlockGeometry() const override;

protected:
    QSize setBlockGeometryInternal(const QRect &widgetGeometry) override;

private:
    QRect evaluateContentLtr(const QRect &widgetGeometry) const;

    QPoint currentTopLeft_ = QPoint(-1, -1);
};

UI_COMPLEX_MESSAGE_NS_END
