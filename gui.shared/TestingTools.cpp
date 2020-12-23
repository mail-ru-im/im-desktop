#include "stdafx.h"

#ifdef IM_AUTO_TESTING
#include "app_config.h"
#endif

namespace Testing
{
    void setAccessibleName(QWidget* target, const QString& name)
    {
        if constexpr (isEnabled())
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

    bool isAutoTestsMode()
    {
#ifdef IM_AUTO_TESTING
        return Ui::GetAppConfig().IsTestingEnable();
#else
        return false;
#endif
    }

    QSize getComboboxItemSize()
    {
#ifdef IM_AUTO_TESTING
        return Utils::scale_value(QSize(140, 50));
#else
        return QSize();
#endif
    }

}
