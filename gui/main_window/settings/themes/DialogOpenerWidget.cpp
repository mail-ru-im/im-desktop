#include "stdafx.h"

#include "DialogOpenerWidget.h"

#include "fonts.h"
#include "utils/utils.h"
#include "styles/ThemeParameters.h"

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

    QSize getIconSize()
    {
        return Utils::scale_value(QSize(20, 20));
    }

    int iconRightMargin()
    {
        return Utils::scale_value(18);
    }
}

namespace Ui
{
    DialogOpenerWidget::DialogOpenerWidget(QWidget* _parent, const QString& _caption)
        : ClickableWidget(_parent)
    {
        setFixedHeight(getWidgetHeight());

        caption_ = TextRendering::MakeTextUnit(_caption);
        caption_->init(Fonts::appFontScaled(15), Styling::getParameters().getColor(Styling::StyleVariable::TEXT_SOLID));
        caption_->evaluateDesiredSize();

        connect(this, &ClickableWidget::hoverChanged, this, Utils::QOverload<>::of(&DialogOpenerWidget::update));
    }

    void DialogOpenerWidget::paintEvent(QPaintEvent*)
    {
        QPainter p(this);
        p.setRenderHint(QPainter::Antialiasing);

        if (isHovered())
            p.fillRect(rect(), Styling::getParameters().getColor(Styling::StyleVariable::BASE_BRIGHT_INVERSE));

        static const auto icon = Utils::mirrorPixmapHor(Utils::renderSvg(qsl(":/controls/back_icon"), getIconSize(), Styling::Buttons::defaultColor()));
        static const auto iconHover = Utils::mirrorPixmapHor(Utils::renderSvg(qsl(":/controls/back_icon"), getIconSize(), Styling::Buttons::hoverColor()));

        const auto x = width() - iconRightMargin() - getIconSize().width();
        const auto y = (height() - getIconSize().height()) / 2;
        p.drawPixmap(x, y, isHovered() ? iconHover : icon);

        caption_->setOffsets(captionLeftOffset(), height() / 2);
        caption_->draw(p, TextRendering::VerPosition::MIDDLE);
    }
}