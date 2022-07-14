#include "stdafx.h"

#include "ThemeParameters.h"
#include "ThemesContainer.h"
#include "ThemeColor.h"
#include "StyleSheetContainer.h"
#include "StyleSheetGenerator.h"

#include "fonts.h"
#include "controls/CustomButton.h"

#include "app_config.h"
#include "../common.shared/config/config.h"

namespace
{
    QString getTextLineEditQss();
    QString getTextLineEditQssNoHeight();
    QString getTextLineEditFocusQss();
    QString getTextLineEditFocusQssNoHeight();
    QString getLineEditQss();
    QString getLineEditFocusQss();
    QString getTextEditQss();
    QString getTextEditBoxQss();
    QString getBackgroundQss();

    const auto scrollbarBg = Styling::ThemeColorKey { Styling::StyleVariable::GHOST_LIGHT_INVERSE };
    const auto scrollbarBtn = Styling::ThemeColorKey { Styling::StyleVariable::GHOST_SECONDARY_INVERSE };
    const auto scrollbarBtnHover = Styling::ThemeColorKey { Styling::StyleVariable::GHOST_SECONDARY_INVERSE_HOVER };
} // namespace

namespace Styling
{
    size_t ThemeParameters::idHash() const
    {
        return theme_->idHash() ^ wallpaper_->idHash();
    }

    QColor ThemeParameters::getColor(const StyleVariable _var) const
    {
        if (wallpaper_)
            return wallpaper_->getColor(_var);

        return theme_->getColor(_var);
    }

    QColor ThemeParameters::getColor(const StyleVariable _var, double alpha) const
    {
        auto c = getColor(_var);
        c.setAlphaF(alpha);
        return c;
    }

    QString ThemeParameters::getColorHex(const StyleVariable _var) const
    {
        return getColor(_var).name(QColor::HexArgb);
    }

    QString ThemeParameters::getColorHex(const StyleVariable _var, double alpha) const
    {
        return getColor(_var, alpha).name(QColor::HexArgb);
    }

    QColor ThemeParameters::getSelectedTabOrRecentItemBackground() const
    {
        return getColor(Styling::StyleVariable::PRIMARY, 0.1);
    }

    QColor ThemeParameters::getColorAlphaBlended(const StyleVariable _bg, const StyleVariable _fg) const
    {
        const auto fg = Styling::getParameters().getColor(_fg);
        const auto bg = Styling::getParameters().getColor(_bg);
        constexpr auto bgAlpha = 1.0;
        const auto alphaDenominator = fg.alphaF() + bgAlpha * (1.0 - fg.alphaF());
        const auto blend = [alphaFg = fg.alphaF(), alphaDenominator](qreal _bg, qreal _fg)
        {
            return (_fg * alphaFg + _bg * (1 - alphaFg)) / alphaDenominator;
        };

        auto result = QColor();
        result.setRgbF(
            blend(bg.redF(), fg.redF()),
            blend(bg.greenF(), fg.greenF()),
            blend(bg.blueF(), fg.blueF())
        );
        return result;
    }

    QString ThemeParameters::getColorAlphaBlendedHex(const StyleVariable _bg, const StyleVariable _fg) const
    {
        return getColorAlphaBlended(_bg, _fg).name(QColor::HexArgb);
    }

    bool ThemeParameters::isChatWallpaperPlainColor() const
    {
        if (const auto wp = wallpaper_ ? wallpaper_ : getThemesContainer().getGlobalWallpaper())
            return !wp->hasWallpaper();

        return false;
    }

    QColor ThemeParameters::getChatWallpaperPlainColor() const
    {
        if (const auto wp = wallpaper_ ? wallpaper_ : getThemesContainer().getGlobalWallpaper())
            return wp->getBgColor();

        return QColor();
    }

