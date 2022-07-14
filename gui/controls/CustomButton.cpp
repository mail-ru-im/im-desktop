#include "stdafx.h"
#include "CustomButton.h"
#include "../utils/utils.h"

#include "TooltipWidget.h"
#include "fonts.h"

namespace
{
    using namespace std::chrono_literals;

    constexpr std::chrono::milliseconds tooltipShowDelay() noexcept { return 400ms; }
    constexpr std::chrono::milliseconds focusInAnimDuration() noexcept { return 50ms; }
    constexpr std::chrono::milliseconds spinFrameRate() noexcept { return 16ms; }

    const int getMargin() noexcept { return Utils::scale_value(8); }
    const int getBubbleCornerRadius() noexcept { return Utils::scale_value(18); }
    const int textIconSpacing() noexcept { return Utils::scale_value(4); }
    constexpr QSize buttonIconSize() noexcept { return QSize(32, 32); }

    void drawBubble(QPainter& _p, const QRect& _widgetRect, const QColor& _color, const int _topMargin, const int _botMargin, const int _radius)
    {
        if (_widgetRect.isEmpty() || !_color.isValid())
            return;

        Utils::PainterSaver ps(_p);
        _p.setRenderHint(QPainter::Antialiasing);

        const auto bubbleRect = _widgetRect.adjusted(0, _topMargin, 0, -_botMargin);

        QPainterPath path;
        path.addRoundedRect(bubbleRect, _radius, _radius);

        Utils::drawBubbleShadow(_p, path, _radius);

        _p.setPen(Qt::NoPen);
        _p.setCompositionMode(QPainter::CompositionMode_Source);
        _p.fillPath(path, _color);
    }
}

namespace Ui
{
    CustomButton::CustomButton(QWidget* _parent, const QString& _svgName, const QSize& _iconSize, const Styling::ColorParameter& _defaultColor)
        : QAbstractButton(_parent)
        , textAlign_(Qt::AlignCenter)
        , textOffsetLeft_(0)
        , spinAngle_(0)
        , offsetHor_(0)
        , offsetVer_(0)
        , align_(Qt::AlignCenter)
        , enableTooltip_(true)
        , active_(false)
        , hovered_(false)
        , pressed_(false)
        , forceHover_(false)
        , bgRect_(rect())
        , shape_(ButtonShape::RECTANGLE)
        , animFocus_(new QVariantAnimation(this))
        , spinTimer_(new QTimer(this))
    {
        if (!_svgName.isEmpty())
            setDefaultImage(_svgName, _defaultColor, _iconSize);

        if (!_iconSize.isEmpty())
            setMinimumSize(Utils::scale_value(_iconSize));
        else if (const auto pixmap = pixmapDefault_.cachedPixmap(); !pixmap.isNull() && !pixmap.size().isEmpty())
            setMinimumSize(Utils::unscale_bitmap(pixmap.size()));

        setCursor(Qt::PointingHandCursor);
        setFocusPolicy(Qt::NoFocus);
        setMouseTracking(true);

        connect(this, &QAbstractButton::toggled, this, &CustomButton::setActive);

        tooltipTimer_.setSingleShot(true);
        tooltipTimer_.setInterval(tooltipShowDelay());

        connect(&tooltipTimer_, &QTimer::timeout, this, &CustomButton::onTooltipTimer);

        animFocus_->setStartValue(0.5);
        animFocus_->setEndValue(1.0);
        animFocus_->setDuration(focusInAnimDuration().count());
        animFocus_->setEasingCurve(QEasingCurve::InOutSine);
        connect(animFocus_, &QVariantAnimation::valueChanged, this, qOverload<>(&CustomButton::update));

        spinTimer_->setInterval(spinFrameRate());
        connect(spinTimer_, &QTimer::timeout, this, [this]() { spinAngle_ = ((spinAngle_ + 6) % 360); update(); });

    }

