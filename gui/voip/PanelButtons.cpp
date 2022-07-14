#include "stdafx.h"

#include "PanelButtons.h"
#include "../controls/ClickWidget.h"
#include "../controls/TooltipWidget.h"
#include "../utils/utils.h"
#include "../styles/ThemeParameters.h"
#include "../previewer/GalleryFrame.h"
#include "../styles/ThemesContainer.h"
#include "fonts.h"

namespace
{
    constexpr std::chrono::milliseconds kTooltipDelay(300);

    QSize menuButtonSize() noexcept
    {
        return Utils::scale_value(QSize{ 20, 20 });
    }

    QPoint menuButtonOffset() noexcept
    {
        return QPoint(Utils::scale_value(2), Utils::scale_value(4));
    }

    int menuButtonBorder(Ui::PanelToolButton::IconStyle _iconStyle) noexcept
    {
        return _iconStyle == Ui::PanelToolButton::Circle ? Utils::scale_value(2) : 0;
    }

    auto menuIconSize() noexcept
    {
        return Utils::scale_value(14);
    }

    auto badgeOffset() noexcept
    {
        return QPoint(-Utils::scale_value(16), -Utils::scale_value(8));
    }

    auto badgeBorder() noexcept
    {
        return Utils::scale_value(2);
    }

    auto badgeHeight() noexcept
    {
        return Utils::scale_value(20);
    }

    auto buttonMinimumSize(Ui::PanelToolButton::IconStyle _iconStyle, Qt::ToolButtonStyle _buttonStyle)
    {
        if (_iconStyle != Ui::PanelToolButton::Circle)
            return QSize();

        switch (_buttonStyle)
        {
        case Qt::ToolButtonTextUnderIcon:
            return Utils::scale_value(QSize(70, 96));
        case Qt::ToolButtonIconOnly:
            return Utils::scale_value(QSize(44, 44));
        case Qt::ToolButtonTextOnly:
        default:
            break;
        }
        return Utils::scale_value(QSize(70, 44));
    }

    auto buttonMaximumSize(Ui::PanelToolButton::IconStyle _iconStyle, Qt::ToolButtonStyle _buttonStyle)
    {
        if (_iconStyle == Ui::PanelToolButton::Circle)
        {
            switch (_buttonStyle)
            {
            case Qt::ToolButtonTextUnderIcon:
                return Utils::scale_value(QSize(70, 96));
            case Qt::ToolButtonIconOnly:
                return Utils::scale_value(QSize(44, 44));
            case Qt::ToolButtonTextOnly:
            default:
                break;
            }
            return Utils::scale_value(QSize(70, 44));
        }
        else
        {
            switch (_buttonStyle)
            {
            case Qt::ToolButtonTextUnderIcon:
                return Utils::scale_value(QSize(70, 96));
            case Qt::ToolButtonIconOnly:
                return Utils::scale_value(QSize(44, 44));
            case Qt::ToolButtonTextOnly:
            default:
                break;
            }
            return Utils::scale_value(QSize(QWIDGETSIZE_MAX, 44));
        }
    }

    auto buttonRectMenuMargins(Qt::ToolButtonStyle _buttonStyle, Ui::PanelToolButton::IconStyle _iconStyle)
    {
        if (_iconStyle == Ui::PanelToolButton::Circle)
        {
            switch (_buttonStyle)
            {
            case Qt::ToolButtonTextBesideIcon:
            case Qt::ToolButtonTextUnderIcon:
                return Utils::scale_value(QMargins(0, -8, 0, 0));
            default:
                break;
            }
            return QMargins{};
        }
        return Utils::scale_value(QMargins(4, 4, 4, 4));
    }

    auto buttonMargins(Qt::ToolButtonStyle _buttonStyle)
    {
        switch (_buttonStyle)
        {
        case Qt::ToolButtonTextUnderIcon:
            return Utils::scale_value(QMargins(4, 12, 4, 0));
        case Qt::ToolButtonTextOnly:
        case Qt::ToolButtonTextBesideIcon:
            return Utils::scale_value(QMargins(8, 4, 8, 4));
        case Qt::ToolButtonIconOnly:
        default:
            return Utils::scale_value(QMargins(4, 4, 4, 4));
        }
    }

    auto buttonRadius() { return Utils::scale_value(6); }

    auto getButtonIconSize() noexcept
    {
        return Utils::scale_value(QSize(32, 32));
    }

    auto getMoreIconSize() noexcept
    {
        return Utils::scale_value(14);
    }

    auto getButtonTextFont()
    {
        return Fonts::appFontScaled(11, platform::is_apple() ? Fonts::FontWeight::Medium : Fonts::FontWeight::SemiBold);
    }

    auto getShadowColor()
    {
        return QColor(0, 0, 0, 255 * 0.35);
    }

    const QPixmap& getMoreIcon(const bool _hovered, const bool _pressed)
    {
        auto makeBtn = [](const auto _var) { return Utils::renderSvg(qsl(":/voip/more_micro"), QSize(getMoreIconSize(), getMoreIconSize()), Styling::getParameters().getColor(_var)); };

        static const QPixmap normalPx = makeBtn(Styling::StyleVariable::TEXT_SOLID_PERMANENT);
        static const QPixmap pressedPx = makeBtn(Styling::StyleVariable::TEXT_SOLID_PERMANENT_ACTIVE);
        static const QPixmap hoveredPx = makeBtn(Styling::StyleVariable::TEXT_SOLID_PERMANENT_HOVER);

        return _pressed ? pressedPx : (_hovered ? hoveredPx : normalPx);
    }