    QPixmap ThemeParameters::getChatWallpaper() const
    {
        const auto colorPixmap = [](const QColor& _color)
        {
            QPixmap px(100, 100);
            px.fill(_color);
            Utils::check_pixel_ratio(px);
            return px;
        };

        const auto wp = wallpaper_ ? wallpaper_ : getThemesContainer().getGlobalWallpaper();

        if (wp)
        {
            if (wp->hasWallpaper())
            {
                if (const auto& pm = wp->getWallpaperImage(); !pm.isNull())
                {
                    return pm;
                }
                else
                {
                    getThemesContainer().requestWallpaper(wp);
                    return colorPixmap(wp->getBgColor());
                }
            }
            else
            {
                return colorPixmap(wp->getBgColor());
            }
        }

        return colorPixmap(getColor(StyleVariable::APP_PRIMARY));
    }

    bool ThemeParameters::isChatWallpaperTiled() const
    {
        if (const auto wp = wallpaper_ ? wallpaper_ : getThemesContainer().getGlobalWallpaper())
            return wp->isTiled();

        return false;
    }

    bool ThemeParameters::isChatWallpaperImageLoaded() const
    {
        if (const auto wp = wallpaper_ ? wallpaper_ : getThemesContainer().getGlobalWallpaper())
            return !wp->getWallpaperImage().isNull();

        return false;
    }

    void ThemeParameters::requestChatWallpaper() const
    {
        if (const auto wp = wallpaper_ ? wallpaper_ : getThemesContainer().getGlobalWallpaper())
            getThemesContainer().requestWallpaper(wp);
    }

    std::unique_ptr<BaseStyleSheetGenerator> ThemeParameters::getScrollBarQss(const int _width, const int _margins) const
    {
        QString qss = ql1s(
            "QScrollBar:vertical{"
            "background-color: %3;"
            "width: %2dip;"
            "margin-top: %1dip;"
            "margin-bottom: %1dip;"
            "margin-right: %1dip;}"
            "QScrollBar:horizontal{"
            "background-color: %3;"
            "height: %2dip;"
            "margin-bottom: %1dip;"
            "margin-right: %1dip;"
            "margin-left: %1dip; }"
            "QScrollBar:hover:vertical{"
            "background-color: %3;"
            "margin-top: %1dip;"
            "margin-bottom: %1dip;"
            "margin-right: %1dip; }"
            "QScrollBar:hover:horizontal{"
            "background-color: %3;"
            "margin-bottom: %1dip;"
            "margin-right: %1dip;"
            "margin-left: %1dip; }"
            "QScrollBar::handle:vertical{"
            "background-color: %4;"
            "min-height: 50dip; }"
            "QScrollBar::handle:horizontal{"
            "background-color: %4;"
            "min-width: 50dip; }"
            "QScrollBar::handle:hover:vertical{"
            "background-color: %5;"
            "min-height: 50dip; }"
            "QScrollBar::handle:hover:horizontal{"
            "background-color: %5;"
            "min-width: 50dip; }"
            "QScrollBar::add-line, QScrollBar::sub-line{"
            "background-color: transparent; }"
            "QScrollBar::sub-page, QScrollBar::add-page{"
            "background: none; }"
        ).arg(
            QString::number(_margins),
            QString::number(_width)
        );

        return std::make_unique<ArrayStyleSheetGenerator>(qss, std::vector<ThemeColorKey> { scrollbarBg, scrollbarBtn, scrollbarBtnHover });
    }

    QString ThemeParameters::getVerticalScrollBarSimpleQss() const
    {
        return qsl("\
            QScrollBar:vertical\
            {\
                width: 4dip;\
                margin-top: 0px;\
                margin-bottom: 0px;\
                margin-right: 0px;\
            }");
    }

    std::unique_ptr<BaseStyleSheetGenerator> ThemeParameters::getPhoneComboboxQss() const
    {
        std::vector<std::unique_ptr<BaseStyleSheetGenerator>> styleSheets;
        styleSheets.reserve(2);
        styleSheets.push_back(getScrollBarQss());
        styleSheets.push_back(std::make_unique<ArrayStyleSheetGenerator>(
            ql1s("QTreeView { background: %1; color: %2;}"),
            std::vector<ThemeColorKey> { ThemeColorKey { StyleVariable::BASE_GLOBALWHITE }, ThemeColorKey { StyleVariable::TEXT_SOLID } }));
        return std::make_unique<CombinedStyleSheetGenerator>(std::move(styleSheets));
    }

