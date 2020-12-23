#include "stdafx.h"

#include "ThemeParameters.h"
#include "ThemesContainer.h"

#include "fonts.h"
#include "controls/CustomButton.h"

#include "app_config.h"
#include "../common.shared/config/config.h"

namespace
{
    QString getTextLineEditQss();
    QString getTextLineEditFocusQss();
    QString getLineEditQss();
    QString getLineEditFocusQss();
    QString getTextEditQss();
    QString getBackgroundQss();

    constexpr auto scrollbarBg = Styling::StyleVariable::GHOST_LIGHT_INVERSE;
    constexpr auto scrollbarBtn = Styling::StyleVariable::GHOST_SECONDARY_INVERSE;
    constexpr auto scrollbarBtnHover = Styling::StyleVariable::GHOST_SECONDARY_INVERSE_HOVER;
}

namespace Styling
{
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

    QString ThemeParameters::getScrollBarQss(const int _width, const int _margins) const
    {
        QString qss = ql1s(
            "QScrollBar:vertical{"
            "background-color: %1;"
            "width: %5dip;"
            "margin-top: %4dip;"
            "margin-bottom: %4dip;"
            "margin-right: %4dip;}"
            "QScrollBar:horizontal{"
            "background-color: %1;"
            "height: %5dip;"
            "margin-bottom: %4dip;"
            "margin-right: %4dip;"
            "margin-left: %4dip; }"
            "QScrollBar:hover:vertical{"
            "background-color: %1;"
            "margin-top: %4dip;"
            "margin-bottom: %4dip;"
            "margin-right: %4dip; }"
            "QScrollBar:hover:horizontal{"
            "background-color: %1;"
            "margin-bottom: %4dip;"
            "margin-right: %4dip;"
            "margin-left: %4dip; }"
            "QScrollBar::handle:vertical{"
            "background-color: %2;"
            "min-height: 50dip; }"
            "QScrollBar::handle:horizontal{"
            "background-color: %2;"
            "min-width: 50dip; }"
            "QScrollBar::handle:hover:vertical{"
            "background-color: %3;"
            "min-height: 50dip; }"
            "QScrollBar::handle:hover:horizontal{"
            "background-color: %3;"
            "min-width: 50dip; }"
            "QScrollBar::add-line, QScrollBar::sub-line{"
            "background-color: transparent; }"
            "QScrollBar::sub-page, QScrollBar::add-page{"
            "background: none; }"
        ).arg(
            getColorHex(scrollbarBg),
            getColorHex(scrollbarBtn),
            getColorHex(scrollbarBtnHover),
            QString::number(_margins),
            QString::number(_width)
        );

        return qss;
    }

    QString ThemeParameters::getPhoneComboboxQss() const
    {
        return  getScrollBarQss() %
            ql1s("QTreeView { background: %1; color: %2;}").arg(
                getColorHex(Styling::StyleVariable::BASE_GLOBALWHITE),
                getColorHex(Styling::StyleVariable::TEXT_SOLID));
    }

    QColor ThemeParameters::getPrimaryTabFocusColor() const
    {
        return getColor(Styling::StyleVariable::PRIMARY, 0.2);
    }

    QColor ThemeParameters::getAppTitlebarColor() const
    {
        auto var = platform::is_apple() ? Styling::StyleVariable::APP_PRIMARY : Styling::StyleVariable::BASE_BRIGHT_INVERSE;
        if (config::get().is_debug())
            var = Styling::StyleVariable::SECONDARY_RAINBOW_BLUE;
        else if (Ui::GetAppConfig().hasCustomDeviceId())
            var = Styling::StyleVariable::SECONDARY_RAINBOW_WARM;

        return getColor(var);
    }

    bool ThemeParameters::isBorderNeeded() const noexcept
    {
        return wallpaper_ && wallpaper_->isBorderNeeded();
    }

    QString ThemeParameters::getBackgroundCommonQss() const
    {
        return getBackgroundQss().replace(ql1s("%BACKGROUND%"), getColorHex(StyleVariable::BASE_GLOBALWHITE));
    }

    QString ThemeParameters::getLineEditCommonQss(bool _isError, int _height) const
    {
        QString qss = getLineEditQss() % getLineEditFocusQss();

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
        qss.replace(ql1s("%HEIGHT%"), QString::number(_height));

        return qss;
    }

    QString ThemeParameters::getTextLineEditCommonQss(bool _isError, int _height) const
    {
        QString qss = getTextLineEditQss() % getTextLineEditFocusQss();

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
        qss.replace(ql1s("%HEIGHT%"), QString::number(_height));

        return qss;
    }

