#include "stdafx.h"

#include "utils/utils.h"
#include "utils/features.h"
#include "utils/InterConnector.h"
#include "utils/DrawUtils.h"
#include "main_window/MainWindow.h"
#include "main_window/ContactDialog.h"
#include "controls/TooltipWidget.h"
#include "styles/ThemeParameters.h"
#include "../MessageStyle.h"
#include "core_dispatcher.h"

#include "AddReactionPlate.h"

namespace
{

constexpr std::chrono::milliseconds animationDuration = std::chrono::milliseconds(130);
constexpr std::chrono::milliseconds itemAnimationStep = std::chrono::milliseconds(20);
constexpr std::chrono::milliseconds tooltipShowDelay = std::chrono::milliseconds(400);
constexpr std::chrono::milliseconds leaveHideDelay = std::chrono::milliseconds(150);

QSize plateSize()
{
    return Utils::scale_value(QSize(336, 52));
}

int32_t plateOffsetV()
{
    return Utils::scale_value(40);
}

int32_t plateOffsetH()
{
    return Utils::scale_value(20);
}

int32_t plateStartOffsetTop()
{
    return Utils::scale_value(72);
}

QSize plateAreaSize()
{
    return Utils::scale_value(QSize(376, 132));
}

int32_t bottomOffsetFromButton()
{
    return Utils::scale_value(4);
}

int32_t plateOffsetVBottomMode()
{
    return plateAreaSize().height() - plateOffsetV() - plateSize().height() - Ui::MessageStyle::Reactions::addReactionButtonSize().height() - bottomOffsetFromButton();
}

int32_t plateTopOffsetFromDialog()
{
    return Utils::scale_value(-20);
}

int32_t borderRadius()
{
    return Utils::scale_value(28);
}

QColor backgroundColor()
{
    return Styling::getParameters().getColor(Styling::StyleVariable::CHAT_PRIMARY);
}

const QColor& myReactionOverlayColor(bool _pressed)
{
    if (_pressed)
    {
        static auto color = []()
        {
            auto color = Styling::getParameters().getColor(Styling::StyleVariable::CHAT_PRIMARY_ACTIVE);
            color.setAlphaF(0.8);
            return color;
        }();
        return color;
    }
    else
    {
        static auto color = []()
        {
            auto color = Styling::getParameters().getColor(Styling::StyleVariable::CHAT_PRIMARY);
            color.setAlphaF(0.8);
            return color;
        }();
        return color;
    }

}

QSize overlaySizeDiff()
{
    return Utils::scale_value(QSize(9, 9));
}

QSize emojiWidgetSize()
{
    return Utils::scale_value(QSize(54, 52));
}

QSize emojiWidgetSizeLarge()
{
    return Utils::scale_value(QSize(76, 76));
}

int32_t emojiSize()
{
    return Utils::scale_bitmap_with_value(40);
}

int32_t emojiSizeLarge()
{
    return Utils::scale_bitmap_with_value(66);
}

int32_t plateMarginH()
{
    return Utils::scale_value(6);
}

int32_t plateMarginV()
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

QSize removeIconSize()
{
    return Utils::scale_value(QSize(20, 20));
}

QSize removeIconSizeBig()
{
    return Utils::scale_value(QSize(32, 32));
}

QPixmap removeIcon(double _scale)
{
    return Utils::renderSvg(qsl(":/controls/close_icon"), _scale * removeIconSize(), Styling::getParameters().getColor(Styling::StyleVariable::BASE_PRIMARY));
}

const QPixmap& removeIconBig()
{
    static const auto icon = Utils::renderSvg(qsl(":/controls/close_icon"), removeIconSizeBig(), Styling::getParameters().getColor(Styling::StyleVariable::BASE_PRIMARY));
    return icon;
}

double emojiOpacityOtherHovered()
{
    return 0.8;
}

}

