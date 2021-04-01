#include "stdafx.h"

#include "PanelButtons.h"
#include "../controls/ClickWidget.h"
#include "../controls/TooltipWidget.h"
#include "../utils/utils.h"
#include "../styles/ThemeParameters.h"
#include "fonts.h"

namespace
{
    auto getButtonSize(Ui::PanelButton::ButtonSize _size) noexcept
    {

        return _size == Ui::PanelButton::ButtonSize::Big
                        ? Utils::scale_value(QSize(70, 96))
                        : Utils::scale_value(QSize(44, 44));
    }

    auto getButtonCircleSize(Ui::PanelButton::ButtonSize _size) noexcept
    {
        return _size == Ui::PanelButton::ButtonSize::Big
                        ? Utils::scale_value(40)
                        : Utils::scale_value(36);
    }

    auto getButtonIconSize() noexcept
    {
        return Utils::scale_value(QSize(32, 32));
    }

    auto getMoreButtonSize() noexcept
    {
        return Utils::scale_value(20);
    }

    auto getGridButtonSize() noexcept
    {
        return Utils::scale_value(QSize(32, 32));
    }

    auto getMoreButtonBorder() noexcept
    {
        return Utils::scale_value(2);
    }

    auto getMoreButtonMarginLeft() noexcept
    {
        return Utils::scale_value(28);
    }

    auto getMoreButtonMarginTop() noexcept
    {
        return Utils::scale_value(26);
    }

    auto getMoreIconSize() noexcept
    {
        return Utils::scale_value(14);
    }

    auto getSpacing() noexcept
    {
        return Utils::scale_value(4);
    }

    auto getMargin() noexcept
    {
        return Utils::scale_value(12);
    }

    auto getButtonTextFont()
    {
        return Fonts::appFontScaled(11, platform::is_apple() ? Fonts::FontWeight::Medium : Fonts::FontWeight::SemiBold);
    }

    auto getButtonCircleVerOffset(Ui::PanelButton::ButtonSize _size) noexcept
    {
        return _size == Ui::PanelButton::ButtonSize::Big
                        ? Utils::scale_value(12)
                        : Utils::scale_value(4);
    }

    auto getButtonTextVerOffset() noexcept
    {
        return Utils::scale_value(60);
    }

    auto getShadowColor()
    {
        return QColor(0, 0, 0, 255 * 0.35);
    }

