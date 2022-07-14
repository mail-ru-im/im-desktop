#include "stdafx.h"
#include "StatusCommonUi.h"
#include "styles/ThemeParameters.h"
#include "controls/DialogButton.h"
#include "utils/utils.h"

namespace Statuses
{
auto editIconColorKey()
{
    return Styling::ThemeColorKey{ Styling::StyleVariable::BASE_GLOBALWHITE };
}

QColor editIconColor()
{
    return Styling::getColor(editIconColorKey());
}

QColor editIconCircleColor()
{
    return Styling::getParameters().getColor(Styling::StyleVariable::PRIMARY);
}

QSize editIconSize() noexcept
{
    return Utils::scale_value(QSize(16, 16));
}

QPixmap editIcon()
{
    static auto pixmap = Utils::StyledPixmap(qsl(":/edit_icon_filled"), editIconSize(), editIconColorKey());
    return pixmap.actualPixmap();
}

QSize editIconRectSize() noexcept
{
    return Utils::scale_value(QSize(20, 20));
}

int iconCircleRadius() noexcept
{
    return Utils::scale_value(8);
}

void drawEditIcon(QPainter& _p, const QPoint& _topLeft)
{
    auto bigCircleRect = QRect(_topLeft, editIconRectSize());

    QPainterPath bigCirclePath;
    bigCirclePath.addEllipse(bigCircleRect);
    _p.fillPath(bigCirclePath, editIconColor());

    QPainterPath iconCircle;
    iconCircle.addEllipse(QRectF(bigCircleRect).center(), iconCircleRadius(), iconCircleRadius());
    _p.fillPath(iconCircle, editIconCircleColor());

    QRect iconRect;
    iconRect.setSize(editIconSize());
    iconRect.moveCenter(bigCircleRect.center());
    _p.drawPixmap(iconRect, editIcon());
}

DialogButton* createButton(QWidget* _parent, const QString& _text, const DialogButtonRole _role, bool _enabled)
{
    auto button = new DialogButton(_parent, _text, _role);
    button->setFixedWidth(Utils::scale_value(116));
    button->setEnabled(_enabled);
    return button;
}


}
