#pragma once

class QWidget;
class QString;

namespace Testing
{
    constexpr bool isEnabled() noexcept
    {
#ifdef IM_AUTO_TESTING
        return true;
#else
        return false;
#endif
    }

    void setAccessibleName(QWidget* target, const QString& name);

    bool isAccessibleRole(const int _role);
}
