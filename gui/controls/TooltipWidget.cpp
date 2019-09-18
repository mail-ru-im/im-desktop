#include "stdafx.h"
#include "TooltipWidget.h"
#include "CustomButton.h"
#include "../fonts.h"
#include "../utils/utils.h"
#include "../utils/InterConnector.h"
#include "../styles/ThemeParameters.h"
#include "../main_window/MainWindow.h"
#include "../main_window/history_control/MessageStyle.h"

namespace
{
    QColor getBackgroundColor()
    {
        return Styling::getParameters().getColor(Styling::StyleVariable::BASE_GLOBALWHITE);
    }

    QColor getLineColor()
    {
        return Styling::getParameters().getColor(Styling::StyleVariable::GHOST_SECONDARY);
    }

    int getGradientWidth()
    {
        return Utils::scale_value(20);
    }

    int getOutSpace()
    {
        return Utils::scale_value(8);
    }

    int getCloseButtonSize()
    {
        return Utils::scale_value(20);
    }

    int getMinArrowOffset(bool _big)
    {
        return _big ? Utils::scale_value(6) : Utils::scale_value(4);
    }

    int getCloseButtonOffset()
    {
        return Utils::scale_value(10);
    }

    int getMaxTextTooltipHeight()
    {
        return Utils::scale_value(260);
    }

    int getVerScrollbarWidth()
    {
        return Utils::scale_value(4);
    }

    int getInvertedOffset()
    {
        return Utils::scale_value(-4);
    }

    int tooltipInterval()
    {
        return 500;
    }

    int getMinWidth()
    {
        return Utils::scale_value(36);
    }

    constexpr std::chrono::milliseconds getDurationAppear() noexcept { return std::chrono::milliseconds(200); }
    constexpr std::chrono::milliseconds getDurationDisappear() noexcept { return std::chrono::milliseconds(50); }
}

namespace Tooltip
{
    using namespace Ui;

    int getCornerRadiusBig()
    {
        return Utils::scale_value(6);
    }

    int getArrowHeight()
    {
        return Utils::scale_value(4);
    }

    int getShadowSize()
    {
        return Utils::scale_value(8);
    }

    int getDefaultArrowOffset()
    {
        return Utils::scale_value(20);
    }

    int getMaxMentionTooltipHeight()
    {
        return Utils::scale_value(240);
    }

    int getMaxMentionTooltipWidth()
    {
        return MessageStyle::getHistoryWidgetMaxWidth() + 2 * Tooltip::getShadowSize();
    }

    int getMentionArrowOffset()
    {
        return Utils::scale_value(10);
    }

