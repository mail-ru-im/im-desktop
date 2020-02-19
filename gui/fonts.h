#pragma once

#include "namespaces.h"
#include "utils/utils.h"

FONTS_NS_BEGIN

    enum class FontFamily
    {
        MIN,

        SOURCE_SANS_PRO,
        SEGOE_UI,
        ROUNDED_MPLUS,
        ROBOTO_MONO,
        SF_PRO_TEXT,

        MAX,
    };

    enum class FontWeight
    {
        Min,

        Light,
        Normal,
        Medium,
        SemiBold,
        Bold,

        Max,
    };

    enum class FontSize
    {
        Small   = 0,
        Medium  = 1,
        Large   = 2,
    };

    enum class FontAdjust
    {
        NoAdjust = 0,
        Adjust = 1,
    };

    QFont appFont(const int32_t _sizePx, const FontAdjust _adjust = FontAdjust::Adjust);

    QFont appFont(const int32_t _sizePx, const FontFamily _family, const FontAdjust _adjust = FontAdjust::Adjust);

    QFont appFont(const int32_t _sizePx, const FontWeight _weight, const FontAdjust _adjust = FontAdjust::Adjust);

    QFont appFont(const int32_t _sizePx, const FontFamily _family, const FontWeight _weight, const FontAdjust _adjust = FontAdjust::Adjust);

    QFont appFontScaled(const int32_t _sizePx);

    QFont appFontScaled(const int32_t _sizePx, const FontWeight _weight);

    QFont appFontScaled(const int32_t _sizePx, const FontFamily _family);

    QFont appFontScaled(const int32_t _sizePx, const FontFamily _family, const FontWeight _weight);

    QFont appFontScaledFixed(const int32_t _sizePx);

    QFont appFontScaledFixed(const int32_t _sizePx, const FontWeight _weight);

    QFont appFontScaledFixed(const int32_t _sizePx, const FontFamily _family);

    QFont appFontScaledFixed(const int32_t _sizePx, const FontFamily _family, const FontWeight _weight);

    QString appFontFullQss(const int32_t _sizePx, const FontFamily _fontFamily, const FontWeight _weight);

    QString appFontFamilyNameQss(const FontFamily _fontFamily, const FontWeight _fontWeight);

    QString appFontWeightQss(const FontWeight _weight);

    FontFamily defaultAppFontFamily();

    QString defaultAppFontQssName();

    QString defaultAppFontQssWeight();

    FontWeight defaultAppFontWeight();

    QString SetFont(QString _qss);


    int adjustFontSize(const int _size);
    FontWeight adjustFontWeight(const FontWeight _weight);

    inline QFont adjustedAppFont(const int _unscaledSize, const Fonts::FontWeight _weight = Fonts::defaultAppFontWeight(), const FontAdjust _adjust = FontAdjust::Adjust)
    {
        return Fonts::appFont(Utils::scale_value(Fonts::adjustFontSize(_unscaledSize)), Fonts::adjustFontWeight(_weight), _adjust);
    }

    inline QFont adjustedAppFont(const int _unscaledSize, const Fonts::FontFamily _family, const Fonts::FontWeight _weight = Fonts::defaultAppFontWeight(), const FontAdjust _adjust = FontAdjust::Adjust)
    {
        return Fonts::appFont(Utils::scale_value(Fonts::adjustFontSize(_unscaledSize)), _family, Fonts::adjustFontWeight(_weight), _adjust);
    }

    bool getFontBoldSetting();
    void setFontBoldSetting(const bool _boldEnabled);

    FontSize getFontSizeSetting();
    void setFontSizeSetting(const FontSize _size);

FONTS_NS_END

namespace std
{
    template<>
    struct hash<Fonts::FontWeight>
    {
        size_t operator()(const Fonts::FontWeight &_v) const
        {
            return (size_t)_v;
        }
    };
}