namespace Ui
{

//////////////////////////////////////////////////////////////////////////
// AddReactionPlate_p
//////////////////////////////////////////////////////////////////////////

class AddReactionPlate_p
{
public:

    AddReactionPlate_p(AddReactionPlate* _q) : q(_q) {}

    EmojiWidget* createItem(const QString& _reaction, const QString& _tooltip)
    {
        auto item = new EmojiWidget(_reaction, _tooltip, q);
        item->setMyReaction(_reaction == reactions_.myReaction_);
        QObject::connect(item, &EmojiWidget::clicked, q, &AddReactionPlate::onItemClicked);
        QObject::connect(item, &EmojiWidget::hovered, q, &AddReactionPlate::onItemHovered);
        QObject::connect(q, &AddReactionPlate::itemHovered, item, &EmojiWidget::onOtherItemHovered);

        return item;
    }

    void createItems()
    {
        if (!reactions_.reactions_.empty())
        {
            std::unordered_map<QString, QString, Utils::QStringHasher> tooltips;
            for (auto& reactionWithTooltip : Features::getReactionsWithTooltipsSet())
                tooltips[reactionWithTooltip.reaction_] = reactionWithTooltip.tooltip_;

            const auto showTooltips = std::all_of(reactions_.reactions_.begin(), reactions_.reactions_.end(), [&tooltips](const Data::Reactions::Reaction& _reaction)
            {
                return tooltips.find(_reaction.reaction_) != tooltips.end();
            });

            for (const auto& reaction : reactions_.reactions_)
                items_.push_back(createItem(reaction.reaction_, showTooltips ? tooltips[reaction.reaction_] : QString()));
        }
        else
        {
            for (auto& reactionWithTooltip : Features::getReactionsWithTooltipsSet())
                items_.push_back(createItem(reactionWithTooltip.reaction_, reactionWithTooltip.tooltip_));
        }
    }

    void setItemsStartPositions()
    {
        auto xOffset = plate_->pos().x() + plateMarginH();
        const auto yOffset = plate_->pos().y();

        for ( auto item : items_)
        {
            item->setStartGeometry(QRect(QPoint(xOffset, yOffset), item->size()));
            xOffset += item->width();
        }
    }

    void startShowAnimation()
    {
        currentAnimation_ = AnimationType::Show;

        startPlateAnimation();

        nextAnimationIndex_ = 0;
        animationTimer_.setInterval(itemAnimationStep.count());
        animationTimer_.start();
    }

    void startHideAnimation()
    {
        if (currentAnimation_ == AnimationType::Hide)
            return;

        Q_EMIT q->plateClosed();
        Q_EMIT Utils::InterConnector::instance().addReactionPlateActivityChanged(chatId_, false);

        currentAnimation_ = AnimationType::Hide;

        nextAnimationIndex_ = 0;
        startNextItemAnimation();

        animationTimer_.setInterval(itemAnimationStep.count());
        animationTimer_.start();
    }

    void startPlateAnimation()
    {
        if (currentAnimation_ == AnimationType::Hide)
            plate_->hideAnimated();
        else
            plate_->showAnimated();
    }

    void startNextItemAnimation()
    {
        const auto itemsSize = items_.size();
        const auto index = outgoingPosition_ ? itemsSize - nextAnimationIndex_ - 1 : nextAnimationIndex_;

        if (index < 0 || index >= itemsSize)
            return;

        auto item = items_[index];

        const bool isHide = currentAnimation_ == AnimationType::Hide;

        if (isHide)
            item->hideAnimated();
        else
            item->showAnimated();

        const auto isLastHide = nextAnimationIndex_ == itemsSize - 1 && isHide;
        if (isLastHide)
            startPlateAnimation();

        nextAnimationIndex_++;

        if (nextAnimationIndex_ == itemsSize)
            animationTimer_.stop();
    }

    std::vector<EmojiWidget*> items_;
    Data::Reactions reactions_;
    QString chatId_;
    int64_t msgId_;
    bool outgoingPosition_;

