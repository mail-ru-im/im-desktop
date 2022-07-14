#include "stdafx.h"
#include "qmltesting.h"

#include "../../../gui.shared/TestingTools.h"

namespace Testing
{
    QmlHelper::QmlHelper(QObject* _parent)
        : QObject(_parent)
    {
    }

    QString QmlHelper::accessibleName(const QString& _name) const
    {
        return Testing::isEnabled() ? _name : QString();
    }
} // namespace Testing