    constexpr std::chrono::milliseconds animDuration() noexcept { return std::chrono::milliseconds(150); }

    QFont getBadgeTextForNumbersFont()
    {
        if constexpr (platform::is_apple())
            return Fonts::appFontScaled(13, Fonts::FontFamily::SF_PRO_TEXT, Fonts::FontWeight::Medium);
        else
            return Fonts::appFontScaled(14, Fonts::FontFamily::SOURCE_SANS_PRO, Fonts::FontWeight::SemiBold);
    }

    auto badgeTextColor()
    {
        return Styling::ThemeColorKey{ Styling::StyleVariable::TEXT_SOLID_PERMANENT };
    }
}

namespace Ui
{
    class PanelToolButtonPrivate
    {
    public:
        PanelToolButton* q;
        QKeySequence shortcut_;
        QPalette palette_;
        QString iconName_;
        QPixmap icon_;
        Qt::Alignment menuAlign_;
        Qt::ToolButtonStyle buttonStyle_;
        PanelToolButton::PopupMode popupMode_;
        PanelToolButton::ButtonRole buttonRole_;
        PanelToolButton::IconStyle iconStyle_;
        QStyle::SubControls hoverControl_;
        QSize badgeSize_;
        QSize sizeHint_;
        int spacing_;

        QPointer<QMenu> menu_;
        std::unique_ptr<TextRendering::TextUnit> textUnit_;
        std::unique_ptr<TextRendering::TextUnit> badgeTextUnit_;
        int count_;

        bool autoRaise_;
        bool tooltipsEnabled_;
        bool tooltipVisible_;

        struct PanelToolButtonOption : QStyleOptionComplex
        {
            QRect badgeRect, iconRect, textRect, menuButtonRect;
        };

        PanelToolButtonPrivate(PanelToolButton* _q)
            : q(_q)
            , menuAlign_(Qt::AlignTop | Qt::AlignHCenter)
            , buttonStyle_(Qt::ToolButtonTextUnderIcon)
            , popupMode_(PanelToolButton::MenuButtonPopup)
            , buttonRole_(PanelToolButton::Regular)
            , iconStyle_(PanelToolButton::Circle)
            , hoverControl_(QStyle::SC_None)
            , spacing_(Utils::scale_value(8))
            , count_(0)
            , autoRaise_(false)
            , tooltipsEnabled_(true)
            , tooltipVisible_(false)
        {}

        void setupPalette()
        {
            using ColorRole = Styling::StyleVariable;
            const auto params = Styling::getParameters();
            palette_.setBrush(QPalette::Shadow, getShadowColor());
            palette_.setBrush(QPalette::ButtonText, params.getColor(ColorRole::GHOST_LIGHT)); // text color
            palette_.setBrush(QPalette::Window, params.getColor(ColorRole::GHOST_PERCEPTIBLE)); // background color
            switch (buttonRole_)
            {
            case PanelToolButton::Attention:
                palette_.setBrush(QPalette::Disabled, QPalette::Base, params.getColor(ColorRole::TEXT_SOLID_PERMANENT)); // normal & checked
                palette_.setBrush(QPalette::Disabled, QPalette::Button, params.getColor(ColorRole::SECONDARY_ATTENTION)); // normal & checked
                palette_.setBrush(QPalette::Disabled, QPalette::Light, params.getColor(ColorRole::SECONDARY_ATTENTION_HOVER)); // hovered & checked
                palette_.setBrush(QPalette::Disabled, QPalette::Highlight, params.getColor(ColorRole::SECONDARY_ATTENTION_ACTIVE)); // pressed & checked

                palette_.setBrush(QPalette::Normal, QPalette::Base, params.getColor(ColorRole::TEXT_SOLID_PERMANENT)); // normal & unchecked
                palette_.setBrush(QPalette::Normal, QPalette::Button, params.getColor(ColorRole::LUCENT_TERTIARY)); // normal & unchecked
                palette_.setBrush(QPalette::Normal, QPalette::Light, params.getColor(ColorRole::LUCENT_TERTIARY_HOVER)); // hovered & unchecked
                palette_.setBrush(QPalette::Normal, QPalette::Highlight, params.getColor(ColorRole::LUCENT_TERTIARY_ACTIVE)); // pressed &checked
                break;
            case PanelToolButton::Regular: // PanelToolButton::Regular is the default
            default:
                palette_.setBrush(QPalette::Normal, QPalette::Base, params.getColor(ColorRole::BASE_GLOBALBLACK_PERMANENT)); // icon background
                palette_.setBrush(QPalette::Normal, QPalette::Button, params.getColor(ColorRole::TEXT_SOLID_PERMANENT)); // normal
                palette_.setBrush(QPalette::Normal, QPalette::Light, params.getColor(ColorRole::TEXT_SOLID_PERMANENT_HOVER)); // hovered
                palette_.setBrush(QPalette::Normal, QPalette::Highlight, params.getColor(ColorRole::TEXT_SOLID_PERMANENT_ACTIVE)); // pressed

                palette_.setBrush(QPalette::Disabled, QPalette::Base, params.getColor(ColorRole::TEXT_SOLID_PERMANENT)); // normal & unchecked
                palette_.setBrush(QPalette::Disabled, QPalette::Button, params.getColor(ColorRole::LUCENT_TERTIARY)); // normal & unchecked
                palette_.setBrush(QPalette::Disabled, QPalette::Light, params.getColor(ColorRole::LUCENT_TERTIARY_HOVER)); // hovered & unchecked
                palette_.setBrush(QPalette::Disabled, QPalette::Highlight, params.getColor(ColorRole::LUCENT_TERTIARY_ACTIVE)); // pressed &checked
                break;
            }
        }

