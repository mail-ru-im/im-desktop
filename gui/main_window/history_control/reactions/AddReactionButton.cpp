#include "stdafx.h"
#include "styles/ThemeParameters.h"
#include "utils/DrawUtils.h"
#include "utils/utils.h"
#include "utils/InterConnector.h"
#include "../MessageStyle.h"
#include "../HistoryControlPageItem.h"
#include "main_window/contact_list/ContactListModel.h"

#include "AddReactionButton.h"

namespace
{

constexpr std::chrono::milliseconds animationDuration = std::chrono::milliseconds(150);
constexpr std::chrono::milliseconds hoverDelay = std::chrono::milliseconds(50);

QSize buttonSize()
{
    return Ui::MessageStyle::Plates::addReactionButtonSize();
}

QSize iconSize()
{
    return Utils::scale_value(QSize(20, 20));
}

int32_t reactionButtonOffsetX()
{
    return Utils::scale_value(6);
}

QColor shadowColor()
{
    return QColor(0, 0, 0, 255);
}

double shadowColorAlpha()
{
    return 0.12;
}

QPixmap reactionButtonIcon(bool _outgoing, bool _pressed)
{
    if (_pressed)
    {
        static auto icon = Utils::StyledPixmap(qsl(":/reaction_icon"), iconSize(), Styling::ThemeColorKey{ Styling::StyleVariable::BASE_GLOBALWHITE });
        return icon.actualPixmap();
    }

    if (_outgoing)
    {
        static auto icon = Utils::StyledPixmap(qsl(":/reaction_icon"), iconSize(), Styling::ThemeColorKey{ Styling::StyleVariable::PRIMARY_PASTEL });
        return icon.actualPixmap();
    }

    static auto icon = Utils::StyledPixmap(qsl(":/reaction_icon"), iconSize(), Styling::ThemeColorKey{ Styling::StyleVariable::BASE_SECONDARY });
    return icon.actualPixmap();
}

QColor backgroundColor(bool _outgoing, bool _hovered, bool _pressed)
{
    if (_pressed)
        return Styling::getParameters().getColor(Styling::StyleVariable::PRIMARY);

    if (_outgoing)
    {
        if (_hovered)
            return Styling::getParameters().getColor(Styling::StyleVariable::CHAT_SECONDARY);

        return Styling::getParameters().getColor(Styling::StyleVariable::CHAT_SECONDARY);
    }
    else
    {
        if (_hovered)
            return Styling::getParameters().getColor(Styling::StyleVariable::CHAT_PRIMARY);

        return Styling::getParameters().getColor(Styling::StyleVariable::CHAT_PRIMARY_HOVER);
    }
}

int bottomOffsetFromMessage(Ui::ReactionsPlateType _type)
{
    if (_type == Ui::ReactionsPlateType::Media)
        return Utils::scale_value(8);
    else
        return 0;
}

bool reactionButtonAllowed(const QString& _aimId, const Data::MessageBuddy& _msg)
{
    if (Logic::getContactListModel()->isThread(_aimId))
        return _msg.Prev_ != -1; // MVP 1: disable reactions for first message

    if (_msg.threadData_.threadFeedMessage_)
        return false;

    const auto role = Logic::getContactListModel()->getYourRole(_aimId);
    const auto notAllowed = (role.isEmpty() && Utils::isChat(_aimId)) || role == u"notamember" || (role == u"readonly" && !Logic::getContactListModel()->isChannel(_aimId));

    return !notAllowed;
}

QSize pressRectSize()
{
    return Utils::scale_value(QSize(36, 36));
}

}

namespace Ui
{

//////////////////////////////////////////////////////////////////////////
// ReactionButton_p
//////////////////////////////////////////////////////////////////////////

class ReactionButton_p
{
public:

    ReactionButton_p(QWidget* _q)
        : animation_(nullptr)
        , q(_q)
    {
    }

    void ensureAnimationInitialized()
    {
        if (animation_)
            return;

        animation_ = new QVariantAnimation(q);
        animation_->setStartValue(0.0);
        animation_->setEndValue(1.0);
        animation_->setEasingCurve(QEasingCurve::InOutSine);
        animation_->setDuration(animationDuration.count());
        QObject::connect(animation_, &QVariantAnimation::valueChanged, q, [this](const QVariant& value)
        {
            opacity_ = value.toDouble();
            shadow_->setColor(shadowColorWithAlpha());
            q->update();
        });
        QObject::connect(animation_, &QVariantAnimation::finished, q, [this]()
        {
            if (animation_->direction() == QAbstractAnimation::Backward)
                q->setVisible(false);

            startQueudAnimation();
        });
    }

