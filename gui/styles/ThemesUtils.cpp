#include "stdafx.h"

#include "ThemesUtils.h"

namespace StylingUtils
{
    QString getString(const rapidjson::Value& _value)
    {
        return QString::fromUtf8(_value.GetString(), _value.GetStringLength());
    }

    QColor getColor(const rapidjson::Value& _value)
    {
        auto color = getString(_value);
        if (!color.startsWith(ql1c('#')))
            color.prepend(ql1c('#'));

        return QColor(color);
    }
}