        void updateTheme()
        {
            setupPalette();
            updateIcon();
            q->update();
        }

        void initStyleOption(PanelToolButtonOption& _option)
        {
            _option.initFrom(q);

            _option.iconRect = iconRect();
            _option.textRect = textRect(_option.iconRect);
            _option.badgeRect = badgeRect(_option.iconRect);
            _option.menuButtonRect = menuButtonRect(_option.iconRect);
            _option.palette = palette_;

            if (q->isDown())
                _option.state |= QStyle::State_Sunken;
            if (q->isChecked())
                _option.state |= QStyle::State_On;
            if (!q->isChecked() && !q->isDown())
                _option.state |= QStyle::State_Raised;

            _option.activeSubControls = hoverControl_;
        }

        QRect iconRect() const
        {
            const QMargins margins = q->contentsMargins();
            switch (buttonStyle_)
            {
            case Qt::ToolButtonIconOnly:
            case Qt::ToolButtonTextUnderIcon:
                return QRect(QPoint(), q->iconSize()).translated(q->width() / 2 - q->iconSize().width() / 2, margins.top());
            case Qt::ToolButtonTextBesideIcon:
            default:
                return QRect(QPoint(), q->iconSize()).translated(margins.left(), margins.top());
            }
            return QRect();
        }

        QRect textRect(const QRect& _iconRect) const
        {
            QRect r;
            if (!textUnit_ || buttonStyle_ == Qt::ToolButtonIconOnly)
                return r;

            r.setSize(textUnit_->cachedSize());
            switch (buttonStyle_)
            {
            case Qt::ToolButtonTextOnly:
                r.moveLeft(q->contentsMargins().left());
                r.moveTop(q->contentsMargins().top());
                break;
            case Qt::ToolButtonTextBesideIcon:
                r.moveLeft(_iconRect.right() + spacing_);
                if constexpr (platform::is_apple())
                    r.moveTop(q->height() / 2 - textUnit_->cachedSize().height() / 2 + 2);
                else
                    r.moveTop(q->height() / 2 - textUnit_->cachedSize().height() / 2);
                break;
            case Qt::ToolButtonTextUnderIcon:
                r.moveLeft(_iconRect.center().x() - r.width() / 2);
                r.moveTop(_iconRect.bottom() + spacing_ + 1);
                break;
            default:
                return QRect();
            }
            return r;
        }

        QRect badgeRect(const QRect& _iconRect) const
        {
            if (count_ < 1 || !badgeTextUnit_)
                return QRect();

            QRect r(0, 0, badgeSize_.width(), badgeSize_.height());
            r.moveTo(_iconRect.topRight() + badgeOffset());
            return r;
        }

        QRect menuButtonRect(const QRect& _iconRect) const
        {
            if (popupMode_ == PanelToolButton::InstantPopup || !menu_)
                return QRect{};

            QRect r(QPoint{}, menuButtonSize());
            const QRect txtRc = textRect(_iconRect);
            if (iconStyle_ == PanelToolButton::Circle)
            {
                switch (buttonStyle_)
                {
                case Qt::ToolButtonTextOnly:
                    r.moveTopLeft({ txtRc.right() + spacing_ + r.width() / 2, txtRc.center().y() - r.height() / 2 });
                    break;
                case Qt::ToolButtonTextBesideIcon:
                case Qt::ToolButtonTextUnderIcon:
                case Qt::ToolButtonIconOnly:
                default:
                    r.moveCenter(_iconRect.bottomRight() - menuButtonOffset());
                    break;
                }
            }
            else
            {
                switch (buttonStyle_)
                {
                case Qt::ToolButtonTextBesideIcon:
                case Qt::ToolButtonTextOnly:
                    r.moveTopLeft({ txtRc.right() + spacing_, txtRc.center().y() - r.height() / 2 });
                    break;
                case Qt::ToolButtonTextUnderIcon:
                case Qt::ToolButtonIconOnly:
                default:
                    r.moveCenter(_iconRect.bottomRight() - menuButtonOffset());
                    break;
                }
            }
            return r;
        }

        QSize textSize() const { return textUnit_ ? textUnit_->cachedSize() : QSize{}; }
        int spacing(const QSize& _textSz) const { return (_textSz.height() > 0 ? spacing_ : 0); }

        void updateIcon()
        {
            if (!iconName_.isEmpty())
            {
                const QPalette::ColorGroup group = q->isChecked() ? QPalette::Normal : QPalette::Disabled;
                icon_ = Utils::renderSvg(iconName_, QSize(), palette_.color(group, QPalette::Base));
            }
        }

        QColor buttonColor(const QPalette& _palette, const QStyle::State _state) const
        {
            const bool enabled = _state & QStyle::State_Enabled;
            const bool pressed = _state & QStyle::State_Sunken;
            const bool hovered = _state & QStyle::State_MouseOver;
            const bool checked = _state & QStyle::State_On;

            QPalette::ColorGroup group = enabled && checked ? QPalette::Normal : QPalette::Disabled;
            if (enabled && pressed)
                return _palette.color(group, QPalette::Highlight);
            else if (enabled && hovered)
                return _palette.color(group, QPalette::Light);
            else
                return _palette.color(group, QPalette::Button);
        }

