#include "stdafx.h"

#include "SpacerSettingsRow.h"
#include "styles/ThemeParameters.h"
#include "utils/utils.h"

namespace Ui
{
    SpacerSettingsRow::SpacerSettingsRow(QWidget* _parent)
        : SimpleListItem(_parent)
    {
        setHoverStateCursor(Qt::ArrowCursor);
        setFixedHeight(Utils::scale_value(8));
    }

    void SpacerSettingsRow::setSelected(bool)
    {
    }

    bool SpacerSettingsRow::isSelected() const
    {
        return false;
    }

    void SpacerSettingsRow::paintEvent(QPaintEvent *)
    {
        QPainter p(this);
        p.fillRect(rect(), Styling::getParameters().getColor(Styling::StyleVariable::BASE_LIGHT));
    }
}