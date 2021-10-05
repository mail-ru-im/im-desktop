#include "stdafx.h"

#include "FormattedTextRendering.h"
#include "styles/ThemeParameters.h"


namespace Ui::TextRendering
{
    void drawPreBlockSurroundings(QPainter& _painter, QRectF _rect)
    {
        const auto thickness = getPreBorderThickness();
        const auto radius = getPreBorderRadius();

        const auto borderColor = Styling::getParameters().getColor(Styling::StyleVariable::PRIMARY);
        auto backgroundColor = borderColor;
        backgroundColor.setAlphaF(getPreBackgoundAlpha());

        const auto painterSaver = Utils::PainterSaver(_painter);

        _painter.setBrush(backgroundColor);
        _painter.setPen(Qt::NoPen);
        _painter.drawRoundedRect(_rect, radius, radius);

        _painter.setBrush(Qt::NoBrush);
        _painter.setPen(QPen(QBrush(borderColor), thickness));
        _painter.drawRoundedRect(_rect.adjusted(thickness, thickness, -2 * thickness, -thickness), radius, radius);
    }

    void drawQuoteBar(QPainter& _painter, QPointF _topLeft, qreal _height)
    {
        const auto painterSaver = Utils::PainterSaver(_painter);

        const auto color = Styling::getParameters().getColor(Styling::StyleVariable::GHOST_SECONDARY_INVERSE);
        _painter.setBrush(color);
        _painter.setPen(Qt::NoPen);
        const auto barRect = QRect(_topLeft.x(), _topLeft.y(), getQuoteBarWidth(), _height);
        _painter.drawRect(barRect);
    }

    void drawUnorderedListBullet(QPainter& _painter, QPoint _baselineLeft, qreal _width, qreal _lineHeight, QColor _color)
    {
        constexpr auto bulletYShiftFactor = 0.9;
        const auto radius = getUnorderedListBulletRadius();
        const auto rect = QRectF(_baselineLeft.x() + 0.5 * _width - radius,
                                 _baselineLeft.y() - 0.5 * (bulletYShiftFactor * _lineHeight - radius) + 0.5,
                                 2.0 * radius,
                                 2.0 * radius);

        const auto painterSaver = Utils::PainterSaver(_painter);
        _painter.setPen(Qt::NoPen);
        _painter.setBrush(_color);
        _painter.drawEllipse(rect);
    }

    void drawOrderedListBullet(QPainter& _painter, QPoint _baselineLeft, qreal _width, const QFont& _font, int _number, QColor _color)
    {
        static const auto textOption = QTextOption(Qt::AlignmentFlag::AlignHCenter
                                                 | Qt::AlignmentFlag::AlignBaseline);
        const auto lineHeight = QFontMetrics(_font).height();
        const auto rect = QRectF(_baselineLeft.x(), _baselineLeft.y() - lineHeight, _width + 1, lineHeight);

        const auto painterSaver = Utils::PainterSaver(_painter);
        _painter.setPen(_color);
        _painter.setFont(_font);
        _painter.drawText(rect, QString::number(_number) % u'.', textOption);
    }
}