    Styling::ThemeColorKey ThemeParameters::getPrimaryTabFocusColorKey() const
    {
        return Styling::ThemeColorKey{ Styling::StyleVariable::PRIMARY, 0.2 };
    }

    QColor ThemeParameters::getPrimaryTabFocusColor() const
    {
        return Styling::getColor(getPrimaryTabFocusColorKey());
    }

    QColor ThemeParameters::getAppTitlebarColor() const
    {
        auto var = platform::is_apple() ? Styling::StyleVariable::APP_PRIMARY : Styling::StyleVariable::BASE_BRIGHT_INVERSE;
        if (config::get().is_debug())
            var = Styling::StyleVariable::SECONDARY_RAINBOW_BLUE;
        else if (config::get().is_develop())
            var = Styling::StyleVariable::SECONDARY_RAINBOW_GREEN;
        else if (Ui::GetAppConfig().hasCustomDeviceId())
            var = Styling::StyleVariable::SECONDARY_RAINBOW_WARM;
        else if (config::get().has_develop_cli_flag())
            var = Styling::StyleVariable::SECONDARY_RAINBOW_PURPLE;

        return Styling::getParameters().getColor(var);
    }

    bool ThemeParameters::isBorderNeeded() const noexcept
    {
        return wallpaper_ && wallpaper_->isBorderNeeded();
    }

    QString ThemeParameters::getLineEditCommonQss(bool _isError, int _height, const StyleVariable _var, const double _alpha) const
    {
        QString qss = getLineEditQss() % getLineEditFocusQss();

        if (_isError)
        {
            qss.replace(ql1s("%BORDER_COLOR%"), getColorHex(StyleVariable::SECONDARY_ATTENTION));
            qss.replace(ql1s("%BORDER_COLOR_FOCUS%"), getColorHex(StyleVariable::SECONDARY_ATTENTION));
        }
        else
        {
            qss.replace(ql1s("%BORDER_COLOR%"), getColorHex(_var, _alpha));
            qss.replace(ql1s("%BORDER_COLOR_FOCUS%"), getColorHex(StyleVariable::PRIMARY));
        }

        qss.replace(ql1s("%BACKGROUND%"), ql1s("transparent"));
        qss.replace(ql1s("%BACKGROUND_FOCUS%"), ql1s("transparent"));
        qss.replace(ql1s("%FONT_COLOR%"), getColorHex(StyleVariable::BASE_PRIMARY));
        qss.replace(ql1s("%FONT_COLOR_FOCUS%"), getColorHex(StyleVariable::TEXT_SOLID));
        qss.replace(ql1s("%HEIGHT%"), QString::number(_height));

        return qss;
    }

    void ThemeParameters::setTextLineEditCommonQssNoHeight(QString& _template, bool _isError) const
    {
        auto& qss = _template;
        if (_isError)
        {
            qss.replace(ql1s("%BORDER_COLOR%"), getColorHex(StyleVariable::SECONDARY_ATTENTION));
            qss.replace(ql1s("%BORDER_COLOR_FOCUS%"), getColorHex(StyleVariable::SECONDARY_ATTENTION));
        }
        else
        {
            qss.replace(ql1s("%BORDER_COLOR%"), getColorHex(StyleVariable::BASE_BRIGHT));
            qss.replace(ql1s("%BORDER_COLOR_FOCUS%"), getColorHex(StyleVariable::PRIMARY));
        }

        qss.replace(ql1s("%BACKGROUND%"), ql1s("transparent"));
        qss.replace(ql1s("%BACKGROUND_FOCUS%"), ql1s("transparent"));
        qss.replace(ql1s("%FONT_COLOR%"), getColorHex(StyleVariable::BASE_PRIMARY));
        qss.replace(ql1s("%FONT_COLOR_FOCUS%"), getColorHex(StyleVariable::TEXT_SOLID));
    }

    QString ThemeParameters::getTextLineEditCommonQssNoHeight(bool _isError) const
    {
        QString qss = getTextLineEditQssNoHeight() % getTextLineEditFocusQssNoHeight();
        setTextLineEditCommonQssNoHeight(qss, _isError);
        return qss;
    }