    void CustomButton::paintEvent(QPaintEvent* _e)
    {
        QPainter p(this);
        p.setRenderHints(QPainter::TextAntialiasing | QPainter::Antialiasing | QPainter::SmoothPixmapTransform);

        QColor brush = backgroundBrushNormal_.actualColor();
        if (!isEnabled())
            brush = backgroundBrushDisabled_.actualColor();
        else if (isPressed())
            brush = backgroundBrushPressed_.actualColor();
        else if (isHovered())
            brush = backgroundBrushHovered_.actualColor();

        if (shape_ == ButtonShape::ROUNDED_RECTANGLE)
        {
            p.setBrush(brush);
            p.setPen(QPen(brush, 0));
            const auto radius = bgRect_.height() / 2;
            p.drawRoundedRect(bgRect_, radius, radius);
        }
        else
        {
            p.fillRect(bgRect_, brush);
        }


        const auto isAnimRunning = animFocus_->state() == QAbstractAnimation::State::Running;
        if (const auto focusColor = focusColor_.actualColor(); focusColor.isValid() && (isAnimRunning || hasFocus()))
        {
            p.setPen(Qt::NoPen);
            p.setBrush(focusColor);

            auto r = rect();
            if (isAnimRunning)
            {
                const auto cur = animFocus_->currentValue().toDouble();
                const auto w = width() * cur;
                r.setSize(QSize(w, w));
                r.moveCenter(rect().center());
            }
            p.drawEllipse(r);
        }

        if (const auto textColor = textColor_.actualColor(); textColor.isValid())
        {
            if (const auto btnText = text(); !btnText.isEmpty())
            {
                p.setPen(textColor);
                p.setFont(font());
                p.drawText(rect().translated(textOffsetLeft_, 0), textAlign_, btnText);
            }
        }

        if (isSpinning() && !pixmapSpinner_.actualPixmap().isNull())
            pixmapToDraw_ = pixmapSpinner_.cachedPixmap();
        else if (forceHover_)
            pixmapToDraw_ = pixmapHover_.actualPixmap();
        else if (!isEnabled() && !pixmapDisabled_.actualPixmap().isNull())
            pixmapToDraw_ = pixmapDisabled_.cachedPixmap();
        else if (pressed_ && !pixmapPressed_.actualPixmap().isNull())
            pixmapToDraw_ = pixmapPressed_.cachedPixmap();
        else if (active_ && !pixmapActive_.actualPixmap().isNull())
            pixmapToDraw_ = pixmapActive_.cachedPixmap();
        else if (hovered_ && !pixmapHover_.actualPixmap().isNull())
            pixmapToDraw_ = pixmapHover_.cachedPixmap();
        else
            pixmapToDraw_ = pixmapDefault_.actualPixmap();

        if (pixmapToDraw_.isNull())
            return;

        const double ratio = Utils::scale_bitmap_ratio();
        const auto diffX = rect().width() - pixmapToDraw_.width() / ratio;
        const auto diffY = rect().height() - pixmapToDraw_.height() / ratio;

        // center by default
        int x = diffX / 2;
        int y = diffY / 2;

        if (align_ & Qt::AlignLeft)
            x = 0;
        else if (align_ & Qt::AlignRight)
            x = diffX;

        if (align_ & Qt::AlignTop)
            y = 0;
        else if (align_ & Qt::AlignBottom)
            y = diffY;

        x += offsetHor_;
        y += offsetVer_;


        if (isSpinning())
        {
            const QPointF center(iconSize().width() * 0.5, iconSize().height() * 0.5);

            Utils::PainterSaver paintGuard(p);
            p.translate(x, y);
            p.translate(center);
            p.rotate(spinAngle_);
            p.translate(-center);
            p.drawPixmap(0, 0, pixmapToDraw_);
        }
        else
        {
            p.drawPixmap(x, y, pixmapToDraw_);
        }

        if (!pixmapOverlay_.isNull())
            p.drawPixmap(overlayCenterPos_ - QPoint(pixmapOverlay_.size().width() / 2, pixmapOverlay_.size().height() / 2), pixmapOverlay_);
    }

    void CustomButton::leaveEvent(QEvent* _e)
    {
        hovered_ = false;
        Q_EMIT changedHover(hovered_);
        enableTooltip_ = false;
        tooltipTimer_.stop();
        update();

        if (textColorNormal_.isValid())
            setTextColor(textColorNormal_);

        QAbstractButton::leaveEvent(_e);
    }

    void CustomButton::enterEvent(QEvent* _e)
    {
        hovered_ = true;
        Q_EMIT changedHover(hovered_);
        enableTooltip_ = true;
        update();

        if (pressed_ && textColorPressed_.isValid())
            setTextColor(textColorPressed_);
        else if (textColorHovered_.isValid())
            setTextColor(textColorHovered_);

        tooltipTimer_.start();

        QAbstractButton::enterEvent(_e);
    }

    void CustomButton::mousePressEvent(QMouseEvent* _e)
    {
        pressed_ = true;
        Q_EMIT changedPress(pressed_);
        enableTooltip_ = false;
        hideToolTip();
        update();

        if (textColorPressed_.isValid())
            setTextColor(textColorPressed_);

        QAbstractButton::mousePressEvent(_e);
    }

