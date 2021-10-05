#include "stdafx.h"

#include "utils/utils.h"
#include "gui_settings.h"
#include "fonts.h"

FONTS_NS_BEGIN

namespace
{
    typedef std::unordered_map<FontWeight, QString> QssWeightsMapT;

    void applyFontFamily(const FontFamily _fontFamily, Out QFont &_font);

    void applyFontWeight(
            const FontFamily _fontFamily,
            const FontWeight _fontWeight,
            const int32_t _sizePx,
            Out QFont &_font);

    const QssWeightsMapT& getCurrentWeightsMap();

    const QssWeightsMapT& getSourceSansProWeightsMap();

    const QssWeightsMapT& getSegoeUIWeightsMap();

    QString evalQssFontWeight(const FontFamily _fontFamily, const FontWeight _fontStyle);

    QString segoeUiFamilyName(const FontWeight _weight);

}

QFont appFont(const int32_t _sizePx, const FontAdjust _adjust)
{
    return appFont(_sizePx, defaultAppFontFamily(), defaultAppFontWeight(), _adjust);
}

QFont appFont(const int32_t _sizePx, const FontFamily _family, const FontAdjust _adjust)
{
    return appFont(_sizePx, _family, defaultAppFontWeight(), _adjust);
}

QFont appFont(const int32_t _sizePx, const FontWeight _weight, const FontAdjust _adjust)
{
    return appFont(_sizePx, defaultAppFontFamily(), _weight, _adjust);
}

QFont appFont(const int32_t _sizePx, const FontFamily _family, const FontWeight _weight, const FontAdjust _adjust)
{
    im_assert(_sizePx > 0);
    im_assert(_sizePx < 500);
    im_assert(_family > FontFamily::MIN);
    im_assert(_family < FontFamily::MAX);
    im_assert(_weight > FontWeight::Min);
    im_assert(_weight < FontWeight::Max);

    QFont result;

    const auto _fSize = (_adjust == FontAdjust::Adjust && platform::is_apple())? _sizePx - 1 : _sizePx;
    result.setPixelSize(_fSize);

    applyFontFamily(_family, Out result);
    applyFontWeight(_family, _weight, _fSize, Out result);

    return result;
}

QFont appFontScaled(const int32_t _sizePx)
{
    return appFont(Utils::scale_value(_sizePx));
}

QFont appFontScaled(const int32_t _sizePx, const FontWeight _weight)
{
    return appFont(Utils::scale_value(_sizePx), _weight);
}

QFont appFontScaled(const int32_t _sizePx, const FontFamily _family)
{
    return appFont(Utils::scale_value(_sizePx), _family);
}

QFont appFontScaled(const int32_t _sizePx, const FontFamily _family, const FontWeight _weight)
{
    return appFont(Utils::scale_value(_sizePx), _family, _weight);
}

QFont appFontScaledFixed(const int32_t _sizePx)
{
    return appFont(Utils::scale_value(_sizePx), FontAdjust::NoAdjust);
}

QFont appFontScaledFixed(const int32_t _sizePx, const FontWeight _weight)
{
    return appFont(Utils::scale_value(_sizePx), _weight, FontAdjust::NoAdjust);
}

QFont appFontScaledFixed(const int32_t _sizePx, const FontFamily _family)
{
    return appFont(Utils::scale_value(_sizePx), _family, FontAdjust::NoAdjust);
}

QFont appFontScaledFixed(const int32_t _sizePx, const FontFamily _family, const FontWeight _weight)
{
    return appFont(Utils::scale_value(_sizePx), _family, _weight, FontAdjust::NoAdjust);
}

FontFamily defaultAppFontFamily()
{
    if constexpr (platform::is_apple())
        return FontFamily::SF_PRO_TEXT;
    else
        return FontFamily::SOURCE_SANS_PRO;
}

FontFamily monospaceAppFontFamily()
{
    if constexpr (platform::is_apple())
        return Fonts::FontFamily::SF_MONO;
    else
        return Fonts::FontFamily::ROBOTO_MONO;
}

FontWeight defaultAppFontWeight()
{
    return FontWeight::Normal;
}