        QPixmap buttonMask(const QSize& _size, PanelToolButtonOption& _option) const
        {
            if (_size.isEmpty())
                return QPixmap();

            QStyle::State state = _option.state;
            if (!(_option.activeSubControls & QStyle::SC_ToolButton))
                state &= ~(QStyle::State_MouseOver | QStyle::State_Sunken);

            const qreal dpr = q->devicePixelRatioF();
            QPixmap pixmap((QSizeF(_size) * dpr).toSize());
            pixmap.setDevicePixelRatio(dpr);
            pixmap.fill(Qt::transparent);

            QPainter painter(&pixmap);
            painter.setRenderHint(QPainter::Antialiasing);
            drawItemBackground(painter, _option.iconRect, buttonColor(_option.palette, state));

            painter.setCompositionMode(QPainter::CompositionMode_Source);
            if (!_option.badgeRect.isNull())
                drawItemBackground(painter, _option.badgeRect, Qt::transparent);
            if (!_option.menuButtonRect.isNull())
                drawItemBackground(painter, _option.menuButtonRect, Qt::transparent);

            return pixmap;
        }

        void drawItemBackground(QPainter& _painter, const QRect& _rect, const QColor& _color) const
        {
            _painter.setPen(Qt::NoPen);
            _painter.setBrush(_color);
            switch (iconStyle_)
            {
            case PanelToolButton::Circle:
                _painter.drawRoundedRect(_rect, Utils::minSide(_rect) / 2, Utils::minSide(_rect) / 2);
                break;
            case PanelToolButton::Rounded:
                _painter.drawRoundedRect(_rect, buttonRadius(), buttonRadius()); // TODO: make it by design!
                break;
            case PanelToolButton::Rectangle:
            default:
                _painter.drawRect(_rect);
                break;
            }
        }

        void drawBackground(QPainter& _painter, const QStyleOptionComplex& _option, QStyle::SubControl _control)
        {
            if (_option.rect.isNull())
                return;

            QStyle::State state = _option.state;
            if (!(_control & _option.activeSubControls))
                state &= ~(QStyle::State_MouseOver | QStyle::State_Sunken);

            if (_control == QStyle::SC_ToolButtonMenu)
                state &= ~QStyle::State_On;

            QColor color = buttonColor(_option.palette, state);
            if (_control == QStyle::SC_CustomBase) // special color for bage
            {
                color = _option.palette.color(QPalette::Disabled, QPalette::Base);
                color.setAlphaF(0.25);
            }

            Utils::PainterSaver guard(_painter);
            drawItemBackground(_painter, _option.rect, color);
        }

        void drawMenuButton(QPainter& _painter, const PanelToolButtonOption& _option)
        {
            if (_option.menuButtonRect.isNull())
                return;

            const bool hovered = _option.activeSubControls & QStyle::SC_ToolButtonMenu;
            const bool pressed = hovered && (_option.state & QStyle::State_Sunken);

            drawIcon(_painter, getMoreIcon(hovered, pressed), _option.menuButtonRect);
        }

        void drawBadge(QPainter& _painter, const QStyleOption& _option)
        {
            const int r = Utils::minSide(_option.rect) / 2;
            _painter.setPen(Qt::NoPen);
            _painter.drawRoundedRect(_option.rect, r, r);

            const auto badgeTextColor = Styling::getParameters().getColor(Styling::StyleVariable::TEXT_SOLID_PERMANENT);
            Utils::drawUnreads(_painter, getBadgeTextForNumbersFont(), Qt::transparent, badgeTextColor, count_, badgeHeight(), _option.rect.x(), _option.rect.y());
        }

        void drawText(QPainter& _painter, const PanelToolButtonOption& _option)
        {
            if (!textUnit_ || _option.textRect.isNull())
                return;

            Utils::PainterSaver guard(_painter);
            _painter.translate(_option.textRect.topLeft());
            textUnit_->draw(_painter, TextRendering::VerPosition::TOP);
        }

        void drawIcon(QPainter& _painter, const QPixmap& _icon, const QRect& _rect)
        {
            // draw icon
            QRect iconRc = _icon.rect();
            iconRc.setSize(_icon.size() / Utils::scale_bitmap_ratio());

            iconRc.moveCenter(_rect.center());
            _painter.drawPixmap(iconRc, _icon);
        }

        void updateContentsMargins()
        {
            q->setContentsMargins(buttonMargins(buttonStyle_));
        }

        void updateTextSize()
        {
            if (!textUnit_)
                return;

            const int maxWidth = buttonMaximumSize(iconStyle_, buttonStyle_).width() -
                                 (q->contentsMargins().left() + q->contentsMargins().right());
            if (textUnit_->cachedSize().width() > maxWidth)
            {
                textUnit_->getHeight(maxWidth);
                textUnit_->elide(maxWidth);
            }
        }

        void updateHoverControl(const QPoint& _pos)
        {
            const QStyle::SubControls lastControl = hoverControl_;
            hoverControl_ = QStyle::SC_None;
            if (q->rect().contains(_pos))
            {
                hoverControl_ |= QStyle::SC_ToolButton;
                if (menu_ && popupMode_ != PanelToolButton::InstantPopup && menuButtonRect(iconRect()).contains(_pos))
                    hoverControl_ |=  QStyle::SC_ToolButtonMenu;
            }

            if (hoverControl_ != lastControl)
                q->update();
        }

