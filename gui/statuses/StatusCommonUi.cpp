#include "stdafx.h"
#include "StatusCommonUi.h"
#include "styles/ThemeParameters.h"
#include "controls/DialogButton.h"
#include "utils/utils.h"

namespace Statuses
{


const QColor& editIconColor()
{
    static auto color = Styling::getParameters().getColor(Styling::StyleVariable::BASE_GLOBALWHITE);
    return color;
}

const QColor& editIconCircleColor()
{
    static auto color = Styling::getParameters().getColor(Styling::StyleVariable::PRIMARY);
    return color;
}

QSize editIconSize()
{
    return Utils::scale_value(QSize(16, 16));
}

const QPixmap& editIcon()
{
    static auto pixmap = Utils::renderSvg(qsl(":/edit_icon_filled"), editIconSize(), editIconColor());
    return pixmap;
}

QSize editIconRectSize()
{
    return Utils::scale_value(QSize(20, 20));
}

double editIconBorder()
{
    return Utils::fscale_value(2);
}

int iconCircleRadius()
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
