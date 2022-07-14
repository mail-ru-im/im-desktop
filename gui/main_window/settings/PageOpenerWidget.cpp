#include "stdafx.h"

#include "PageOpenerWidget.h"

#include "fonts.h"
#include "utils/utils.h"
#include "styles/ThemeParameters.h"
#include "PageOpenerWidget.h"

namespace
{
    int getWidgetHeight()
    {
        return Utils::scale_value(44);
    }

    int captionLeftOffset()
    {
        return Utils::scale_value(20);
    }

    int textRightOffset()
    {
        return Utils::scale_value(16);
    }

    QSize getIconSize()
    {
        return Utils::scale_value(QSize(20, 20));
    }

    int iconRightMargin()
    {
        return Utils::scale_value(18);
    }

    QPixmap makeIcon(QColor _color)
    {
        return Utils::mirrorPixmapHor(Utils::renderSvg(qsl(":/controls/back_icon"), getIconSize(), _color));
    }
}

namespace Ui
{
    PageOpenerWidget::PageOpenerWidget(QWidget* _parent, const QString& _caption, const QString& _text)
        : ClickableWidget(_parent)
    {
        setFixedHeight(getWidgetHeight());

        setCaption(_caption);
        if (!_text.isEmpty())
            setText(_text);

        connect(this, &ClickableWidget::hoverChanged, this, qOverload<>(&PageOpenerWidget::update));
        connect(this, &ClickableWidget::pressChanged, this, qOverload<>(&PageOpenerWidget::update));
    }

    void PageOpenerWidget::setCaption(const QString& _text)
    {
        caption_ = TextRendering::MakeTextUnit(_text);
        caption_->init({ Fonts::appFontScaled(15), Styling::ThemeColorKey{ Styling::StyleVariable::TEXT_SOLID } });
        caption_->evaluateDesiredSize();
        update();
    }

    void PageOpenerWidget::setText(const QString& _text)
    {
        text_ = TextRendering::MakeTextUnit(_text);
        text_->init({ Fonts::appFontScaled(15), Styling::ThemeColorKey{ Styling::StyleVariable::BASE_PRIMARY } });
        text_->evaluateDesiredSize();
        update();
    }

    void PageOpenerWidget::paintEvent(QPaintEvent*)
    {
        QPainter p(this);
        p.setRenderHints(QPainter::Antialiasing | QPainter::SmoothPixmapTransform);

        if (isHovered() || isPressed())
            p.fillRect(rect(), Styling::getParameters().getColor(Styling::StyleVariable::BASE_BRIGHT_INVERSE));

        if (text_)
        {
            const auto color = isPressed()
                ? Styling::StyleVariable::BASE_PRIMARY_ACTIVE
                : isHovered() ? Styling::StyleVariable::BASE_PRIMARY_HOVER : Styling::StyleVariable::BASE_PRIMARY;
            text_->setColor(Styling::ThemeColorKey{ color });

            text_->setOffsets(width() - text_->cachedSize().width() - textRightOffset(), height() / 2);
            text_->draw(p, TextRendering::VerPosition::MIDDLE);
        }
        else
        {
            static QPixmap icon;
            static QPixmap iconHover;
            static QPixmap iconPressed;

            static Styling::ThemeChecker checker;
            if (checker.checkAndUpdateHash() || icon.isNull())
            {
                icon = makeIcon(Styling::Buttons::defaultColor());
                iconHover = makeIcon(Styling::Buttons::hoverColor());
                iconPressed = makeIcon(Styling::Buttons::pressedColor());
            }

            const auto x = width() - iconRightMargin() - getIconSize().width();
            const auto y = (height() - getIconSize().height()) / 2;
            p.drawPixmap(x, y, isPressed() ? iconPressed : isHovered() ? iconHover : icon);
        }

        if (caption_)
        {
            caption_->setOffsets(captionLeftOffset(), height() / 2);
            caption_->draw(p, TextRendering::VerPosition::MIDDLE);
        }
    }
}