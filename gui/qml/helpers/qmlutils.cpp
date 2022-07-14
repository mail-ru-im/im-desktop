#include "stdafx.h"
#include "qmlutils.h"

#include "../../utils/utils.h"

namespace Utils
{
    QmlHelper::QmlHelper(QObject* _parent)
        : QObject(_parent)
    {
    }

    int QmlHelper::scaleValue(int _value) const
    {
        return Utils::scale_value(_value);
    }
} // namespace Utils
