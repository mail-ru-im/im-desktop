#pragma once

#include "utils/utils.h"

namespace Ui
{
namespace TextRendering
{
    inline int getQuoteBarWidth() noexcept { return Utils::scale_value(4); }

    inline int getQuoteTextLeftMargin() noexcept { return Utils::scale_value(16); }

    inline int getPreHPadding() noexcept { return Utils::scale_value(12); }

    inline int getPreVPadding() noexcept { return Utils::scale_value(4); }

    inline int getPreBorderRadius() noexcept { return Utils::scale_value(4); }

    inline int getPreBorderThickness() noexcept { return Utils::scale_value(1); }

    inline int getPreMinWidth() noexcept { return Utils::scale_value(30); }

    inline int getParagraphVMargin() noexcept { return Utils::scale_value(6); }

    inline int getOrderedListLeftMargin(int _numItems = -1) noexcept { return _numItems < 1000 ? Utils::scale_value(32) : Utils::scale_value(40) ; }

    inline int getUnorderedListLeftMargin() noexcept { return getOrderedListLeftMargin(); }

    inline int getUnorderedListBulletRadius() { return Utils::scale_value(2); }

    constexpr qreal getPreBackgoundAlpha() { return 0.08; }

    //! Draw rounded rect around the block
    void drawPreBlockSurroundings(QPainter& _painter, QRectF _rect);

    void drawQuoteBar(QPainter& _painter, QPointF _topLeft, qreal _height);

    void drawUnorderedListBullet(QPainter& _painter, QPoint _baselineLeft, qreal _width, qreal _lineHeight, QColor _color);

    //! Line height is to be calculated from top to descent line
    void drawOrderedListBullet(QPainter& _painter, QPoint _baselineLeft, qreal _width, const QFont& _font, int _number, QColor _color);

}
}