    QString ThemeParameters::getLineEditDisabledQss(int _height) const
    {
        auto qss = getLineEditQss();
        qss.replace(ql1s("%BACKGROUND%"), ql1s("transparent"));
        qss.replace(ql1s("%BORDER_COLOR%"), getColorHex(StyleVariable::BASE_PRIMARY));
        qss.replace(ql1s("%HEIGHT%"), QString::number(_height));

        return qss;
    }

    QString ThemeParameters::getLineEditCustomQss(const QColor & _lineFocusColor, const QColor& _lineNoFocusColor, int _height) const
    {
        QString qss = getLineEditQss() % getLineEditFocusQss();

        if (_lineNoFocusColor.isValid())
            qss.replace(ql1s("%BORDER_COLOR%"), _lineNoFocusColor.name());
        else
            qss.replace(ql1s("%BORDER_COLOR%"), ql1s("transparent"));

        if (_lineFocusColor.isValid())
            qss.replace(ql1s("%BORDER_COLOR_FOCUS%"), _lineFocusColor.name());
        else
            qss.replace(ql1s("%BORDER_COLOR_FOCUS%"), ql1s("transparent"));

        qss.replace(ql1s("%BACKGROUND%"), ql1s("transparent"));
        qss.replace(ql1s("%BACKGROUND_FOCUS%"), ql1s("transparent"));
        qss.replace(ql1s("%HEIGHT%"), QString::number(_height));

        return qss;
    }

    QString ThemeParameters::getTextEditCommonQss(bool _hasBorder) const
    {
        QString qss = getTextEditQss();

        qss.replace(ql1s("%BORDER_COLOR%"), getColorHex(_hasBorder ? StyleVariable::BASE_BRIGHT : StyleVariable::BASE_GLOBALWHITE));
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

    QString ThemeParameters::getContextMenuQss(int _itemHeight, int _paddingLeft, int _paddingRight, int _paddingIcon) const
    {
        QString qss = getContextMenuCommon(_itemHeight, _paddingLeft, _paddingRight, _paddingIcon);

        qss.replace(ql1s("%BACKGROUND%"), getColorHex(Styling::StyleVariable::BASE_GLOBALWHITE));
        qss.replace(ql1s("%BACKGROUND_SELECTED%"), getColorHex(Styling::StyleVariable::BASE_BRIGHT_INVERSE));
        qss.replace(ql1s("%BORDER_COLOR%"), getColorHex(Styling::StyleVariable::BASE_BRIGHT_INVERSE));
        qss.replace(ql1s("%SEPARATOR%"), getColorHex(Styling::StyleVariable::BASE_BRIGHT));
        qss.replace(ql1s("%FONT_COLOR%"), getColorHex(Styling::StyleVariable::TEXT_SOLID));
        qss.replace(ql1s("%FONT_DISABLED_COLOR%"), getColorHex(Styling::StyleVariable::BASE_TERTIARY));

        return qss;
    }

    QString ThemeParameters::getContextMenuDarkQss(int _itemHeight, int _paddingLeft, int _paddingRight, int _paddingIcon) const
    {
        QString qss = getContextMenuCommon(_itemHeight, _paddingLeft, _paddingRight, _paddingIcon);

        qss.replace(ql1s("%BACKGROUND%"), getColorHex(Styling::StyleVariable::BASE_GLOBALWHITE_PERMANENT));
        qss.replace(ql1s("%BACKGROUND_SELECTED%"), getColorHex(Styling::StyleVariable::BASE_GLOBALWHITE_PERMANENT_HOVER));
        qss.replace(ql1s("%BORDER_COLOR%"), getColorHex(Styling::StyleVariable::BASE_GLOBALWHITE_PERMANENT_HOVER));
        qss.replace(ql1s("%SEPARATOR%"), getColorHex(Styling::StyleVariable::LUCENT_TERTIARY_ACTIVE));
        qss.replace(ql1s("%FONT_COLOR%"), getColorHex(Styling::StyleVariable::TEXT_SOLID_PERMANENT));
        qss.replace(ql1s("%FONT_DISABLED_COLOR%"), getColorHex(Styling::StyleVariable::TEXT_SOLID_PERMANENT_ACTIVE));

        return qss;
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

        return Fonts::SetFont(qss);
    }

    QString ThemeParameters::getSmilesQss() const
    {
        QString qss = ql1s(
            "Ui--Smiles--AddButton{"
            "padding : 0;"
            "margin : 0;}"
            "Ui--Smiles--TabButton{"
            "padding : 0;"
            "margin : 0;"
            "border: none;"
            "background-color: %1;"
            "background-repeat: no-repeat;"
            "background-position: center center;}"
            "Ui--Smiles--TabButton::hover{"
            "background-color: %2;"
            "background-repeat: no-repeat;"
            "background-position: center center;}"
            "Ui--Smiles--TabButton::checked{"
            "background-color: %3;"
            "border-bottom-color: %4;"
            "border-bottom-width: 2dip;"
            "border-bottom-style: solid;"
            "background-repeat: no-repeat;"
            "background-position: center center;}"
            "Ui--Smiles--TabButton[underline=\"false\"]{"
            "background: %1;}"
            "Ui--Smiles--TabButton::hover[underline=\"false\"]{"
            "border-bottom: none;"
            "background-color: %2;}"
            "Ui--Smiles--TabButton::checked[underline=\"false\"]{"
            "border-bottom: none;"
            "background-color: %2;}"
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
            "border-top-color: %3;"
            "border-top-width: 1dip;"
            "border-top-style: solid;"
            "background-color: %1;}"
        )
            .arg(getColorHex(StyleVariable::BASE_GLOBALWHITE), getColorHex(StyleVariable::BASE_BRIGHT_INVERSE),
                getColorHex(StyleVariable::BASE_BRIGHT), getColorHex(StyleVariable::PRIMARY));
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
            "border-color: %1;"
            "border-style: none;"
            "padding-left: 15dip;"
            "padding-right: 15dip;"
            "padding-top: 12dip;"
            "padding-bottom: 10dip;}"
            "QPushButton#successImage{"
            "outline: none;"
            "border-image: url(:/placeholders/good_100);}"
        ).arg(getColorHex(StyleVariable::BASE_SECONDARY));

        return qss;
    }