    PlateWithShadow* plate_;
    QTimer animationTimer_;
    QTimer leaveTimer_;
    size_t nextAnimationIndex_ = 0;

    enum class AnimationType
    {
        None,
        Show,
        Hide
    };

    AnimationType currentAnimation_ = AnimationType::None;

    AddReactionPlate* q;
};

//////////////////////////////////////////////////////////////////////////
// AddReactionPlate
//////////////////////////////////////////////////////////////////////////

AddReactionPlate::AddReactionPlate(const Data::Reactions& _messageReactions)
    : d(std::make_unique<AddReactionPlate_p>(this))
{
    setParent(Utils::InterConnector::instance().getMainWindow());
    setFixedSize(plateAreaSize());
    setMouseTracking(true);
    grabKeyboard();

    d->reactions_ = _messageReactions;

    d->plate_ = new PlateWithShadow(this);
    d->plate_->move(plateOffsetH(), plateStartOffsetTop());

    d->createItems();
    d->setItemsStartPositions();

    connect(d->plate_, &PlateWithShadow::hideFinished, this, [this](){ hide(); deleteLater(); });
    connect(d->plate_, &PlateWithShadow::plateClosed, this, &AddReactionPlate::plateClosed);

    connect(&d->animationTimer_, &QTimer::timeout, this, &AddReactionPlate::onAnimationTimer);
    connect(&Utils::InterConnector::instance(), &Utils::InterConnector::multiselectChanged, this, &AddReactionPlate::onMultiselectChanged);

    d->leaveTimer_.setInterval(leaveHideDelay.count());
    connect(&d->leaveTimer_, &QTimer::timeout, this, &AddReactionPlate::onLeaveTimer);
}

AddReactionPlate::~AddReactionPlate()
{

}

void AddReactionPlate::setOutgoingPosition(bool _outgoingPosition)
{
    d->outgoingPosition_ = _outgoingPosition;
}

void AddReactionPlate::setMsgId(int64_t _msgId)
{
    d->msgId_ = _msgId;
}

void AddReactionPlate::setChatId(const QString& _chatId)
{
    d->chatId_ = _chatId;
}

void AddReactionPlate::showOverButton(const QPoint& _buttonTopCenterGlobal)
{
    auto contactDialog = Utils::InterConnector::instance().getContactDialog();
    if (!contactDialog)
        return;

    auto dialogRect = contactDialog->rect();

    const auto left = _buttonTopCenterGlobal.x() - plateAreaSize().width() / 2;
    const auto right = left + plateAreaSize().width();
    auto top = _buttonTopCenterGlobal.y() - bottomOffsetFromButton() - plateSize().height() - plateOffsetV();

    if (contactDialog->mapFromGlobal(QPoint(left, top)).y() < dialogRect.top() + plateTopOffsetFromDialog())
        top = _buttonTopCenterGlobal.y() - plateOffsetVBottomMode();

    auto topLeft = QPoint(left, top);
    const auto topRight = QPoint(right, top);

    if (contactDialog->mapFromGlobal(topLeft).x() < dialogRect.left())
        topLeft.setX(contactDialog->mapToGlobal(dialogRect.topLeft()).x());
    if (contactDialog->mapFromGlobal(topRight).x() > dialogRect.right())
        topLeft.setX(contactDialog->mapToGlobal(dialogRect.topRight()).x() - plateAreaSize().width());

    Q_EMIT plateShown();
    Q_EMIT Utils::InterConnector::instance().addReactionPlateActivityChanged(d->chatId_, true);

    d->startShowAnimation();

    move(parentWidget()->mapFromGlobal(topLeft));
    show();
}

void AddReactionPlate::mousePressEvent(QMouseEvent* _event)
{

}

void AddReactionPlate::mouseReleaseEvent(QMouseEvent* _event)
{
    d->startHideAnimation();

    QWidget::mouseReleaseEvent(_event);
}

void AddReactionPlate::keyPressEvent(QKeyEvent* _event)
{
    d->startHideAnimation();
    QWidget::keyPressEvent(_event);
}

void AddReactionPlate::wheelEvent(QWheelEvent* _event)
{
    d->startHideAnimation();

    QWidget::wheelEvent(_event);
}

void AddReactionPlate::leaveEvent(QEvent* _event)
{
    d->leaveTimer_.start();

    QWidget::leaveEvent(_event);
}

void AddReactionPlate::enterEvent(QEvent* _event)
{
    d->leaveTimer_.stop();

    QWidget::enterEvent(_event);
}

void AddReactionPlate::onLeaveTimer()
{
    if (!rect().contains(mapFromGlobal(QCursor::pos())))
        d->startHideAnimation();
}

void AddReactionPlate::onAnimationTimer()
{
    d->startNextItemAnimation();
}

void AddReactionPlate::onItemClicked(const QString& _reaction)
{
    if (_reaction == d->reactions_.myReaction_)
        Q_EMIT removeReactionClicked();
    else
        Q_EMIT addReactionClicked(_reaction);
}

void AddReactionPlate::onItemHovered(bool _hovered)
{
    Q_EMIT itemHovered(_hovered);
}

void AddReactionPlate::onMultiselectChanged()
{
    d->startHideAnimation();
}

//////////////////////////////////////////////////////////////////////////
// EmojiWidget_p
//////////////////////////////////////////////////////////////////////////

class EmojiWidget_p
{
public:
    EmojiWidget_p(QWidget* _q) : animation_(new QVariantAnimation(_q)), q(_q) {}