    QString ThemeParameters::getTextLineEditCommonQss(bool _isError, int _height) const
    {
        QString qss = getTextLineEditQss() % getTextLineEditFocusQss();
        setTextLineEditCommonQssNoHeight(qss, _isError);
        qss.replace(ql1s("%HEIGHT%"), QString::number(_height));
        return qss;
    }

    QString ThemeParameters::getLineEditDisabledQss(int _height, const StyleVariable _var, const double _alpha) const
    {
        auto qss = getLineEditQss();
        qss.replace(ql1s("%BACKGROUND%"), ql1s("transparent"));
        qss.replace(ql1s("%BORDER_COLOR%"), getColorHex(_var, _alpha));
        qss.replace(ql1s("%HEIGHT%"), QString::number(_height));

        return qss;
    }

    QString ThemeParameters::getLineEditCustomQss(StyleVariable _lineFocusColor, StyleVariable _lineNoFocusColor, int _height) const
    {
        QString qss = getLineEditQss() % getLineEditFocusQss();

        if (StyleVariable::INVALID != _lineNoFocusColor)
            qss.replace(ql1s("%BORDER_COLOR%"), getColorHex(_lineNoFocusColor));
        else
            qss.replace(ql1s("%BORDER_COLOR%"), ql1s("transparent"));

        if (StyleVariable::INVALID != _lineFocusColor)
            qss.replace(ql1s("%BORDER_COLOR_FOCUS%"), getColorHex(_lineFocusColor));
        else
            qss.replace(ql1s("%BORDER_COLOR_FOCUS%"), ql1s("transparent"));

        qss.replace(ql1s("%BACKGROUND%"), ql1s("transparent"));
        qss.replace(ql1s("%BACKGROUND_FOCUS%"), ql1s("transparent"));
        qss.replace(ql1s("%HEIGHT%"), QString::number(_height));

        return qss;
    }

    QString ThemeParameters::getTextEditBoxCommonQss() const
    {
        QString qss = getTextEditBoxQss();
        qss.replace(ql1s("%BORDER_COLOR%"), getColorHex(StyleVariable::BASE_BRIGHT));
        qss.replace(ql1s("%BORDER_FOCUS_COLOR%"), getColorHex(StyleVariable::PRIMARY));
        return qss;
    }

    QString ThemeParameters::getTextEditCommonQss(bool _hasBorder, const StyleVariable _var) const
    {
        QString qss = getTextEditQss();

        qss.replace(ql1s("%BORDER_COLOR%"), getColorHex(_hasBorder ? _var : StyleVariable::BASE_GLOBALWHITE));
        qss.replace(ql1s("%BORDER_FOCUS_COLOR%"), getColorHex(_hasBorder ? StyleVariable::PRIMARY : StyleVariable::BASE_GLOBALWHITE));
        return qss;
    }

    QString ThemeParameters::getTextEditBadQss() const
    {
        QString qss = getTextEditQss();

        qss.replace(ql1s("%BORDER_COLOR%"), getColorHex(StyleVariable::SECONDARY_ATTENTION));
        qss.replace(ql1s("%BORDER_FOCUS_COLOR%"), getColorHex(StyleVariable::SECONDARY_ATTENTION));
        return qss;
    }

