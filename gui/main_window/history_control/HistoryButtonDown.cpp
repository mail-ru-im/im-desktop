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
    , counter_(0)
    , icon_(Utils::renderSvgScaled(_imageName, QSize(iconSize, iconSize), Styling::getParameters().getColor(Styling::StyleVariable::BASE_SECONDARY)))
    , iconHover_(Utils::renderSvgScaled(_imageName, QSize(iconSize, iconSize), Styling::getParameters().getColor(Styling::StyleVariable::BASE_SECONDARY_HOVER)))
{
    setFixedSize(Utils::scale_value(QSize(buttonSize, buttonSize)));

    connect(this, &HistoryButton::hoverChanged, this, Utils::QOverload<>::of(&HistoryButton::update));
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

        const auto icon = isHovered() ? iconHover_ : icon_;
        const auto ratio = Utils::scale_bitmap_ratio();
        const auto x = (getBubbleSize() - icon.width() / ratio) / 2;
        const auto y = (getBubbleSize() - icon.height() / ratio) / 2;
        p.drawPixmap(x, y, icon);
    }

    if (counter_ > 0)
    {
        static auto font = Fonts::appFont(getCounterFontSize(), Fonts::FontWeight::SemiBold);
        static auto bgColor = Styling::getParameters().getColor(Styling::StyleVariable::PRIMARY);
        static auto textColor = Styling::getParameters().getColor(Styling::StyleVariable::TEXT_SOLID_PERMANENT);

        const auto size = Utils::getUnreadsSize(font, true, counter_, getCounterBubbleSize());
        const auto x = width() - size.width();
        const auto y = Utils::scale_value(2);

        Utils::drawUnreads(&p, font, bgColor, textColor, QColor(), counter_, getCounterBubbleSize(), x, y);
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
    emit sendWheelEvent(_event);
}

UI_NS_END