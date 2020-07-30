#include "stdafx.h"
#include "styles/ThemeParameters.h"
#include "utils/DrawUtils.h"
#include "utils/utils.h"
#include "utils/InterConnector.h"
#include "../MessageStyle.h"
#include "../HistoryControlPageItem.h"
#include "main_window/contact_list/ContactListModel.h"

#include "animation/animation.h"
#include "AddReactionButton.h"

namespace
{

constexpr std::chrono::milliseconds animationDuration = std::chrono::milliseconds(150);
constexpr std::chrono::milliseconds hoverDelay = std::chrono::milliseconds(50);

QSize buttonSize()
{
    return Ui::MessageStyle::Reactions::addReactionButtonSize();
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

const QPixmap reactionButtonIcon(bool _outgoing, bool _pressed)
{
    if (_pressed)
    {
        static auto icon = Utils::renderSvg(qsl(":/reaction_icon"), iconSize(), Styling::getParameters().getColor(Styling::StyleVariable::BASE_GLOBALWHITE));
        return icon;
    }

    if (_outgoing)
    {
        static auto icon = Utils::renderSvg(qsl(":/reaction_icon"), iconSize(), Styling::getParameters().getColor(Styling::StyleVariable::PRIMARY_PASTEL));
        return icon;
    }
    else
    {
        static auto icon = Utils::renderSvg(qsl(":/reaction_icon"), iconSize(), Styling::getParameters().getColor(Styling::StyleVariable::BASE_SECONDARY));
        return icon;
    }
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

bool reactionButtonAllowed(const QString& _chatId)
{
    const auto role = Logic::getContactListModel()->getYourRole(_chatId);
    const auto notAllowed = role == ql1s("notamember") || (role == ql1s("readonly") && !Logic::getContactListModel()->isChannel(_chatId));

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

    ReactionButton_p(QWidget* _q) : q(_q) {}

    void startShowAnimation()
    {
        const auto isPending = item_->buddy().IsPending();
        const auto allowed = reactionButtonAllowed(item_->getContact());
        if (Utils::InterConnector::instance().isMultiselect() || !allowed || isPending)
            return;

        q->move(calcPosition());
        q->setVisible(true);
        auto updateCallback = [this]()
        {
            opacity_ = animation_.current();
            shadow_->setColor(shadowColorWithAlpha());
            q->update();
        };
        auto finishedCallback = [this](){ startQueudAnimation(); };

        animation_.finish();
        animation_.start(std::move(updateCallback), std::move(finishedCallback), 0, 1, animationDuration.count(), anim::sineInOut);
    }

    void startHideAnimation()
    {
        auto updateCallback = [this]()
        {
            opacity_ = animation_.current();
            shadow_->setColor(shadowColorWithAlpha());
            q->update();
        };
        auto finishedCallback = [this](){ q->setVisible(false); startQueudAnimation(); };

        animation_.finish();
        animation_.start(std::move(updateCallback), std::move(finishedCallback), 1, 0, animationDuration.count(), anim::sineInOut);
    }

    void startQueudAnimation()
    {
        if (queuedAnimation_ == QueuedAnimation::None)
            return;

        if (queuedAnimation_ == QueuedAnimation::Show && !visible())
            startShowAnimation();

        if (queuedAnimation_ == QueuedAnimation::Hide && visible())
            startHideAnimation();

        queuedAnimation_ = QueuedAnimation::None;
    }

    QPoint calcPosition()
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

    bool pressed()
    {
        return pressed_ || forcePressed_;
    }

    bool visible()
    {
        return q->isVisible() || forceVisible_;
    }

    QRect pressRect()
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

    QColor shadowColorWithAlpha()
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
    anim::Animation animation_;
    QueuedAnimation queuedAnimation_ = QueuedAnimation::None;
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

AddReactionButton::~AddReactionButton()
{

}

void AddReactionButton::setOutgoingPosition(bool _outgoingPosition)
{
    d->outgoingPosition_ = _outgoingPosition;
}

void AddReactionButton::onMouseLeave()
{
    if (isVisible() && !d->forceVisible_)
    {
        if (!d->animation_.isRunning())
            d->startHideAnimation();
        else
            d->queuedAnimation_ = ReactionButton_p::QueuedAnimation::Hide;
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
        if (!d->animation_.isRunning())
            d->startHideAnimation();
        else
            d->queuedAnimation_ = ReactionButton_p::QueuedAnimation::Hide;
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
    if (mouseOverPressRect)
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

void AddReactionButton::onAddReactionPlateClosed()
{
    d->forcePressed_ = false;
    d->forceVisible_ = false;

    onMouseMove(d->item_->mapFromGlobal(QCursor::pos()));

    repaint();
}

void AddReactionButton::onPlatePositionChanged()
{
    setGeometry(d->updateGeometry_helper(d->item_->messageRect()));
}

void AddReactionButton::onMultiselectChanged()
{
    if (isVisible())
        d->startHideAnimation();
}

void AddReactionButton::onHoverTimer()
{
    const auto pos = mapFromGlobal(mapToParent(QCursor::pos()));
    if (d->lastHoverRegion_.contains(pos))
    {
        if (!d->animation_.isRunning())
            d->startShowAnimation();
        else
            d->queuedAnimation_ = ReactionButton_p::QueuedAnimation::Show;
    }
}

}