    static QString getContextMenuCommon(int _itemHeight, int _paddingLeft, int _paddingRight, int _paddingIcon)
    {
        QString qss = qsl(
            "QMenu{"
            "background-color: %BACKGROUND%;"
            "border: 1px solid %BORDER_COLOR%;"
            "padding-top: 4dip;"
            "padding-bottom: 4dip;"
            "border-radius: 0;"
            "}"
            "QMenu::item{"
            "background-color: %BACKGROUND%;"
            "height: %ITEM_HEIGHT%;"
            "padding-left: %PADDING_LEFT%;"
            "padding-right: %PADDING_RIGHT%;"
            "color: %FONT_COLOR%;"
            "}"
            "QMenu::item:selected{"
            "background-color: %BACKGROUND_SELECTED%;"
            "}"
            "QMenu::item:disabled{"
            "background-color: %BACKGROUND%;"
            "color: %FONT_DISABLED_COLOR%;"
            "}"
            "QMenu::item:disabled:selected{"
            "background-color: %BACKGROUND%;"
            "color: %FONT_DISABLED_COLOR%;"
            "}"
            "QMenu::icon{"
            "padding-left: %ICON_PADDING%;"
            "}"
            "QMenu::separator{"
            "background-color: %SEPARATOR%;"
            "height: 1dip;"
            "margin-top: 4dip;"
            "margin-bottom: 4dip;"
            "}"
        );

        qss.replace(ql1s("%ITEM_HEIGHT%"), QString::number(_itemHeight));
        qss.replace(ql1s("%PADDING_LEFT%"), QString::number(_paddingLeft));
        qss.replace(ql1s("%PADDING_RIGHT%"), QString::number(_paddingRight));
        qss.replace(ql1s("%ICON_PADDING%"), QString::number(_paddingIcon));

        return qss;
    }

    std::unique_ptr<BaseStyleSheetGenerator> ThemeParameters::getContextMenuQss(int _itemHeight, int _paddingLeft, int _paddingRight, int _paddingIcon) const
    {
        std::vector<std::pair<QString, StyleVariable>> colors;
        colors.reserve(6);

        colors.emplace_back(ql1s("%BACKGROUND%"), Styling::StyleVariable::BASE_GLOBALWHITE);
        colors.emplace_back(ql1s("%BACKGROUND_SELECTED%"), Styling::StyleVariable::BASE_BRIGHT_INVERSE);
        colors.emplace_back(ql1s("%BORDER_COLOR%"), Styling::StyleVariable::BASE_BRIGHT_INVERSE);
        colors.emplace_back(ql1s("%SEPARATOR%"), Styling::StyleVariable::BASE_BRIGHT);
        colors.emplace_back(ql1s("%FONT_COLOR%"), Styling::StyleVariable::TEXT_SOLID);
        colors.emplace_back(ql1s("%FONT_DISABLED_COLOR%"), Styling::StyleVariable::BASE_TERTIARY);

        return std::make_unique<MapStyleSheetGenerator>(getContextMenuCommon(_itemHeight, _paddingLeft, _paddingRight, _paddingIcon), std::move(colors));
    }

    std::unique_ptr<BaseStyleSheetGenerator> ThemeParameters::getContextMenuDarkQss(int _itemHeight, int _paddingLeft, int _paddingRight, int _paddingIcon) const
    {
        std::vector<std::pair<QString, StyleVariable>> colors;
        colors.reserve(6);

        colors.emplace_back(ql1s("%BACKGROUND%"), Styling::StyleVariable::BASE_GLOBALWHITE_PERMANENT);
        colors.emplace_back(ql1s("%BACKGROUND_SELECTED%"), Styling::StyleVariable::BASE_GLOBALWHITE_PERMANENT_HOVER);
        colors.emplace_back(ql1s("%BORDER_COLOR%"), Styling::StyleVariable::BASE_GLOBALWHITE_PERMANENT_HOVER);
        colors.emplace_back(ql1s("%SEPARATOR%"), Styling::StyleVariable::LUCENT_TERTIARY_ACTIVE);
        colors.emplace_back(ql1s("%FONT_COLOR%"), Styling::StyleVariable::TEXT_SOLID_PERMANENT);
        colors.emplace_back(ql1s("%FONT_DISABLED_COLOR%"), Styling::StyleVariable::TEXT_SOLID_PERMANENT_ACTIVE);

        return std::make_unique<MapStyleSheetGenerator>(getContextMenuCommon(_itemHeight, _paddingLeft, _paddingRight, _paddingIcon), std::move(colors));
    }

