#include "stdafx.h"
#include "CustomButton.h"
#include "../utils/utils.h"

#include "TooltipWidget.h"

namespace
{
    using namespace std::chrono_literals;

    constexpr std::chrono::milliseconds tooltipShowDelay() noexcept { return 400ms; }
    constexpr std::chrono::milliseconds focusInAnimDuration() noexcept { return 50ms; }
}

namespace Ui
{
    CustomButton::CustomButton(QWidget* _parent, const QString& _svgName, const QSize& _iconSize, const QColor& _defaultColor)
        : QAbstractButton(_parent)
        , textAlign_(Qt::AlignCenter)
        , textOffsetLeft_(0)
        , offsetHor_(0)
        , offsetVer_(0)
        , align_(Qt::AlignCenter)
        , enableTooltip_(true)
        , active_(false)
        , hovered_(false)
        , pressed_(false)
        , bgRect_(rect())
        , shape_(ButtonShape::RECTANGLE)
    {
        if (!_svgName.isEmpty())
            setDefaultImage(_svgName, _defaultColor, _iconSize);

        if (!_iconSize.isEmpty())
            setMinimumSize(Utils::scale_value(_iconSize));
        else if (!pixmapDefault_.isNull() && !pixmapDefault_.size().isEmpty())
            setMinimumSize(Utils::unscale_bitmap(pixmapDefault_.size()));

        setCursor(Qt::PointingHandCursor);
        setFocusPolicy(Qt::NoFocus);
        setMouseTracking(true);

        connect(this, &QAbstractButton::toggled, this, &CustomButton::setActive);

        tooltipTimer_.setSingleShot(true);
        tooltipTimer_.setInterval(tooltipShowDelay().count());

        connect(&tooltipTimer_, &QTimer::timeout, this, &CustomButton::onTooltipTimer);
    }

    void CustomButton::paintEvent(QPaintEvent* _e)
    {
        QPainter p(this);
        p.setRenderHints(QPainter::TextAntialiasing | QPainter::Antialiasing);

        if (bgColor_.isValid())
            p.fillRect(bgRect_, bgColor_);

        if (focusColor_.isValid() && (animFocus_.isRunning() || hasFocus()))
        {
            p.setPen(Qt::NoPen);
            p.setBrush(focusColor_);

            auto r = rect();
            if (animFocus_.isRunning())
            {
                const auto cur = animFocus_.current();
                const auto w = width() * cur;
                r.setSize(QSize(w, w));
                r.moveCenter(rect().center());
            }
            p.drawEllipse(r);
        }

        if (textColor_.isValid())
        {
            if (const auto btnText = text(); !btnText.isEmpty())
            {
                p.setPen(textColor_);
                p.setFont(font());
                p.drawText(rect().translated(textOffsetLeft_, 0), textAlign_, btnText);
            }
        }

        if (!isEnabled() && !pixmapDisabled_.isNull())
            pixmapToDraw_ = pixmapDisabled_;
        else if (pressed_ && !pixmapPressed_.isNull())
            pixmapToDraw_ = pixmapPressed_;
        else if (active_ && !pixmapActive_.isNull())
            pixmapToDraw_ = pixmapActive_;
        else if (hovered_ && !pixmapHover_.isNull())
            pixmapToDraw_ = pixmapHover_;
        else
            pixmapToDraw_ = pixmapDefault_;


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

        p.drawPixmap(x, y, pixmapToDraw_);
        if (!pixmapOverlay_.isNull())
            p.drawPixmap(overlayCenterPos_ - QPoint(pixmapOverlay_.size().width()/2, pixmapOverlay_.size().height()/2), pixmapOverlay_);
    }

    void CustomButton::leaveEvent(QEvent* _e)
    {
        hovered_ = false;
        emit changedHover(hovered_);
        enableTooltip_ = false;
        tooltipTimer_.stop();
        update();

        if (textColor_.isValid())
            setTextColor(textColor_);

        QAbstractButton::leaveEvent(_e);
    }

    void CustomButton::enterEvent(QEvent* _e)
    {
        hovered_ = true;
        emit changedHover(hovered_);
        enableTooltip_ = true;
        update();

        if (pressed_ && textColorPressed_.isValid())
            setTextColor(textColorPressed_);
        else if (textColorHovered_.isValid())
            setTextColor(textColorHovered_);

        tooltipTimer_.start();

        QAbstractButton::enterEvent(_e);
    }

