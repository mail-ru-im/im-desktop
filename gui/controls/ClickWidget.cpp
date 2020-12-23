#include "stdafx.h"

#include "ClickWidget.h"
#include "TooltipWidget.h"
#include "styles/ThemeParameters.h"

namespace
{
    constexpr std::chrono::milliseconds focusInAnimDuration() noexcept { return std::chrono::milliseconds(50); }
    constexpr std::chrono::milliseconds tooltipShowDelay() noexcept { return std::chrono::milliseconds(400); }
}

namespace Ui
{
    ClickableWidget::ClickableWidget(QWidget* _parent)
        : QWidget(_parent)
        , animFocus_(new QVariantAnimation(this))
    {
        setMouseTracking(true);
        setAttribute(Qt::WA_Hover, true);
        setCursor(Qt::PointingHandCursor);
        setFocusPolicy(Qt::NoFocus);

        tooltipTimer_.setSingleShot(true);
        tooltipTimer_.setInterval(tooltipShowDelay());
        connect(&tooltipTimer_, &QTimer::timeout, this, &ClickableWidget::onTooltipTimer);

        animFocus_->setStartValue(0.5);
        animFocus_->setEndValue(1.0);
        animFocus_->setDuration(focusInAnimDuration().count());
        animFocus_->setEasingCurve(QEasingCurve::InOutSine);
        connect(animFocus_, &QVariantAnimation::valueChanged, this, qOverload<>(&ClickableWidget::update));
    }

    bool ClickableWidget::isHovered() const
    {
        return isHovered_;
    }

    void ClickableWidget::setHovered(const bool _isHovered)
    {
        if (isHovered_ == _isHovered)
            return;

        isHovered_ = _isHovered;
        update();
        Q_EMIT hoverChanged(isHovered_, QPrivateSignal());
    }

    bool ClickableWidget::isPressed() const
    {
        return isPressed_;
    }

    void ClickableWidget::setPressed(const bool _isPressed)
    {
        if (isPressed_ == _isPressed)
            return;

        isPressed_ = _isPressed;
        Q_EMIT pressChanged(isPressed_, QPrivateSignal());
    }

    void ClickableWidget::click(const ClickType _how)
    {
        if (isEnabled())
        {
            Q_EMIT clicked(QPrivateSignal());
            Q_EMIT clickedWithButton(_how, QPrivateSignal());
        }
    }

    void ClickableWidget::setFocusColor(const QColor & _color)
    {
        focusColor_ = _color;
        if (hasFocus())
            update();
    }

    void ClickableWidget::setHoverColor(const QColor& _color)
    {
        hoverColor_ = _color;
    }

    void ClickableWidget::setBackgroundColor(const QColor& _color)
    {
        bgColor_ = _color;
    }

    void ClickableWidget::setTooltipText(const QString& _text)
    {
        tooltipText_ = _text;
    }

    const QString& ClickableWidget::getTooltipText() const
    {
        return tooltipText_;
    }

    void ClickableWidget::overrideNextInFocusChain(QWidget* _next)
    {
        focusChainNextOverride_ = _next;
    }

    QWidget* ClickableWidget::nextInFocusChain() const
    {
        if (focusChainNextOverride_)
            return focusChainNextOverride_;
        return QWidget::nextInFocusChain();
    }

    void ClickableWidget::mousePressEvent(QMouseEvent* _e)
    {
        _e->ignore();
        if (_e->button() == Qt::LeftButton)
        {
            _e->accept();
            isPressed_ = true;
            Q_EMIT pressed(QPrivateSignal());
            Q_EMIT pressChanged(isPressed_, QPrivateSignal());
        }

        enableTooltip_ = false;
        hideToolTip();

        setHovered(rect().contains(_e->pos()));
    }

    void ClickableWidget::mouseReleaseEvent(QMouseEvent* _e)
    {
        _e->ignore();
        if (isHovered_ && _e->button() == Qt::LeftButton)
        {
            _e->accept();
            click(ClickType::ByMouse);
        }

        isPressed_ = false;
        Q_EMIT released(QPrivateSignal());
        Q_EMIT pressChanged(isPressed_, QPrivateSignal());
    }

    void ClickableWidget::paintEvent(QPaintEvent*)
    {
        if (bgColor_.isValid())
        {
            QPainter p(this);
            p.setRenderHints(QPainter::Antialiasing);
            p.setPen(Qt::NoPen);
            p.fillRect(rect(), bgColor_);
        }

        const auto isAnimRunning = animFocus_->state() == QAbstractAnimation::State::Running;

        if (focusColor_.isValid() && (isAnimRunning || hasFocus()))
        {
            QPainter p(this);
            p.setRenderHints(QPainter::Antialiasing);
            p.setPen(Qt::NoPen);
            p.setBrush(focusColor_);

            auto r = rect();
            if (isAnimRunning)
            {
                const auto w = width() * animFocus_->currentValue().toDouble();
                r.setSize(QSize(w, w));
                r.moveCenter(rect().center());
            }
            p.drawEllipse(r);
        }

        if (isHovered_ && hoverColor_.isValid())
        {
            QPainter p(this);
            p.setRenderHints(QPainter::Antialiasing);
            p.setPen(Qt::NoPen);
            p.fillRect(rect(), hoverColor_);
        }
    }