    QString ThemeParameters::getStickersQss() const
    {
        QString qss = ql1s(
            "QScrollArea{"
            "border-width: 1dip;"
            "border-style: solid;"
            "border-color: %1;"
            "border-left: none;"
            "border-right: none;}"
            "QLabel#loading_state{"
            "font-size: 14dip;"
            "font-family: %FONT_FAMILY_MEDIUM%;"
            "font-weight: %FONT_WEIGHT_MEDIUM%;"
            "color: %2;}"
            "QWidget#scroll_area_widget{"
            "background-color: %3;"
            "border: none;}"
            "Ui--PackWidget{"
            "background-color: %3;"
            "border: none;}"
            "Ui--StickerPackInfo{"
            "background-color: %3;"
            "border: none;}"
        ).arg(getColorHex(StyleVariable::BASE_GLOBALWHITE), getColorHex(StyleVariable::BASE_SECONDARY), getColorHex(StyleVariable::BASE_GLOBALWHITE));

        return qss;
    }

    QString ThemeParameters::getSmilesQss() const
    {
        const auto qss = ql1s(
            "Ui--Smiles--AddButton{"
                "padding : 0;"
                "margin : 0;}"
            "Ui--Smiles--TabButton{"
                "padding : 0;"
                "margin : 0;"
                "border: none;}"
            "Ui--Smiles--Toolbar{"
                "padding : 0;"
                "margin : 0;"
                "background-color: %1;"
                "border-color: %2;"
                "border-width: 1dip;"
                "border-style: solid;"
                "border-right: none;"
                "border-left: none;"
                "border-top: none;}"
            "Ui--Smiles--Toolbar#smiles_cat_selector{"
                "border-bottom: none;}"
            "QWidget#scroll_area_widget{"
                "background-color: %1;}"
            "QTableView{"
                "background-color: %1;"
                "border: none; }"
            "QScrollArea{"
                "border: none;"
                "background-color: %1;}"
            "Ui--Smiles--SmilesMenu{"
                "background-color: %1;}"
        ).arg(getColorHex(StyleVariable::BASE_GLOBALWHITE), getColorHex(StyleVariable::BASE_BRIGHT_INVERSE));
        return qss;
    }

    QString ThemeParameters::getGeneralSettingsQss() const
    {
        QString qss = qsl(
            "QWidget{"
            "background-color: transparent;"
            "border: none;}"
            "QPushButton{"
            "font-size: 15dip;}"
            "QPushButton::menu-indicator{"
            "image: url(:/controls/arrow_a_100);"
            "width: 12dip;"
            "height: 8dip;"
            "subcontrol-origin: padding;"
            "subcontrol-position: center right;}"
        );

        return qss;
    }

    QString ThemeParameters::getContactUsQss() const
    {
        QString qss = ql1s(
            "QWidget[fileWidget=\"true\"]{"
            "background-color: transparent;"
            "border-radius: 8dip;"
            "border-style: none;"
            "padding-left: 15dip;"
            "padding-right: 15dip;"
            "padding-top: 12dip;"
            "padding-bottom: 10dip;}"
            "QPushButton#successImage{"
            "outline: none;"
            "border-image: url(:/placeholders/good_100);}"
        );

        return qss;
    }

    std::unique_ptr<BaseStyleSheetGenerator> ThemeParameters::getComboBoxQss() const
    {
        QString menuQss = ql1s(
            "QComboBox{"
            "background-color: %1;"
            "border-bottom: 1px solid %5;"
            "min-height: 24dip;"
            "margin-bottom: 8dip;"
            "height: 26dip;"
            "color: %5}"
            "QComboBox::on{"
            "background-color: %1;"
            "border-bottom: 1px solid %5;"
            "margin-bottom: 8dip;"
            "color: %5}"
            "QComboBox::drop-down {"
            "border: 0px;}"
            "QComboBox QAbstractItemView {"
            "border: 2px solid %2;"
            "background-color: %1;"
            "color: %4;"
            "selection-background-color: %3;"
            "selection-color: %4;"
            "outline: none;}"
            "QComboBox::down-arrow{"
            "image: url(:/controls/arrow_a_100);"
            "border: 0px;"
            "outline: none;"
            "width: 12dip;"
            "height: 8dip; }");
        return std::make_unique<Styling::ArrayStyleSheetGenerator>(menuQss,
            std::vector<ThemeColorKey> {
                                         ThemeColorKey { StyleVariable::BASE_GLOBALWHITE },
                                         ThemeColorKey { StyleVariable::BASE_BRIGHT },
                                         ThemeColorKey { StyleVariable::BASE_BRIGHT_INVERSE },
                                         ThemeColorKey { StyleVariable::TEXT_SOLID},
                                         ThemeColorKey { StyleVariable::BASE_PRIMARY } });
    }

