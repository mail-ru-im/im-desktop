#include "stdafx.h"

#include "HistoryButtonDown.h"
#include "../../utils/utils.h"
#include "../../fonts.h"

#include "styles/ThemeParameters.h"

namespace
{
    constexpr auto iconSize = 24;

    int getBubbleTopMargin()
    {
        return Utils::scale_value(6);
    }

    int getCounterBubbleSize()
    {
        return Utils::scale_value(20);
    }

    int getCounterFontSize()
    {
        return Utils::scale_value(12);
    }

    int getBubbleSize()
    {
        return Utils::scale_value(Ui::HistoryButton::bubbleSize);
    }
}

UI_NS_BEGIN

HistoryButton::HistoryButton(QWidget* _parent, const QString& _imageName)
    : ClickableWidget(_parent)
    , icon_(Utils::StyledPixmap::scaled(_imageName, QSize(iconSize, iconSize), Styling::ThemeColorKey{ Styling::StyleVariable::BASE_SECONDARY }))
    , iconHover_(Utils::StyledPixmap::scaled(_imageName, QSize(iconSize, iconSize), Styling::ThemeColorKey{ Styling::StyleVariable::BASE_SECONDARY_HOVER }))
    , counter_(0)
{
    setFixedSize(Utils::scale_value(QSize(buttonSize, buttonSize)));

    connect(this, &HistoryButton::hoverChanged, this, qOverload<>(&HistoryButton::update));
}

void HistoryButton::paintEvent(QPaintEvent *_event)
{
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);

    {
        Utils::PainterSaver ps(p);
        p.translate(0, getBubbleTopMargin());

        static const auto path = []()
        {
            QPainterPath p;
            p.addEllipse(0, 0, getBubbleSize(), getBubbleSize());
            return p;
        }();

        Utils::drawBubbleShadow(p, path, getBubbleSize() / 2);

        p.setPen(Qt::NoPen);
        p.setBrush(Styling::getParameters().getColor( isHovered() ? Styling::StyleVariable::BASE_BRIGHT_INVERSE : Styling::StyleVariable::BASE_GLOBALWHITE));
        p.drawPath(path);

        const auto icon = (isHovered() ? iconHover_ : icon_).actualPixmap();
        const auto ratio = Utils::scale_bitmap_ratio();
        const auto x = (getBubbleSize() - icon.width() / ratio) / 2;
        const auto y = (getBubbleSize() - icon.height() / ratio) / 2;
        p.drawPixmap(x, y, icon);
    }

    if (counter_ > 0)
    {
        static const auto font = Fonts::appFont(getCounterFontSize(), Fonts::FontWeight::SemiBold);
        static Styling::ColorContainer bgColor = Styling::ThemeColorKey{ Styling::StyleVariable::PRIMARY };
        static Styling::ColorContainer textColor = Styling::ThemeColorKey{ Styling::StyleVariable::BASE_GLOBALWHITE };

        const auto size = Utils::getUnreadsSize(font, true, counter_, getCounterBubbleSize());
        const auto x = width() - size.width();
        const auto y = Utils::scale_value(2);

        Utils::drawUnreads(&p, font, bgColor.actualColor(), textColor.actualColor(), QColor(), counter_, getCounterBubbleSize(), x, y);
    }
}

void HistoryButton::setCounter(const int _numToSet)
{
    counter_ = _numToSet;
    update();
}

void HistoryButton::addCounter(const int _numToAdd)
{
    counter_ += _numToAdd;
    update();
}

int HistoryButton::getCounter() const
{
    return counter_;
}

void HistoryButton::wheelEvent(QWheelEvent* _event)
{
    Q_EMIT sendWheelEvent(_event);
}

UI_NS_END