    const QPixmap& getGridButtonIcon(Ui::GridButtonState _state, bool _hovered, const bool _pressed)
    {
        auto makeBtn = [](const auto _path, const auto _var)
        {
            return Utils::renderSvg(_path, getGridButtonSize(), Styling::getParameters().getColor(_var));
        };

        auto makeBtnAll = [&makeBtn](const auto _var)
        {
            return makeBtn(qsl(":/voip/conference_one_icon"), _var);
        };

        auto makeBtnOne = [&makeBtn](const auto _var)
        {
            return makeBtn(qsl(":/voip/conference_all_icon"), _var);
        };

        static const QPixmap normalAllPx = makeBtnAll(Styling::StyleVariable::TEXT_SOLID_PERMANENT);
        static const QPixmap pressedAllPx = makeBtnAll(Styling::StyleVariable::TEXT_SOLID_PERMANENT_ACTIVE);
        static const QPixmap hoveredAllPx = makeBtnAll(Styling::StyleVariable::TEXT_SOLID_PERMANENT_HOVER);

        static const QPixmap normalOnePx = makeBtnOne(Styling::StyleVariable::TEXT_SOLID_PERMANENT);
        static const QPixmap pressedOnePx = makeBtnOne(Styling::StyleVariable::TEXT_SOLID_PERMANENT_ACTIVE);
        static const QPixmap hoveredOnePx = makeBtnOne(Styling::StyleVariable::TEXT_SOLID_PERMANENT_HOVER);

        if (_state == Ui::GridButtonState::ShowAll)
            return _pressed ? pressedAllPx : (_hovered ? hoveredAllPx : normalAllPx);
        return _pressed ? pressedOnePx : (_hovered ? hoveredOnePx : normalOnePx);
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

    auto badgeNotchHeight() noexcept
    {
        return Utils::scale_value(24);
    }
    auto badgeHeight() noexcept
    {
        return Utils::scale_value(20);
    }
    auto badgeNotchPadding() noexcept
    {
        return Utils::scale_value(2);
    }

    auto badgeMarginLeft() noexcept
    {
        return Utils::scale_value(26);
    }
    auto badgeMarginTop() noexcept
    {
        return getButtonCircleSize(Ui::PanelButton::ButtonSize::Big) - badgeMarginLeft() - badgeHeight();
    }
    auto badgeNotchMarginLeft() noexcept
    {
        return badgeMarginLeft() - badgeNotchPadding();
    }
    auto badgeNotchMarginTop() noexcept
    {
        return badgeMarginTop() - badgeNotchPadding();
    }

    QFont getBadgeTextForNumbersFont()
    {
        if constexpr (platform::is_apple())
            return Fonts::appFontScaled(13, Fonts::FontFamily::SF_PRO_TEXT, Fonts::FontWeight::Medium);
        else
            return Fonts::appFontScaled(14, Fonts::FontFamily::SOURCE_SANS_PRO, Fonts::FontWeight::SemiBold);
    }

    QColor badgeTextColor()
    {
        return Styling::getParameters().getColor(Styling::StyleVariable::TEXT_SOLID_PERMANENT);
    }
}

namespace Ui
{
    MoreButton::MoreButton(QWidget* _parent)
        : ClickableWidget(_parent)
    {
        setFixedSize(getMoreButtonSize(), getMoreButtonSize());
        setCursor(Qt::PointingHandCursor);
    }

    void MoreButton::paintEvent(QPaintEvent* _event)
    {
        const auto color = Styling::getParameters().getColor(isPressed() ? Styling::StyleVariable::LUCENT_TERTIARY_ACTIVE
            : (isHovered() ? Styling::StyleVariable::LUCENT_TERTIARY_HOVER
                : Styling::StyleVariable::LUCENT_TERTIARY));

        QPainter p(this);
        p.setRenderHints(QPainter::Antialiasing | QPainter::SmoothPixmapTransform);
        p.fillRect(rect(), Qt::transparent);
        p.setPen(Qt::NoPen);
        p.setBrush(color);
        const auto border = getMoreButtonBorder();
        p.drawEllipse(rect().adjusted(border, border, -border, -border));

        const auto delta = (getMoreButtonSize() - getMoreIconSize()) / 2;
        p.drawPixmap(delta, delta, getMoreIcon(isHovered(), isPressed()));
    }

    PanelButton::PanelButton(QWidget* _parent, const QString& _text, const QString& _iconName, ButtonSize _size, ButtonStyle _style, bool _hasMore)
        : ClickableWidget(_parent)
        , iconName_(_iconName)
        , textColor_(Styling::getParameters().getColor(Styling::StyleVariable::GHOST_LIGHT))
        , size_(_size)
        , textUnit_(nullptr)
        , more_(nullptr)
        , badgeTextUnit_(nullptr)
        , count_(0)
    {
        const auto size = getButtonSize(size_);

        setFixedSize(size);

        if (!_text.isEmpty())
        {
            textUnit_ = TextRendering::MakeTextUnit(_text, {}, TextRendering::LinksVisible::DONT_SHOW_LINKS);
            textUnit_->init(getButtonTextFont(), textColor_, QColor(), QColor(), QColor(), TextRendering::HorAligment::CENTER, 1);
            textUnit_->evaluateDesiredSize();

            if (textUnit_->cachedSize().width() > size.width())
            {
                textUnit_->getHeight(size.width());
                textUnit_->elide(size.width());
            }

            textUnit_->setOffsets((size.width() - textUnit_->cachedSize().width()) / 2, getButtonTextVerOffset());
            textUnit_->setShadow(0, Utils::scale_value(1), getShadowColor());
        }

        connect(this, &PanelButton::hoverChanged, this, qOverload<>(&PanelButton::update));
        connect(this, &PanelButton::pressChanged, this, qOverload<>(&PanelButton::update));

        if (_hasMore)
        {
            auto layout = Utils::emptyVLayout(this);
            more_ = new MoreButton(this);
            layout->addWidget(more_);
            connect(more_, &ClickableWidget::clicked, this, &PanelButton::moreButtonClicked);
        }

        updateStyle(_style);
    }

