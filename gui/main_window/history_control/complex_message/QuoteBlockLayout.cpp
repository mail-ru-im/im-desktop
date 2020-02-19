#include "stdafx.h"

#include "../../../controls/TextUnit.h"
#include "../../../controls/PictureWidget.h"
#include "../../../controls/LabelEx.h"
#include "../../../utils/utils.h"
#include "../MessageStyle.h"
#include "QuoteBlock.h"
#include "QuoteBlockLayout.h"


UI_COMPLEX_MESSAGE_NS_BEGIN

QuoteBlockLayout::QuoteBlockLayout() = default;

QuoteBlockLayout::~QuoteBlockLayout() = default;

QRect QuoteBlockLayout::getBlockGeometry() const
{
    auto &block = *blockWidget<QuoteBlock>();
    return block.Geometry_;
}

QSize QuoteBlockLayout::setBlockGeometryInternal(const QRect &widgetGeometry)
{
    QLayout::setGeometry(widgetGeometry);

    const auto contentLtr = evaluateContentLtr(widgetGeometry);

    const auto enoughSpace = (contentLtr.width() > 0);
    if (!enoughSpace)
        return widgetGeometry.size();

    auto senderBotMargin = 0;
    if (auto &block = *blockWidget<QuoteBlock>(); block.senderLabel_)
    {
        if (contentLtr.topLeft() != currentTopLeft_)
        {
            const auto offHor = contentLtr.left() + block.getLeftOffset();
            const auto offVer = contentLtr.top() + MessageStyle::Quote::getSenderTopOffset();
            block.senderLabel_->setOffsets(offHor, offVer);
            currentTopLeft_ = contentLtr.topLeft();
        }

        if (!block.Blocks_.empty() && block.Blocks_.front()->getContentType() != IItemBlock::ContentType::Text)
            senderBotMargin = MessageStyle::getSenderBottomMarginLarge();
    }

    return QSize(widgetGeometry.width(), MessageStyle::getSenderHeight() + senderBotMargin);
}

QRect QuoteBlockLayout::evaluateContentLtr(const QRect &widgetGeometry) const
{
    return widgetGeometry;
}

UI_COMPLEX_MESSAGE_NS_END