    void CustomButton::mouseReleaseEvent(QMouseEvent* _e)
    {
        pressed_ = false;
        Q_EMIT changedPress(pressed_);
        update();

        if (textColorNormal_.isValid())
            setTextColor(textColorNormal_);

        Q_EMIT clickedWithButtons(_e->button());

        QAbstractButton::mouseReleaseEvent(_e);
    }

    void CustomButton::resizeEvent(QResizeEvent* _event)
    {
        if (shape_ == ButtonShape::CIRCLE)
            setBackgroundRound();
        else
            bgRect_.setSize(size());
    }

    void CustomButton::keyPressEvent(QKeyEvent* _event)
    {
        _event->ignore();
        if ((_event->key() == Qt::Key_Enter || _event->key() == Qt::Key_Return) && hasFocus())
        {
            Q_EMIT clicked();
            _event->accept();
        }
    }

    void CustomButton::focusInEvent(QFocusEvent* _event)
    {
        animateFocusIn();

        Tooltip::forceShow(true);
        enableTooltip_ = true;
        tooltipTimer_.start();
        QAbstractButton::focusInEvent(_event);
    }

    void CustomButton::focusOutEvent(QFocusEvent* _event)
    {
        animateFocusOut();

        Tooltip::forceShow(false);
        enableTooltip_ = false;
        tooltipTimer_.stop();
        hideToolTip();

        QAbstractButton::focusOutEvent(_event);
    }

    void CustomButton::setIconAlignment(const Qt::Alignment _flags)
    {
        align_ = _flags;
        update();
    }

    void CustomButton::setIconOffsets(const int _horOffset, const int _verOffset)
    {
        offsetHor_ = _horOffset;
        offsetVer_ = _verOffset;
        update();
    }

    void CustomButton::setSpinning(bool _on)
    {
        if (_on)
            spinTimer_->start();
        else
            spinTimer_->stop();
    }

    bool CustomButton::isSpinning() const
    {
        return spinTimer_->isActive();
    }

    void CustomButton::setActive(const bool _isActive)
    {
        active_ = _isActive;

        if (pixmapActive_.cachedPixmap().isNull())
            return;

        update();
    }

    bool CustomButton::isActive() const
    {
        return active_;
    }

    bool CustomButton::isHovered() const
    {
        return hovered_;
    }

    bool CustomButton::isPressed() const
    {
        return pressed_;
    }

    void CustomButton::setBackgroundNormal(const Styling::ColorParameter& _brush)
    {
        backgroundBrushNormal_ = Styling::ColorContainer{ _brush };
        update();
    }

    void CustomButton::setBackgroundDisabled(const Styling::ColorParameter& _brush)
    {
        backgroundBrushDisabled_ = Styling::ColorContainer{ _brush };
        update();
    }

    void CustomButton::setBackgroundHovered(const Styling::ColorParameter& _brush)
    {
        backgroundBrushHovered_ = Styling::ColorContainer{ _brush };
        update();
    }

    void CustomButton::setBackgroundPressed(const Styling::ColorParameter& _brush)
    {
        backgroundBrushPressed_ = Styling::ColorContainer{ _brush };
        update();
    }

    void CustomButton::setTextColor(const Styling::ColorParameter& _color)
    {
        if (_color.color() != textColor_.actualColor())
        {
            textColor_ = Styling::ColorContainer{ _color };
            update();
        }
    }

    void CustomButton::setNormalTextColor(const Styling::ColorParameter& _color)
    {
        textColorNormal_ = _color;
        setTextColor(textColorNormal_);
    }

    void CustomButton::setHoveredTextColor(const Styling::ColorParameter& _color)
    {
        textColorHovered_ = _color;
    }

    void CustomButton::setPressedTextColor(const Styling::ColorParameter& _color)
    {
        textColorPressed_ = _color;
    }

    QSize CustomButton::sizeHint() const
    {
        QFontMetrics fm(font());
        int width = fm.horizontalAdvance(text());
        int height = fm.height();
        return QSize(width, height);
    }

    void CustomButton::setBackgroundRound()
    {
        bgRect_.setWidth(Utils::unscale_bitmap(pixmapDefault_.cachedPixmap().rect()).width() / std::sqrt(2.5));
        bgRect_.setHeight(bgRect_.width());

        int x = (width() - bgRect_.width()) / 2;
        int y = (height() - bgRect_.height()) / 2;
        if (align_ & Qt::AlignLeft)
            x = 0;
        else if (align_ & Qt::AlignRight)
            x = width() - bgRect_.width();

        if (align_ & Qt::AlignTop)
            y = 0;
        else if (align_ & Qt::AlignBottom)
            y = height() - bgRect_.height();

        x += offsetHor_;
        y += offsetVer_;
        bgRect_.moveTopLeft(QPoint(x, y));
    }

