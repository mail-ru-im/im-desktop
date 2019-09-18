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
    {
        setMouseTracking(true);
        setAttribute(Qt::WA_Hover, true);
        setCursor(Qt::PointingHandCursor);
        setFocusPolicy(Qt::NoFocus);

        tooltipTimer_.setSingleShot(true);
        tooltipTimer_.setInterval(tooltipShowDelay().count());
        connect(&tooltipTimer_, &QTimer::timeout, this, &ClickableWidget::onTooltipTimer);
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
        emit hoverChanged(isHovered_, QPrivateSignal());
    }

    bool ClickableWidget::isPressed() const
    {
        return isPressed_;
    }

    void ClickableWidget::click(const ClickType _how)
    {
        emit clicked(_how, QPrivateSignal());
    }

    void ClickableWidget::setFocusColor(const QColor & _color)
    {
        focusColor_ = _color;
        if (hasFocus())
            update();
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
            emit pressed(QPrivateSignal());
            emit pressChanged(isPressed_, QPrivateSignal());
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
        emit released(QPrivateSignal());
        emit pressChanged(isPressed_, QPrivateSignal());
    }

    void ClickableWidget::paintEvent(QPaintEvent*)
    {
        if (focusColor_.isValid() && (animFocus_.isRunning() || hasFocus()))
        {
            QPainter p(this);
            p.setRenderHints(QPainter::Antialiasing);
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
    }

    void ClickableWidget::enterEvent(QEvent* _e)
    {
        enableTooltip_ = true;
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
            Tooltip::show(getTooltipText(), QRect(mapToGlobal(r.topLeft()), r.size()), { -1, -1 }, Tooltip::ArrowDirection::Down);
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

        animFocus_.finish();
        animFocus_.start([this]() { update(); }, 0.5, 1.0, focusInAnimDuration().count(), anim::sineInOut);
    }

    void ClickableWidget::animateFocusOut()
    {
        if (!isVisible())
            return;

        animFocus_.finish();
        animFocus_.start([this]() { update(); }, 1.0, 0.5, focusInAnimDuration().count(), anim::sineInOut);
    }

    ClickableTextWidget::ClickableTextWidget(QWidget* _parent, const QFont& _font, const QColor& _color, const TextRendering::HorAligment _textAlign)
        : ClickableWidget(_parent)
        , fullWidth_(0)
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
        text_->setOffsets(0, height() / 2);
        fullWidth_ = text_->desiredWidth();

        updateGeometry();
        elideText();
    }

    QSize ClickableTextWidget::sizeHint() const
    {
        return QSize(fullWidth_, height());
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
