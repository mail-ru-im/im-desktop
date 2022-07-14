#include "stdafx.h"

#include "StatusTooltip.h"
#include "controls/BigEmojiWidget.h"
#include "controls/TextWidget.h"
#include "styles/ThemeParameters.h"
#include "main_window/MainWindow.h"
#include "main_window/containers/StatusContainer.h"
#include "utils/InterConnector.h"
#include "utils/utils.h"
#include "fonts.h"
#include "main_window/containers/LastseenContainer.h"
#include "StatusUtils.h"

namespace
{
    constexpr auto tooltipDelay() noexcept { return std::chrono::milliseconds(250); }
    constexpr auto animationDuration() noexcept { return std::chrono::milliseconds(150); }
    constexpr auto emojiAnimationDuration() noexcept { return std::chrono::milliseconds(100); }
    constexpr auto emojiAnimationDelay() noexcept { return std::chrono::milliseconds(50); }

    QFont statusDescriptionFont()
    {
        if constexpr (platform::is_apple())
            return Fonts::appFontScaled(15, Fonts::FontWeight::Medium);
        else
            return Fonts::appFontScaled(16);
    }

    auto descriptionFontColor()
    {
        return Styling::ThemeColorKey{ Styling::StyleVariable::TEXT_SOLID };
    }

    auto durationFontColor(bool active = false)
    {
        return Styling::ThemeColorKey{ active ? Styling::StyleVariable::PRIMARY : Styling::StyleVariable::BASE_PRIMARY };
    }

    int textSideMargin()
    {
        return Utils::scale_value(8);
    }

    int tooltipTextMaxWidth()
    {
        return Utils::scale_value(284) - 2 * textSideMargin();
    }

    int tooltipTextMinWidth()
    {
        return Utils::scale_value(104) - 2 * textSideMargin();
    }

    int topModeTopMargin()
    {
        return Utils::scale_value(28);
    }

    int topModeBottomMargin()
    {
        if constexpr (platform::is_apple())
            return Utils::scale_value(12);

        return Utils::scale_value(14);
    }

    int bottomModeTopMargin()
    {
        return Utils::scale_value(60);
    }

    int bottomModeBottomMargin()
    {
        return Utils::scale_value(8);
    }

    int bottomModeEmojiTopMargin()
    {
        return Utils::scale_value(16);
    }

    int tooltipBorderRadius()
    {
        return Utils::scale_value(12);
    }

    int emojiShiftMargin()
    {
        return Utils::scale_value(16);
    }

    QColor backgroundColor()
    {
        static Styling::ColorContainer color = Styling::ThemeColorKey{ Styling::StyleVariable::BASE_GLOBALWHITE_SECONDARY };
        return color.actualColor();
    }

    int arrowHeight()
    {
        return Utils::scale_value(8);
    }

    int arrowWidth()
    {
        return Utils::scale_value(14);
    }

    QColor shadowColor()
    {
        return QColor(0, 0, 0, 255);
    }

    double shadowColorAlpha()
    {
        return 0.3;
    }

    int shadowMarginV()
    {
        return Utils::scale_value(8);
    }

    int shadowMarginH()
    {
        return Utils::scale_value(16);
    }

    int screenEdgeMargin()
    {
        return Utils::scale_value(8);
    }

    int animationDistance()
    {
        return Utils::scale_value(20);
    }

    constexpr int maxDescriptionLength() noexcept
    {
        return 120;
    }

    constexpr bool shadowEnabled() noexcept
    {
        return !platform::is_windows();
    }

    constexpr bool drawLining() noexcept
    {
        return !shadowEnabled();
    }

    QColor liningColor()
    {
        static Styling::ColorContainer color = Styling::ThemeColorKey{ Styling::StyleVariable::GHOST_SECONDARY };
        return color.actualColor();
    }

    enum class ArrowPosition
    {
        Top,
        Bottom
    };