        QString formatToolTipText(const QString& _tooltipText, const QString& _buttonText, const QString& _shortcutText)
        {
            QString btnText = _buttonText;
            QString result;
            if (!_tooltipText.isEmpty())
                result += _tooltipText;
            if (!_buttonText.isEmpty() && _tooltipText.isEmpty())
                result += btnText.replace(ql1c('\n'), ql1c(' '));
            if (!_shortcutText.isEmpty() && buttonStyle_ != Qt::ToolButtonIconOnly)
                result += ql1s(" (") % _shortcutText % ql1c(')');
            return result;
        }

        void hideToolTip()
        {
            Tooltip::hide();
            tooltipVisible_ = false;
        }

        void showToolTip()
        {
            if (!tooltipsEnabled_)
                return;

            const QString toolTipText = formatToolTipText(q->toolTip(), q->text(), shortcut_.toString(QKeySequence::NativeText));
            const QRect area(q->mapToGlobal({}), q->size());
            if (!toolTipText.isEmpty())
            {
                Tooltip::show(toolTipText, area, {}, Tooltip::ArrowDirection::Down, Tooltip::ArrowPointPos::Top, Ui::TextRendering::HorAligment::LEFT, {}, Tooltip::TooltipMode::Multiline, q->parentWidget());
                tooltipVisible_ = true;
            }
        }
    };

    PanelToolButton::PanelToolButton(QWidget* _parent)
        : PanelToolButton(QString(), _parent)
    { }