    void PanelButton::updateStyle(ButtonStyle _style, const QString& _iconName, const QString& _text)
    {
        QColor iconColor;
        const auto params = Styling::getParameters();

        if (!_text.isEmpty())
        {
            if (!textUnit_)
            {
                textUnit_ = TextRendering::MakeTextUnit(_text, {}, TextRendering::LinksVisible::DONT_SHOW_LINKS);
                textUnit_->init(getButtonTextFont(), textColor_, QColor(), QColor(), QColor(), TextRendering::HorAligment::CENTER, 1);
            }

            textUnit_->setText(_text);
            textUnit_->evaluateDesiredSize();
            textUnit_->setOffsets((getButtonSize(size_).width() - textUnit_->cachedSize().width()) / 2, getButtonTextVerOffset());
            textUnit_->setShadow(0, Utils::scale_value(1), getShadowColor());
        }

        switch (_style)
        {
        case ButtonStyle::Normal:
            iconColor = params.getColor(Styling::StyleVariable::BASE_GLOBALBLACK_PERMANENT);
            circleNormal_ = params.getColor(Styling::StyleVariable::TEXT_SOLID_PERMANENT);
            circleHovered_ = params.getColor(Styling::StyleVariable::TEXT_SOLID_PERMANENT_HOVER);
            circlePressed_ = params.getColor(Styling::StyleVariable::TEXT_SOLID_PERMANENT_ACTIVE);
            break;

        case ButtonStyle::Transparent:
            iconColor = params.getColor(Styling::StyleVariable::TEXT_SOLID_PERMANENT);
            circleNormal_ = params.getColor(Styling::StyleVariable::LUCENT_TERTIARY);
            circleHovered_ = params.getColor(Styling::StyleVariable::LUCENT_TERTIARY_HOVER);
            circlePressed_ = params.getColor(Styling::StyleVariable::LUCENT_TERTIARY_ACTIVE);
            break;

        case ButtonStyle::Red:
            iconColor = params.getColor(Styling::StyleVariable::TEXT_SOLID_PERMANENT);
            circleNormal_ = params.getColor(Styling::StyleVariable::SECONDARY_ATTENTION);
            circleHovered_ = params.getColor(Styling::StyleVariable::SECONDARY_ATTENTION_HOVER);
            circlePressed_ = params.getColor(Styling::StyleVariable::SECONDARY_ATTENTION_ACTIVE);
            break;

        default:
            Q_UNREACHABLE();
            return;
        }

        if (!_iconName.isEmpty())
            iconName_ = _iconName;

        if (!iconName_.isEmpty())
            icon_ = Utils::renderSvg(iconName_, getButtonIconSize(), iconColor);

        update();
    }

    void PanelButton::setCount(int _count)
    {
        if (count_ != _count && _count >= 0)
        {
            count_ = _count;
            QString badgeText = _count > 0 ? QString::number(_count) : QString();
            if (badgeTextUnit_)
            {
                badgeTextUnit_->setText(badgeText);
            }
            else
            {
                badgeTextUnit_ = TextRendering::MakeTextUnit(badgeText);
                badgeTextUnit_->init(getBadgeTextForNumbersFont(), badgeTextColor(), QColor(), QColor(), QColor(), TextRendering::HorAligment::CENTER);
            }
            update();
        }
    }