    void drawTooltip(QPainter& _p, const QRect& _rect, ArrowPosition _arrowPosition, int _arrowX)
    {
        _p.setRenderHint(QPainter::Antialiasing);

        const auto bottomArrow = _arrowPosition == ArrowPosition::Bottom;

        _arrowX = std::max(_rect.left() + arrowWidth() / 2 + tooltipBorderRadius(), std::min(_arrowX, _rect.right() - arrowWidth() / 2 - tooltipBorderRadius()));

        const auto contentRect = bottomArrow ? _rect.adjusted(0, 0, 0, -arrowHeight()) : _rect.adjusted(0, arrowHeight(), 0, 0);
        QPainterPath rectPath;
        rectPath.addRoundedRect(contentRect, tooltipBorderRadius(), tooltipBorderRadius());
        _p.fillPath(rectPath, backgroundColor());

        if (drawLining())
        {
            _p.setPen(liningColor());
            _p.drawPath(rectPath);
        }

        QPainterPath arrowPath;

        const auto arrowBaseY = bottomArrow ? contentRect.bottom() : contentRect.top() + Utils::scale_value(1);
        const auto arrowTipY = bottomArrow ? arrowBaseY + arrowHeight() : arrowBaseY - arrowHeight();

        const auto a = QPoint(_arrowX - arrowWidth() / 2, arrowBaseY);
        const auto b = QPoint(_arrowX,  arrowTipY);
        const auto c = QPoint(_arrowX + arrowWidth() / 2, arrowBaseY);

        arrowPath.moveTo(a);
        arrowPath.lineTo(b);
        arrowPath.lineTo(c);
        arrowPath.closeSubpath();
        _p.fillPath(arrowPath, backgroundColor());

        if (drawLining())
        {
            const auto arrowLiningBaseY = arrowBaseY + Utils::scale_value(bottomArrow ? 1 : -1);

            _p.drawLine(QPoint(a.x(), arrowLiningBaseY), b);
            _p.drawLine(b, QPoint(c.x(), arrowLiningBaseY));
        }
    }
}

namespace Ui
{

class StatusTooltip_p
{
public:
    QString contactId_;
    QPointer<QWidget> object_;
    QPointer<QWidget> parent_;
    StatusTooltip::RectCallback rectCallback_;
    QPointer<StatusTooltipWidget> tooltip_;
    Qt::CursorShape objectCursorShape_ = Qt::PointingHandCursor;
    QTimer* timer_ = nullptr;
};


StatusTooltip::~StatusTooltip() = default;

void StatusTooltip::objectHovered(StatusTooltip::RectCallback _rectCallback,
                                  const QString& _contactId,
                                  QWidget* _object,
                                  QWidget* _parent,
                                  Qt::CursorShape _objectCursorShape)
{
    d->object_ = _object;
    d->parent_ = _parent;
    d->contactId_ = _contactId;
    d->objectCursorShape_ = _objectCursorShape;
    d->rectCallback_ = _rectCallback;

    d->timer_->start();
}

void StatusTooltip::updatePosition()
{
    if (d->tooltip_)
        d->tooltip_->updatePosition();
}

void StatusTooltip::hide()
{
    if (d->tooltip_)
        d->tooltip_->close();
}

StatusTooltip* StatusTooltip::instance()
{
    static StatusTooltip instance;
    return &instance;
}

void StatusTooltip::onTimer()
{
    if (d->object_ && d->object_->isVisible())
    {
        auto objectRect = d->rectCallback_();

        if (!d->tooltip_ && d->object_ && d->object_->isVisible() && objectRect.contains(QCursor::pos()))
        {
            auto status = Logic::GetStatusContainer()->getStatus(d->contactId_);
            const auto isBot = Logic::GetLastseenContainer()->isBot(d->contactId_);
            if (!status.isEmpty() || isBot)
            {
                if (status.isEmpty() && isBot)
                    status = Statuses::getBotStatus();
                d->tooltip_ = new StatusTooltipWidget(status, d->object_, d->parent_, d->objectCursorShape_, !isBot);
                d->tooltip_->setRectCallback(d->rectCallback_);

                d->tooltip_->showAt(objectRect);
            }
        }
    }
}

StatusTooltip::StatusTooltip()
    : d(std::make_unique<StatusTooltip_p>())
{
    d->timer_ = new QTimer(this);
    d->timer_->setSingleShot(true);
    d->timer_->setInterval(tooltipDelay());
    connect(d->timer_, &QTimer::timeout, this, &StatusTooltip::onTimer);
}

class StatusTooltipWidget_p
{
public:
    StatusTooltipWidget_p(QWidget* _q) : q(_q) {}
    QWidget* q = nullptr;
    QRect objectRectGlobal_;
    QRect contentRectGlobal_;
    Qt::CursorShape objectCursorShape_ = Qt::PointingHandCursor;
    StatusTooltip::RectCallback rectCallBack_;
    QWidget* content_ = nullptr;
    BigEmojiWidget* emoji_ = nullptr;
    ArrowPosition arrowPosition_;
    QSpacerItem* topSpacer_ = nullptr;
    QSpacerItem* bottomSpacer_ = nullptr;
    QVariantAnimation* animation_ = nullptr;
    QVariantAnimation* emojiAnimation_ = nullptr;
    QGraphicsDropShadowEffect* shadow_ = nullptr;
    QPointer<QWidget> object_ = nullptr;
    QPointer<QWidget> parent_ = nullptr;
    QTimer* emojiAnimationTimer_ = nullptr;
    QTimer* visibilityTimer_ = nullptr;
    QVBoxLayout* contentLayout_ = nullptr;
    TextWidget* description_ = nullptr;
    TextWidget* duration_ = nullptr;
    QPoint contentPos_;
    QPoint emojiPos_;
    double opacity_ = 0;