    QString ThemeParameters::getComboBoxQss() const
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
            "height: 8dip; }")
            .arg(getColorHex(Styling::StyleVariable::BASE_GLOBALWHITE),
                 getColorHex(Styling::StyleVariable::BASE_BRIGHT),
                 getColorHex(Styling::StyleVariable::BASE_BRIGHT_INVERSE),
                 getColorHex(Styling::StyleVariable::TEXT_SOLID),
                 getColorHex(Styling::StyleVariable::BASE_PRIMARY)
            );
        return menuQss;
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

    QString ThemeParameters::getLoginPageQss() const
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
        ).arg(
            getColorHex(StyleVariable::BASE_GLOBALWHITE),
            getColorHex(StyleVariable::TEXT_SOLID)
        );
        return qss;
    }

    QString ThemeParameters::getFlatMenuQss(const StyleVariable _borderColor) const
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
            ).arg(
                getColorHex(Styling::StyleVariable::BASE_GLOBALWHITE),
                getColorHex(_borderColor),
                getColorHex(Styling::StyleVariable::BASE_BRIGHT_INVERSE),
                getColorHex(Styling::StyleVariable::TEXT_SOLID)
            );

        return qss;
    }

    ThemeParameters getParameters(const QString& _aimId)
    {
        ThemeParameters params(getThemesContainer().getCurrentTheme(), _aimId.isEmpty() ? nullptr : getThemesContainer().getContactWallpaper(_aimId));
        return params;
    }

    namespace Scrollbar
    {
        QColor getScrollbarBackgroundColor()
        {
            return Styling::getParameters().getColor(scrollbarBg);
        }

        QColor getScrollbarButtonColor()
        {
            return Styling::getParameters().getColor(scrollbarBtn);
        }

        QColor getScrollbarButtonHoveredColor()
        {
            return Styling::getParameters().getColor(scrollbarBtnHover);
        }
    }

    namespace Buttons
    {
        void setButtonDefaultColors(Ui::CustomButton* _button)
        {
            assert(_button);
            if (_button)
            {
                _button->setDefaultColor(defaultColor());
                _button->setHoverColor(hoverColor());
                _button->setPressedColor(pressedColor());
                _button->setActiveColor(activeColor());
            }
        }

        QColor defaultColor()
        {
            return Styling::getParameters().getColor(Styling::StyleVariable::BASE_SECONDARY);
        }

        QColor hoverColor()
        {
            return Styling::getParameters().getColor(Styling::StyleVariable::BASE_SECONDARY_HOVER);
        }

        QColor pressedColor()
        {
            return Styling::getParameters().getColor(Styling::StyleVariable::BASE_SECONDARY_ACTIVE);
        }

        QColor activeColor()
        {
            return Styling::getParameters().getColor(Styling::StyleVariable::PRIMARY);
        }
    }
}

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

    QString getBackgroundQss()
    {
        return qsl("background-color: %BACKGROUND%");
    }
}