    void startShowAnimation()
    {
        const auto isPending = item_->buddy().IsPending();
        const auto allowed = reactionButtonAllowed(item_->getContact(), item_->buddy());
        if (!allowed || isPending || Utils::InterConnector::instance().isMultiselect(item_->getContact()))
            return;

        q->move(calcPosition());
        q->setVisible(true);

        ensureAnimationInitialized();
        animation_->stop();
        animation_->setDirection(QAbstractAnimation::Forward);
        animation_->start();
    }

    void startHideAnimation()
    {
        ensureAnimationInitialized();
        animation_->stop();
        animation_->setDirection(QAbstractAnimation::Backward);
        animation_->start();
    }

    void startQueudAnimation()
    {
        if (queuedAnimationType_ == QueuedAnimation::None)
            return;

        if (queuedAnimationType_ == QueuedAnimation::Show && !visible())
            startShowAnimation();

        if (queuedAnimationType_ == QueuedAnimation::Hide && visible())
            startHideAnimation();

        queuedAnimationType_ = QueuedAnimation::None;
    }

    QPoint calcPosition() const
    {
        const auto messageRect = item_->messageRect();
        QRect geometry;

        geometry.setSize(buttonSize());

        if (outgoingPosition_)
            geometry.moveLeft(messageRect.left() - (buttonSize().width() - reactionButtonOffsetX()));
        else
            geometry.moveLeft(messageRect.right() - reactionButtonOffsetX());

        geometry.moveBottom(messageRect.bottom() - bottomOffsetFromMessage(item_->reactionsPlateType()));

        auto plateRect = item_->reactionsPlateRect();
        if (geometry.intersects(plateRect))
            geometry.moveBottom(plateRect.top() - Utils::scale_value(4));

        return geometry.topLeft();
    }

    bool pressed() const
    {
        return pressed_ || forcePressed_;
    }

    bool visible() const
    {
        return q->isVisible() || forceVisible_;
    }

    QRect pressRect() const
    {
        return QRect(q->geometry().center() - QPoint(pressRectSize().width() / 2, pressRectSize().height() / 2), pressRectSize());
    }

    QRect updateGeometry_helper(const QRect& _messageRect)
    {
        cachedMessageRect_ = _messageRect;

        const auto iconOffset = (buttonSize().width() - iconSize().width()) / 2;
        iconRect_ = QRect(QPoint(iconOffset, iconOffset), iconSize());

        return QRect(calcPosition(), buttonSize());
    }

    QColor shadowColorWithAlpha() const
    {
        auto color = shadowColor();
        color.setAlphaF(shadowColorAlpha() * opacity_);
        return color;
    }

    void initShadow()
    {
        shadow_ = new QGraphicsDropShadowEffect(q);
        shadow_->setBlurRadius(8);
        shadow_->setOffset(0, Utils::scale_value(1));
        shadow_->setColor(shadowColorWithAlpha());

        q->setGraphicsEffect(shadow_);
    }

    enum class QueuedAnimation
    {
        None,
        Show,
        Hide
    };

    QRect rect_;
    QRect iconRect_;
    QRect cachedMessageRect_;
    QPoint pressPos_;
    QRegion lastHoverRegion_;
    QTimer hoverTimer_;
    bool outgoingPosition_ = true;
    bool mouseOver_ = false;
    bool mouseOverPressArea_ = false;
    bool pressed_ = false;
    bool visible_ = false;
    bool forcePressed_ = false;
    bool forceVisible_ = false;
    double opacity_ = 0;
    QVariantAnimation* animation_;
    QueuedAnimation queuedAnimationType_ = QueuedAnimation::None;
    HistoryControlPageItem* item_;
    QGraphicsDropShadowEffect* shadow_;