    enum class AnimationType
    {
        None,
        Show,
        Hide,
        HoverIn,
        HoverOut
    };

    void startAnimation(AnimationType _type)
    {
        switch (_type)
        {
            case AnimationType::Hide:
            case AnimationType::Show:
                startVisibilityAnimation(_type);
                break;
            case AnimationType::HoverIn:
            case AnimationType::HoverOut:
                startHoverAnimation(_type);
                break;
            default:
                break;
        }
    }

    void startVisibilityAnimation(AnimationType _type)
    {
        switch (currentAnimation_)
        {
            case AnimationType::Hide:
            case AnimationType::Show:
                queuedAnimation_ = _type;
                return;
            default:
                break;
        }

        const bool isHide = _type == AnimationType::Hide;
        const bool isShow = _type == AnimationType::Show;

        const auto diffY = Utils::scale_value(32);
        auto startGeometry = isShow ? startGeometry_ : startGeometry_.translated(0, -diffY);
        emojiSize_ = QSize(emojiSize(), emojiSize());

        const auto sign = isHide ? 1 : -1;

        animation_->disconnect(q);
        animation_->stop();
        animation_->setStartValue(0.0);
        animation_->setEndValue(1.0);
        animation_->setEasingCurve(QEasingCurve::InOutSine);
        animation_->setDuration(animationDuration.count());

        QObject::connect(animation_, &QVariantAnimation::valueChanged, q, [this, startGeometry, diffY, sign, isHide]() {
            const auto current = animation_->currentValue().toDouble();
            q->setGeometry(startGeometry.translated(0, sign * diffY * current));

            if (isHide)
                opacity_ = 1 - current;
            else
                opacity_ = current;
        });

        QObject::connect(animation_, &QVariantAnimation::stateChanged, q, [this, isHide]() {
            if (animation_->state() == QAbstractAnimation::Stopped)
            {
                hoverEnabled_ = !isHide;
                currentAnimation_ = AnimationType::None;
                startQueuedAnimation();
            }
        });
        animation_->start();

        hoverEnabled_ = false;

        if (!isHide)
            q->show();

        currentAnimation_ = _type;
    }

