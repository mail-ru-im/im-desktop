#include "stdafx.h"

#include "SubmitButton.h"
#include "CircleHover.h"
#include "utils/utils.h"
#include "styles/ThemeParameters.h"
#include "controls/TooltipWidget.h"

#include "media/permissions/MediaCapturePermissions.h"

namespace
{
    constexpr std::chrono::milliseconds getAnimDuration() noexcept { return std::chrono::milliseconds(100); }
    constexpr std::chrono::milliseconds longTapTimeout() noexcept { return std::chrono::milliseconds(250); }

    int normalIconSize()
    {
        return Utils::scale_value(32);
    }

    int minIconSize()
    {
        return Utils::scale_value(20);
    }

    QPixmap makeEditIcon(const Styling::StyleVariable _bgColor)
    {
        const QSize iconSize(normalIconSize(), normalIconSize());
        QPixmap res(Utils::scale_bitmap(iconSize));
        res.fill(Qt::transparent);
        Utils::check_pixel_ratio(res);

        {
            QPainter p(&res);
            p.setRenderHint(QPainter::Antialiasing);

            p.setPen(Qt::NoPen);
            p.setBrush(Styling::getParameters().getColor(Styling::StyleVariable::TEXT_SOLID_PERMANENT));

            const auto r = Utils::scale_bitmap_ratio();
            const auto whiteWidth = res.width() / std::sqrt(2.5) / r;
            p.drawEllipse((res.width() / r - whiteWidth) / 2, (res.height() / r - whiteWidth) / 2, whiteWidth, whiteWidth);

            p.drawPixmap(0, 0, Utils::renderSvg(qsl(":/apply_edit"), iconSize, Styling::getParameters().getColor(_bgColor)));
        }
        return res;
    }

    QPixmap makeSendIcon(const Styling::StyleVariable _bgColor, const Ui::InputStyleMode _mode)
    {
        const QSize iconSize(normalIconSize(), normalIconSize());

        if (_mode == Ui::InputStyleMode::CustomBg)
        {
            QPixmap res(Utils::scale_bitmap(iconSize));
            res.fill(Qt::transparent);
            Utils::check_pixel_ratio(res);

            {
                QPainter p(&res);
                p.setRenderHint(QPainter::Antialiasing);

                p.setPen(Qt::NoPen);
                p.setBrush(Styling::getParameters().getColor(_bgColor));

                p.drawEllipse(Utils::unscale_bitmap(res.rect()));

                const auto pm = Utils::renderSvgScaled(qsl(":/send"), QSize(20, 20), Styling::getParameters().getColor(Styling::StyleVariable::BASE_GLOBALWHITE));
                p.drawPixmap(Utils::scale_value(6), (res.height() - pm.height()) / 2 / Utils::scale_bitmap_ratio(), pm);
            }
            return res;
        }

        return Utils::renderSvg(qsl(":/send"), iconSize, Styling::getParameters().getColor(_bgColor));
    }

    QPixmap makePttIcon(const QColor& _color)
    {
        const QSize iconSize(normalIconSize(), normalIconSize());
        return Utils::renderSvg(qsl(":/send_ptt"), iconSize, _color);
    }
}

namespace Ui
{
    SubmitButton::SubmitButton(QWidget* _parent)
        : ClickableWidget(_parent)
        , anim_(new QVariantAnimation(this))
    {
        setFocusPolicy(Qt::TabFocus);
        setFocusColor(focusColorPrimary());

        setState(State::Send, StateTransition::Force);

        connect(this, &ClickableWidget::hoverChanged, this, [this]()
        {
            onHoverChanged();
            onMouseStateChanged();
        });
        connect(this, &ClickableWidget::pressChanged, this, &SubmitButton::onMouseStateChanged);

        longTapTimer_.setSingleShot(true);
        longTapTimer_.setInterval(longTapTimeout());

        connect(&longTapTimer_, &QTimer::timeout, this, [this]() {
            if (isHovered())
                Q_EMIT longTapped(QPrivateSignal());
        });

        longPressTimer_.setSingleShot(true);
        longPressTimer_.setInterval(longTapTimeout());
        connect(&longPressTimer_, &QTimer::timeout, this, [this]()
        {
            if (hasFocus())
            {
                setIsUnderLongPress(true);
                Q_EMIT longPressed(QPrivateSignal());
            }
        });

        anim_->setStartValue(0.0);
        anim_->setEndValue(1.0);
        anim_->setDuration(getAnimDuration().count());
        anim_->setEasingCurve(QEasingCurve::InOutSine);
        connect(anim_, &QVariantAnimation::valueChanged, this, qOverload<>(&ClickableWidget::update));
        connect(anim_, &QVariantAnimation::finished, this, [this]()
            {
                currentIcon_.normal_ = nextIcon_;
                update();
            });
    }

