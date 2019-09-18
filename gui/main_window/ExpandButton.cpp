#include "ExpandButton.h"

#include "../utils/utils.h"
#include "../controls/TextUnit.h"
#include "../styles/ThemeParameters.h"
#include "../fonts.h"

namespace
{
    int iconSize()
    {
        return Utils::scale_value(20);
    }

    int badgeTopOffset()
    {
        return Utils::scale_value(3);
    }

    int badgeTextPadding()
    {
        return Utils::scale_value(4);
    }

    int badgeBorderWidth()
    {
        return Utils::scale_value(1);
    }

    int badgeHeight()
    {
        return Utils::scale_value(18);
    }

    int badgeOffset()
    {
        return Utils::scale_value(36);
    }

    QColor badgeTextColor()
    {
        return Styling::getParameters().getColor(Styling::StyleVariable::TEXT_SOLID_PERMANENT);
    }

    QColor badgeBalloonColor()
    {
        return Styling::getParameters().getColor(Styling::StyleVariable::SECONDARY_ATTENTION);
    }

    QFont getBadgeTextFont()
    {
        const auto static f = Fonts::appFontScaled(9);
        return f;
    }
}

namespace Ui
{
    ExpandButton::ExpandButton(QWidget* _parent)
        : ClickableWidget(_parent)
    {
        setAttribute(Qt::WA_Hover, true);
    }

    ExpandButton::~ExpandButton() = default;

    void ExpandButton::setBadgeText(const QString& _text)
    {
        if (badgeText_ != _text)
        {
            badgeText_ = _text;
            if (badgeTextUnit_)
            {
                badgeTextUnit_->setText(badgeText_);
            }
            else
            {
                badgeTextUnit_ = TextRendering::MakeTextUnit(badgeText_);
                badgeTextUnit_->init(getBadgeTextFont(), badgeTextColor(), QColor(), QColor(), QColor(), TextRendering::HorAligment::CENTER);
            }
            update();
        }
    }

    void ExpandButton::paintEvent(QPaintEvent*)
    {
        const auto size = iconSize();
        const auto bgColor = Styling::getParameters().getColor(Styling::StyleVariable::BASE_GLOBALWHITE);
        const static auto pixmap = Utils::renderSvg(qsl(":/tab/expand"), { size, size }, Styling::getParameters().getColor(Styling::StyleVariable::PRIMARY));

        QPainter p(this);
        p.setRenderHint(QPainter::Antialiasing);

        const auto r = rect();
        Utils::drawBackgroundWithBorders(
            p,
            r,
            bgColor,
            Styling::getParameters().getColor(Styling::StyleVariable::BASE_BRIGHT),
            Qt::AlignTop
        );

        p.drawPixmap((r.width() - size) / 2, (r.height() - size) / 2, pixmap);

        if (!badgeText_.isEmpty()) // duplicate with tabbar.cpp
        {
            const auto desiredWidth = badgeTextUnit_->desiredWidth();
            const auto textHeight = badgeTextUnit_->getHeight(desiredWidth);
            {
                Utils::PainterSaver saver(p);
                const static QPen pen(bgColor, badgeBorderWidth());
                p.setPen(pen);
                p.setBrush(badgeBalloonColor());
                const auto h = badgeHeight() - badgeBorderWidth() * 2;

                const auto balllonWidth = badgeText_.size() == 1 ? h : std::max(desiredWidth, h) + badgeTextPadding() * 2;

                const auto balloonX = badgeOffset();

                p.drawRoundedRect(balloonX, badgeTopOffset(), balllonWidth, h, h / 2.0, h / 2.0);

                badgeTextUnit_->setOffsets(balloonX + (balllonWidth + badgeBorderWidth() - desiredWidth) / 2.0, badgeTopOffset() + (h + badgeBorderWidth()) / 2);
            }
            badgeTextUnit_->draw(p, TextRendering::VerPosition::MIDDLE);
        }
    }

    bool ExpandButton::event(QEvent* _event)
    {
        switch (_event->type())
        {
        case QEvent::HoverEnter:
        {
            setCursor(Qt::PointingHandCursor);
        }
        break;
        case QEvent::HoverLeave:
        {
            setCursor(Qt::ArrowCursor);
        }
        break;
        default:
            break;
        }

        return QWidget::event(_event);
    }
}