    void startHoverAnimation(AnimationType _type)
    {
        const bool isHoverOut = _type == AnimationType::HoverOut;

        auto startGeometry = q->geometry();
        auto startSize = emojiSize_;
        auto diff = isHoverOut ? emojiSize() - emojiSize_.width() : emojiSizeLarge() - emojiSize_.width();
        if (!isHoverOut && !isLarge_)
        {
            QRect largeGeometry;
            largeGeometry.setSize(emojiWidgetSizeLarge());
            largeGeometry.moveCenter(startGeometry.center());
            q->setGeometry(largeGeometry);
            isLarge_= true;
        }

        const auto coef = animation_->state() == QAbstractAnimation::Running ? animation_->currentValue().toDouble() : 1.;

        animation_->disconnect(q);
        animation_->stop();
        animation_->setStartValue(1.0 - coef);
        animation_->setEndValue(1.0);
        animation_->setEasingCurve(QEasingCurve::InOutSine);
        animation_->setDuration(animationDuration.count() * coef);

        QObject::connect(animation_, &QVariantAnimation::valueChanged, q, [this, startSize, diff]() {
            auto d = diff * animation_->currentValue().toDouble();
            emojiSize_ = startSize + QSize(d, d);
            q->update();
        });
        QObject::connect(animation_, &QVariantAnimation::stateChanged, q, [this, isHoverOut, startGeometry]() {
            if (animation_->state() == QAbstractAnimation::Stopped)
            {
                if (isHoverOut)
                {
                    QRect regularGeometry;
                    regularGeometry.setSize(emojiWidgetSize());
                    regularGeometry.moveCenter(startGeometry.center());
                    q->setGeometry(regularGeometry);
                    isLarge_ = false;
                }

                currentAnimation_ = AnimationType::None;
            }
        });

        animation_->start();

        currentAnimation_ = _type;
    }

    void startQueuedAnimation()
    {
        if (queuedAnimation_ != AnimationType::None)
        {
            startAnimation(queuedAnimation_);
            queuedAnimation_ = AnimationType::None;
        }
    }

    void onHovered(bool _hovered)
    {
        if (_hovered != hovered_)
        {
            startAnimation(_hovered ? AnimationType::HoverIn : AnimationType::HoverOut);
            hovered_ = _hovered;

            if (hovered_)
            {
                tooltipTimer_.start();
                opacity_ = 1;
                q->raise();
            }
            else
            {
                hideToolTip();
            }
        }
    }

    void drawMyReactionOverlay(QPainter& _p)
    {
        QPainterPath path;
        auto rect = q->rect();

        QRect ellipseRect = QRect({0, 0}, Utils::unscale_bitmap(emojiSize_) + overlaySizeDiff());
        ellipseRect.moveCenter(rect.center());

        path.addEllipse(ellipseRect);
        _p.fillPath(path, myReactionOverlayColor(pressed_));

        auto icon = removeIcon(static_cast<double>(emojiSize_.width()) / emojiSize());
        auto iconSize = Utils::unscale_bitmap(icon.size());
        _p.drawPixmap((rect.width() - iconSize.width()) / 2, (rect.height() - iconSize.height()) / 2, icon);
    }

    void drawEmoji(QPainter& _p)
    {
        auto rect = q->rect();
        if (scaledEmoji_.size() != emojiSize_)
            scaledEmoji_ = emoji_.scaled(emojiSize_, Qt::KeepAspectRatio, Qt::SmoothTransformation);

        auto imageRect = QRect({0, 0}, Utils::unscale_bitmap(emojiSize_));

        imageRect.moveCenter(rect.center());

        _p.drawImage(imageRect, scaledEmoji_);
    }

    void showTooltip()
    {
        if (!tooltip_.isEmpty())
            Tooltip::show(myReaction_ ? QT_TRANSLATE_NOOP("reactions", "Remove") : tooltip_, QRect(q->mapToGlobal({0, 0}), q->size()), {-1, -1}, Tooltip::ArrowDirection::Down);
    }

    void hideToolTip()
    {
        Tooltip::hide();
    }

