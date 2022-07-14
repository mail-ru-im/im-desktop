#pragma once

#include "../../fonts.h"

namespace Fonts
{
    /**
    * @example Usage in Qml:
    * Text {
    *     font: Fonts.appFontScaled(14, FontFamily.SF_MONO, FontWeight.Bold)
    * }
    */
    class QmlAppFontHelper : public QObject
    {
        Q_OBJECT
    public:
        explicit QmlAppFontHelper(QObject* _parent = nullptr);

    public Q_SLOTS:
        QFont appFontScaled(int _sizePx) const;
        QFont appFontScaled(int _sizePx, Fonts::FontWeight _weight) const;
        QFont appFontScaled(int _sizePx, Fonts::FontFamily _family, Fonts::FontWeight _weight) const;
    };
} // namespace Fonts
