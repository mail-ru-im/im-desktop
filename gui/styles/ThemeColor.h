#pragma once

#include "StyleVariable.h"

namespace Styling
{
    struct ThemeColorKey
    {
        ThemeColorKey();
        explicit ThemeColorKey(StyleVariable _style, const QString& _aimId = {});
        explicit ThemeColorKey(StyleVariable _style, double _alpha, const QString& _aimId = {});

        QString aimId() const { return aimId_; }
        StyleVariable style() const { return style_; }
        std::optional<double> alpha() const { return alpha_; }
        bool isValid() const { return style_ != StyleVariable::INVALID; }

        void setAlpha(double _alpha);

    private:
        friend bool operator==(const ThemeColorKey& _lhs, const ThemeColorKey& _rhs);

    private:
        QString aimId_;
        std::optional<double> alpha_;
        StyleVariable style_;
    };

    struct ColorParameter
    {
        ColorParameter();
        ColorParameter(const ThemeColorKey& _key);
        explicit ColorParameter(const QColor& _color);

        ThemeColorKey key() const;
        QColor color() const;
        bool isValid() const;

    private:
        std::variant<ThemeColorKey, QColor> color_;
    };

    struct ThemeChecker
    {
        explicit ThemeChecker(const QString& _aimId = {});

        bool needUpdate() const;
        void updateHash();
        bool checkAndUpdateHash();

    private:
        size_t currentThemeHash() const;

    private:
        QString aimId_;
        size_t lastHash_;
    };

    struct ColorContainer
    {
        ColorContainer();
        ColorContainer(const ThemeColorKey& _key);
        explicit ColorContainer(const QColor& _color);
        explicit ColorContainer(const ColorParameter& _color);

        ThemeColorKey key() const { return key_; }
        QColor cachedColor() const { return cachedColor_; }
        QColor actualColor();
        bool isValid() const;

        bool canUpdateColor() const;
        void updateColor();

    private:
        friend bool operator==(const ColorContainer& _lhs, const ColorContainer& _rhs);

    private:
        ThemeColorKey key_;
        ThemeChecker themeChecker_;
        QColor cachedColor_;
    };

    [[nodiscard]] QColor getColor(const ThemeColorKey& _colorKey);
    [[nodiscard]] QString getColorHex(const ThemeColorKey& _colorKey);
}