    SubmitButton::~SubmitButton() = default;

    void SubmitButton::setState(const State _state, const StateTransition _transition)
    {
        if (_state == getState() && _transition != StateTransition::Force)
            return;

        anim_->stop();
        if (state_ != _state)
            hideToolTip();

        state_ = _state;
        auto icons = getStatePixmaps();
        if (_transition == StateTransition::Force)
        {
            currentIcon_ = std::move(icons);
            update();
        }
        else
        {
            nextIcon_ = icons.normal_;
            currentIcon_.hover_ = icons.hover_;
            currentIcon_.pressed_ = icons.pressed_;

            anim_->start();
        }

        setTooltipText(getStateTooltip());
    }

    void SubmitButton::updateStyleImpl(const InputStyleMode)
    {
        setState(getState(), StateTransition::Force);
    }

    void SubmitButton::onTooltipTimer()
    {
        if (isHovered() || hasFocus() || (enableCircleHover_ && underMouse_))
            showToolTip();
    }

    void SubmitButton::setCircleHover(std::unique_ptr<CircleHover>&& _circle)
    {
        _circle->setDestWidget(this);
        circleHover_ = std::move(_circle);
    }

    void SubmitButton::enableCircleHover(bool _val)
    {
        if (enableCircleHover_ != _val)
        {
            enableCircleHover_ = _val;
            if (!enableCircleHover_)
                setIsUnderLongPress(false);
            updateHoverCircle(isHovered() || (isUnderLongPress_ && hasFocus()));
            setFocusColor(enableCircleHover_ ? Qt::transparent : focusColorPrimary());
        }
    }

    void SubmitButton::setRectExtention(const QMargins& _m)
    {
        extention_ = _m;
    }

    void SubmitButton::paintEvent(QPaintEvent* _event)
    {
        const auto customFocusDraw = styleMode_ != InputStyleMode::Default && (state_ == State::Edit || state_ == State::Send);
        if (!customFocusDraw)
            ClickableWidget::paintEvent(_event);

        QPainter p(this);
        p.setRenderHints(QPainter::Antialiasing | QPainter::SmoothPixmapTransform);

        const auto drawIcon = [this, &p, range = normalIconSize() - minIconSize()](const auto& _icon, const auto _valSize, const auto _valOpacity)
        {
            Utils::PainterSaver ps(p);
            p.setOpacity(_valOpacity);

            const auto iconWidth = _valSize * range + minIconSize();
            const auto x = (width() - iconWidth) / 2;
            const auto y = (height() - iconWidth) / 2;
            p.drawPixmap(QRect(x, y, iconWidth, iconWidth), _icon);
        };

        if (anim_->state() == QAbstractAnimation::State::Running)
        {
            const auto animValue = anim_->currentValue().toDouble();
            drawIcon(currentIcon_.normal_, 1. - animValue, 1. - animValue);
            drawIcon(nextIcon_, animValue, animValue);
        }
        else
        {
            const auto& icon = isPressed() ? currentIcon_.pressed_ : (isHovered() ? currentIcon_.hover_ : currentIcon_.normal_);
            const auto isAnimFocusRunning = animFocus_->state() == QAbstractAnimation::State::Running;
            if (customFocusDraw && (hasFocus() || isAnimFocusRunning))
            {
                if (isAnimFocusRunning)
                {
                    drawIcon(icon, 1.0, 1.0);
                    drawIcon(currentIcon_.hover_, 1.0, animFocus_->currentValue().toDouble());
                }
                else
                {
                    drawIcon(currentIcon_.hover_, 1.0, 1.0);
                }
            }
            else
            {
                drawIcon(icon, 1.0, 1.0);
            }
        }
    }

