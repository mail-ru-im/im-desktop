#include "stdafx.h"

#include "../../../controls/TextUnit.h"

#include "../../../utils/utils.h"

#include "../MessageStyle.h"
#include "../MessageStatusWidget.h"

#include "TextBlock.h"

#include "TextBlockLayout.h"
#include "ComplexMessageItem.h"

UI_COMPLEX_MESSAGE_NS_BEGIN

TextBlockLayout::TextBlockLayout() = default;

TextBlockLayout::~TextBlockLayout() = default;

void TextBlockLayout::markDirty()
{
    prevWidth_ = 0;
}

QSize TextBlockLayout::setBlockGeometryInternal(const QRect &widgetGeometry)
{
    QLayout::setGeometry(widgetGeometry);

    const auto contentLtr = evaluateContentLtr(widgetGeometry);

    const auto enoughSpace = (contentLtr.width() > 0);
    if (!enoughSpace)
        return widgetGeometry.size();

    return setTextControlGeometry(contentLtr);
}

QRect TextBlockLayout::evaluateContentLtr(const QRect &widgetGeometry) const
{
    return widgetGeometry;
}

QSize TextBlockLayout::setTextControlGeometry(const QRect &contentLtr)
{
    im_assert(contentLtr.width() > 0);

    if (const auto contentWidth = contentLtr.width(); contentWidth != prevWidth_)
    {
        auto block = blockWidget<TextBlock>();
        im_assert(block);

        if (block && block->textUnit_)
        {
            block->textUnit_->getHeight(contentWidth, Ui::TextRendering::CallType::FORCE);

            const auto maxLineWidth = block->textUnit_->getMaxLineWidth();
            QSize textSize(maxLineWidth, block->textUnit_->cachedSize().height());

            const auto cm = block->getParentComplexMessage();
            if (cm && cm->isLastBlock(block) && block->textUnit_->getLinesCount() > 0)
            {
                const auto oneLiner = block->textUnit_->getLinesCount() == 1;
                const auto mightStretch = block->textUnit_->mightStretchForLargerWidth();

                const auto textWidth = (oneLiner && !mightStretch)
                    ? block->textUnit_->desiredWidth()
                    : maxLineWidth;
                textSize.setWidth(textWidth);

                if (const auto timeWidget = cm->getTimeWidget())
                {
                    const auto timeWidth = timeWidget->width() + MessageStyle::getTimeLeftSpacing();
                    if (oneLiner && !mightStretch && contentWidth - textWidth >= timeWidth)
                        textSize.rwidth() += timeWidth;
                    else if (mightStretch || (textWidth - block->textUnit_->getLastLineWidth() < timeWidth))
                        textSize.rheight() += MessageStyle::getShiftedTimeTopMargin() + timeWidget->height();
                }
            }

            cachedSize_ = textSize;
            prevWidth_ = contentWidth;
        }
    }

    cachedSize_.setHeight(qMax(1, cachedSize_.height()));
    cachedSize_.setWidth(qMax(1, cachedSize_.width()));

    return cachedSize_;
}

UI_COMPLEX_MESSAGE_NS_END