    QString ThemeParameters::getTitleQss() const
    {
        return qsl(
            "QLabel#windowIcon{"
            "margin-top: 6dip;"
            "margin-bottom: 6dip;"
            "margin-left: 12dip;"
            "width: 16dip;"
            "height: 16dip;}"
        );
    }

    std::unique_ptr<BaseStyleSheetGenerator> ThemeParameters::getLoginPageQss() const
    {
        QString qss = ql1s(
            "Ui--LoginPage{"
            "background-color: %1;"
            "color: %2;}"
            "QLabel#hint{"
            "color: %2;}"
            "QWidget#mainWidget{"
            "background-color: transparent;}"
            "QWidget#errorWidget{"
            "min-height: 30dip;}"
        );
        return std::make_unique<ArrayStyleSheetGenerator>(
            qss,
            std::vector<ThemeColorKey> { ThemeColorKey { StyleVariable::BASE_GLOBALWHITE }, ThemeColorKey { StyleVariable::TEXT_SOLID } });
    }

    std::unique_ptr<BaseStyleSheetGenerator> ThemeParameters::getFlatMenuQss(StyleVariable _borderColor) const
    {
        QString qss = ql1s(
                "QMenu {"
                "background-color: %1;"
                "border: 2px solid %2;"
                "}"
                "QMenu::item { background: transparent;"
                "height: 28dip;"
                "padding-left: 20dip;"
                "padding-right: 64dip;"
                "padding-top: 4dip;"
                "padding-bottom: 4dip;"
                "color: %4;}"
                "QMenu::item:selected { background: %3; }"
            );

        return std::make_unique<ArrayStyleSheetGenerator>(
            qss,
            std::vector<ThemeColorKey> { ThemeColorKey { StyleVariable::BASE_GLOBALWHITE },
                                         ThemeColorKey { _borderColor },
                                         ThemeColorKey { StyleVariable::BASE_BRIGHT_INVERSE},
                                         ThemeColorKey { StyleVariable::TEXT_SOLID } });
    }

    ThemeParameters getParameters(const QString& _aimId)
    {
        ThemeParameters params(getThemesContainer().getCurrentTheme(), _aimId.isEmpty() ? getThemesContainer().getGlobalWallpaper() : getThemesContainer().getContactWallpaper(_aimId));
        return params;
    }

    namespace Scrollbar
    {
        QColor getScrollbarBackgroundColor()
        {
            return Styling::getParameters().getColor(scrollbarBg.style());
        }

        QColor getScrollbarButtonColor()
        {
            return Styling::getParameters().getColor(scrollbarBtn.style());
        }

        QColor getScrollbarButtonHoveredColor()
        {
            return Styling::getParameters().getColor(scrollbarBtnHover.style());
        }
    }

    namespace Buttons
    {
        void setButtonDefaultColors(Ui::CustomButton* _button)
        {
            im_assert(_button);
            if (_button)
            {
                _button->setDefaultColor(defaultColorKey());
                _button->setHoverColor(hoverColorKey());
                _button->setPressedColor(pressedColorKey());
                _button->setActiveColor(activeColorKey());
            }
        }

        Styling::ThemeColorKey defaultColorKey()
        {
            return Styling::ThemeColorKey{ Styling::StyleVariable::BASE_SECONDARY };
        }

        QColor defaultColor()
        {
            return Styling::getColor(defaultColorKey());
        }

        Styling::ThemeColorKey hoverColorKey()
        {
            return Styling::ThemeColorKey{ Styling::StyleVariable::BASE_SECONDARY_HOVER };
        }

        QColor hoverColor()
        {
            return Styling::getColor(hoverColorKey());
        }