    PanelToolButton::PanelToolButton(const QString& _text, QWidget* _parent)
        : QAbstractButton(_parent)
        , d(std::make_unique<PanelToolButtonPrivate>(this))
    {
        d->setupPalette();
        d->updateContentsMargins();
        setSizePolicy(QSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed, QSizePolicy::ToolButton));
        setText(_text);
        setAttribute(Qt::WA_Hover);
        setMouseTracking(true);
        setCheckable(false);
        setChecked(false);
        connect(this, &PanelToolButton::toggled, this, [this]() { d->updateIcon(); update(); });
        connect(&Styling::getThemesContainer(), &Styling::ThemesContainer::globalThemeChanged, this, [this]() { d->updateTheme(); });
    }

    PanelToolButton::~PanelToolButton() = default;

    void PanelToolButton::setKeySequence(const QKeySequence& _shortcut)
    {
        d->shortcut_ = _shortcut;
    }

    QKeySequence PanelToolButton::keySequence() const
    {
        return d->shortcut_;
    }

    void PanelToolButton::setTooltipsEnabled(bool _on)
    {
        d->tooltipsEnabled_ = _on;
    }

    bool PanelToolButton::isTooltipsEnabled() const
    {
        return d->tooltipsEnabled_;
    }

    void PanelToolButton::setText(const QString& _text)
    {
        const QString t = text();
        if (_text == text())
            return;

        const bool requireToolTip = d->tooltipVisible_;
        if (requireToolTip)
            d->hideToolTip();

        QAbstractButton::setText(_text);
        if (requireToolTip)
            d->showToolTip();

        if (_text.isEmpty())
        {
            d->textUnit_.reset();
            return;
        }

        if (!d->textUnit_)
        {
            const auto textColor_ = Styling::ThemeColorKey{ Styling::StyleVariable::GHOST_LIGHT };
            d->textUnit_ = TextRendering::MakeTextUnit(_text, {}, TextRendering::LinksVisible::DONT_SHOW_LINKS);
            TextRendering::TextUnit::InitializeParameters params{ getButtonTextFont(), textColor_ };
            params.align_ = TextRendering::HorAligment::CENTER;
            params.maxLinesCount_ = 2;
            d->textUnit_->init(params);
        }
        else
        {
            d->textUnit_->setText(_text);
        }
        d->textUnit_->evaluateDesiredSize();
        const int w = d->textUnit_->desiredWidth();
        d->textUnit_->getHeight(w);
        d->updateTextSize();

        d->textUnit_->setShadow(0, Utils::scale_value(1), d->palette_.color(QPalette::Shadow));
        d->sizeHint_ = QSize();
        update();

    }

    void PanelToolButton::setIcon(const QString& _iconName)
    {
        if (d->iconName_ == _iconName)
            return;

        d->iconName_ = _iconName;
        d->updateIcon();
        update();
    }

    void PanelToolButton::setBadgeCount(int _count)
    {
        if (d->count_ == _count)
            return;

        d->count_ = _count;
        d->badgeSize_ = d->count_ > 0 ? Utils::getUnreadsBadgeSize(d->count_, badgeHeight() + 2 * badgeBorder()) : QSize();

        const QString badgeText = Utils::getUnreadsBadgeStr(d->count_);

        if (d->badgeTextUnit_)
        {
            d->badgeTextUnit_->setText(badgeText);
        }
        else
        {
            d->badgeTextUnit_ = TextRendering::MakeTextUnit(badgeText);
            TextRendering::TextUnit::InitializeParameters params{ getBadgeTextForNumbersFont(), badgeTextColor() };
            params.align_ = TextRendering::HorAligment::CENTER;
            d->badgeTextUnit_->init(params);
        }
        d->sizeHint_ = QSize();
        update();
    }

    int PanelToolButton::badgeCount() const
    {
        return d->count_;
    }

    void PanelToolButton::setPopupMode(PanelToolButton::PopupMode _mode)
    {
        d->popupMode_ = _mode;
        d->sizeHint_ = QSize();
        update();
    }

    PanelToolButton::PopupMode PanelToolButton::popupMode() const
    {
        return d->popupMode_;
    }

    void PanelToolButton::setButtonRole(PanelToolButton::ButtonRole _role)
    {
        if (d->buttonRole_ == _role)
            return;

        d->buttonRole_ = _role;
        d->setupPalette();
        d->updateIcon();
        update();
    }

    PanelToolButton::ButtonRole PanelToolButton::role() const
    {
        return d->buttonRole_;
    }

    void PanelToolButton::setIconStyle(PanelToolButton::IconStyle _style)
    {
        if (d->iconStyle_ == _style)
            return;

        d->iconStyle_ = _style;
        d->sizeHint_ = QSize();
        d->updateTextSize();
        d->updateIcon();
        update();
    }

    PanelToolButton::IconStyle PanelToolButton::iconStyle() const
    {
        return d->iconStyle_;
    }

    void PanelToolButton::setButtonStyle(Qt::ToolButtonStyle _style)
    {
        if (d->buttonStyle_ == _style)
            return;

        d->buttonStyle_ = _style;
        d->sizeHint_ = QSize();
        d->updateTextSize();
        d->updateContentsMargins();
        update();
    }

    Qt::ToolButtonStyle PanelToolButton::buttonStyle() const
    {
        return d->buttonStyle_;
    }

    void PanelToolButton::setAutoRaise(bool _on)
    {
        if (d->autoRaise_ == _on)
            return;

        d->autoRaise_ = _on;
        update();
    }

    bool PanelToolButton::isAutoRaise() const
    {
        return d->autoRaise_;
    }

    void PanelToolButton::setMenuAlignment(Qt::Alignment _align)
    {
        d->menuAlign_ = _align;
    }

    Qt::Alignment PanelToolButton::menuAlignment() const
    {
        return d->menuAlign_;
    }

    void PanelToolButton::setMenu(QMenu* _menu)
    {
        if (d->menu_ == _menu)
            return;

        d->menu_ = _menu;
        d->sizeHint_ = QSize();
        update();
    }

    QMenu* PanelToolButton::menu() const
    {
        return d->menu_;
    }

    QSize PanelToolButton::sizeHint() const
    {
        if (d->sizeHint_.isValid())
            return d->sizeHint_;

        const QRect iconRect(QPoint(), iconSize());
        const QSize textSize = d->textSize();
        int w = 0, h = 0;

        if (d->buttonStyle_ == Qt::ToolButtonIconOnly)
        {
            QRect r = iconRect;
            if (d->popupMode_ == MenuButtonPopup && d->menu_)
                r |= d->menuButtonRect(iconRect);
            w = r.width();
            h = r.height();
        }
        else if (d->buttonStyle_ == Qt::ToolButtonTextOnly)
        {
            w = textSize.width() + d->spacing(textSize);
            h = textSize.height();
            if (d->popupMode_ == MenuButtonPopup && d->menu_)
            {
                w += menuButtonSize().width();
                h = std::max(h, menuButtonSize().height());
            }
        }
        else if (d->buttonStyle_ == Qt::ToolButtonTextUnderIcon)
        {
            QRect r = iconRect;
            if (d->popupMode_ == MenuButtonPopup && d->menu_)
                r |= d->menuButtonRect(iconRect);

            w = std::max(textSize.width(), r.width());
            h = r.height() + d->spacing(textSize) + textSize.height();
        }
        else if (d->buttonStyle_ == Qt::ToolButtonTextBesideIcon)
        {
            w = iconRect.width() + d->spacing(textSize) + textSize.width();
            if (d->popupMode_ == MenuButtonPopup && d->menu_)
                w += (d->spacing_ + d->menuButtonRect(iconRect).width());

            h = std::max(iconRect.height(), textSize.height());
        }

        w += contentsMargins().left() + contentsMargins().right();
        h += contentsMargins().top() + contentsMargins().bottom();

        d->sizeHint_ = QSize(w, h);

        if (d->iconStyle_ == Circle)
        {
            d->sizeHint_ = d->sizeHint_.expandedTo(buttonMinimumSize(d->iconStyle_, d->buttonStyle_));
            d->sizeHint_ = d->sizeHint_.boundedTo(buttonMaximumSize(d->iconStyle_, d->buttonStyle_));
        }
        return d->sizeHint_;
    }

    QSize PanelToolButton::minimumSizeHint() const
    {
        return sizeHint();
    }

    void PanelToolButton::showMenu(QMenu* _menu)
    {
        if (!_menu)
            return;

        if (platform::is_apple() && d->tooltipsEnabled_)
        {
            connect(_menu, &QMenu::aboutToHide, this, [this]() { d->tooltipsEnabled_ = true; });
            d->tooltipsEnabled_ = false;
        }

        d->hideToolTip();
        aboutToShowMenu(QPrivateSignal{});

        const QRect rc = rect();
        if (auto customMenu = qobject_cast<Previewer::CustomMenu*>(_menu))
            customMenu->exec(this, d->menuAlign_, buttonRectMenuMargins(d->buttonStyle_, d->iconStyle_));
        else
            _menu->exec(mapToGlobal(rc.bottomLeft()));
    }

    bool PanelToolButton::event(QEvent* _event)
    {
        if (_event->type() == QEvent::HoverLeave)
            d->hideToolTip();

        if constexpr (platform::is_apple())
        {
            // MacOS often lost some events like tooltip,
            // so here we force tooltip to be shown
            if (_event->type() == QEvent::HoverMove && d->tooltipsEnabled_)
            {
                QTimer::singleShot(kTooltipDelay, this, [this]()
                {
                    if (isVisible() && !d->tooltipVisible_ && rect().contains(mapFromGlobal(QCursor::pos())))
                        d->showToolTip();
                });
            }
        }

        switch (_event->type())
        {
        case QEvent::HoverLeave:
        case QEvent::HoverEnter:
        case QEvent::HoverMove:
            d->updateHoverControl(static_cast<const QHoverEvent*>(_event)->pos());
            break;
        case QEvent::ToolTip:
            d->showToolTip();
            return true;
        case QEvent::ToolTipChange:
            d->hideToolTip();
            d->showToolTip();
            break;
        case QEvent::Hide:
            d->hideToolTip();
            break;
        default:
            break;
        }
        return QAbstractButton::event(_event);
    }

    void PanelToolButton::keyPressEvent(QKeyEvent* _event)
    {
        if (_event->key() == Qt::Key_Space && _event->modifiers() == Qt::NoModifier)
        {
            QWidget::keyPressEvent(_event);
            return;
        }
        QAbstractButton::keyPressEvent(_event);
    }

    void PanelToolButton::keyReleaseEvent(QKeyEvent* _event)
    {
        if (_event->key() == Qt::Key_Space && _event->modifiers() == Qt::NoModifier)
        {
            QWidget::keyPressEvent(_event);
            return;
        }
        QAbstractButton::keyPressEvent(_event);
    }

    void PanelToolButton::mousePressEvent(QMouseEvent* _event)
    {
        if ((_event->button() == Qt::LeftButton) &&
            (d->popupMode_ == InstantPopup ||
             d->menuButtonRect(d->iconRect()).contains(_event->pos())))
        {
            repaint();
            showMenu(d->menu_);
            return;
        }
        QAbstractButton::mousePressEvent(_event);
    }

    void PanelToolButton::paintEvent(QPaintEvent* _event)
    {
        PanelToolButtonPrivate::PanelToolButtonOption option;
        d->initStyleOption(option);

        QPainter painter(this);
        painter.setRenderHints(QPainter::Antialiasing | QPainter::SmoothPixmapTransform);

        if (!(option.state & QStyle::State_Enabled))
            painter.setOpacity(0.5);

        switch (d->iconStyle_)
        {
        case Circle:
            painter.drawPixmap(0, 0, d->buttonMask(size(), option));
            break;
        case Rounded:
            painter.setPen(Qt::NoPen);
            painter.setBrush(option.palette.window());
            painter.drawRoundedRect(option.rect, buttonRadius(), buttonRadius());
            break;
        case Rectangle:
        default:
            painter.fillRect(option.rect, option.palette.window());
            break;
        }

        if (d->iconStyle_ != Circle)
        {
            if (!d->autoRaise_)
                d->drawBackground(painter, option, QStyle::SC_ToolButton);
            else if (option.state & QStyle::State_MouseOver)
                d->drawBackground(painter, option, QStyle::SC_ToolButton);
        }
        if (d->menu_ && d->popupMode_ == MenuButtonPopup)
        {
            const int border = menuButtonBorder(d->iconStyle_);
            option.rect =  option.menuButtonRect.adjusted(border, border, -border, -border);
            d->drawBackground(painter, option, QStyle::SC_ToolButtonMenu);
            d->drawMenuButton(painter, option);
        }

        if (d->count_ > 0)
        {
            const int border = badgeBorder();
            option.rect = option.badgeRect.adjusted(border, border, -border, -border);
            d->drawBackground(painter, option, QStyle::SC_CustomBase);
            d->drawBadge(painter, option);
        }

        option.rect = rect();
        d->drawIcon(painter, d->icon_, option.iconRect);
        d->drawText(painter, option);
    }


    TransparentPanelButton::TransparentPanelButton(QWidget* _parent, const QString& _iconName, const QString& _tooltipText, Qt::Alignment _align, bool _isAnimated)
        : ClickableWidget(_parent)
        , align_(_align)
        , anim_(nullptr)
        , currentAngle_(0)
        , dir_(RotateDirection::Left)
        , rotateMode_(RotateAnimationMode::Full)
    {
        if (_isAnimated)
        {
            anim_ = new QVariantAnimation(this);
            anim_->setDuration(animDuration().count());
            anim_->setEasingCurve(QEasingCurve::InOutSine);
            connect(anim_, &QVariantAnimation::valueChanged, this, [this](const QVariant& value)
            {
                currentAngle_ = value.toDouble();
                update();
            });
        }

        auto makeIcon = [&_iconName](bool _hovered, bool _pressed)
        {
            const auto color = _hovered ? Styling::StyleVariable::TEXT_SOLID_PERMANENT_HOVER
                : (_pressed ? Styling::StyleVariable::TEXT_SOLID_PERMANENT_ACTIVE
                    : Styling::StyleVariable::TEXT_SOLID_PERMANENT);
            return Utils::StyledPixmap{ _iconName, getButtonIconSize(), Styling::ThemeColorKey{ color } };
        };

        iconNormal_ = makeIcon(false, false);
        iconHovered_ = makeIcon(true, false);
        iconPressed_ = makeIcon(false, true);

        connect(this, &ClickableWidget::clicked, this, &TransparentPanelButton::onClicked);

        setFixedSize(Utils::scale_value(QSize(36, 36)));
        setTooltipText(_tooltipText);
    }

    void TransparentPanelButton::paintEvent(QPaintEvent* _event)
    {
        ClickableWidget::paintEvent(_event);

        QPainter p(this);
        p.setRenderHints(QPainter::Antialiasing | QPainter::SmoothPixmapTransform);

        const auto buttonSize = getButtonIconSize().width();
        const auto offsets = getIconOffset();
        const auto x = offsets.x() + buttonSize / 2;
        const auto y = offsets.y() + buttonSize / 2;
        p.translate(x, y);
        p.rotate(currentAngle_);
        p.translate(-x, -y);
        const auto pm = (isHovered() ? iconHovered_ : (isPressed() ? iconPressed_ : iconNormal_)).actualPixmap();
        p.drawPixmap(offsets, pm);
    }

    void TransparentPanelButton::mouseMoveEvent(QMouseEvent* _e)
    {
        ClickableWidget::mouseMoveEvent(_e);
        if (!Tooltip::isVisible() && !tooltipTimer_.isActive() && getTooltipArea().contains(_e->pos()))
            tooltipTimer_.start();
    }

    void TransparentPanelButton::setTooltipBoundingRect(const QRect& _r)
    {
        tooltipBoundingRect_ = _r;
    }

    void TransparentPanelButton::changeState()
    {
        rotateMode_ = RotateAnimationMode::ShowFinalState;
        onClicked();
        update();
        rotateMode_ = RotateAnimationMode::Full;
    }

    void TransparentPanelButton::showToolTip()
    {
        const auto& tooltipText = getTooltipText();
        if (tooltipText.isEmpty())
            return;

        const auto area = getTooltipArea();
        Tooltip::show(tooltipText, { mapToGlobal(area.topLeft()), area.size() }, { 0, 0 }, Tooltip::ArrowDirection::Down, Tooltip::ArrowPointPos::Top, TextRendering::HorAligment::LEFT, tooltipBoundingRect_, Tooltip::TooltipMode::Multiline, parentWidget());
    }

    void TransparentPanelButton::onClicked()
    {
        rotate(dir_);
        dir_ = (dir_ == RotateDirection::Right) ? RotateDirection::Left : RotateDirection::Right;
    }

    void TransparentPanelButton::rotate(const RotateDirection _dir)
    {
        if (anim_)
        {
            const auto endAngle = 180.0 * (_dir == RotateDirection::Left ? -1 : 0);
            if (rotateMode_ == RotateAnimationMode::Full)
            {
                anim_->stop();
                anim_->setStartValue(currentAngle_);
                anim_->setEndValue(endAngle);
                anim_->start();
            }
            else
            {
                currentAngle_ = endAngle;
            }
        }
    }

    QPoint TransparentPanelButton::getIconOffset() const
    {
        const auto buttonSize = getButtonIconSize().width();
        const auto dX = (align_ & Qt::AlignLeft) ? 0 : width() - buttonSize;
        const auto dY = (height() - buttonSize) / 2;

        return QPoint(dX, dY);
    }

    QRect TransparentPanelButton::getTooltipArea() const
    {
        const auto topLeftPos = rect().topLeft() + QPoint(getIconOffset().x(), 0);
        return { topLeftPos, QSize(getButtonIconSize().width(), height()) };
    }

    QString microphoneIcon(bool _checked)
    {
        return _checked ? ql1s(":/voip/microphone_icon") : ql1s(":/voip/microphone_off_icon");
    }

    QString speakerIcon(bool _checked)
    {
        return _checked ? ql1s(":/voip/speaker_icon") : ql1s(":/voip/speaker_off_icon");
    }

    QString videoIcon(bool _checked)
    {
        return _checked ? ql1s(":/voip/videocall_icon") : ql1s(":/voip/videocall_off_icon");
    }

    QString screensharingIcon()
    {
        return ql1s(":/voip/sharescreen_icon");
    }

    QString hangupIcon()
    {
        return ql1s(":/voip/endcall_icon");
    }

    QString microphoneButtonText(bool _checked)
    {
        return _checked ? QT_TRANSLATE_NOOP("voip_video_panel", "Mute") : QT_TRANSLATE_NOOP("voip_video_panel", "Unmute");
    }

    QString videoButtonText(bool _checked)
    {
        return _checked ? QT_TRANSLATE_NOOP("voip_video_panel", "Stop\nVideo") : QT_TRANSLATE_NOOP("voip_video_panel", "Start\nVideo");
    }

    QString speakerButtonText(bool _checked)
    {
        return _checked ? QT_TRANSLATE_NOOP("voip_video_panel", "Disable\nSound") : QT_TRANSLATE_NOOP("voip_video_panel", "Enable\nSound");
    }

    QString screensharingButtonText(bool _checked)
    {
        return _checked ? QT_TRANSLATE_NOOP("voip_video_panel", "Stop\nShare") : QT_TRANSLATE_NOOP("voip_video_panel", "Share\nScreen");
    }

    QString stopCallButtonText()
    {
        return QT_TRANSLATE_NOOP("voip_video_panel", "End call");
    }
}
