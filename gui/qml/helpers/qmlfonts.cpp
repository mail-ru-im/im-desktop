#include "stdafx.h"
#include "qmlfonts.h"
#include "../../fonts.h"

namespace Fonts
{
    QmlAppFontHelper::QmlAppFontHelper(QObject* _parent)
        : QObject(_parent)
    {
    }

    QFont QmlAppFontHelper::appFontScaled(int _sizePx) const
    {
        return Fonts::appFontScaled(_sizePx);
    }

    QFont QmlAppFontHelper::appFontScaled(int _sizePx, Fonts::FontWeight _weight) const
    {
        return Fonts::appFontScaled(_sizePx, _weight);
    }

    QFont QmlAppFontHelper::appFontScaled(int _sizePx, Fonts::FontFamily _family, Fonts::FontWeight _weight) const
    {
        return Fonts::appFontScaled(_sizePx, _family, _weight);
    }

} // namespace Fonts