        Styling::ThemeColorKey pressedColorKey()
        {
            return Styling::ThemeColorKey{ Styling::StyleVariable::BASE_SECONDARY_ACTIVE };
        }

        QColor pressedColor()
        {
            return Styling::getColor(pressedColorKey());
        }

        Styling::ThemeColorKey activeColorKey()
        {
            return Styling::ThemeColorKey{ Styling::StyleVariable::PRIMARY };
        }
    } // namespace Buttons
} // namespace Styling

namespace
{
    QString getTextLineEditQss()
    {
        return qsl(
            "QTextBrowser {"
            "min-height: %HEIGHT%dip; max-height: %HEIGHT%dip;"
            "background-color: %BACKGROUND%;"
            "border-style: none;"
            "border-bottom-color: %BORDER_COLOR%;"
            "border-bottom-width: 1dip;"
            "border-bottom-style: solid; }"
        );
    }

    QString getTextLineEditQssNoHeight()
    {
        return qsl(
            "QTextBrowser {"
            "background-color: %BACKGROUND%;"
            "border-style: none;"
            "border-bottom-color: %BORDER_COLOR%;"
            "border-bottom-width: 1dip;"
            "border-bottom-style: solid; }"
        );
    }

    QString getTextLineEditFocusQssNoHeight()
    {
        return qsl(
            "QTextBrowser:focus {"
            "background-color: %BACKGROUND_FOCUS%;"
            "border-style: none;"
            "border-bottom-color: %BORDER_COLOR_FOCUS%;"
            "border-bottom-width: 1dip;"
            "border-bottom-style: solid;}"
        );
    }
    QString getTextLineEditFocusQss()
    {
        return qsl(
            "QTextBrowser:focus {"
            "min-height: %HEIGHT%dip; max-height: %HEIGHT%dip;"
            "background-color: %BACKGROUND_FOCUS%;"
            "border-style: none;"
            "border-bottom-color: %BORDER_COLOR_FOCUS%;"
            "border-bottom-width: 1dip;"
            "border-bottom-style: solid;}"
        );
    }

    QString getLineEditQss()
    {
        return qsl(
            "QLineEdit {"
            "min-height: %HEIGHT%dip; max-height: %HEIGHT%dip;"
            "background-color: %BACKGROUND%;"
            "border-style: none;"
            "border-bottom-color: %BORDER_COLOR%;"
            "border-bottom-width: 1dip;"
            "border-bottom-style: solid; }"
        );
    }

    QString getLineEditFocusQss()
    {
        return qsl(
            "QLineEdit:focus {"
            "min-height: %HEIGHT%dip; max-height: %HEIGHT%dip;"
            "background-color: %BACKGROUND_FOCUS%;"
            "border-style: none;"
            "border-bottom-color: %BORDER_COLOR_FOCUS%;"
            "border-bottom-width: 1dip;"
            "border-bottom-style: solid;}"
        );
    }

    QString getTextEditQss()
    {
        return qsl(
            "QTextBrowser {"
            "background-color: transparent;"
            "border-style: none;"
            "border-bottom-color: %BORDER_COLOR%;"
            "border-bottom-width: 1dip;"
            "border-bottom-style: solid;}"
            "QTextBrowser:focus {"
            "background-color: transparent;"
            "border-style: none;"
            "border-bottom-color: %BORDER_FOCUS_COLOR%;"
            "border-bottom-width: 1dip;"
            "border-bottom-style: solid; }"
        );
    }

    QString getTextEditBoxQss()
    {
        return qsl(
            "Ui--TextEditBox {"
            "background-color: transparent;"
            "border-style: none;"
            "border-bottom-color: %BORDER_COLOR%;"
            "border-bottom-width: 1dip;"
            "border-bottom-style: solid;}"
            "Ui--TextEditBox:focus {"
            "background-color: transparent;"
            "border-style: none;"
            "border-bottom-color: %BORDER_FOCUS_COLOR%;"
            "border-bottom-width: 1dip;"
            "border-bottom-style: solid; }"
        );
    }

    QString getBackgroundQss()
    {
        return qsl("background-color: %BACKGROUND%");
    }
}
