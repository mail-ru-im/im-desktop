#include "stdafx.h"

#include "../../../utils/utils.h"

#include "../MessageStyle.h"

#include "PttBlock.h"


#include "PttBlockLayout.h"

UI_COMPLEX_MESSAGE_NS_BEGIN

PttBlockLayout::PttBlockLayout() = default;

PttBlockLayout::~PttBlockLayout() = default;

QSize PttBlockLayout::setBlockGeometryInternal(const QRect &geometry)
{
    auto &block = *blockWidget<PttBlock>();

    const auto maxWidth = MessageStyle::Ptt::getPttBlockMaxWidth() + (block.isStandalone() ? 0 : 2 * MessageStyle::getBubbleHorPadding());
    const auto blockWidth = std::min(geometry.width(), maxWidth);
    const auto textSize = setDecodedTextHorGeometry(block, blockWidth);

    QSize bubbleSize(blockWidth, MessageStyle::Ptt::getPttBlockHeight() + (block.isStandalone() ? getTopMargin() : 2 * MessageStyle::getBubbleVerPadding()));

    const auto hasVisisbleDecodedText = (!textSize.isEmpty() && !block.isDecodedTextCollapsed());
    if (hasVisisbleDecodedText)
    {
        bubbleSize.rheight() += MessageStyle::Ptt::getDecodedTextVertPadding();
        bubbleSize.rheight() += textSize.height();
    }

    contentRect_ = QRect(geometry.topLeft(), bubbleSize);

    ctrlButtonRect_ = setCtrlButtonGeometry(block, contentRect_);
    textButtonRect_ = setTextButtonGeometry(block, contentRect_);

    if (hasVisisbleDecodedText)
        setDecodedTextGeometry(block, contentRect_, textSize);

    return contentRect_.size();
}

QRect PttBlockLayout::setCtrlButtonGeometry(PttBlock &pttBlock, const QRect &bubbleGeometry)
{
    assert(!bubbleGeometry.isEmpty());

    const QPoint btnPos(
        pttBlock.isStandalone() ? 0 : MessageStyle::getBubbleHorPadding(),
        pttBlock.isStandalone() ? getTopMargin() : MessageStyle::getBubbleVerPadding());
    const QRect btnRect(btnPos, pttBlock.getCtrlButtonSize());

    pttBlock.setCtrlButtonGeometry(btnRect);

    return btnRect;
}

QSize PttBlockLayout::setDecodedTextHorGeometry(PttBlock &pttBlock, int32_t bubbleWidth)
{
    assert(bubbleWidth > 0);

    if (!pttBlock.hasDecodedText())
        return QSize();

    if (!pttBlock.isStandalone())
        bubbleWidth -= 2 * MessageStyle::getBubbleHorPadding();
    const auto textHeight = pttBlock.setDecodedTextWidth(bubbleWidth);
    return QSize(bubbleWidth, textHeight);
}

void PttBlockLayout::setDecodedTextGeometry(PttBlock &_pttBlock, const QRect &_contentRect, const QSize &_textSize)
{
    assert(!_contentRect.isEmpty());
    assert(!_textSize.isEmpty());

    const auto x = _pttBlock.isStandalone() ? 0 : MessageStyle::getBubbleHorPadding();
    const auto w = _pttBlock.isStandalone() ? _textSize.width() : _textSize.width() - 2 * MessageStyle::getBubbleHorPadding();
    QRect textRect(x, 0, w, _textSize.height());

    const auto textBottomY = _contentRect.bottom() - (_pttBlock.isStandalone() ? 0 : MessageStyle::getBubbleVerPadding());
    textRect.moveBottom(textBottomY);

    _pttBlock.setDecodedTextOffsets(textRect.x(), textRect.y());
}

QRect PttBlockLayout::setTextButtonGeometry(PttBlock &pttBlock, const QRect &bubbleGeometry)
{
    assert(!bubbleGeometry.isEmpty());

    const auto buttonSize = pttBlock.getTextButtonSize();
    const auto x = bubbleGeometry.right() + 1 - buttonSize.width() - (pttBlock.isStandalone() ? MessageStyle::Ptt::getTextButtonMarginRight() : MessageStyle::getBubbleHorPadding());
    const auto y = bubbleGeometry.top() + (pttBlock.isStandalone() ? getTopMargin() : MessageStyle::getBubbleVerPadding());

    const QPoint buttonPos(x, y);
    pttBlock.setTextButtonGeometry(buttonPos);

    return QRect(buttonPos, buttonSize);
}

const QRect& PttBlockLayout::getContentRect() const
{
    assert(!contentRect_.isEmpty());
    return contentRect_;
}

const QRect& PttBlockLayout::getCtrlButtonRect() const
{
    assert(!ctrlButtonRect_.isEmpty());
    return ctrlButtonRect_;
}

const QRect& PttBlockLayout::getTextButtonRect() const
{
    assert(!textButtonRect_.isEmpty());
    return textButtonRect_;
}

const QRect& PttBlockLayout::getFilenameRect() const
{
    static QRect empty;
    return empty;
}

QRect PttBlockLayout::getFileSizeRect() const
{
    static QRect empty;
    return empty;
}

QRect PttBlockLayout::getShowInDirLinkRect() const
{
    static QRect empty;
    return empty;
}

int PttBlockLayout::getTopMargin() const
{
    auto &block = *blockWidget<PttBlock>();

    if (block.isStandalone())
    {
        if (block.isSenderVisible())
            return MessageStyle::Ptt::getTopMargin();

        return 0;
    }

    return MessageStyle::getBubbleVerPadding();
}

UI_COMPLEX_MESSAGE_NS_END