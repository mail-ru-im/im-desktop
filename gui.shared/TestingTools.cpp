#include "stdafx.h"

namespace Testing
{
    void setAccessibleName(QWidget* target, const QString& name)
    {
        if (isEnabled())
        {
            //assert(!!target);
            if (target)
            {
                target->setAccessibleName(name);
                target->setAccessibleDescription(name);
            }
        }
    }

    bool isAccessibleRole(const int _role)
    {
        return isEnabled() && (_role == Qt::AccessibleDescriptionRole || _role == Qt::AccessibleTextRole);
    }
}