    QString reaction_;
    double opacity_ = 0;
    QImage emoji_;
    QVariantAnimation* animation_;
    bool hovered_ = false;
    bool pressed_ = false;
    bool hoverEnabled_ = false;
    bool myReaction_ = false;
    QTimer tooltipTimer_;
    QString tooltip_;
    QRect startGeometry_;
    bool isLarge_ = false;
    QSize emojiSize_;
    QImage scaledEmoji_;

    AnimationType currentAnimation_ = AnimationType::None;
    AnimationType queuedAnimation_ = AnimationType::None;

    QWidget* q;
};

//////////////////////////////////////////////////////////////////////////
// EmojiWidget
//////////////////////////////////////////////////////////////////////////

EmojiWidget::EmojiWidget(const QString& _reaction, const QString& _tooltip, QWidget* _parent)
    : QWidget(_parent),
      d(std::make_unique<EmojiWidget_p>(this))
{
    setMouseTracking(true);
    setGeometry(0, 0, emojiWidgetSize().width(), emojiWidgetSize().height());
    setVisible(false);

    auto code = Emoji::EmojiCode::fromQString(_reaction);

    d->emojiSize_ = QSize(emojiSize(), emojiSize());
    d->emoji_ = Emoji::GetEmoji(code, emojiSizeLarge());

    d->reaction_ = _reaction;
    d->tooltip_ = _tooltip;

    d->tooltipTimer_.setSingleShot(true);
    d->tooltipTimer_.setInterval(tooltipShowDelay.count());
    connect(&d->tooltipTimer_, &QTimer::timeout, this, &EmojiWidget::onTooltipTimer);
}

EmojiWidget::~EmojiWidget()
{

}

void EmojiWidget::setMyReaction(bool _myReaction)
{
    d->myReaction_ = _myReaction;
}

void EmojiWidget::setStartGeometry(const QRect& _rect)
{
    setGeometry(_rect);
    d->startGeometry_ = _rect;
}

void EmojiWidget::showAnimated()
{
    d->startAnimation(EmojiWidget_p::AnimationType::Show);
}

void EmojiWidget::hideAnimated()
{
    d->startAnimation(EmojiWidget_p::AnimationType::Hide);
    d->hideToolTip();
}

QString EmojiWidget::reaction() const
{
    return d->reaction_;
}

void EmojiWidget::onOtherItemHovered(bool _hovered)
{
    if (!d->hovered_ && d->hoverEnabled_)
    {
        d->opacity_ = _hovered ? emojiOpacityOtherHovered() : 1;
        update();
    }
}

void EmojiWidget::onTooltipTimer()
{
    if (d->hovered_ && d->hoverEnabled_)
        d->showTooltip();
}

void EmojiWidget::mouseMoveEvent(QMouseEvent* _event)
{
    if (d->hoverEnabled_)
    {
        setCursor(Qt::PointingHandCursor);
        const auto hovered = d->hovered_;
        d->onHovered(true);

        if (hovered != d->hovered_)
            update();
    }

    QWidget::mouseMoveEvent(_event);
}

void EmojiWidget::mousePressEvent(QMouseEvent* _event)
{
    if (_event->button() == Qt::LeftButton)
    {
        d->pressed_ = true;
        update();
    }

    QWidget::mousePressEvent(_event);
}

void EmojiWidget::mouseReleaseEvent(QMouseEvent* _event)
{
    if (_event->button() == Qt::LeftButton)
    {
        if (rect().contains(_event->pos()))
            Q_EMIT clicked(reaction());
    }

    d->pressed_ = false;

    QWidget::mouseReleaseEvent(_event);
}

void EmojiWidget::leaveEvent(QEvent* _event)
{
    if (d->hoverEnabled_)
    {
        Q_EMIT hovered(false);
        d->onHovered(false);
        update();
    }

    QWidget::leaveEvent(_event);
}

void EmojiWidget::enterEvent(QEvent* _event)
{
    if (d->hoverEnabled_)
        Q_EMIT hovered(true);

    QWidget::enterEvent(_event);
}

void EmojiWidget::paintEvent(QPaintEvent* _event)
{
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);
    p.setOpacity(d->opacity_);