QString appFontFullQss(const int32_t _sizePx, const FontFamily _fontFamily, const FontWeight _fontWeight)
{
    im_assert(_sizePx > 0);
    im_assert(_sizePx < 1000);
    im_assert(_fontFamily > FontFamily::MIN);
    im_assert(_fontFamily < FontFamily::MAX);
    im_assert(_fontWeight > FontWeight::Min);
    im_assert(_fontWeight < FontWeight::Max);


    const auto weight = evalQssFontWeight(_fontFamily, _fontWeight);
    const auto weight_qss = weight.isEmpty() ? QStringView() : QStringView(u"; font-weight: ");

    return u"font-size: "
        % QString::number(_sizePx)
        % u"px; font-family: \""
        % appFontFamilyNameQss(_fontFamily, _fontWeight)
        % u'"'
        % weight_qss
        % weight;
}

QString appFontFamilyNameQss(const FontFamily _fontFamily, const FontWeight _fontWeight)
{
    im_assert(_fontFamily > FontFamily::MIN);
    im_assert(_fontFamily < FontFamily::MAX);

    switch (_fontFamily)
    {
        case FontFamily::SOURCE_SANS_PRO:
            return qsl("Source Sans Pro");

        case FontFamily::SEGOE_UI:
            return segoeUiFamilyName(_fontWeight);

        case FontFamily::SF_PRO_TEXT:
            return qsl("SF Pro Text");

        default:
            break;
    }

    im_assert(!"unexpected font family");
    return qsl("Comic Sans");
}

QString appFontWeightQss(const FontWeight _weight)
{
    const auto &weightMap = getCurrentWeightsMap();

    const auto iter = weightMap.find(_weight);
    if (iter == weightMap.end())
    {
        im_assert(!"unknown font weight");
        return defaultAppFontQssWeight();
    }

    const auto &fontWeight = iter->second;

    return fontWeight;
}

QString defaultAppFontQssName()
{
    return appFontFamilyNameQss(defaultAppFontFamily(), FontWeight::Normal);
}

QString defaultAppFontQssWeight()
{
    const auto &weights = getCurrentWeightsMap();

    auto iter = weights.find(defaultAppFontWeight());
    im_assert(iter != weights.end());

    return iter->second;
}

QString SetFont(QString _qss)
{
    QString result(std::move(_qss));

    const auto fontFamily = appFontFamilyNameQss(defaultAppFontFamily(), FontWeight::Normal);
    const auto fontFamilyMedium = appFontFamilyNameQss(defaultAppFontFamily(), FontWeight::SemiBold);
    const auto fontFamilyLight = appFontFamilyNameQss(defaultAppFontFamily(), FontWeight::Light);

    result.replace(ql1s("%FONT_FAMILY%"), fontFamily);
    result.replace(ql1s("%FONT_FAMILY_MEDIUM%"), fontFamilyMedium);
    result.replace(ql1s("%FONT_FAMILY_LIGHT%"), fontFamilyLight);

    const auto fontWeightQss = appFontWeightQss(FontWeight::Normal);
    const auto fontWeightMedium = appFontWeightQss(FontWeight::SemiBold);
    const auto fontWeightLight = appFontWeightQss(FontWeight::Light);

    result.replace(ql1s("%FONT_WEIGHT%"), fontWeightQss);
    result.replace(ql1s("%FONT_WEIGHT_MEDIUM%"), fontWeightMedium);
    result.replace(ql1s("%FONT_WEIGHT_LIGHT%"), fontWeightLight);

    return result;
}

int adjustFontSize(const int _size)
{
    switch (getFontSizeSetting())
    {
        case FontSize::Large:
            return _size + 2;

        case FontSize::Small:
            return _size - 1;

        default:
            break;
    }

    return _size;
}

FontWeight adjustFontWeight(const FontWeight _weight)
{
    if (getFontBoldSetting())
    {
        im_assert(_weight != FontWeight::Min && _weight != FontWeight::Max);

        switch (_weight)
        {
            case FontWeight::Light:
                return FontWeight::Normal;

            case FontWeight::Normal:
                return FontWeight::Medium;

            case FontWeight::Medium:
                return FontWeight::SemiBold;

            case FontWeight::SemiBold:
                return FontWeight::Bold;

            default:
                break;
        }
    }
    return _weight;
}

bool getFontBoldSetting()
{
    return Ui::get_gui_settings()->get_value<bool>(settings_appearance_bold_text, false);
}