    void CustomButton::setShape(ButtonShape _shape)
    {
        shape_ = _shape;
    }

    void CustomButton::setTextAlignment(const Qt::Alignment _flags)
    {
        textAlign_ = _flags;
        update();
    }

    void CustomButton::setTextLeftOffset(const int _offset)
    {
        textOffsetLeft_ = _offset;
        update();
    }

    void CustomButton::forceHover(bool _force)
    {
        forceHover_ = _force;
        update();
    }

    void CustomButton::setFocusColor(const Styling::ColorParameter& _color)
    {
        focusColor_ = Styling::ColorContainer{ _color };
        if (hasFocus())
            update();
    }

    Utils::StyledPixmap CustomButton::getPixmap(const QString& _path, const Styling::ColorParameter& _color, const QSize& _size) const
    {
        return Utils::StyledPixmap::scaled(_path, _size.isEmpty() ? svgSize_ : _size, _color);
    }

    void CustomButton::onTooltipTimer()
    {
        if (isHovered() || hasFocus())
            showToolTip();
    }

    void CustomButton::showToolTip()
    {
        if (enableTooltip_)
        {
            if (const auto t = getCustomToolTip(); !t.isEmpty())
            {
                const auto r = rect();
                Tooltip::show(t, QRect(mapToGlobal(r.topLeft()), r.size()), { 0, 0 }, Tooltip::ArrowDirection::Down);
            }
        }
    }

    void CustomButton::hideToolTip()
    {
        Tooltip::forceShow(false);
        Tooltip::hide();
    }

    void CustomButton::animateFocusIn()
    {
        if (!isVisible())
            return;

        animFocus_->stop();
        animFocus_->setDirection(QAbstractAnimation::Forward);
        animFocus_->start();
    }

    void CustomButton::animateFocusOut()
    {
        if (!isVisible())
            return;

        animFocus_->stop();
        animFocus_->setDirection(QAbstractAnimation::Backward);
        animFocus_->start();
    }

    void CustomButton::setDefaultColor(const Styling::ColorParameter& _color)
    {
        setDefaultImage(svgPath_, _color);
    }

    void CustomButton::setHoverColor(const Styling::ColorParameter& _color)
    {
        setHoverImage(svgPath_, _color);
    }

    void CustomButton::setActiveColor(const Styling::ColorParameter& _color)
    {
        setActiveImage(svgPath_, _color);
    }

    void CustomButton::setDisabledColor(const Styling::ColorParameter& _color)
    {
        setDisabledImage(svgPath_, _color);
    }

    void CustomButton::setPressedColor(const Styling::ColorParameter& _color)
    {
        setPressedImage(svgPath_, _color);
    }

    void CustomButton::addOverlayPixmap(const QPixmap& _pixmap, QPoint _pos)
    {
        pixmapOverlay_ = _pixmap;
        overlayCenterPos_ = _pos;
        update();
    }

    void CustomButton::setDefaultImage(const QString& _svgPath, const Styling::ColorParameter& _color, const QSize& _size)
    {
        svgPath_ = _svgPath;
        if (!_size.isEmpty())
            svgSize_ = _size;

        pixmapDefault_ = getPixmap(_svgPath, _color, _size);

        update();
    }

    void CustomButton::setHoverImage(const QString& _svgPath, const Styling::ColorParameter& _color, const QSize& _size)
    {
        pixmapHover_ = getPixmap(_svgPath, _color, _size);
        update();
    }

    void CustomButton::setActiveImage(const QString& _svgPath, const Styling::ColorParameter& _color, const QSize& _size)
    {
        pixmapActive_ = getPixmap(_svgPath, _color, _size);
        update();
    }

    void CustomButton::setDisabledImage(const QString& _svgPath, const Styling::ColorParameter& _color, const QSize& _size)
    {
        pixmapDisabled_ = getPixmap(_svgPath, _color, _size);
        update();
    }

    void CustomButton::setPressedImage(const QString& _svgPath, const Styling::ColorParameter& _color, const QSize& _size)
    {
        pixmapPressed_ = getPixmap(_svgPath, _color, _size);
        update();
    }

    void CustomButton::setSpinnerImage(const QString& _svgPath, const Styling::ColorParameter& _color, const QSize& _size)
    {
        pixmapSpinner_ = getPixmap(_svgPath, _color, _size);
    }