    void PanelButton::paintEvent(QPaintEvent* _event)
    {
        QColor circleColor;
        QColor textColor = textColor_;

        if (isEnabled() && isPressed())
            circleColor = circlePressed_;
        else if (isEnabled() && isHovered())
            circleColor = circleHovered_;
        else
            circleColor = circleNormal_;

        QPainter p(this);
        p.setRenderHints(QPainter::Antialiasing | QPainter::SmoothPixmapTransform);
        p.setPen(Qt::NoPen);
        p.fillRect(rect(), Qt::transparent);

        if (!isEnabled())
        {
            circleColor.setAlphaF(circleColor.alphaF() * 0.5f);
            textColor.setAlphaF(textColor.alphaF() * 0.5f);
            p.setOpacity(0.5f);
        }

        const auto circleSize = getButtonCircleSize(size_);
        const auto circlePlace = QRect((width() - circleSize) / 2, getButtonCircleVerOffset(size_), circleSize, circleSize);
        const bool isDoubleDigitBadge = count_ > 9;
        if (circleColor.isValid())
        {
            QPixmap circle(Utils::scale_bitmap(QSize(circleSize, circleSize)));
            circle.fill(Qt::transparent);
            {
                QPainter _p(&circle);
                _p.setRenderHints(QPainter::Antialiasing | QPainter::SmoothPixmapTransform);
                _p.setPen(Qt::NoPen);
                _p.setBrush(circleColor);
                _p.drawEllipse(circle.rect());
            }
            Utils::check_pixel_ratio(circle);

            QPixmap cut = circle;
            if (more_ || count_ > 0)
            {
                QPainter _p(&cut);
                _p.setRenderHints(QPainter::Antialiasing | QPainter::SmoothPixmapTransform);
                _p.setPen(Qt::NoPen);
                _p.setBrush(Qt::transparent);
                _p.setCompositionMode(QPainter::CompositionMode_Source);

                if (more_)
                {
                    const auto r = getMoreButtonSize();
                    QRect smallCircle(getMoreButtonMarginLeft(), getMoreButtonMarginTop(), r, r);
                    _p.drawEllipse(smallCircle);
                }
                if (count_ > 0)
                {
                    const auto r = badgeNotchHeight();
                    if (isDoubleDigitBadge)
                    {
                        QRect smallCircle(badgeNotchMarginLeft(), badgeNotchMarginTop(), 2 * r, r);
                        _p.drawRoundedRect(smallCircle, r / 2, r / 2);
                    }
                    else
                    {
                        QRect smallCircle(badgeNotchMarginLeft(), badgeNotchMarginTop(), r, r);
                        _p.drawEllipse(smallCircle);
                    }
                }
            }
            p.drawPixmap(circlePlace, cut);
        }

        const auto ratio = Utils::scale_bitmap_ratio();
        p.drawPixmap(circlePlace.x() + (circlePlace.width() - icon_.width() / ratio) / 2,
                     circlePlace.y() + (circlePlace.height() - icon_.height() / ratio) / 2,
                     icon_);

        if (textUnit_)
        {
            textUnit_->setColor(textColor);
            textUnit_->draw(p, TextRendering::VerPosition::TOP);
        }

        if (more_)
            more_->move(circlePlace.left() + getMoreButtonMarginLeft(), circlePlace.top() + getMoreButtonMarginTop());

        if (count_ > 0)
        {
            const auto color = Styling::getParameters().getColor(Styling::StyleVariable::TEXT_SOLID_PERMANENT, 0.25);
            const auto badgeTextColor = Styling::getParameters().getColor(Styling::StyleVariable::TEXT_SOLID_PERMANENT);
            Utils::drawUnreads(p, getBadgeTextForNumbersFont(), color, badgeTextColor, count_, badgeHeight(), circlePlace.x() + badgeMarginLeft(), circlePlace.y() + badgeMarginTop());
        }
    }

    TransparentPanelButton::TransparentPanelButton(QWidget* _parent, const QString& _iconName, const QString& _tooltipText, Qt::Alignment _align, bool _isAnimated)
        : ClickableWidget(_parent)
        , tooltipTimer_(new QTimer(this))
        , tooltipText_(_tooltipText)
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
            return Utils::renderSvg(_iconName, getButtonIconSize(), Styling::getParameters().getColor(color));
        };