    void ClickableWidget::enterEvent(QEvent* _e)
    {
        enableTooltip_ = true;
        if (isEnabled())
            tooltipTimer_.start();

        setHovered(true);
    }

    void ClickableWidget::leaveEvent(QEvent* _e)
    {
        enableTooltip_ = false;
        tooltipTimer_.stop();

        setHovered(false);
    }

    void ClickableWidget::mouseMoveEvent(QMouseEvent* _e)
    {
        setHovered(rect().contains(_e->pos()));
        Q_EMIT moved(QPrivateSignal());
    }

    void ClickableWidget::keyPressEvent(QKeyEvent* _event)
    {
        _event->ignore();
        if ((_event->key() == Qt::Key_Enter || _event->key() == Qt::Key_Return) && hasFocus())
        {
            click(ClickType::ByKeyboard);
            _event->accept();
        }
    }

    void ClickableWidget::focusInEvent(QFocusEvent*)
    {
        animateFocusIn();

        Tooltip::forceShow(true);
        enableTooltip_ = true;
        tooltipTimer_.start();
    }

    void ClickableWidget::focusOutEvent(QFocusEvent*)
    {
        animateFocusOut();

        Tooltip::forceShow(false);
        enableTooltip_ = false;
        tooltipTimer_.stop();
        hideToolTip();
    }

    void ClickableWidget::showEvent(QShowEvent*)
    {
        Q_EMIT shown(QPrivateSignal());
    }

    bool ClickableWidget::focusNextPrevChild(bool _next)
    {
        if (_next && focusChainNextOverride_)
        {
            focusChainNextOverride_->setFocus(Qt::TabFocusReason);
            return true;
        }
        return QWidget::focusNextPrevChild(_next);
    }

    void ClickableWidget::onTooltipTimer()
    {
        if (isHovered() || hasFocus())
            showToolTip();
    }

    bool ClickableWidget::canShowTooltip()
    {
        return enableTooltip_ && !getTooltipText().isEmpty();
    }

    void ClickableWidget::showToolTip()
    {
        if (canShowTooltip())
        {
            const auto r = rect();
            Tooltip::show(getTooltipText(), QRect(mapToGlobal(r.topLeft()), r.size()), {0, 0}, Tooltip::ArrowDirection::Down);
        }
    }

    void ClickableWidget::hideToolTip()
    {
        Tooltip::forceShow(false);
        Tooltip::hide();
    }

    void ClickableWidget::animateFocusIn()
    {
        if (!isVisible())
            return;

        animFocus_->stop();
        animFocus_->setDirection(QAbstractAnimation::Forward);
        animFocus_->start();
    }

    void ClickableWidget::animateFocusOut()
    {
        if (!isVisible())
            return;

        animFocus_->stop();
        animFocus_->setDirection(QAbstractAnimation::Backward);
        animFocus_->start();
    }

    ClickableTextWidget::ClickableTextWidget(QWidget* _parent, const QFont& _font, const QColor& _color, const TextRendering::HorAligment _textAlign)
        : ClickableWidget(_parent)
    {
        text_ = TextRendering::MakeTextUnit(QString(), Data::MentionMap(), TextRendering::LinksVisible::DONT_SHOW_LINKS, TextRendering::ProcessLineFeeds::REMOVE_LINE_FEEDS);
        text_->init(_font, _color, QColor(), QColor(), QColor(), _textAlign, 1);

        setSizePolicy(QSizePolicy::Maximum, QSizePolicy::Fixed);
    }

    ClickableTextWidget::ClickableTextWidget(QWidget* _parent, const QFont& _font, const Styling::StyleVariable _color, const TextRendering::HorAligment _textAlign)
        : ClickableTextWidget(_parent, _font, Styling::getParameters().getColor(_color), _textAlign)
    {
    }

    void ClickableTextWidget::setText(const QString& _text)
    {
        text_->setText(_text);
        text_->setOffsets(leftPadding_, height() / 2);
        fullWidth_ = text_->desiredWidth();

        updateGeometry();
        elideText();
    }

    QSize ClickableTextWidget::sizeHint() const
    {
        return QSize(fullWidth_  + leftPadding_, height());
    }

    void ClickableTextWidget::setColor(const QColor& _color)
    {
        text_->setColor(_color);
        update();
    }

    void ClickableTextWidget::setColor(const Styling::StyleVariable _color)
    {
        setColor(Styling::getParameters().getColor(_color));
    }

    void ClickableTextWidget::setFont(const QFont& _font)
    {
        text_->init(_font, text_->getColor(), QColor(), QColor(), QColor(), text_->getAlign(), 1);

        elideText();
    }

    void ClickableTextWidget::setLeftPadding(int _x)
    {
        if (leftPadding_ != _x)
        {
            leftPadding_ = _x;
            setText(text_->getSourceText());
        }
    }

    void ClickableTextWidget::paintEvent(QPaintEvent* _e)
    {
        ClickableWidget::paintEvent(_e);

        QPainter p(this);
        text_->draw(p, TextRendering::VerPosition::MIDDLE);
    }

    void ClickableTextWidget::resizeEvent(QResizeEvent* _e)
    {
        elideText();

        ClickableWidget::resizeEvent(_e);
    }

    void ClickableTextWidget::elideText()
    {
        text_->getHeight(width());
        update();
    }
}
