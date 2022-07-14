#include "stdafx.h"
#include "ThemeColor.h"
#include "ThemeParameters.h"

namespace Styling
{
    QColor getColor(const ThemeColorKey& _colorKey)
    {
        const auto params = getParameters(_colorKey.aimId());
        const auto alpha = _colorKey.alpha();
        return alpha ? params.getColor(_colorKey.style(), *alpha) : params.getColor(_colorKey.style());
    }

    QString getColorHex(const ThemeColorKey& _colorKey)
    {
        const auto params = getParameters(_colorKey.aimId());
        const auto alpha = _colorKey.alpha();
        return alpha ? params.getColorHex(_colorKey.style(), *alpha) : params.getColorHex(_colorKey.style());
    }

    ThemeColorKey::ThemeColorKey()
        : ThemeColorKey(StyleVariable::INVALID)
    {}

    ThemeColorKey::ThemeColorKey(StyleVariable _style, const QString& _aimId)
        : aimId_ { _aimId }
        , alpha_ { std::nullopt }
        , style_ { _style }
    {}

    ThemeColorKey::ThemeColorKey(StyleVariable _style, double _alpha, const QString& _aimId)
        : aimId_ { _aimId }
        , alpha_ { _alpha }
        , style_ { _style }
    {}

    void ThemeColorKey::setAlpha(double _alpha)
    {
        im_assert(_alpha <= 1.0 && _alpha >= 0.0);
        alpha_ = _alpha;
    }

    bool operator==(const ThemeColorKey& _lhs, const ThemeColorKey& _rhs)
    {
        return _lhs.aimId() == _rhs.aimId() && _lhs.alpha() == _rhs.alpha() && _lhs.style() == _rhs.style();
    }

    ColorParameter::ColorParameter()
        : ColorParameter(QColor())
    {}

    ColorParameter::ColorParameter(const ThemeColorKey& _key)
        : color_ { _key }
    {}

    ColorParameter::ColorParameter(const QColor& _color)
        : color_ { _color }
    {}

    ThemeColorKey ColorParameter::key() const
    {
        if (auto key = std::get_if<ThemeColorKey>(&color_))
            return *key;
        return {};
    }

    QColor ColorParameter::color() const
    {
        if (auto key = std::get_if<ThemeColorKey>(&color_))
            return getColor(*key);
        return *std::get_if<QColor>(&color_);
    }

    bool ColorParameter::isValid() const
    {
        if (const auto key = std::get_if<ThemeColorKey>(&color_))
            return key->isValid();
        return std::get_if<QColor>(&color_)->isValid();
    }

    bool operator==(const ColorContainer& _lhs, const ColorContainer& _rhs)
    {
        if (_lhs.key_.isValid() || _rhs.key_.isValid())
            return _lhs.key_ == _rhs.key_;

        return _lhs.cachedColor_ == _rhs.cachedColor_;
    }

    ThemeChecker::ThemeChecker(const QString& _aimId)
        : aimId_ { _aimId }
        , lastHash_ { currentThemeHash() }
    {}

    bool ThemeChecker::needUpdate() const { return currentThemeHash() != lastHash_; }

    void ThemeChecker::updateHash() { lastHash_ = currentThemeHash(); }

    bool ThemeChecker::checkAndUpdateHash()
    {
        if (const auto lastHash = currentThemeHash(); lastHash != lastHash_)
        {
            lastHash_ = lastHash;
            return true;
        }
        return false;
    }

    size_t ThemeChecker::currentThemeHash() const { return getParameters(aimId_).idHash(); }

    ColorContainer::ColorContainer()
        : ColorContainer(ThemeColorKey {})
    {}

    ColorContainer::ColorContainer(const ThemeColorKey& _key)
        : key_ { _key }
        , themeChecker_ { key_.aimId() }
        , cachedColor_ { _key.isValid() ? getColor(_key) : QColor {} }
    {}

    ColorContainer::ColorContainer(const QColor& _color)
        : key_ {}
        , themeChecker_ {}
        , cachedColor_ { _color }
    {}

    ColorContainer::ColorContainer(const ColorParameter& _color)
        : key_ { _color.key() }
        , themeChecker_ { key_.aimId() }
        , cachedColor_ { _color.color() }
    {}

    QColor ColorContainer::actualColor()
    {
        updateColor();
        return cachedColor_;
    }

    bool ColorContainer::isValid() const { return key_.isValid() || cachedColor_.isValid(); }

    bool ColorContainer::canUpdateColor() const { return key_.isValid() && themeChecker_.needUpdate(); }

    void ColorContainer::updateColor()
    {
        if (canUpdateColor())
        {
            cachedColor_ = getColor(key_);
            themeChecker_.updateHash();
        }
    }
} // namespace Styling