    void drawTooltip(QPainter& _p, const QRect& _tooltipRect, const int _arrowOffset, const ArrowDirection _direction)
    {
        static const Utils::SvgLayers layers =
        {
            { qsl("borderbg"), getBackgroundColor() },
            { qsl("border"), getLineColor() },
            { qsl("bg"), getBackgroundColor() },
        };
        static const auto leftTop = Utils::renderSvgLayered(qsl(":/tooltip/corner"), layers);
        static const auto rightTop = Utils::mirrorPixmapHor(leftTop);
        static const auto rightBottom = Utils::mirrorPixmapVer(rightTop);
        static const auto leftBottom = Utils::mirrorPixmapVer(leftTop);
        static const auto topSide = Utils::renderSvgLayered(qsl(":/tooltip/side"), layers);
        static const auto rightSide = topSide.transformed(QTransform().rotate(90));
        static const auto bottomSide = Utils::mirrorPixmapVer(topSide);
        static const auto leftSide = topSide.transformed(QTransform().rotate(-90));

        static const auto arrowTop = Utils::renderSvgLayered(qsl(":/tooltip/tongue"), layers);
        static const auto arrowBottom = Utils::mirrorPixmapVer(arrowTop);

        assert(_direction != ArrowDirection::Auto);
        const auto& arrowP = _direction == ArrowDirection::Up ? arrowTop : arrowBottom;

        const auto arrowH = _direction != ArrowDirection::None ? getArrowHeight() : 0;
        const auto rcBubble = _tooltipRect.adjusted(0, 0, 0, -arrowH);

        const QRect fillRect(
            rcBubble.left() + Utils::unscale_bitmap(leftTop.width()),
            rcBubble.top() + Utils::unscale_bitmap(leftTop.height()),
            rcBubble.width() - Utils::unscale_bitmap(leftTop.width()) - Utils::unscale_bitmap(rightTop.width()),
            rcBubble.height() - Utils::unscale_bitmap(leftTop.height()) - Utils::unscale_bitmap(leftBottom.height()));

        Utils::PainterSaver ps(_p);
        _p.fillRect(fillRect, getBackgroundColor());

        // corners
        _p.drawPixmap(QRect(0, 0, Utils::unscale_bitmap(leftTop.width()), Utils::unscale_bitmap(leftTop.height())), leftTop);
        _p.drawPixmap(QRect(rcBubble.width() - Utils::unscale_bitmap(rightTop.width()), 0, Utils::unscale_bitmap(rightTop.width()), Utils::unscale_bitmap(rightTop.height())), rightTop);
        _p.drawPixmap(QRect(rcBubble.width() - Utils::unscale_bitmap(rightBottom.width()), rcBubble.height() - Utils::unscale_bitmap(rightBottom.height()),
            Utils::unscale_bitmap(rightBottom.width()), Utils::unscale_bitmap(rightBottom.height())), rightBottom);
        _p.drawPixmap(QRect(0, rcBubble.height() - Utils::unscale_bitmap(leftBottom.height()), Utils::unscale_bitmap(leftBottom.width()), Utils::unscale_bitmap(leftBottom.height())), leftBottom);

        // lines
        _p.drawPixmap(
            QRect(
                Utils::unscale_bitmap(leftTop.width()),
                0,
                rcBubble.width() - Utils::unscale_bitmap(leftTop.width()) - Utils::unscale_bitmap(rightTop.width()),
                Utils::unscale_bitmap(topSide.height())
            ), topSide);

        _p.drawPixmap(
            QRect(
                Utils::unscale_bitmap(leftTop.width()),
                rcBubble.height() - Utils::unscale_bitmap(bottomSide.height()),
                rcBubble.width() - Utils::unscale_bitmap(leftBottom.width()) - Utils::unscale_bitmap(rightBottom.width()),
                Utils::unscale_bitmap(bottomSide.height())
            ), bottomSide);

        _p.drawPixmap(QRect(rcBubble.width() - Utils::unscale_bitmap(rightSide.width()), Utils::unscale_bitmap(rightTop.height()), Utils::unscale_bitmap(rightSide.width()),
            rcBubble.height() - Utils::unscale_bitmap(rightTop.height()) - Utils::unscale_bitmap(rightBottom.height())), rightSide);
        _p.drawPixmap(QRect(0, Utils::unscale_bitmap(leftTop.height()), Utils::unscale_bitmap(leftSide.width()), rcBubble.height() - Utils::unscale_bitmap(leftTop.height()) - Utils::unscale_bitmap(leftBottom.height())), leftSide);

        // arrow
        if (_direction != ArrowDirection::None)
            _p.drawPixmap(QRect(_arrowOffset, _direction == ArrowDirection::Up ? (getInvertedOffset()) : (rcBubble.height() - Utils::unscale_bitmap(bottomSide.height())), Utils::unscale_bitmap(arrowP.width()), Utils::unscale_bitmap(arrowP.height())), arrowP);
    }

