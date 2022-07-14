#include "stdafx.h"
#include "qmlstyling.h"

#include "../../styles/StyleVariable.h"
#include "../../styles/ThemeParameters.h"

namespace Styling
{
    QmlHelper::QmlHelper(QObject* _parent)
        : QObject(_parent)
    {
    }

    QColor QmlHelper::getColor(Styling::StyleVariable _styleVariable, double _opacity) const
    {
        return getColor({}, _styleVariable, _opacity);
    }

    QColor QmlHelper::getColor(const QString& _aimId, Styling::StyleVariable _styleVariable, double _opacity) const
    {
        return Styling::getParameters(_aimId).getColor(_styleVariable, _opacity);
    }
} // namespace Styling