    void SubmitButton::leaveEvent(QEvent* _e)
    {
        longTapTimer_.stop();

        ClickableWidget::leaveEvent(_e);
    }

    void SubmitButton::mousePressEvent(QMouseEvent* _e)
    {
        longTapTimer_.start();

        ClickableWidget::mousePressEvent(_e);
    }

    void SubmitButton::mouseReleaseEvent(QMouseEvent* _e)
    {
        longTapTimer_.stop();
        ClickableWidget::mouseReleaseEvent(_e);
        _e->accept();
    }

    void SubmitButton::mouseMoveEvent(QMouseEvent* _e)
    {
        ClickableWidget::mouseMoveEvent(_e);
        if (!isHovered())
            longTapTimer_.stop();

        if (isPressed())
        {
            onHoverChanged();
            Q_EMIT mouseMovePressed(QPrivateSignal());
        }
    }

    void SubmitButton::focusInEvent(QFocusEvent* _event)
    {
        ClickableWidget::focusInEvent(_event);
        onMouseStateChanged();
        if (isUnderLongPress_)
            updateHoverCircle(true);
    }

    void SubmitButton::focusOutEvent(QFocusEvent* _event)
    {
        ClickableWidget::focusOutEvent(_event);
        onMouseStateChanged();

        longPressTimer_.stop();
        if (isUnderLongPress_)
            updateHoverCircle(false);
    }

    static constexpr bool isEnterKey(int key) noexcept
    {
        return key == Qt::Key_Enter || key == Qt::Key_Return;
    }

    static bool hasPermission()
    {
        return media::permissions::checkPermission(media::permissions::DeviceType::Microphone) == media::permissions::Permission::Allowed;
    }

    void SubmitButton::keyPressEvent(QKeyEvent* _event)
    {
        if (hasPermission())
        {
            _event->ignore();
            if constexpr (platform::is_apple())
            {
                if (_event->isAutoRepeat())
                {
                    if (!enableCircleHover_ && !isUnderLongPress_ && isEnterKey(_event->key()) && !longPressTimer_.isActive() && hasFocus())
                        longPressTimer_.start();
                    _event->accept();
                }
            }
        }
        else
        {
            if (_event->isAutoRepeat())
                _event->accept();
            else
                ClickableWidget::keyPressEvent(_event);
        }
    }

    void SubmitButton::keyReleaseEvent(QKeyEvent* _event)
    {
        if (hasPermission())
        {
            _event->ignore();
            if (_event->isAutoRepeat())
            {
                if (!enableCircleHover_ && !isUnderLongPress_ && isEnterKey(_event->key()) && !longPressTimer_.isActive() && hasFocus())
                    longPressTimer_.start();
                _event->accept();
            }
            else
            {
                if (isUnderLongPress_ && !isEnterKey(_event->key()))
                    return;

                longPressTimer_.stop();
                setIsUnderLongPress(false);
                if (isEnterKey(_event->key()) && hasFocus())
                {
                    click(ClickType::ByKeyboard);
                    _event->accept();
                }
            }
        }
        else
        {
            if (_event->isAutoRepeat())
                _event->accept();
            else
                ClickableWidget::keyReleaseEvent(_event);
        }
    }

    bool SubmitButton::focusNextPrevChild(bool _next)
    {
        if (enableCircleHover_ && !isUnderLongPress_)
            return false;

        return ClickableWidget::focusNextPrevChild(_next);
    }

    void SubmitButton::onMouseStateChanged()
    {
        if (anim_->state() == QAbstractAnimation::State::Running)
            setState(getState(), StateTransition::Force);

        update();
    }