    void initTooltipAnimation()
    {
        animation_ = new QVariantAnimation(q);

        double startValue = 0;

        animation_->setStartValue(startValue);
        animation_->setEndValue(1.);
        animation_->setEasingCurve(QEasingCurve::InOutSine);
        animation_->setDuration(animationDuration().count());

        auto onValueChanged = [this](const QVariant& value)
        {
            const auto sign = arrowPosition_ == ArrowPosition::Bottom ? 1 : -1;
            const auto translation = QPoint(0, sign * (1 - value.toDouble()) * animationDistance());
            content_->move(contentPos_ + translation);
            opacity_ = value.toDouble();
            description_->setOpacity(opacity_);
            duration_->setOpacity(opacity_);

            if (shadow_)
                shadow_->setColor(shadowColorWithAlpha());

            q->update();
        };

        onValueChanged(startValue);

        QObject::connect(animation_, &QVariantAnimation::valueChanged, q, onValueChanged);
    }

    void initEmojiAnimation()
    {
        emojiAnimation_ = new QVariantAnimation(q);

        double startValue = 0;

        emojiAnimation_->setStartValue(startValue);
        emojiAnimation_->setEndValue(1.);
        emojiAnimation_->setEasingCurve(QEasingCurve::InOutSine);
        emojiAnimation_->setDuration(emojiAnimationDuration().count());

        auto onValueChanged = [this](const QVariant& value)
        {
            const auto sign = arrowPosition_ == ArrowPosition::Bottom ? 1 : -1;
            const auto translation = QPoint(0, sign * (1 - value.toDouble()) * animationDistance());
            emoji_->setOpacity(value.toDouble());
            emoji_->move(emojiPos_ + translation);
        };

        onValueChanged(startValue);

        QObject::connect(emojiAnimation_, &QVariantAnimation::valueChanged, q, onValueChanged);
        QObject::connect(emojiAnimation_, &QVariantAnimation::finished, q, [this]()
        {
            if (emojiAnimation_->direction() == QVariantAnimation::Backward)
            {
                q->close();
            }
            else
            {
                auto contentGeometry = content_->geometry();
                contentRectGlobal_ = QRect(q->mapToGlobal(contentGeometry.topLeft()), contentGeometry.size());
            }
        });
    }