    void CustomButton::clearIcon()
    {
        pixmapDefault_ = Utils::StyledPixmap{};
        pixmapActive_ = Utils::StyledPixmap{};
        pixmapDisabled_ = Utils::StyledPixmap{};
        pixmapPressed_ = Utils::StyledPixmap{};
        pixmapHover_ = Utils::StyledPixmap{};
        update();
    }

    void CustomButton::setCustomToolTip(const QString& _toopTip)
    {
        toolTip_ = _toopTip;
    }

    const QString& CustomButton::getCustomToolTip() const
    {
        return toolTip_;
    }

    RoundButton::RoundButton(QWidget* _parent, int _radius)
        : ClickableWidget(_parent)
        , radius_(_radius == 0 ? getBubbleCornerRadius() : _radius)
    {
        connect(this, &RoundButton::hoverChanged, this, qOverload<>(&RoundButton::update));
        connect(this, &RoundButton::pressChanged, this, qOverload<>(&RoundButton::update));
    }

    void RoundButton::setColors(const Styling::ThemeColorKey& _bgNormal, const Styling::ThemeColorKey& _bgHover, const Styling::ThemeColorKey& _bgActive)
    {
        bgNormal_ = _bgNormal;
        bgHover_ = _bgHover;
        bgActive_ = _bgActive;
        update();
    }

    void RoundButton::setTextColor(const Styling::ThemeColorKey& _color)
    {
        textColor_ = _color;

        if (text_)
            text_->setColor(_color);

        update();
    }

    void RoundButton::setText(const QString& _text, int _size)
    {
        if (_text.isEmpty())
        {
            text_.reset();
        }
        else
        {
            text_ = TextRendering::MakeTextUnit(_text, {}, TextRendering::LinksVisible::DONT_SHOW_LINKS, TextRendering::ProcessLineFeeds::REMOVE_LINE_FEEDS);
            text_->init({ Fonts::appFontScaled(_size), textColor_ });
            text_->evaluateDesiredSize();
        }

        update();
    }

    void RoundButton::setIcon(const QString& _iconPath, int _size)
    {
        icon_ = Utils::StyledPixmap::scaled(_iconPath, _size == 0 ? buttonIconSize() : QSize(_size, _size), textColor_);
        update();
    }

    void RoundButton::setIcon(const Utils::StyledPixmap& _icon)
    {
        icon_ = _icon;
        update();
    }

    void RoundButton::forceHover(bool _force)
    {
        forceHover_ = _force;
        update();
    }

    int RoundButton::textDesiredWidth() const
    {
        if (text_)
            return text_->desiredWidth() + getMargin() * 2;

        return 0;
    }

    void RoundButton::paintEvent(QPaintEvent*)
    {
        QPainter p(this);
        p.setRenderHint(QPainter::Antialiasing);
        p.setRenderHint(QPainter::SmoothPixmapTransform);

        if (const auto bg = getBgColor(); bg.isValid())
            drawBubble(p, rect(), bg, getMargin(), getMargin(), radius_);

        const auto pixmap = icon_.actualPixmap();

        const auto hasIcon = !pixmap.isNull();
        const auto hasText = !!text_;

        if (!hasIcon && !hasText)
            return;

        const auto r = Utils::scale_bitmap_ratio();
        const auto iconWidth = hasIcon ? pixmap.width() / r : 0;
        const auto textWidth = hasText ? text_->desiredWidth() : 0;
        const auto fullWidth = iconWidth + textWidth + ((hasText && hasIcon) ? textIconSpacing() : 0);

        if (hasIcon)
        {
            const auto x = (width() - fullWidth) / 2;
            const auto y = (height() - pixmap.height() / r) / 2;
            p.drawPixmap(x, y, pixmap);
        }

        if (hasText)
        {
            const auto x = (width() - fullWidth) / 2 + (fullWidth - textWidth);
            const auto y = height() / 2;
            text_->setOffsets(x, y);

            text_->draw(p, TextRendering::VerPosition::MIDDLE);
        }
    }

    void RoundButton::resizeEvent(QResizeEvent* _event)
    {
        bubblePath_ = QPainterPath();
        update();
    }

    QSize RoundButton::getTextSize() const
    {
        if (text_)
            return text_->cachedSize();
        return {};
    }

    int RoundButton::getRadius() const
    {
        return radius_;
    }

    QColor RoundButton::getBgColor() const
    {
        if (forceHover_)
            return bgHover_.actualColor();

        QColor bg;
        if (const auto active = bgActive_.actualColor(); isPressed() && active.isValid())
            bg = active;
        else if (const auto hovered = bgHover_.actualColor(); isHovered() && hovered.isValid())
            bg = hovered;
        else
            bg = bgNormal_.actualColor();

        return bg;
    }
}