    void CustomButton::mousePressEvent(QMouseEvent * _e)
    {
        pressed_ = true;
        emit changedPress(pressed_);
        enableTooltip_ = false;
        hideToolTip();
        update();

        if (textColorPressed_.isValid())
            setTextColor(textColorPressed_);

        QAbstractButton::mousePressEvent(_e);
    }

    void CustomButton::mouseReleaseEvent(QMouseEvent * _e)
    {
        pressed_ = false;
        emit changedPress(pressed_);
        update();

        if (textColor_.isValid())
            setTextColor(textColor_);

        emit clickedWithButtons(_e->button());

        QAbstractButton::mouseReleaseEvent(_e);
    }

    void CustomButton::resizeEvent(QResizeEvent * _event)
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
            emit clicked();
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

    void CustomButton::setActive(const bool _isActive)
    {
        active_ = _isActive;

        if (pixmapActive_.isNull())
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

    void CustomButton::setBackgroundColor(const QColor& _color)
    {
        bgColor_ = _color;
        update();
    }

    void CustomButton::setTextColor(const QColor& _color)
    {
        if (_color != textColor_)
        {
            textColor_ = _color;
            update();
        }
    }

    void CustomButton::setHoveredTextColor(const QColor& _color)
    {
        textColorHovered_ = _color;
    }

    void CustomButton::setPressedTextColor(const QColor & _color)
    {
        textColorPressed_ = _color;
    }

    QSize CustomButton::sizeHint() const
    {
        QFontMetrics fm(font());
        int width = fm.width(text());
        int height = fm.height();
        return QSize(width, height);
    }

    void CustomButton::setBackgroundRound()
    {
        bgRect_.setWidth(Utils::unscale_bitmap(pixmapDefault_.rect()).width() / std::sqrt(2.5));
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

    void CustomButton::setFocusColor(const QColor& _color)
    {
        focusColor_ = _color;
        if (hasFocus())
            update();
    }

    QPixmap CustomButton::getPixmap(const QString& _path, const QColor& _color, const QSize& _size) const
    {
        return Utils::renderSvgScaled(_path, _size.isEmpty() ? svgSize_ : _size, _color);
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
                Tooltip::show(t, QRect(mapToGlobal(r.topLeft()), r.size()), { -1, -1 }, Tooltip::ArrowDirection::Down);
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

        animFocus_.finish();
        animFocus_.start([this]() { update(); }, 0.5, 1.0, focusInAnimDuration().count(), anim::sineInOut);
    }

    void CustomButton::animateFocusOut()
    {
        if (!isVisible())
            return;

        animFocus_.finish();
        animFocus_.start([this]() { update(); }, 1.0, 0.5, focusInAnimDuration().count(), anim::sineInOut);
    }

    void CustomButton::setDefaultColor(const QColor& _color)
    {
        setDefaultImage(svgPath_, _color);
    }

    void CustomButton::setHoverColor(const QColor& _color)
    {
        setHoverImage(svgPath_, _color);
    }

    void CustomButton::setActiveColor(const QColor& _color)
    {
        setActiveImage(svgPath_, _color);
    }

    void CustomButton::setDisabledColor(const QColor& _color)
    {
        setDisabledImage(svgPath_, _color);
    }

    void CustomButton::setPressedColor(const QColor& _color)
    {
        setPressedImage(svgPath_, _color);
    }

    void CustomButton::addOverlayPixmap(const QPixmap &_pixmap, QPoint _pos)
    {
        pixmapOverlay_ = _pixmap;
        overlayCenterPos_ = _pos;
        update();
    }

    void CustomButton::setDefaultImage(const QString& _svgPath, const QColor& _color, const QSize& _size)
    {
        svgPath_ = _svgPath;
        if (!_size.isEmpty())
            svgSize_ = _size;

        pixmapDefault_ = getPixmap(_svgPath, _color, _size);

        update();
    }

    void CustomButton::setHoverImage(const QString& _svgPath, const QColor& _color, const QSize& _size)
    {
        pixmapHover_ = getPixmap(_svgPath, _color, _size);
        update();
    }

    void CustomButton::setActiveImage(const QString& _svgPath, const QColor& _color, const QSize& _size)
    {
        pixmapActive_ = getPixmap(_svgPath, _color, _size);
        update();
    }

    void CustomButton::setDisabledImage(const QString& _svgPath, const QColor& _color, const QSize& _size)
    {
        pixmapDisabled_ = getPixmap(_svgPath, _color, _size);
        update();
    }

    void CustomButton::setPressedImage(const QString& _svgPath, const QColor& _color, const QSize& _size)
    {
        pixmapPressed_ = getPixmap(_svgPath, _color, _size);
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
}