    bool isHideInProgress()
    {
        return (animation_->state() == QVariantAnimation::Running || emojiAnimation_->state() == QVariantAnimation::Running)
                && animation_->direction() == QVariantAnimation::Backward;
    }

    void startTooltipShowAnimation()
    {
        animation_->setDirection(QVariantAnimation::Forward);
        startTooltipAnimation();
    }

    void startTooltipHideAnimation()
    {
        if (isHideInProgress())
            return;

        animation_->setDirection(QVariantAnimation::Backward);
        startTooltipAnimation();
    }

    void startTooltipAnimation()
    {
        animation_->start();
        emojiAnimationTimer_->start();
    }

    void startEmojiAnimation()
    {
        emojiAnimation_->setDirection(animation_->direction());
        emojiAnimation_->start();
    }

    void handleKeyEvent(QKeyEvent* _event)
    {
        startTooltipHideAnimation();

        if (auto focusWidget = QApplication::focusWidget())
        {
            auto e(*_event);
            QApplication::sendEvent(focusWidget, &e);
        }
    }

    QColor shadowColorWithAlpha()
    {
        auto color = shadowColor();
        color.setAlphaF(shadowColorAlpha() * opacity_);
        return color;
    }

    void initShadow()
    {
        shadow_ = new QGraphicsDropShadowEffect(q);
        shadow_->setBlurRadius(Utils::scale_value(16));
        shadow_->setOffset(0, Utils::scale_value(2));
        shadow_->setColor(shadowColorWithAlpha());

        q->setGraphicsEffect(shadow_);
    }

    void initAnimations()
    {
        initTooltipAnimation();
        initEmojiAnimation();
    }