    d->drawEmoji(p);

    if (d->myReaction_)
        d->drawMyReactionOverlay(p);

    QWidget::paintEvent(_event);
}

//////////////////////////////////////////////////////////////////////////
// PlateWithShadow_p
//////////////////////////////////////////////////////////////////////////

class PlateWithShadow_p
{
public:

    PlateWithShadow_p(PlateWithShadow* _q) : animation_(new QVariantAnimation(_q)), q(_q) {}

    void startShowAnimation()
    {
        currentAnimation_ = AnimationType::Show;

        startPlateAnimation();
    }

    void startHideAnimation()
    {
        currentAnimation_ = AnimationType::Hide;

        startPlateAnimation();
    }

    void startPlateAnimation()
    {
        auto startValue = 0;
        auto endValue = 1;

        const auto isHide = currentAnimation_ == AnimationType::Hide;

        const auto diffY = Utils::scale_value(32);
        auto startGeometry = q->geometry();
        const auto sign = isHide ? 1 : -1;

        auto finishedCallback = [this, isHide]()
        {
            if (isHide)
                Q_EMIT q->hideFinished();
        };

        animation_->disconnect(q);
        animation_->stop();
        animation_->setStartValue(startValue);
        animation_->setEndValue(endValue);
        animation_->setEasingCurve(QEasingCurve::InOutSine);
        animation_->setDuration(animationDuration.count());

        QObject::connect(animation_, &QVariantAnimation::valueChanged, q, [this, startGeometry, diffY, sign, isHide]() {
            const auto current = animation_->currentValue().toDouble();
            q->setGeometry(startGeometry.translated(0, sign * diffY * current));

            if (isHide)
                opacity_ = 1 - current;
            else
                opacity_ = current;

            shadow_->setColor(shadowColorWithAlpha());
        });
        QObject::connect(animation_, &QVariantAnimation::stateChanged, q, [this, isHide]() {
            if (animation_->state() == QAbstractAnimation::Stopped)
            {
                if (isHide)
                    Q_EMIT q->hideFinished();
            }
        });

        animation_->start();
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
        shadow_->setBlurRadius(16);
        shadow_->setOffset(0, Utils::scale_value(1));
        shadow_->setColor(shadowColorWithAlpha());

        q->setGraphicsEffect(shadow_);
    }

    double opacity_ = 0;
    QVariantAnimation* animation_;
    QGraphicsDropShadowEffect* shadow_ = nullptr;

    enum class AnimationType
    {
        None,
        Show,
        Hide
    };

    AnimationType currentAnimation_ = AnimationType::None;

    PlateWithShadow* q;
};

//////////////////////////////////////////////////////////////////////////
// PlateWithShadow
//////////////////////////////////////////////////////////////////////////

PlateWithShadow::PlateWithShadow(QWidget* _parent)
    : QWidget(_parent),
      d(std::make_unique<PlateWithShadow_p>(this))
{
    setFixedSize(plateSize());
    setMouseTracking(true);
    setVisible(false);

    d->initShadow();
}

PlateWithShadow::~PlateWithShadow()
{

}

void PlateWithShadow::showAnimated()
{
    d->startShowAnimation();
    show();
}

void PlateWithShadow::hideAnimated()
{
    d->startHideAnimation();
}

void PlateWithShadow::paintEvent(QPaintEvent* _event)
{
    QPainter p(this);

    p.setRenderHint(QPainter::Antialiasing);
    p.setOpacity(d->opacity_);

    QPainterPath path;
    path.addRoundedRect(rect(), borderRadius(), borderRadius());
    p.fillPath(path, backgroundColor());

    QWidget::paintEvent(_event);
}

}