        iconNormal_ = makeIcon(false, false);
        iconHovered_ = makeIcon(true, false);
        iconPressed_ = makeIcon(false, true);

        connect(this, &ClickableWidget::clicked, this, &TransparentPanelButton::onClicked);

        setFixedSize(getButtonSize(PanelButton::ButtonSize::Small));
        tooltipTimer_->setSingleShot(true);
        tooltipTimer_->setInterval(Tooltip::getDefaultShowDelay());
        connect(tooltipTimer_, &QTimer::timeout, this, &TransparentPanelButton::onTooltipTimer);
    }

    void TransparentPanelButton::paintEvent(QPaintEvent* _event)
    {
        ClickableWidget::paintEvent(_event);

        QPainter p(this);
        p.setRenderHints(QPainter::Antialiasing | QPainter::SmoothPixmapTransform);

        const auto buttonSize = getButtonIconSize().width();
        const auto offsets = getIconOffset();
        p.translate(offsets.x() + buttonSize / 2, offsets.y() + buttonSize / 2);
        p.rotate(currentAngle_);
        p.translate(-(offsets.x() + buttonSize / 2), -(offsets.y() + buttonSize / 2));
        const auto pm = isHovered() ? iconHovered_ : (isPressed() ? iconPressed_ : iconNormal_);
        p.drawPixmap(offsets, pm);
    }

    void TransparentPanelButton::enterEvent(QEvent* _e)
    {
        ClickableWidget::enterEvent(_e);
        tooltipTimer_->start();
    }

    void TransparentPanelButton::leaveEvent(QEvent* _e)
    {
        ClickableWidget::leaveEvent(_e);
        tooltipTimer_->stop();
        hideTooltip();
    }

    void TransparentPanelButton::setTooltipText(const QString& _text)
    {
        tooltipText_ = _text;
    }

    const QString& TransparentPanelButton::getTooltipText() const
    {
        return tooltipText_;
    }

    void TransparentPanelButton::hideTooltip()
    {
        Tooltip::forceShow(false);
        Tooltip::hide();
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

    void TransparentPanelButton::onTooltipTimer()
    {
        if (isHovered())
            showToolTip();
    }

    void TransparentPanelButton::showToolTip()
    {
        if (tooltipText_.isEmpty())
            return;

        const auto offsets = getIconOffset();
        auto pos = mapToGlobal(rect().topLeft());
        pos.rx() += offsets.x();
        pos.ry() += offsets.y();
        Tooltip::show(tooltipText_, QRect(pos, getButtonIconSize()), { 0, 0 }, Tooltip::ArrowDirection::Down, Tooltip::ArrowPointPos::Top, tooltipBoundingRect_);
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


    GridButton::GridButton(QWidget *_parent)
        : ClickableWidget(_parent)
        , state_(GridButtonState::ShowBig)
    {
        text_ = TextRendering::MakeTextUnit(QT_TRANSLATE_NOOP("voip_video_panel", "SHOW ALL"), {}, TextRendering::LinksVisible::DONT_SHOW_LINKS);
        text_->init(getButtonTextFont(), Styling::getParameters().getColor(Styling::StyleVariable::TEXT_SOLID_PERMANENT), QColor(), QColor(), QColor(), TextRendering::HorAligment::CENTER, 1);
        text_->evaluateDesiredSize();

        const auto textSize = text_->cachedSize();
        setFixedHeight(getGridButtonSize().height());
        setFixedWidth(getMargin() + getGridButtonSize().width() + textSize.width() + getSpacing());

        auto yOffset = (height() - textSize.height()) / 2;
        if constexpr (platform::is_apple())
            yOffset += Utils::scale_value(1);
        text_->setOffsets(getMargin(), yOffset);
    }

    void GridButton::paintEvent(QPaintEvent *_event)
    {
        QPainter p(this);
        p.setRenderHints(QPainter::Antialiasing | QPainter::TextAntialiasing | QPainter::SmoothPixmapTransform);
        p.fillRect(rect(), Qt::transparent);
        const auto icon = getGridButtonIcon(state_, isHovered(), isPressed());

        auto xOffset = 0;
        if (state_ == GridButtonState::ShowAll)
        {
            const auto color = (isPressed() ? Styling::StyleVariable::TEXT_SOLID_PERMANENT_ACTIVE
                              : isHovered() ? Styling::StyleVariable::TEXT_SOLID_PERMANENT_HOVER
                              : Styling::StyleVariable::TEXT_SOLID_PERMANENT);

            text_->setColor(color);
            text_->draw(p);
            xOffset = getMargin() + text_->cachedSize().width() + getSpacing();
        }
        p.drawPixmap(xOffset, 0, icon);
    }

    void GridButton::setState(const GridButtonState _state)
    {
        state_ = _state;
        if (state_ == GridButtonState::ShowAll)
            setFixedWidth(getMargin() + getGridButtonSize().width() + text_->cachedSize().width() + getSpacing());
        else
            setFixedWidth(getGridButtonSize().width());

        update();
    }

    QString getMicrophoneButtonText(PanelButton::ButtonStyle _style, PanelButton::ButtonSize _size)
    {
        if (_size == PanelButton::ButtonSize::Big)
        {
            if (_style == PanelButton::ButtonStyle::Normal)
                return QT_TRANSLATE_NOOP("voip_video_panel", "Mute");
            return QT_TRANSLATE_NOOP("voip_video_panel", "Unmute");
        }
        else
        {
            if (_style == PanelButton::ButtonStyle::Normal)
                return QT_TRANSLATE_NOOP("voip_video_panel_mini", "Mute");
            return QT_TRANSLATE_NOOP("voip_video_panel_mini", "Unmute");
        }
    }

    QString getVideoButtonText(PanelButton::ButtonStyle _style, PanelButton::ButtonSize _size)
    {
        if (_size == PanelButton::ButtonSize::Big)
        {
            if (_style == PanelButton::ButtonStyle::Normal)
                return QT_TRANSLATE_NOOP("voip_video_panel", "Stop\nVideo");
            return QT_TRANSLATE_NOOP("voip_video_panel", "Start\nVideo");
        }
        else
        {
            if (_style == PanelButton::ButtonStyle::Normal)
                return QT_TRANSLATE_NOOP("voip_video_panel_mini", "Stop video");
            return QT_TRANSLATE_NOOP("voip_video_panel_mini", "Start video");
        }
    }

    QString getSpeakerButtonText(PanelButton::ButtonStyle _style, PanelButton::ButtonSize _size)
    {
        if (_size == PanelButton::ButtonSize::Big)
        {
            if (_style == PanelButton::ButtonStyle::Normal)
                return QT_TRANSLATE_NOOP("voip_video_panel", "Disable\nSound");
            return QT_TRANSLATE_NOOP("voip_video_panel", "Enable\nSound");
        }
        else
        {
            if (_style == PanelButton::ButtonStyle::Normal)
                return QT_TRANSLATE_NOOP("voip_video_panel_mini", "Disable sound");
            return QT_TRANSLATE_NOOP("voip_video_panel_mini", "Enable sound");
        }
    }

    QString getScreensharingButtonText(PanelButton::ButtonStyle _style, PanelButton::ButtonSize _size)
    {
        if (_size == PanelButton::ButtonSize::Big)
        {
            if (_style == PanelButton::ButtonStyle::Normal)
                return QT_TRANSLATE_NOOP("voip_video_panel", "Stop\nShare");
            return QT_TRANSLATE_NOOP("voip_video_panel", "Share\nScreen");
        }
        else
        {
            if (_style == PanelButton::ButtonStyle::Normal)
                return QT_TRANSLATE_NOOP("voip_video_panel_mini", "Stop share");
            return QT_TRANSLATE_NOOP("voip_video_panel_mini", "Start share");
        }
    }
}