    QRect objectRect()
    {
        return QRect(q->mapFromGlobal(objectRectGlobal_.topLeft()), objectRectGlobal_.size());
    }
};

StatusTooltipWidget::StatusTooltipWidget(const Statuses::Status& _status, QWidget* _object, QWidget* _parent, Qt::CursorShape _objectCursorShape, bool _showTimeString)
    : QWidget(_parent)
    , d(std::make_unique<StatusTooltipWidget_p>(this))
{
    setWindowFlags(Qt::FramelessWindowHint | Qt::ToolTip | Qt::NoDropShadowWindowHint);
    setAttribute(Qt::WA_TranslucentBackground);
    setAttribute(Qt::WA_DeleteOnClose);
    setMouseTracking(true);
    grabKeyboard();

    d->object_ = _object;
    d->parent_ = _parent;
    d->objectCursorShape_ = _objectCursorShape;

    if (_object)
        _object->installEventFilter(this);

    d->emoji_ = new BigEmojiWidget(_status.toString(), Utils::scale_value(44), this);
    d->content_ = new QWidget(this);
    d->contentLayout_ = Utils::emptyVLayout(d->content_);

    d->topSpacer_ = new QSpacerItem(0, topModeTopMargin());

    auto description = _status.getDescription();
    if (description.size() > maxDescriptionLength())
    {
        const auto ellipsis = getEllipsis();
        description.truncate(maxDescriptionLength() - ellipsis.size());
        description += ellipsis;
    }

    d->description_ = new TextWidget(d->content_, description, Data::MentionMap(), TextRendering::LinksVisible::DONT_SHOW_LINKS);
    TextRendering::TextUnit::InitializeParameters params{ statusDescriptionFont(), descriptionFontColor() };
    params.align_ = TextRendering::HorAligment::CENTER;
    d->description_->init(params);
    d->description_->setMaxWidthAndResize(std::max(std::min(tooltipTextMaxWidth(), d->description_->getDesiredWidth()), tooltipTextMinWidth()));

    d->duration_ = new TextWidget(d->content_, _status.getTimeString());
    params.setFonts(Fonts::appFontScaled(14));
    params.color_ = durationFontColor();
    d->duration_->init(params);
    d->duration_->setMaxWidthAndResize(std::max(std::min(tooltipTextMaxWidth(), d->duration_->getDesiredWidth()), tooltipTextMinWidth()));
    d->duration_->setVisible(_showTimeString);
    d->bottomSpacer_ = new QSpacerItem(0, topModeBottomMargin());

    d->contentLayout_->addSpacerItem(d->topSpacer_);
    d->contentLayout_->addWidget(d->description_, 0, Qt::AlignHCenter);
    d->contentLayout_->addWidget(d->duration_, 0, Qt::AlignHCenter);
    d->contentLayout_->addSpacerItem(d->bottomSpacer_);

    auto contentSize = d->content_->sizeHint();
    d->content_->setFixedSize(contentSize.width() + 2 * textSideMargin(), contentSize.height());
    d->content_->setAttribute(Qt::WA_TransparentForMouseEvents);

    d->emojiAnimationTimer_ = new QTimer(this);
    d->emojiAnimationTimer_->setSingleShot(true);
    d->emojiAnimationTimer_->setInterval(emojiAnimationDelay());
    connect(d->emojiAnimationTimer_, &QTimer::timeout, this, &StatusTooltipWidget::onEmojiAnimationTimer);

    d->visibilityTimer_ = new QTimer(this);
    d->visibilityTimer_->setInterval(100);
    connect(d->visibilityTimer_, &QTimer::timeout, this, &StatusTooltipWidget::onVisibilityTimer);
    d->visibilityTimer_->start();

    Testing::setAccessibleName(this, qsl("AS statusTooltip"));

    if (shadowEnabled())
        d->initShadow();

    d->initAnimations();

    setCursor(d->objectCursorShape_);
}

StatusTooltipWidget::~StatusTooltipWidget() = default;

void StatusTooltipWidget::showAt(const QRect& _objectRect)
{
    d->objectRectGlobal_ = _objectRect;
    auto screen = QGuiApplication::screenAt(_objectRect.center());
    if (!screen)
    {
        deleteLater();
        return;
    }

    const auto screenGeometry = screen->geometry();
    QPoint topLeft;

    // screen geometry constraints
    auto left = std::max(_objectRect.center().x() - d->content_->width() / 2 - shadowMarginH(), screenGeometry.left() + screenEdgeMargin() - shadowMarginH());
    left = std::min(left, screenGeometry.right() - screenEdgeMargin() - d->content_->width() - shadowMarginH());

    topLeft.setX(left);

    // calc height for top position
    auto totalHeight = d->content_->height() + emojiShiftMargin();

    auto fitsInScreen = _objectRect.top() + arrowHeight() - totalHeight > screenGeometry.top();
    auto fitsInParentRect = d->parent_ ? d->parent_->mapFromGlobal(_objectRect.topLeft()).y() >= 0 : true;

    if (fitsInScreen && fitsInParentRect)
    {
        setFixedSize(d->content_->width() + 2 * shadowMarginH(), totalHeight + 2 * shadowMarginV());
        d->arrowPosition_ = ArrowPosition::Bottom;
        d->contentPos_ = {shadowMarginH(), shadowMarginV() + emojiShiftMargin()};
        d->emojiPos_ = {(d->content_->width() - d->emoji_->width()) / 2 + shadowMarginH(), shadowMarginV()};

        topLeft.setY(_objectRect.top() - totalHeight - 2 * shadowMarginV());
    }
    else
    {
        d->topSpacer_->changeSize(0, bottomModeTopMargin());
        d->bottomSpacer_->changeSize(0, bottomModeBottomMargin());
        d->contentLayout_->invalidate();

        d->content_->setFixedHeight(d->content_->sizeHint().height());
        setFixedSize(d->content_->width() + 2 * shadowMarginH(), d->content_->height() + 2 * shadowMarginV());
        d->arrowPosition_ = ArrowPosition::Top;
        d->contentPos_ = {shadowMarginH(), shadowMarginV()};
        d->emojiPos_ = {(d->content_->width() - d->emoji_->width()) / 2 + shadowMarginH(), shadowMarginV() + bottomModeEmojiTopMargin()};

        topLeft.setY(_objectRect.bottom());
    }

    move(topLeft);
    d->startTooltipShowAnimation();
    show();
}

void StatusTooltipWidget::setRectCallback(StatusTooltip::RectCallback _callback)
{
    d->rectCallBack_ = std::move(_callback);
}

void StatusTooltipWidget::updatePosition()
{
    if (d->object_ && d->rectCallBack_)
    {
        auto objectRect = d->rectCallBack_();
        auto diff = objectRect.topLeft() - d->objectRectGlobal_.topLeft();

        if (diff.manhattanLength() > 0)
            close();
    }
}

void StatusTooltipWidget::paintEvent(QPaintEvent* _event)
{
    QPainter p(this);

    p.setOpacity(d->opacity_);
    drawTooltip(p, d->content_->geometry(), d->arrowPosition_, d->objectRect().center().x());
}

void StatusTooltipWidget::mouseMoveEvent(QMouseEvent* _event)
{
    if (d->objectRectGlobal_.contains(_event->globalPos()))
        setCursor(d->objectCursorShape_);
    else if (d->contentRectGlobal_.contains(_event->globalPos()))
        setCursor(Qt::ArrowCursor);

    if (d->object_ && d->objectRectGlobal_.contains(_event->globalPos()))
    {
        QMouseEvent e(QEvent::MouseMove, d->object_->mapFromGlobal(_event->globalPos()), _event->button(), _event->buttons(), _event->modifiers());
        QApplication::sendEvent(d->object_, &e);
    }
}

void StatusTooltipWidget::mousePressEvent(QMouseEvent* _event)
{
    if (d->object_ && d->objectRectGlobal_.contains(_event->globalPos()))
    {
        QMouseEvent e(QEvent::MouseButtonPress, d->object_->mapFromGlobal(_event->globalPos()), _event->button(), _event->buttons(), _event->modifiers());
        QApplication::sendEvent(d->object_, &e);
    }
}

void StatusTooltipWidget::mouseReleaseEvent(QMouseEvent* _event)
{
    if (d->object_ && d->objectRectGlobal_.contains(_event->globalPos()))
    {
        QMouseEvent e(QEvent::MouseButtonRelease, d->object_->mapFromGlobal(_event->globalPos()), _event->button(), _event->buttons(), _event->modifiers());
        QApplication::sendEvent(d->object_, &e);
    }
    close();
}

void StatusTooltipWidget::wheelEvent(QWheelEvent* _event)
{
    if (d->object_ && d->objectRectGlobal_.contains(_event->globalPos()))
    {
        QWheelEvent e(d->object_->mapFromGlobal(_event->globalPos()), _event->delta(), _event->buttons(), _event->modifiers());
        QApplication::sendEvent(d->object_, &e);
    }
}

void StatusTooltipWidget::onEmojiAnimationTimer()
{
    d->startEmojiAnimation();
}

void StatusTooltipWidget::onVisibilityTimer()
{
    auto pos = mapFromGlobal(QCursor::pos());
    if (!d->objectRect().contains(pos))
        d->startTooltipHideAnimation();
}

bool StatusTooltipWidget::eventFilter(QObject* _object, QEvent* _event)
{
    if (_object == d->object_ && (_event->type() == QEvent::Wheel || _event->type() == QEvent::Move))
        QMetaObject::invokeMethod(this, &StatusTooltipWidget::updatePosition, Qt::QueuedConnection);
    else if (_object == d->object_ && _event->type() == QEvent::MouseButtonRelease)
        close();

    return QObject::eventFilter(_object, _event);
}

void StatusTooltipWidget::keyPressEvent(QKeyEvent* _event)
{
    d->handleKeyEvent(_event);
    QWidget::keyPressEvent(_event);
}

void StatusTooltipWidget::keyReleaseEvent(QKeyEvent* _event)
{
    d->handleKeyEvent(_event);
    QWidget::keyPressEvent(_event);
}

}