    void drawBigTooltip(QPainter& _p, const QRect& _tooltipRect, const int _arrowOffset, const ArrowDirection _direction)
    {
        Utils::PainterSaver ps(_p);
        _p.setRenderHint(QPainter::Antialiasing);
        _p.setPen(Qt::NoPen);

        const auto bgColor = Styling::getParameters().getColor(Styling::StyleVariable::BASE_GLOBALWHITE);
        const auto lineColor = Styling::getParameters().getColor(Styling::StyleVariable::BASE_BRIGHT_ACTIVE);
        const auto radius = getCornerRadiusBig();
        const auto arrowH = _direction != ArrowDirection::None ? getArrowHeight() : 0;
        const auto bubbleRect = _tooltipRect.adjusted(
            Tooltip::getShadowSize(),
            Tooltip::getShadowSize() + (_direction == ArrowDirection::Down ? 0 : arrowH),
            -Tooltip::getShadowSize(),
            -Tooltip::getShadowSize() + (_direction == ArrowDirection::Down ? -arrowH : 0)
        );

        const auto lineWidth = Utils::scale_value(1);
        const auto outlineRect = bubbleRect.adjusted(-lineWidth, -lineWidth, lineWidth, lineWidth);
        _p.setBrush(bgColor);
        _p.drawRoundedRect(outlineRect, radius, radius);
        _p.setBrush(lineColor);
        _p.drawRoundedRect(outlineRect, radius, radius);

        _p.setBrush(bgColor);
        _p.drawRoundedRect(bubbleRect, radius, radius);

        if (_direction != ArrowDirection::None)
        {
            assert(_direction != ArrowDirection::Auto);

            static const Utils::SvgLayers layers =
            {
                { qsl("border"), lineColor },
                { qsl("bg"), bgColor },
            };
            static const auto arrowBottom = Utils::renderSvgLayered(qsl(":/tooltip/tongue_big"), layers);
            static const auto arrowTop = Utils::mirrorPixmapVer(arrowBottom);
            const auto x = _arrowOffset;
            const auto y = _direction == ArrowDirection::Down
                ? bubbleRect.bottom() + 1
                : bubbleRect.top() - arrowTop.height() / Utils::scale_bitmap_ratio();
            _p.drawPixmap(x, y, _direction == ArrowDirection::Down ? arrowBottom : arrowTop);
        }
    }

    std::unique_ptr<TextTooltip> g_tooltip;

    TextTooltip* getDefaultTooltip()
    {
        if (!g_tooltip)
            g_tooltip = std::make_unique<TextTooltip>(nullptr, true);

        return g_tooltip.get();
    }

    void resetDefaultTooltip()
    {
        g_tooltip.reset();
    }

    void show(const QString& _text, const QRect& _objectRect, const QSize& _maxSize, ArrowDirection _direction, Tooltip::ArrowPointPos _arrowPos)
    {
        auto t = getDefaultTooltip();
        t->setPointWidth(_objectRect.width());
        QRect r;
        if (auto w = Utils::InterConnector::instance().getMainWindow())
            r = QRect(w->mapToGlobal(w->rect().topLeft()), w->mapToGlobal(w->rect().bottomRight()));

        t->showTooltip(_text, _objectRect, _maxSize.isValid() ? _maxSize : QSize(-1, getMaxTextTooltipHeight()), r, _direction, _arrowPos);
    }

    void forceShow(bool _force)
    {
        getDefaultTooltip()->setForceShow(_force);
    }

    void hide()
    {
        getDefaultTooltip()->hideTooltip(true);
    }

    QString text()
    {
        return getDefaultTooltip()->getText();
    }

    bool isVisible()
    {
        return getDefaultTooltip()->isTooltipVisible();
    }

    bool canWheel()
    {
        return getDefaultTooltip()->canWheel();
    }

    void wheel(QWheelEvent* _e)
    {
        return getDefaultTooltip()->wheel(_e);
    }
}

namespace Ui
{
    GradientWidget::GradientWidget(QWidget* _parent, const QColor& _left, const QColor& _right)
        : QWidget(_parent)
        , left_(_left)
        , right_(_right)
    {
        setAttribute(Qt::WA_TransparentForMouseEvents);
    }

