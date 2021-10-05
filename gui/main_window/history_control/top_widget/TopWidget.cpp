#include "stdafx.h"

#include "TopWidget.h"
#include "controls/CustomButton.h"
#include "styles/ThemeParameters.h"
#include "utils/utils.h"

namespace Ui
{
    TopWidget::TopWidget(QWidget* parent)
        : QStackedWidget(parent)
    {
        updateStyle();
        setFixedHeight(Utils::getTopPanelHeight());
    }

    void TopWidget::updateStyle()
    {
        bg_ = Styling::getParameters().getColor(Styling::StyleVariable::BASE_GLOBALWHITE);
        border_ = Styling::getParameters().getColor(Styling::StyleVariable::BASE_BRIGHT);

        const auto buttons = findChildren<CustomButton*>();
        for (auto btn : buttons)
            Styling::Buttons::setButtonDefaultColors(btn);

        update();
    }

    void TopWidget::paintEvent(QPaintEvent*)
    {
        QPainter p(this);
        Utils::drawBackgroundWithBorders(p, rect(), bg_, border_, Qt::AlignBottom);
    }
}