    QWidget* q; // pointer to public class
};

//////////////////////////////////////////////////////////////////////////
// ReactionButton
//////////////////////////////////////////////////////////////////////////

AddReactionButton::AddReactionButton(HistoryControlPageItem* _item)
    : QWidget(_item),
      d(std::make_unique<ReactionButton_p>(this))
{
    Testing::setAccessibleName(this, qsl("AS Reaction button"));

    d->item_ = _item;

    setCursor(Qt::PointingHandCursor);
    setMouseTracking(true);
    setVisible(false);

    d->initShadow();

    d->hoverTimer_.setSingleShot(true);
    d->hoverTimer_.setInterval(hoverDelay.count());

    connect(&Utils::InterConnector::instance(), &Utils::InterConnector::multiselectChanged, this, &AddReactionButton::onMultiselectChanged);
    connect(&d->hoverTimer_, &QTimer::timeout, this, &AddReactionButton::onHoverTimer);
}

AddReactionButton::~AddReactionButton() = default;

void AddReactionButton::setOutgoingPosition(bool _outgoingPosition)
{
    d->outgoingPosition_ = _outgoingPosition;
}

void AddReactionButton::onMouseLeave()
{
    if (isVisible() && !d->forceVisible_)
    {
        d->ensureAnimationInitialized();
        if (d->animation_->state() != QAbstractAnimation::Running)
            d->startHideAnimation();
        else
            d->queuedAnimationType_ = ReactionButton_p::QueuedAnimation::Hide;
    }

    d->mouseOver_ = false;
    update();
}

void AddReactionButton::onMouseMove(const QPoint& _pos, const QRegion& _hoverRegion)
{
    const auto mouseOverButton = d->pressRect().contains(_pos);
    const auto mouseOverRegion = _hoverRegion.contains(_pos) || mouseOverButton;
    if (mouseOverRegion && !isVisible())
    {
        d->lastHoverRegion_ = _hoverRegion;
        if (!d->hoverTimer_.isActive())
            d->hoverTimer_.start();
    }
    else if (!mouseOverRegion && isVisible() && !d->forceVisible_)
    {
        d->ensureAnimationInitialized();
        if (d->animation_->state() != QAbstractAnimation::Running)
            d->startHideAnimation();
        else
            d->queuedAnimationType_ = ReactionButton_p::QueuedAnimation::Hide;
    }

    if (mouseOverButton != d->mouseOver_)
    {
        d->mouseOver_ = mouseOverButton;
        update();
    }

    updateCursorShape(_pos);
}

void AddReactionButton::onMousePress(const QPoint& _pos)
{
    if (!isVisible())
        return;

    d->pressed_ = d->pressRect().contains(_pos);

    if (d->pressed_)
        update();
}

void AddReactionButton::onMouseRelease(const QPoint& _pos)
{
    if (d->pressed_ && d->pressRect().contains(_pos) && isVisible())
        Q_EMIT clicked();

    d->pressed_ = false;
}

void AddReactionButton::onMessageSizeChanged()
{
    const auto messageRect = d->item_->messageRect();
    if (messageRect != d->cachedMessageRect_)
        setGeometry(d->updateGeometry_helper(messageRect));
}

void AddReactionButton::mouseMoveEvent(QMouseEvent* _event)
{
    onMouseMove(mapToParent(_event->pos()));
    QWidget::mouseMoveEvent(_event);
}

void AddReactionButton::mousePressEvent(QMouseEvent* _event)
{
    if (_event->button() == Qt::LeftButton)
        onMousePress(mapToParent(_event->pos()));

    _event->accept();
}

void AddReactionButton::mouseReleaseEvent(QMouseEvent* _event)
{
    if (_event->button() == Qt::LeftButton)
        onMouseRelease(mapToParent(_event->pos()));

    _event->accept();
}

void AddReactionButton::paintEvent(QPaintEvent* _event)
{
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);

    p.setOpacity(d->opacity_);

    QPainterPath path;
    path.addEllipse(rect());

    p.fillPath(path, backgroundColor(d->outgoingPosition_, d->mouseOver_, d->pressed()));
    p.drawPixmap(d->iconRect_, reactionButtonIcon(d->outgoingPosition_, d->pressed()));

    QWidget::paintEvent(_event);
}

void AddReactionButton::showEvent(QShowEvent* _event)
{
    raise();
    QWidget::showEvent(_event);
}

void AddReactionButton::updateCursorShape(const QPoint& _pos)
{
    const auto mouseOverPressRect = d->pressRect().contains(_pos);
    if (mouseOverPressRect && reactionButtonAllowed(d->item_->getContact(), d->item_->buddy()))
        d->item_->setCursor(Qt::PointingHandCursor);
    if (mouseOverPressRect != d->mouseOverPressArea_)
    {
        if (!mouseOverPressRect)
            d->item_->setCursor(Qt::ArrowCursor);
        d->mouseOverPressArea_ = mouseOverPressRect;
    }
}

void AddReactionButton::onAddReactionPlateShown()
{
    d->forcePressed_ = true;
    d->forceVisible_ = true;

    update();
}

void AddReactionButton::onAddReactionPlateCloseStarted()
{
    d->forcePressed_ = false;
    d->forceVisible_ = false;

    onMouseMove(d->item_->mapFromGlobal(QCursor::pos()));

    update();
}

void AddReactionButton::onAddReactionPlateCloseFinished()
{
    onMouseMove(d->item_->mapFromGlobal(QCursor::pos()));
}

void AddReactionButton::onPlatePositionChanged()
{
    setGeometry(d->updateGeometry_helper(d->item_->messageRect()));
}

void AddReactionButton::onMultiselectChanged()
{
    if (isVisible() && d->item_->isMultiselectEnabled())
        d->startHideAnimation();
}

void AddReactionButton::onHoverTimer()
{
    const auto pos = mapFromGlobal(mapToParent(QCursor::pos()));
    if (d->lastHoverRegion_.contains(pos))
    {
        d->ensureAnimationInitialized();
        if (d->animation_->state() != QAbstractAnimation::Running)
            d->startShowAnimation();
        else
            d->queuedAnimationType_ = ReactionButton_p::QueuedAnimation::Show;
    }
}

}
