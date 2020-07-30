#pragma once

#include "../../../namespaces.h"

UI_COMPLEX_MESSAGE_NS_BEGIN

class ComplexMessageItem;
class IItemBlock;

class ComplexMessageItemLayout final : public QLayout
{
public:
    ComplexMessageItemLayout(ComplexMessageItem *parent);

    virtual ~ComplexMessageItemLayout() override;

    virtual void setGeometry(const QRect &r) override;

    virtual void addItem(QLayoutItem *item) override;

    virtual QLayoutItem *itemAt(int index) const override;

    virtual QLayoutItem *takeAt(int index) override;

    virtual int count() const override;

    virtual QSize sizeHint() const override;

    virtual void invalidate() override;

    bool isOverAvatar(const QPoint &pos) const;

    QRect getAvatarRect() const;

    const QRect& getBlocksContentRect() const;

    const QRect& getBubbleRect() const;

    QPoint getShareButtonPos(const IItemBlock &block, const bool isBubbleRequired) const;

    void onBlockSizeChanged();

    QPoint getInitialTimePosition() const;

    void setCustomBubbleBlockHorPadding(const int32_t _padding);

private:
    QRect evaluateAvatarRect(const QRect& _widgetContentLtr) const;

    QRect evaluateBlocksBubbleGeometry(const bool isMarginRequired, const QRect& _blocksContentLtr) const;

    QRect evaluateBlocksContainerLtr(
        const QRect& _avatarRect,
        const QRect& _widgetContentLtr) const;

    QRect evaluateBlockLtr(
        const QRect &blocksContentLtr,
        IItemBlock *block,
        const int32_t blockY,
        const bool isBubbleRequired);

    QRect evaluateSenderContentLtr(const QRect& _widgetContentLtr, const QRect& _avatarRect) const;

    QRect evaluateWidgetContentLtr(const int32_t widgetWidth) const;

    bool hasSeparator(const IItemBlock *block) const;

    bool isOutgoingPosition() const;

    bool isHeaderOrSticker() const;

    QRect setBlocksGeometry(
        const bool isBubbleRequired,
        const QRect& _messageRect,
        const QRect& _senderRect);

    void setGeometryInternal(const int32_t widgetWidth);

    void setTimePosition(const QRect& _availableGeometry);

    ComplexMessageItem *Item_;

    QRect LastGeometry_;

    QRect BubbleRect_;

    int32_t WidgetHeight_;

    QRect AvatarRect_;

    QRect BlocksContentRect_;

    QPoint initialTimePosition_;

    int32_t bubbleBlockHorPadding_;
};

UI_COMPLEX_MESSAGE_NS_END