    void GradientWidget::updateColors(const QColor& _left, const QColor& _right)
    {
        left_ = _left;
        right_ = _right;
        update();
    }

    void GradientWidget::paintEvent(QPaintEvent* _event)
    {
        QLinearGradient gradient(rect().topLeft(), rect().topRight());
        gradient.setColorAt(0, left_);
        gradient.setColorAt(1, right_);

        QPainter p(this);
        p.fillRect(rect(), gradient);
    }

    TooltipWidget::TooltipWidget(QWidget* _parent, QWidget* _content, int _pointWidth, const QMargins& _margins, bool _canClose, bool _bigArrow, bool _horizontalScroll)
        : QWidget(_parent)
        , contentWidget_(_content)
        , buttonClose_(nullptr)
        , animScroll_(nullptr)
        , arrowOffset_(Tooltip::getDefaultArrowOffset())
        , pointWidth_(_pointWidth)
        , gradientRight_(new GradientWidget(this, Qt::transparent, getBackgroundColor()))
        , gradientLeft_(new GradientWidget(this, getBackgroundColor(), Qt::transparent))
        , opacityEffect_(new QGraphicsOpacityEffect(this))
        , canClose_(_canClose)
        , bigArrow_(_bigArrow)
        , isCursorOver_(false)
        , horizontalScroll_(_horizontalScroll)
        , arrowInverted_(false)
        , margins_(_margins)
        , isArrowVisible_(true)
    {
        scrollArea_ = new QScrollArea(this);
        scrollArea_->setStyleSheet(qsl("background: transparent;"));
        scrollArea_->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
        scrollArea_->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);

        contentWidget_->setParent(scrollArea_);
        scrollArea_->setWidget(contentWidget_);
        scrollArea_->setFocusPolicy(Qt::NoFocus);
        scrollArea_->setFrameShape(QFrame::NoFrame);
        Utils::ApplyStyle(scrollArea_->verticalScrollBar(), Styling::getParameters().getScrollBarQss(4, 0));

        auto margins = getMargins();
        scrollArea_->move(margins.left() + Tooltip::getShadowSize(), margins.top() + Tooltip::getShadowSize());

        Utils::grabTouchWidget(scrollArea_->viewport(), true);
        Utils::grabTouchWidget(contentWidget_);

        if (canClose_)
        {
            buttonClose_ = new CustomButton(this, qsl(":/controls/close_icon"), QSize(12, 12));
            Styling::Buttons::setButtonDefaultColors(buttonClose_);
            buttonClose_->setFixedSize(getCloseButtonSize(), getCloseButtonSize());
            QObject::connect(buttonClose_, &CustomButton::clicked, this, &TooltipWidget::closeButtonClicked);
        }

        setFocusPolicy(Qt::NoFocus);

        QObject::connect(scrollArea_->horizontalScrollBar(), &QScrollBar::valueChanged, this, &TooltipWidget::onScroll);