    void SubmitButton::onHoverChanged()
    {
        if (const auto newUnderMouse = containsCursorUnder(); newUnderMouse != underMouse_)
        {
            underMouse_ = newUnderMouse;
            updateHoverCircle(underMouse_ && !isUnderLongPress_);
        }
    }

    void SubmitButton::updateHoverCircle(bool _hover)
    {
        if (!circleHover_)
            return;

        if (enableCircleHover_)
        {
            enableTooltip_ = _hover;
            if (_hover)
            {
                tooltipTimer_.start();
                circleHover_->showAnimated();
            }
            else
            {
                tooltipTimer_.stop();
                circleHover_->hideAnimated();
            }
        }
        else
        {
            hideToolTip();
            circleHover_->forceHide();
        }
    }

    void SubmitButton::showToolTip()
    {
        if (enableCircleHover_)
            Tooltip::forceShow(true);
        ClickableWidget::showToolTip();
    }

    SubmitButton::IconStates SubmitButton::getStatePixmaps() const
    {
        const bool alternate = styleMode_ == InputStyleMode::CustomBg;
        switch (state_)
        {
        case State::Ptt:
        {
            static const IconStates iconsDefault =
            {
                makePttIcon(Styling::InputButtons::Default::defaultColor()),
                makePttIcon(Styling::InputButtons::Default::hoverColor()),
                makePttIcon(Styling::InputButtons::Default::pressedColor())
            };
            static const IconStates iconsCustom =
            {
                makePttIcon(Styling::InputButtons::Alternate::defaultColor()),
                makePttIcon(Styling::InputButtons::Alternate::hoverColor()),
                makePttIcon(Styling::InputButtons::Alternate::pressedColor())
            };

            return alternate ? iconsCustom : iconsDefault;
        }

        case State::Send:
        {
            static const IconStates iconsDefault =
            {
                makeSendIcon(Styling::StyleVariable::PRIMARY, InputStyleMode::Default),
                makeSendIcon(Styling::StyleVariable::PRIMARY_HOVER, InputStyleMode::Default),
                makeSendIcon(Styling::StyleVariable::PRIMARY_ACTIVE, InputStyleMode::Default)
            };
            static const IconStates iconsCustom =
            {
                makeSendIcon(Styling::StyleVariable::PRIMARY, InputStyleMode::CustomBg),
                makeSendIcon(Styling::StyleVariable::PRIMARY_HOVER, InputStyleMode::CustomBg),
                makeSendIcon(Styling::StyleVariable::PRIMARY_ACTIVE, InputStyleMode::CustomBg)
            };

            return alternate ? iconsCustom : iconsDefault;
        }

        case State::Edit:
        {
            static const IconStates icons =
            {
                makeEditIcon(Styling::StyleVariable::PRIMARY),
                makeEditIcon(Styling::StyleVariable::PRIMARY_HOVER),
                makeEditIcon(Styling::StyleVariable::PRIMARY_ACTIVE)
            };
            return icons;
        }

        default:
            im_assert(!"invalid state");
            break;
        }
        return IconStates();
    }


    QString SubmitButton::getStateTooltip() const
    {
        switch (state_)
        {
        case State::Ptt:
            return QT_TRANSLATE_NOOP("input_widget", "Voice message");

        case State::Send:
            return QT_TRANSLATE_NOOP("input_widget", "Send");

        case State::Edit:
            return QT_TRANSLATE_NOOP("input_widget", "Edit");

        default:
            im_assert(!"invalid state");
            break;
        }


        return QString();
    }

    void SubmitButton::setIsUnderLongPress(bool _v)
    {
        if (_v != isUnderLongPress_)
            isUnderLongPress_ = _v;
    }

    bool SubmitButton::containsCursorUnder() const
    {
        if (enableCircleHover_)
            return rect().marginsAdded(extention_).contains(mapFromGlobal(QCursor::pos()));
        return rect().contains(mapFromGlobal(QCursor::pos()));
    }
}