void setFontBoldSetting(const bool _boldEnabled)
{
    Ui::get_gui_settings()->set_value<bool>(settings_appearance_bold_text, _boldEnabled);
}

FontSize getFontSizeSetting()
{
    return (Fonts::FontSize)Ui::get_gui_settings()->get_value<int>(settings_appearance_text_size, (int)Fonts::FontSize::Medium);
}

void setFontSizeSetting(const FontSize _size)
{
    Ui::get_gui_settings()->set_value<int>(settings_appearance_text_size, (int)_size);
}

QFont getInputTextFont()
{
    auto font = Fonts::appFontScaled(15);

    if constexpr (platform::is_apple())
        font.setLetterSpacing(QFont::AbsoluteSpacing, -0.25);

    return font;
}

QFont getInputMonospaceTextFont()
{
    auto font = Fonts::appFontScaled(14, Fonts::monospaceAppFontFamily());

    if constexpr (platform::is_apple())
        font.setLetterSpacing(QFont::AbsoluteSpacing, -0.25);

    return font;
}


namespace
{
    const QssWeightsMapT& getCurrentWeightsMap()
    {
        return getSourceSansProWeightsMap();
    }

    const QssWeightsMapT& getSourceSansProWeightsMap()
    {
        static const QssWeightsMapT weightMap =
                {
                        { FontWeight::Light, qsl("300") },
                        { FontWeight::Normal, qsl("400") },
                        { FontWeight::SemiBold, qsl("600") },
                        { FontWeight::Bold, qsl("700") },
                };

        return weightMap;
    }

    const QssWeightsMapT& getSegoeUIWeightsMap()
    {
        static const QssWeightsMapT weightMap =
                {
                        { FontWeight::Light, qsl("200") },
                        { FontWeight::Normal, qsl("400") },
                        { FontWeight::SemiBold, qsl("450") },
                        { FontWeight::Bold, qsl("700") }
                };

        return weightMap;
    }

    QString evalQssFontWeight(const FontFamily _fontFamily, const FontWeight _fontWeight)
    {
        im_assert(_fontFamily > FontFamily::MIN);
        im_assert(_fontFamily < FontFamily::MAX);
        im_assert(_fontWeight > FontWeight::Min);
        im_assert(_fontWeight < FontWeight::Max);

        if (_fontFamily == FontFamily::SEGOE_UI)
        {
            return QString();
        }

        if (_fontFamily == FontFamily::SOURCE_SANS_PRO || _fontFamily == FontFamily::SF_PRO_TEXT)
        {
            return appFontWeightQss(_fontWeight);
        }

        im_assert(!"unknown font family / style comnbination");
        return defaultAppFontQssWeight();
    }

    QString segoeUiFamilyName(const FontWeight _weight)
    {
        im_assert(_weight > FontWeight::Min);
        im_assert(_weight < FontWeight::Max);

        QString familyName;
        familyName.reserve(1024);

        familyName += ql1s("Segoe UI");

        switch (_weight)
        {
            case FontWeight::Light:
                familyName += ql1s(" Light");
                break;

            case FontWeight::Normal:
                break;

            case FontWeight::SemiBold:
                familyName += ql1s(" Semibold");
                break;

            default:
                im_assert(!"unknown font weight");
                break;
        }

        return familyName;
    }

    void applyFontFamily(const FontFamily _fontFamily, Out QFont &_font)
    {
        im_assert(_fontFamily > FontFamily::MIN);
        im_assert(_fontFamily < FontFamily::MAX);

        switch (_fontFamily)
        {
            case FontFamily::SEGOE_UI:
                _font.setFamily(qsl("Segoe UI"));
                return;

            case FontFamily::SOURCE_SANS_PRO:
                _font.setFamily(qsl("Source Sans Pro"));
                return;

            case FontFamily::ROUNDED_MPLUS:
                _font.setFamily(qsl("Rounded Mplus 1c Bold"));
                return;

            case FontFamily::ROBOTO_MONO:
                _font.setFamily(qsl("Roboto Mono"));
                return;

            case FontFamily::SF_PRO_TEXT:
                _font.setFamily(qsl("SF Pro Text"));
                return;

            case FontFamily::SF_MONO:
                _font.setFamily(qsl("SF Mono"));
                return;

            default:
                im_assert(!"unexpected font family");
                return;
        }
    }