        setGraphicsEffect(opacityEffect_);
    }

    void TooltipWidget::closeButtonClicked(bool)
    {
        hideAnimated();
    }

    void TooltipWidget::paintEvent(QPaintEvent*)
    {
        const auto aDir = isArrowVisible_
            ? (arrowInverted_ ? Tooltip::ArrowDirection::Up : Tooltip::ArrowDirection::Down)
            : Tooltip::ArrowDirection::None;

        QPainter p(this);
        if (bigArrow_)
            drawBigTooltip(p, rect(), arrowOffset_, aDir);
        else
            drawTooltip(p, rect(), arrowOffset_, aDir);
    }

    void TooltipWidget::wheelEvent(QWheelEvent* _e)
    {
        if (!horizontalScroll_)
            return;

        const int numDegrees = _e->delta() / 8;
        const int numSteps = numDegrees / 15;

        if (!numSteps || !numDegrees)
            return;

        if (numSteps > 0)
            scrollStep(Direction::left);
        else
            scrollStep(Direction::right);

        _e->accept();
    }

    void TooltipWidget::enterEvent(QEvent* _e)
    {
        isCursorOver_ = true;
        return QWidget::enterEvent(_e);
    }

    void TooltipWidget::leaveEvent(QEvent* _e)
    {
        isCursorOver_ = false;
        return QWidget::leaveEvent(_e);
    }

    void TooltipWidget::scrollStep(Direction _direction)
    {
        QRect viewRect = scrollArea_->viewport()->geometry();
        auto scrollbar = scrollArea_->horizontalScrollBar();

        int maxVal = scrollbar->maximum();
        int minVal = scrollbar->minimum();
        int curVal = scrollbar->value();

        int step = viewRect.width() / 2;

        int to = 0;

        if (_direction == TooltipWidget::Direction::right)
        {
            to = curVal + step;
            if (to > maxVal)
            {
                to = maxVal;
            }
        }
        else
        {
            to = curVal - step;
            if (to < minVal)
            {
                to = minVal;
            }

        }

        QEasingCurve easing_curve = QEasingCurve::InQuad;
        int duration = 300;

        if (!animScroll_)
            animScroll_ = new QPropertyAnimation(scrollbar, QByteArrayLiteral("value"), this);

        animScroll_->stop();
        animScroll_->setDuration(duration);
        animScroll_->setStartValue(curVal);
        animScroll_->setEndValue(to);
        animScroll_->setEasingCurve(easing_curve);
        animScroll_->start();
    }

    QMargins TooltipWidget::getMargins() const
    {
        return margins_;
    }

    bool TooltipWidget::isScrollVisible() const
    {
        return scrollArea_->verticalScrollBar()->isVisible() || scrollArea_->horizontalScrollBar()->isVisible();
    }

    void TooltipWidget::wheel(QWheelEvent* _e)
    {
        QApplication::sendEvent(scrollArea_->verticalScrollBar(), _e);
    }

    void TooltipWidget::scrollToTop()
    {
        scrollArea_->horizontalScrollBar()->setValue(0);
    }

    void TooltipWidget::ensureVisible(const int _x, const int _y, const int _xmargin, const int _ymargin)
    {
        scrollArea_->ensureVisible(_x, _y, _xmargin, _ymargin);
    }

    void TooltipWidget::setArrowVisible(const bool _visible)
    {
        isArrowVisible_ = _visible;
        update();
    }

    void TooltipWidget::showAnimated(const QPoint _pos, const QSize& _maxSize, const QRect& _rect, Tooltip::ArrowDirection _direction)
    {
        opacityEffect_->setOpacity(0.0);

        opacityAnimation_.finish();
        opacityAnimation_.start(
            [this]() { opacityEffect_->setOpacity(opacityAnimation_.current()); },
            0.0,
            1.0,
            getDurationAppear().count());

        showUsual(_pos, _maxSize, _rect, _direction);
    }

    void TooltipWidget::showUsual(const QPoint _pos, const QSize& _maxSize, const QRect& _rect, Tooltip::ArrowDirection _direction)
    {
        const int margin_left = getOutSpace();
        const int margin_right = getOutSpace();

        auto s = contentWidget_->size();
        int need_width = s.width();

        auto margins = getMargins();
        auto add_width = margins.left() + margins.right() + Tooltip::getShadowSize() * 2;
        if (canClose_)
            add_width += getCloseButtonOffset();

        need_width += add_width;

        int w = need_width;

        bool showGradient = false;

        if (_maxSize.width() != -1)
        {
            const int maxW = _maxSize.width() - margin_left - margin_right;
            if (need_width > maxW)
            {
                w = maxW;

                showGradient = true;
            }
        }

        auto addHeight = Tooltip::getArrowHeight() + margins.top() + margins.bottom() + Tooltip::getShadowSize() * 2;
        auto h = s.height() + addHeight;
        if (_maxSize.height() != -1 && h > _maxSize.height())
        {
            h = _maxSize.height();
            w += getVerScrollbarWidth();
        }

        w = std::max(w, getMinWidth());
        if (_rect.isValid())
            w = std::min(w, _rect.width());

        setFixedSize(w, h);
        scrollArea_->setFixedSize(w - add_width, h - addHeight);

        if (buttonClose_)
            buttonClose_->move(w - getCloseButtonSize() - Tooltip::getShadowSize(), Tooltip::getShadowSize());

        const int pointCenterX = _pos.x() + pointWidth_ / 2;

        int xPos = pointCenterX - w / 2;

        if (xPos < margin_left)
            xPos = margin_left;

        if (_maxSize.width() != -1 && w > _maxSize.width() - 2 * margin_right - 1 && xPos + w > _maxSize.width() - margin_right)
            xPos = std::abs(parentWidget()->width() - w) / 2;

        if (_rect.isValid())
            xPos = std::max(xPos, _rect.left());

        int yOffset = 0;
        arrowInverted_ = false;
        if (_rect.isValid())
        {
            auto topR = _rect.topRight();
            const auto rightEdge = topR.x();
            if (xPos + w > rightEdge)
            {
                xPos -= (xPos + w - rightEdge);
            }

            if (_direction == Tooltip::ArrowDirection::Up || (_direction == Tooltip::ArrowDirection::Auto && (_pos.y() - height() < topR.y())))
            {
                yOffset = height() + pointWidth_ + Tooltip::getArrowHeight();
                arrowInverted_ = true;
            }
        }

        arrowOffset_ = pointCenterX - xPos - getMinArrowOffset(bigArrow_);

        move(QPoint(xPos, _pos.y() - height() + yOffset));

        gradientRight_->setVisible(showGradient);

        if (showGradient)
            gradientRight_->raise();

        gradientLeft_->setVisible(false);

        show();
    }

    void TooltipWidget::hideAnimated()
    {
        opacityAnimation_.finish();
        opacityAnimation_.start(
            [this]() { opacityEffect_->setOpacity(opacityAnimation_.current()); },
            [this]() { hide(); },
            1.0,
            0.0,
            getDurationDisappear().count());
    }

    void TooltipWidget::setPointWidth(int _width)
    {
        pointWidth_ = _width;
    }

    void TooltipWidget::resizeEvent(QResizeEvent * _e)
    {
        const auto rcArea = scrollArea_->geometry();

        gradientRight_->setGeometry(rcArea.right() + 1 - getGradientWidth(), rcArea.top(), getGradientWidth(), rcArea.height());
        gradientLeft_->setGeometry(rcArea.left(), rcArea.top(), getGradientWidth(), rcArea.height());

        QWidget::resizeEvent(_e);
    }

    void TooltipWidget::onScroll(int _value)
    {
        int maximum = scrollArea_->horizontalScrollBar()->maximum();

        bool gradientLeftVisible = true;
        bool gradientRightVisible = true;

        if (_value == 0)
        {
            gradientLeftVisible = false;
        }
        else if (_value == maximum)
        {
            gradientRightVisible = false;
        }

        if (gradientLeftVisible != gradientLeft_->isVisible())
        {
            gradientLeft_->setVisible(gradientLeftVisible);
            if (gradientLeftVisible)
                gradientLeft_->raise();
        }

        if (gradientRightVisible != gradientRight_->isVisible())
        {
            gradientRight_->setVisible(gradientRightVisible);
            if (gradientRightVisible)
                gradientRight_->raise();
        }
    }

    void TextWidget::setMaxWidth(int _width)
    {
        maxWidth_ = _width;
        text_->getHeight(maxWidth_);
        update();
    }

    void TextWidget::setMaxWidthAndResize(int _width)
    {
        maxWidth_ = _width;
        text_->getHeight(maxWidth_);
        setFixedSize(text_->cachedSize());
        update();
    }

    void TextWidget::setText(const QString& _text, const QColor& _color)
    {
        text_->setText(_text, _color);
        text_->getHeight(maxWidth_ ? maxWidth_ : text_->desiredWidth());
        setFixedSize(text_->cachedSize());
        update();
    }

    void TextWidget::setOpacity(qreal _opacity)
    {
        opacity_ = _opacity;
        update();
    }

    QString TextWidget::getText() const
    {
        return text_->getText();
    }

    void TextWidget::paintEvent(QPaintEvent* _e)
    {
        QPainter p(this);
        p.setOpacity(opacity_);
        text_->draw(p);
    }

    TextTooltip::TextTooltip(QWidget* _parent, bool _general)
        : QWidget(_parent)
        , text_(nullptr)
        , tooltip_(nullptr)
        , forceShow_(false)
    {
        text_ = new TextWidget(_parent, QString(), Data::MentionMap(), TextRendering::LinksVisible::DONT_SHOW_LINKS,
                                                                       TextRendering::ProcessLineFeeds::KEEP_LINE_FEEDS,
                                                                       TextRendering::EmojiSizeType::TOOLTIP);
        text_->init(Fonts::appFontScaled(11), Styling::getParameters().getColor(Styling::StyleVariable::TEXT_SOLID));
        tooltip_ = new TooltipWidget(_parent, text_, 0, QMargins(Utils::scale_value(8), Utils::scale_value(4), Utils::scale_value(8), Utils::scale_value(4)), false, false);
        if (_general)
        {
            tooltip_->setWindowFlags(Qt::ToolTip | Qt::FramelessWindowHint | Qt::NoDropShadowWindowHint);
            tooltip_->setAttribute(Qt::WA_TranslucentBackground);
            tooltip_->setAttribute(Qt::WA_NoSystemBackground);
        }

        timer_.setInterval(tooltipInterval());
        connect(&timer_, &QTimer::timeout, this, &TextTooltip::check);
    }

    void TextTooltip::setPointWidth(int _width)
    {
        tooltip_->setPointWidth(_width);
    }

    void TextTooltip::showTooltip(const QString& _text, const QRect& _objectRect, const QSize& _maxSize, const QRect& _rect, Tooltip::ArrowDirection _direction, Tooltip::ArrowPointPos _arrowPos)
    {
        current_ = _text;
        text_->setText(_text);

        QPoint p;
        switch (_arrowPos)
        {
        case Tooltip::ArrowPointPos::Top:
            p = _objectRect.topLeft();
            break;
        case Tooltip::ArrowPointPos::Center:
            p = QPoint(_objectRect.left(), _objectRect.top() + _objectRect.height() / 2);
            break;
        case Tooltip::ArrowPointPos::Bottom:
            p = _objectRect.bottomLeft();
            break;
        default:
            break;
        }

        if (auto w = qobject_cast<QWidget*>(parent()))
            p = w->mapFromGlobal(p);

        if (isTooltipVisible())
            tooltip_->showUsual(p, _maxSize, _rect, _direction);
        else
            tooltip_->showAnimated(p, _maxSize, _rect, _direction);

        objectRect_ = _objectRect;
        timer_.start();
    }

    void TextTooltip::hideTooltip(bool _force)
    {
        if (_force)
            tooltip_->hide();
        else
            tooltip_->hideAnimated();
    }

    QString TextTooltip::getText() const
    {
        return current_;
    }

    bool TextTooltip::isTooltipVisible() const
    {
        return tooltip_->isVisible();
    }

    bool TextTooltip::canWheel()
    {
        return isTooltipVisible() && tooltip_->isScrollVisible();
    }

    void TextTooltip::wheel(QWheelEvent* _e)
    {
        tooltip_->wheel(_e);
    }

    void TextTooltip::check()
    {
        if (objectRect_.contains(QCursor::pos()) || tooltip_->needToShow() || forceShow_)
            return;

        hideTooltip();
        timer_.stop();
        objectRect_ = QRect();
    }

    void TextTooltip::setForceShow(bool _force)
    {
        forceShow_ = _force;
    }
}
