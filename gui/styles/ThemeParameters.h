#pragma once

#include "StyleVariable.h"

namespace Ui
{
    class CustomButton;
}

namespace Styling
{
    using ThemePtr = std::shared_ptr<class Theme>;
    using WallpaperPtr = std::shared_ptr<class ThemeWallpaper>;
    struct ThemeColorKey;
    class BaseStyleSheetGenerator;

    class ThemeParameters
    {
    public:
        ThemeParameters(ThemePtr _theme, WallpaperPtr _wall) : theme_(_theme), wallpaper_(_wall) { im_assert(_theme); }
        size_t idHash() const;

        [[nodiscard]] QColor getColor(const StyleVariable _var) const;
        [[nodiscard]] QColor getColor(const StyleVariable _var, double alpha) const;
        [[nodiscard]] QString getColorHex(const StyleVariable _var) const; // get color string in #AARRGGBB format
        [[nodiscard]] QString getColorHex(const StyleVariable _var, double alpha) const; // get color string in #AARRGGBB format

        //! Used when a color with alpha is to be painted above some background color. Background's alpha is ignored
        //! It seems this algo is different from figma's algo which leads to 1% transparency discrepancy
        [[nodiscard]] QColor getColorAlphaBlended(const StyleVariable _bg, const StyleVariable _fg) const;
        [[nodiscard]] QString getColorAlphaBlendedHex(const StyleVariable _bg, const StyleVariable _fg) const;

        [[nodiscard]] QColor getSelectedTabOrRecentItemBackground() const;

        [[nodiscard]] bool isChatWallpaperPlainColor() const;
        [[nodiscard]] QColor getChatWallpaperPlainColor() const;
        [[nodiscard]] QPixmap getChatWallpaper() const;
        [[nodiscard]] bool isChatWallpaperTiled() const;
        [[nodiscard]] bool isChatWallpaperImageLoaded() const;
        void requestChatWallpaper() const;

        [[nodiscard]] bool isBorderNeeded() const noexcept;

        [[nodiscard]] QString getLineEditCommonQss(bool _isError = false, int _height = 48, const StyleVariable _var = StyleVariable::BASE_BRIGHT, const double _alpha = 1.0) const;
        [[nodiscard]] QString getTextLineEditCommonQss(bool _isError = false, int _height = 48) const;
        [[nodiscard]] QString getTextLineEditCommonQssNoHeight(bool _isError = false) const;
        [[nodiscard]] QString getLineEditDisabledQss(int _height = 48, const StyleVariable _var = StyleVariable::BASE_PRIMARY, const double _alpha = 1.0) const;
        [[nodiscard]] QString getLineEditCustomQss(StyleVariable _lineFocusColor, StyleVariable _lineNoFocusColor, int _height = 48) const;

        [[nodiscard]] QString getTextEditBoxCommonQss() const;
        [[nodiscard]] QString getTextEditCommonQss(bool _hasBorder = false, const StyleVariable _var = StyleVariable::BASE_BRIGHT) const;
        [[nodiscard]] QString getTextEditBadQss() const;

        [[nodiscard]] std::unique_ptr<BaseStyleSheetGenerator> getContextMenuQss(int _itemHeight, int _paddingLeft, int _paddingRight, int _paddingIcon) const;
        [[nodiscard]] std::unique_ptr<BaseStyleSheetGenerator> getContextMenuDarkQss(int _itemHeight, int _paddingLeft, int _paddingRight, int _paddingIcon) const;
        [[nodiscard]] std::unique_ptr<BaseStyleSheetGenerator> getFlatMenuQss(StyleVariable _borderColor) const;

        [[nodiscard]] QString getStickersQss() const;
        [[nodiscard]] QString getSmilesQss() const;

        [[nodiscard]] QString getGeneralSettingsQss() const;
        [[nodiscard]] QString getContactUsQss() const;
        [[nodiscard]] std::unique_ptr<BaseStyleSheetGenerator> getComboBoxQss() const;

        [[nodiscard]] QString getTitleQss() const;
        [[nodiscard]] std::unique_ptr<BaseStyleSheetGenerator> getLoginPageQss() const;

        [[nodiscard]] std::unique_ptr<BaseStyleSheetGenerator> getScrollBarQss(const int _width = 8, const int _margins = 4) const;
        [[nodiscard]] QString getVerticalScrollBarSimpleQss() const;
        [[nodiscard]] std::unique_ptr<BaseStyleSheetGenerator> getPhoneComboboxQss() const;

        [[nodiscard]] Styling::ThemeColorKey getPrimaryTabFocusColorKey() const;
        [[nodiscard]] QColor getPrimaryTabFocusColor() const;

        [[nodiscard]] QColor getAppTitlebarColor() const;

    protected:
        void setTextLineEditCommonQssNoHeight(QString& _template, bool _isError) const;

    private:
        ThemePtr theme_;
        WallpaperPtr wallpaper_;
    };

    [[nodiscard]] ThemeParameters getParameters(const QString& _aimId = {});

    namespace Scrollbar
    {
        QColor getScrollbarBackgroundColor();
        QColor getScrollbarButtonColor();
        QColor getScrollbarButtonHoveredColor();
    } // namespace Scrollbar

    namespace Buttons
    {
        void setButtonDefaultColors(Ui::CustomButton* _button);
        Styling::ThemeColorKey defaultColorKey();
        QColor defaultColor();
        Styling::ThemeColorKey hoverColorKey();
        QColor hoverColor();
        Styling::ThemeColorKey pressedColorKey();
        QColor pressedColor();
        Styling::ThemeColorKey activeColorKey();
    } // namespace Buttons
} // namespace Styling