    void applyFontWeight(
            const FontFamily _fontFamily,
            const FontWeight _fontWeight,
            int32_t _sizePx,
            Out QFont &_font)
    {
        im_assert(_fontFamily > FontFamily::MIN);
        im_assert(_fontFamily < FontFamily::MAX);
        im_assert(_fontWeight > FontWeight::Min);
        im_assert(_fontWeight < FontWeight::Max);
        im_assert(_sizePx > 0);
        im_assert(_sizePx < 1000);

        switch (_fontFamily)
        {
        case FontFamily::SEGOE_UI:
        {
            switch (_fontWeight)
            {
            case FontWeight::Light:
                _font.setFamily(qsl("Segoe UI Light"));
                break;

            case FontWeight::Normal:
                break;

            case FontWeight::SemiBold:
                _font.setFamily(qsl("Segoe UI Semibold"));
                break;

            default:
                im_assert(!"unexpected font style");
            }
            break;
        }

        case FontFamily::SOURCE_SANS_PRO:
        {
            switch (_fontWeight)
            {
            case FontWeight::Normal:
                _font.setWeight(QFont::Weight::Normal);
                break;
            case FontWeight::Light:
                _font.setWeight(QFont::Weight::Light);
                break;
            case FontWeight::Medium:
            case FontWeight::SemiBold:
            {
                if constexpr (platform::is_windows())
                    _font.setFamily(qsl("Source Sans Pro Semibold"));
                else
                    _font.setWeight(QFont::Weight::DemiBold);
                break;
            }
            case FontWeight::Bold:
                _font.setWeight(QFont::Weight::Bold);
                break;
            default:
                im_assert(!"unexpected font style");
                break;
            }
            break;
        }

        case FontFamily::SF_PRO_TEXT:
        {
            switch (_fontWeight)
            {
            case FontWeight::Normal:
                _font.setWeight(QFont::Weight::Normal);
                break;
            case FontWeight::Light:
                _font.setWeight(QFont::Weight::Light);
                break;
            case FontWeight::Medium:
                _font.setWeight(QFont::Weight::Medium);
                _font.setHintingPreference(QFont::PreferVerticalHinting);
                break;
            case FontWeight::SemiBold:
                _font.setWeight(QFont::Weight::DemiBold);
                _font.setHintingPreference(QFont::PreferVerticalHinting);
                break;
            case FontWeight::Bold:
                _font.setWeight(QFont::Weight::Bold);
                _font.setHintingPreference(QFont::PreferVerticalHinting);
                break;
            default:
                break;
            }
            break;
        }

        case FontFamily::ROBOTO_MONO:
        {
            switch (_fontWeight)
            {
            case FontWeight::Light:
                im_assert(!"unexpected font style");
                [[fallthrough]];
            case FontWeight::Normal:
                _font.setWeight(QFont::Weight::Normal);
                break;
            case FontWeight::Medium:
                _font.setWeight(QFont::Weight::Medium);
                _font.setHintingPreference(QFont::PreferVerticalHinting);
                break;
            case FontWeight::SemiBold:
                im_assert(!"unexpected font style");
                [[fallthrough]];
            case FontWeight::Bold:
                _font.setWeight(QFont::Weight::Bold);
                _font.setHintingPreference(QFont::PreferVerticalHinting);
                break;
            default:
                break;
            }
            break;
        }

        case FontFamily::SF_MONO:
        {
            switch (_fontWeight)
            {
            case FontWeight::Light:
            case FontWeight::Normal:
                _font.setWeight(QFont::Weight::Normal);
                break;
            case FontWeight::Medium:
                _font.setWeight(QFont::Weight::Medium);
                _font.setHintingPreference(QFont::PreferVerticalHinting);
                break;
            case FontWeight::SemiBold:
                im_assert(!"unexpected font style");
                [[fallthrough]];
            case FontWeight::Bold:
                _font.setWeight(QFont::Weight::Bold);
                break;
            default:
                break;
            }
            break;
        }

        case FontFamily::ROUNDED_MPLUS:
            _font.setWeight(QFont::Weight::Bold);
            break;

        default:
            im_assert(!"unexpected font family");
        }
    }
}

FONTS_NS_END
