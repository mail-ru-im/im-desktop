#include "stdafx.h"

#include "core_dispatcher.h"
#include "HistoryControlPageItem.h"
#include "../../gui_settings.h"
#include "../../utils/utils.h"
#include "../../cache/avatars/AvatarStorage.h"
#include "../../utils/InterConnector.h"
#include "../../controls/TooltipWidget.h"
#include "../../styles/ThemeParameters.h"
#include "../contact_list/ContactListModel.h"
#include "../contact_list/FavoritesUtils.h"
#include "../contact_list/ServiceContacts.h"
#include "../containers/FriendlyContainer.h"
#include "../containers/ThreadSubContainer.h"
#include "../MainPage.h"
#include "core_dispatcher.h"
#include "MessageStyle.h"
#include "LastStatusAnimation.h"
#include "statuses/StatusTooltip.h"
#include "statuses/StatusUtils.h"
#include "main_window/containers/StatusContainer.h"
#include "main_window/history_control/MessagesScrollArea.h"
#include "main_window/history_control/ThreadPlate.h"
#include "utils/features.h"
#include "main_window/history_control/reactions/AddReactionPlate.h"
#include "corner_menu/CornerMenu.h"

#include "ServiceMessageItem.h"

namespace
{
    constexpr int ANIM_MAX_VALUE = 100;
    constexpr int ADD_OFFSET = 10;
    constexpr int MULTISELECT_MARK_SIZE = 20;
    constexpr int MULTISELECT_MARK_OFFSET = 12;

    constexpr int cornerMenuHorOffset = 4;
    constexpr int cornerMenuHorOffsetOverflow = 12;
    constexpr int cornerMenuVerOffsetMediaOverflow = 30;
    constexpr int scrollbarWidth = 12;
    constexpr auto cornerMenuHoverDelay = std::chrono::milliseconds(50);
}

namespace Ui
{
    HistoryControlPageItem::HistoryControlPageItem(QWidget *parent)
        : QWidget(parent)
        , isChat_(false)
        , Selected_(false)
        , HasTopMargin_(false)
        , HasAvatar_(false)
        , hasSenderName_(false)
        , isChainedToPrev_(false)
        , isChainedToNext_(false)
        , HasAvatarSet_(false)
        , isDeleted_(false)
        , isEdited_(false)
        , isContextMenuEnabled_(true)
        , isMultiselectEnabled_(true)
        , selectedTop_(false)
        , selectedBottom_(false)
        , hoveredTop_(false)
        , hoveredBottom_(false)
        , selectionCenter_(0)
        , lastStatus_(LastStatus::None)
        , lastStatusAnimation_(nullptr)
        , addAnimation_(nullptr)
        , removeAnimation_(nullptr)
        , heightAnimation_(nullptr)
        , intersected_(false)
        , wasSelected_(false)
        , isUnsupported_(false)
        , nextHasSenderName_(false)
        , nextIsOutgoing_(false)
        , initialized_(false)
        , threadUpdateData_{ std::make_shared<Data::ThreadUpdate>() }
    {
        QuoteAnimation_ = new QuoteColorAnimation(this);
        connect(Logic::GetAvatarStorage(), &Logic::AvatarStorage::avatarChanged, this, &HistoryControlPageItem::avatarChanged);
        connect(&Utils::InterConnector::instance(), &Utils::InterConnector::multiselectSpaceClicked, this, [this]
        {
            if (Utils::InterConnector::instance().isMultiselect(aimId_) && Utils::InterConnector::instance().currentMultiselectMessage() == getId())
            {
                setSelected(!isSelected());
                if (isSelected())
                    Q_EMIT Utils::InterConnector::instance().messageSelected(getId(), getInternalId());
                update();
            }
        });

        connect(&Utils::InterConnector::instance(), &Utils::InterConnector::selectionStateChanged, this, [this] (const QString&, qint64 _id, const QString& _internalId, bool _selected)
        {
            if (prev_.getId() == _id && prev_.getInternalId() == _internalId)
            {
                selectedTop_ = _selected;
                update();
            }
            else if (next_.getId() == _id && next_.getInternalId() == _internalId)
            {
                selectedBottom_ = _selected;
                update();
            }
        });

        connect(&Utils::InterConnector::instance(), &Utils::InterConnector::multiSelectCurrentMessageChanged, this, [this]
        {
            auto id = Utils::InterConnector::instance().currentMultiselectMessage();
            if (prev_.getId() == id && id != -1)
            {
                hoveredTop_ = true;
            }
            else if (next_.getId() == id && id != -1)
            {
                hoveredBottom_ = true;
            }
            else
            {
                hoveredBottom_ = false;
                hoveredTop_ = false;
            }
            update();
        });

        connect(&Utils::InterConnector::instance(), &Utils::InterConnector::multiselectChanged, this, [this]
        {
            const auto sel = Utils::InterConnector::instance().isMultiselect(aimId_);
            if (!sel)
                selectionCenter_ = 0;

            if (threadPlate_)
                threadPlate_->setVisible(!sel);

            hideCornerMenuForce();
        });

        connect(get_gui_settings(), &qt_gui_settings::changed, this, [this](const QString& _key)
        {
            if (_key == ql1s(settings_show_reactions))
                onConfigChanged();
        });

        connect(&Utils::InterConnector::instance(), &Utils::InterConnector::omicronUpdated, this, &HistoryControlPageItem::onConfigChanged);
        connect(GetDispatcher(), &core_dispatcher::externalUrlConfigUpdated, this, &HistoryControlPageItem::onConfigChanged);

        setMouseTracking(true);
    }

    HistoryControlPageItem::~HistoryControlPageItem()
    {
        if (hasThread() && !isHeadless())
            Logic::ThreadSubContainer::instance().unsubscribe(getThreadId());
    }

    bool HistoryControlPageItem::isOutgoingPosition() const
    {
        return msg_.isOutgoingPosition();
    }

    void HistoryControlPageItem::clearSelection(bool)
    {
        if (Selected_)
            update();

        Selected_ = false;
        Q_EMIT selectionChanged();
        Q_EMIT Utils::InterConnector::instance().updateSelectedCount(aimId_);
    }

    bool HistoryControlPageItem::hasAvatar() const
    {
        return HasAvatar_;
    }

    bool HistoryControlPageItem::hasSenderName() const
    {
        return hasSenderName_;
    }

    bool HistoryControlPageItem::hasTopMargin() const
    {
        return HasTopMargin_;
    }

    void HistoryControlPageItem::setSelected(const bool _isSelected)
    {
        const auto prevState = Selected_;
        if (prevState != _isSelected)
        {
            update();
            Q_EMIT selectionChanged();
        }

        Selected_ = _isSelected;

        if (prevState != _isSelected)
        {
            Q_EMIT Utils::InterConnector::instance().selectionStateChanged(aimId_, getId(), getInternalId(), Selected_);
            Q_EMIT Utils::InterConnector::instance().updateSelectedCount(aimId_);
        }
    }

    void HistoryControlPageItem::setTopMargin(const bool value)
    {
        if (HasTopMargin_ == value)
            return;

        HasTopMargin_ = value;

        updateSize();
    }

    void HistoryControlPageItem::selectByPos(const QPoint&, const QPoint&, const QPoint&, const QPoint&)
    {

    }

    bool HistoryControlPageItem::isSelected() const
    {
        return Selected_;
    }

    void HistoryControlPageItem::onActivityChanged(const bool isActive)
    {
        const auto isInit = (isActive && !initialized_);
        if (isInit)
        {
            initialized_ = true;
            initialize();
            if (isEditable())
                startSpellChecking();
        }
    }

    void HistoryControlPageItem::onVisibleRectChanged(const QRect& /*_visibleRect*/)
    {

    }

    void HistoryControlPageItem::onDistanceToViewportChanged(const QRect& /*_widgetAbsGeometry*/, const QRect& /*_viewportVisibilityAbsRect*/)
    {}

    void HistoryControlPageItem::setHasAvatar(const bool value)
    {
        HasAvatar_ = value;
        HasAvatarSet_ = true;

        updateSize();
    }

    void HistoryControlPageItem::setHasSenderName(const bool _hasSender)
    {
        if (std::exchange(hasSenderName_, _hasSender) != _hasSender)
            updateSize();
    }

    void HistoryControlPageItem::setChainedToPrev(const bool _isChained)
    {
        if (isChainedToPrev_ != _isChained)
        {
            isChainedToPrev_ = _isChained;

            onChainsChanged();
            updateSize();
        }
    }

    void HistoryControlPageItem::setChainedToNext(const bool _isChained)
    {
        if (isChainedToNext_ != _isChained)
        {
            isChainedToNext_ = _isChained;

            onChainsChanged();
            updateSize();
        }
    }

    void HistoryControlPageItem::setPrev(const Logic::MessageKey& _key)
    {
        prev_ = _key;
    }

    void HistoryControlPageItem::setNext(const Logic::MessageKey& _key)
    {
        next_= _key;
    }

    void HistoryControlPageItem::setContact(const QString& _aimId)
    {
        im_assert(!_aimId.isEmpty());

        if (_aimId == aimId_)
            return;

        im_assert(aimId_.isEmpty());

        aimId_ = _aimId;
        QuoteAnimation_->setAimId(_aimId);
    }

    const QString& HistoryControlPageItem::getPageId() const
    {
        if (isThreadFeedMessage())
            return ServiceContacts::contactId(ServiceContacts::ContactType::ThreadsFeed);

        return aimId_;
    }

    void HistoryControlPageItem::setSender(const QString& /*_sender*/)
    {

    }

    void HistoryControlPageItem::updateFriendly(const QString&, const QString&)
    {
    }

    void HistoryControlPageItem::setDeliveredToServer(const bool _delivered)
    {

    }

    void HistoryControlPageItem::drawLastStatusIconImpl(QPainter& _p, int _rightPadding, int _bottomPadding)
    {
        if (lastStatusAnimation_)
        {
            const QRect rc = rect();

            const auto iconSize = MessageStyle::getLastStatusIconSize();

            lastStatusAnimation_->drawLastStatus(
                _p,
                rc.right() - MessageStyle::getRightBubbleMargin(aimId_) - _rightPadding + MessageStyle::getLastStatusLeftMargin(),
                rc.bottom() - iconSize.height() - _bottomPadding - bottomOffset(),
                iconSize.width(),
                iconSize.height());
        }
    }

    int HistoryControlPageItem::maxHeadsCount() const
    {
        if (headsAtBottom())
            return 5;

        const auto w = width();

        for (auto i = 5; i >= 4; --i)
        {
            if (w >= MessageStyle::getHeadsWidth(i))
                return i;
        }

        return 3;
    }

    void HistoryControlPageItem::ensureAddAnimationInitialized()
    {
        if (addAnimation_)
            return;

        addAnimation_ = new QVariantAnimation(this);
        addAnimation_->setStartValue(0);
        addAnimation_->setEndValue(ANIM_MAX_VALUE);
        addAnimation_->setDuration(200);
        addAnimation_->setEasingCurve(QEasingCurve::OutExpo);
        connect(addAnimation_, &QVariantAnimation::valueChanged, this, qOverload<>(&HistoryControlPageItem::update));
        connect(addAnimation_, &QVariantAnimation::finished, this, [this]()
        {
            for (auto& h : heads_)
                h.adding_ = false;

            updateSize();
        });
    }

    void HistoryControlPageItem::ensureRemoveAnimationInitialized()
    {
        if (removeAnimation_)
            return;

        removeAnimation_ = new QVariantAnimation(this);
        removeAnimation_->setStartValue(ANIM_MAX_VALUE);
        removeAnimation_->setEndValue(0);
        removeAnimation_->setEasingCurve(QEasingCurve::OutExpo);
        connect(removeAnimation_, &QVariantAnimation::valueChanged, this, qOverload<>(&HistoryControlPageItem::update));
        connect(removeAnimation_, &QVariantAnimation::finished, this, [this]()
        {
            heads_.erase(std::remove_if(heads_.begin(), heads_.end(),
                [](const auto& x) { return x.removing_; }),
                heads_.end());

            updateSize();
        });
    }

    void HistoryControlPageItem::ensureHeightAnimationInitialized()
    {
        if (heightAnimation_)
            return;

        heightAnimation_ = new QVariantAnimation(this);
        heightAnimation_->setStartValue(0);
        heightAnimation_->setEndValue(ANIM_MAX_VALUE);
        heightAnimation_->setDuration(200);
        heightAnimation_->setEasingCurve(QEasingCurve::OutExpo);
        connect(heightAnimation_, &QVariantAnimation::valueChanged, this, &HistoryControlPageItem::updateSize);
    }

    void HistoryControlPageItem::updateThreadSubscription()
    {
        const auto canSub = supportsOverlays() && !isHeadless();
        const auto conn =
            canSub &&
            Features::isThreadsEnabled() &&
            (isThreadFeedMessage() || (Utils::isChat(aimId_) && !Logic::getContactListModel()->isThread(aimId_)));

        if (conn)
            connect(GetDispatcher(), &core_dispatcher::threadUpdates, this, &HistoryControlPageItem::onThreadUpdates, Qt::UniqueConnection);
        else
            disconnect(GetDispatcher(), &core_dispatcher::threadUpdates, this, &HistoryControlPageItem::onThreadUpdates);

        if (canSub && hasThread())
            Logic::ThreadSubContainer::instance().subscribe(getThreadId());
    }

    void HistoryControlPageItem::onThreadUpdates(const Data::ThreadUpdates& _updates)
    {
        if (buddy().task_)
            return;
        if (const auto it = _updates.messages_.find(getThreadMsgId()); it != _updates.messages_.end())
            applyThreadUpdate(it->second);
    }

    void HistoryControlPageItem::initializeReactionsPlate()
    {
        if (!supportsOverlays())
            return;

        if (!reactions_ && MessageReactions::enabled())
        {
            reactions_ = std::make_unique<MessageReactions>(this);

            if (!Features::isMessageCornerMenuEnabled())
                reactions_->initAddReactionButton();

            if (const auto& reactions = buddy().reactions_; reactions && reactions->exist_)
                reactions_->subscribe();

            if (threadPlate_)
                connect(reactions_.get(), &MessageReactions::platePositionChanged, threadPlate_, &ThreadPlate::updatePosition, Qt::UniqueConnection);
        }
    }

    void HistoryControlPageItem::initializeCornerMenu()
    {
        if (!supportsOverlays() || !Features::isMessageCornerMenuEnabled() || cornerMenu_)
            return;

        const auto buttons = getCornerMenuButtons();
        if (buttons.empty())
            return;

        cornerMenu_ = new CornerMenu(this);
        connect(cornerMenu_, &CornerMenu::buttonClicked, this, &HistoryControlPageItem::onCornerMenuClicked);
        connect(cornerMenu_, &CornerMenu::hidden, cornerMenu_, &CornerMenu::deleteLater);
        connect(cornerMenu_, &CornerMenu::mouseLeft, this, [this]()
        {
            QTimer::singleShot(cornerMenuHoverDelay, this, [this]() { checkCornerMenuNeeded(QCursor::pos()); });
        });

        for (auto btn : buttons)
            cornerMenu_->addButton(btn);

        positionCornerMenu();
    }

    bool HistoryControlPageItem::hasReactionButton() const
    {
        auto reactionButtonAllowed = [this]()
        {
            const auto role = Logic::getContactListModel()->getYourRole(aimId_);
            const auto notAllowed = (role.isEmpty() && Utils::isChat(aimId_)) || role == u"notamember" || (role == u"readonly" && !Logic::getContactListModel()->isChannel(aimId_));
            return !notAllowed;
        };

        return MessageReactions::enabled()
            && !Utils::InterConnector::instance().isMultiselect(aimId_)
            && !buddy().IsPending()
            && !isThreadFeedMessage()
            && (Logic::getContactListModel()->isThread(aimId_) || reactionButtonAllowed());
    }

    bool HistoryControlPageItem::hasThreadButton() const
    {
        return canBeThreadParent()
            && Utils::isChat(aimId_)
            && Features::isThreadsEnabled()
            && !Utils::InterConnector::instance().isMultiselect(aimId_)
            && !buddy().IsPending()
            && Logic::getContactListModel()->contains(aimId_)
            && !Logic::getContactListModel()->isThread(aimId_)
            && Logic::getContactListModel()->areYouAllowedToWriteInThreads(aimId_);
    }

    QRect HistoryControlPageItem::getCornerMenuRect() const
    {
        const auto buttons = getCornerMenuButtons();
        const auto r = cornerMenuContentRect();
        const auto s = CornerMenu::calcSize(buttons.size());

        const auto outgoing = isOutgoingPosition();
        const auto availableWidth = outgoing ? r.left() : (width() - r.right());
        const auto horMargin = Utils::scale_value(cornerMenuHorOffset);
        const auto scrollWidth = outgoing ? 0 : Utils::scale_value(scrollbarWidth);
        const auto fitInWidth = availableWidth - s.width() - 2 * horMargin - scrollWidth >= 0;
        const auto singleMedia = isSingleMedia();

        const auto x = [fitInWidth, &s, &r, horMargin, scrollWidth, singleMedia, outgoing, this]()
        {
            const auto edge = outgoing ? horMargin : (width() - s.width() - horMargin - scrollWidth);

            if (outgoing)
                return std::max(r.left() - s.width() - horMargin, edge);
            else if (singleMedia && !fitInWidth)
                return std::min(r.right() + 1 - (s.width() - Utils::scale_value(cornerMenuHorOffsetOverflow)), edge);
            else
                return std::min(r.right() + 1 + horMargin, edge);
        }();

        const auto y = [fitInWidth, &s, &r, singleMedia, outgoing]()
        {
            const auto base = r.bottom() + 1 - s.height();
            const auto adjust = (singleMedia && !fitInWidth && !outgoing)
                ? Utils::scale_value(cornerMenuVerOffsetMediaOverflow)
                : 0;
            return base - adjust;
        }();

        return { { x - CornerMenu::shadowOffset(), y - CornerMenu::shadowOffset() }, s };
    }

    bool HistoryControlPageItem::isInCornerMenuHoverArea(const QPoint& _pos) const
    {
        if (!Features::isMessageCornerMenuEnabled())
            return false;

        if (Utils::InterConnector::instance().isMultiselect(aimId_))
            return false;

        const auto contentRect = cornerMenuContentRect();
        const auto menuRect = getCornerMenuRect();
        const auto area = contentRect.united(menuRect);
        return area.contains(_pos);
    }

    std::vector<MenuButtonType> HistoryControlPageItem::getCornerMenuButtons() const
    {
        std::vector<MenuButtonType> buttons;
        if (hasReactionButton())
            buttons.push_back(MenuButtonType::Reaction);
        if (hasThreadButton())
            buttons.push_back(MenuButtonType::Thread);

        if (isOutgoingPosition())
            std::reverse(buttons.begin(), buttons.end());

        return buttons;
    }

    void HistoryControlPageItem::positionCornerMenu()
    {
        if (cornerMenu_)
        {
            if (const auto r = getCornerMenuRect(); r.isValid())
            {
                cornerMenu_->move(r.topLeft());
                cornerMenu_->raise();
            }
        }
    }

    void HistoryControlPageItem::onCornerMenuClicked(MenuButtonType _type)
    {
        im_assert(cornerMenu_);
        if (_type == MenuButtonType::Reaction)
        {
            auto updateBtnState = [this, _type](auto _plateVisible)
            {
                if (cornerMenu_)
                {
                    cornerMenu_->setForceVisible(_plateVisible);
                    cornerMenu_->setButtonSelected(_type, _plateVisible);
                    cornerMenu_->updateHoverState();
                }
            };

            auto plate = MessageReactions::createAddReactionPlate(this, reactions_.get());
            connect(plate, &AddReactionPlate::plateShown, this, [updateBtnState]()
            {
                updateBtnState(true);
            });
            connect(plate, &AddReactionPlate::plateCloseStarted, this, [this]()
            {
                if (cornerMenu_)
                    cornerMenu_->setForceVisible(false);
            });
            connect(plate, &AddReactionPlate::plateCloseFinished, this, [this, updateBtnState]()
            {
                updateBtnState(false);
                checkCornerMenuNeeded(QCursor::pos());
            });

            if (const auto btnRectGlobal = cornerMenu_->getButtonRect(_type); btnRectGlobal.isValid())
            {
                const auto btnTopCenter = btnRectGlobal.center() - QPoint(0, btnRectGlobal.height() / 2);
                plate->showOverButton(btnTopCenter);
            }
        }
        else if (_type == MenuButtonType::Thread)
        {
            openThread();
        }
    }

    void HistoryControlPageItem::avatarChanged(const QString& _aimid)
    {
        if (std::any_of(heads_.cbegin(), heads_.cend(), [&_aimid](const auto& h) { return h.aimid_ == _aimid; }))
            update();
    }

    void HistoryControlPageItem::onConfigChanged()
    {
        updateThreadSubscription();

        if (!supportsOverlays())
            return;

        if (MessageReactions::enabled())
        {
            initializeReactionsPlate();
        }
        else if (reactions_)
        {
            reactions_->deleteControls();
            reactions_.reset();
        }

        updateSize();
    }

    void HistoryControlPageItem::openThread() const
    {
        auto& interConnector = Utils::InterConnector::instance();
        if (hasThread() && interConnector.getSidebarAimid().contains(getThreadId()) && interConnector.isSidebarVisible())
            Q_EMIT interConnector.setSidebarVisible(false);
        else
            Q_EMIT interConnector.openThread(getThreadId(), getThreadMsgId(), getPageId());
    }

    void HistoryControlPageItem::applyThreadUpdate(const std::shared_ptr<Data::ThreadUpdate>& _update)
    {
        setThreadId(_update->threadId_);

        threadUpdateData_ = _update;

        if (!threadPlate_ && threadUpdateData_->repliesCount_ > 0 && supportsOverlays() && Features::isThreadsEnabled())
        {
            threadPlate_ = new ThreadPlate(this);
            connect(threadPlate_, &ThreadPlate::clicked, this, &HistoryControlPageItem::openThread);
            Testing::setAccessibleName(threadPlate_, u"AS ThreadPlate " % QString::number(getId()));

            if (reactions_)
                connect(reactions_.get(), &MessageReactions::platePositionChanged, threadPlate_, &ThreadPlate::updatePosition, Qt::UniqueConnection);

            threadPlate_->show();
            updateSize();
            onSizeChanged();
        }

        if (threadPlate_)
            threadPlate_->updateWith(threadUpdateData_);
    }

    std::shared_ptr<Data::ThreadUpdate> HistoryControlPageItem::getthreadUpdate() const
    {
        return threadUpdateData_;
    }

    bool HistoryControlPageItem::hasThread() const
    {
        return !getThreadId().isEmpty();
    }

    bool HistoryControlPageItem::hasThreadWithReplies() const
    {
        if (hasThread() && threadUpdateData_->repliesCount_ > 0)
            return true;
        if (const auto parentTopic = buddy().threadData_.parentTopic_; parentTopic && (Data::ParentTopic::Type::message == parentTopic->type_))
        {
            const auto messageParentTopic = std::static_pointer_cast<Data::MessageParentTopic>(std::move(parentTopic));
            return isThreadFeedMessage() && !messageParentTopic->chat_.isEmpty();
        }
        return false;
    }

    const QString& HistoryControlPageItem::getThreadId() const
    {
        return buddy().threadData_.id_;
    }

    void HistoryControlPageItem::setThreadId(const QString& _threadId)
    {
        buddy().threadData_.id_ = _threadId;
    }

    bool HistoryControlPageItem::isHeadless() const
    {
        return parentWidget() == nullptr;
    }

    bool HistoryControlPageItem::isThreadFeedMessage() const
    {
        return buddy().threadData_.threadFeedMessage_;
    }

    void HistoryControlPageItem::drawLastStatusIcon(QPainter& _p,  LastStatus _lastStatus, const QString& _aimid, const QString& _friendly, int _rightPadding)
    {
        if (!heads_.isEmpty() || Utils::InterConnector::instance().isMultiselect(aimId_))
            return;

        switch (lastStatus_)
        {
        case LastStatus::None:
        case LastStatus::Read:
            break;
        case LastStatus::Pending:
        case LastStatus::DeliveredToServer:
        case LastStatus::DeliveredToPeer:
            drawLastStatusIconImpl(_p, _rightPadding, 0);
            break;
        default:
            break;
        }
    }

    void HistoryControlPageItem::mouseMoveEvent(QMouseEvent* e)
    {
        if (Utils::InterConnector::instance().isMultiselect(aimId_))
        {
            if (isMultiselectEnabled())
                setCursor(Qt::PointingHandCursor);
        }
        else if (!heads_.isEmpty())
        {
            if (showHeadsTooltip(rect(), e->pos()))
                setCursor(Qt::PointingHandCursor);
        }

        checkCornerMenuNeeded(mapToGlobal(e->pos()));

        if (reactions_)
            reactions_->onMouseMove(e->pos());

        if (Statuses::isStatusEnabled())
        {
            const auto tooltipRect = avatarRect();

            if (!Utils::InterConnector::instance().isMultiselect(aimId_) && (hasAvatar() || needsAvatar()) && tooltipRect.contains(e->pos()))
            {
                StatusTooltip::instance()->objectHovered([this]()
                {
                    if (Utils::InterConnector::instance().isMultiselect(aimId_))
                        return QRect();

                    const auto rect = avatarRect();
                    return QRect(mapToGlobal(rect.topLeft()), rect.size());
                }, msg_.GetChatSender(), this, window());
            }
        }

        return QWidget::mouseMoveEvent(e);
    }

    void HistoryControlPageItem::mousePressEvent(QMouseEvent* e)
    {
        pressPoint = e->pos();

        if (reactions_)
            reactions_->onMousePress(e->pos());

        return QWidget::mousePressEvent(e);
    }

    void HistoryControlPageItem::mouseReleaseEvent(QMouseEvent* e)
    {
        const auto pos = e->pos();
        if (Utils::clicked(pressPoint, pos))
        {
            if (isMultiselectEnabled() && Utils::InterConnector::instance().isMultiselect(aimId_))
            {
                if (e->button() == Qt::LeftButton)
                {
                    setSelected(!isSelected());
                    if (isSelected())
                        Q_EMIT Utils::InterConnector::instance().messageSelected(getId(), getInternalId());

                    update();
                }
            }
            else if (!heads_.isEmpty())
            {
                clickHeads(rect(), pos, e->button() == Qt::LeftButton);
            }
        }

        if (reactions_ && !Utils::InterConnector::instance().isMultiselect(aimId_))
            reactions_->onMouseRelease(pos);

        return QWidget::mouseReleaseEvent(e);
    }

    void HistoryControlPageItem::enterEvent(QEvent* _event)
    {
        Utils::InterConnector::instance().setCurrentMultiselectMessage(isMultiselectEnabled() ? getId() : -1);

        checkCornerMenuNeeded(QCursor::pos());

        QWidget::enterEvent(_event);
    }

    void HistoryControlPageItem::leaveEvent(QEvent* _event)
    {
        setCornerMenuVisible(false);

        if (reactions_)
            reactions_->onMouseLeave();

        QWidget::leaveEvent(_event);
    }

    void HistoryControlPageItem::wheelEvent(QWheelEvent* _event)
    {
        if constexpr (platform::is_apple())
            Tooltip::hide();

        QWidget::wheelEvent(_event);
    }

    void HistoryControlPageItem::resizeEvent(QResizeEvent* _event)
    {
        if (cornerMenu_)
            cornerMenu_->updateHoverState();
        QWidget::resizeEvent(_event);
    }

    void HistoryControlPageItem::moveEvent(QMoveEvent* _event)
    {
        checkCornerMenuNeeded(QCursor::pos());

        QWidget::moveEvent(_event);
    }

    bool HistoryControlPageItem::showHeadsTooltip(const QRect& _rc, const QPoint& _pos)
    {
        const int avatarSize = MessageStyle::getLastReadAvatarSize();
        auto xMargin = avatarSize + MessageStyle::getLastReadAvatarMargin();
        auto maxCount = maxHeadsCount();
        auto i = heads_.size() > maxCount ? maxCount : heads_.size() - 1;
        for (; i >= 0; --i)
        {
            const auto r = QRect(_rc.right() - xMargin, _rc.bottom() + 1 - avatarSize - MessageStyle::getLastReadAvatarBottomMargin(), avatarSize, avatarSize);
            if (r.contains(_pos))
            {
                auto text = heads_[i].friendly_;
                if (i == maxCount)
                {
                    for (auto j = i + 1; j < heads_.size(); ++j)
                    {
                        text += QChar::LineFeed;
                        text += heads_[j].friendly_;
                    }
                }

                if (!text.isEmpty())
                    Tooltip::show(text, QRect(mapToGlobal(r.topLeft()), QSize(avatarSize, avatarSize)));

                return i != maxCount || (i == maxCount && heads_.size() == maxCount + 1);
            }

            xMargin += (avatarSize + MessageStyle::getLastReadAvatarOffset());
        }

        return false;
    }

    bool HistoryControlPageItem::clickHeads(const QRect& _rc, const QPoint& _pos, bool _left)
    {
        const int avatarSize = MessageStyle::getLastReadAvatarSize();
        auto xMargin = avatarSize + MessageStyle::getLastReadAvatarMargin();
        auto maxCount = maxHeadsCount();
        auto i = heads_.size() > maxCount ? maxCount : heads_.size() - 1;
        for (; i >= 0; --i)
        {
            const auto r = QRect(_rc.right() - xMargin, _rc.bottom() + 1 - avatarSize - MessageStyle::getLastReadAvatarBottomMargin(), avatarSize, avatarSize);
            if (r.contains(_pos) && (i != maxCount || (i == maxCount && heads_.size() == maxCount + 1)))
            {
                if (_left)
                {
                    Utils::openDialogOrProfile(heads_.at(i).aimid_, Utils::OpenDOPParam::aimid);
                    Tooltip::hide();
                }
                else
                {
                    Q_EMIT mention(heads_.at(i).aimid_, heads_.at(i).friendly_);
                }
                return true;
            }

            xMargin += (avatarSize + MessageStyle::getLastReadAvatarOffset());
        }

        return false;
    }

    bool HistoryControlPageItem::handleSelectByPos(const QPoint& from, const QPoint& to, const QPoint& areaFrom, const QPoint& areaTo)
    {
        if (Utils::InterConnector::instance().isMultiselect(aimId_))
        {
            if (prevFrom_ != areaFrom && prevTo_ != areaTo)
            {
                intersected_ = false;
                wasSelected_ = isSelected();
            }

            const auto isSelectionTopToBottom = (from.y() <= to.y());
            const auto &topPoint = (isSelectionTopToBottom ? from : to);
            const auto &bottomPoint = (isSelectionTopToBottom ? to : from);

            const auto y = selectionCenter_ == 0 ? mapToGlobal(rect().center()).y() : mapToGlobal(QPoint(0, selectionCenter_)).y();

            const auto intersects = QRect(topPoint, bottomPoint).intersects(QRect(QPoint(mapToGlobal(rect().topLeft()).x(), y), QSize(rect().width(), 1)));
            if (intersected_ == intersects)
                return true;

            if (intersects)
                setSelected(!isSelected());
            else if (intersected_)
                setSelected(wasSelected_);

            intersected_ = intersects;
            prevFrom_ = areaFrom;
            prevTo_ = areaTo;

            update();
            return true;
        }

        return false;
    }

    void HistoryControlPageItem::initialize()
    {
        initializeReactionsPlate();
        updateThreadSubscription();
    }

    int64_t HistoryControlPageItem::getThreadMsgId() const
    {
        if (isThreadFeedMessage())
        {
            if (const auto parentTopic = buddy().threadData_.parentTopic_; parentTopic && (Data::ParentTopic::Type::message == parentTopic->type_))
            {
                const auto messageParentTopic = std::static_pointer_cast<Data::MessageParentTopic>(parentTopic);
                return messageParentTopic->msgId_;
            }
        }
        return getId();
    }

    void HistoryControlPageItem::copyPlates(const HistoryControlPageItem* _other)
    {
        if (!_other)
            return;

        if (_other->reactions_)
            setReactions(_other->reactions_->getReactions());
        if (_other->hasThread())
            applyThreadUpdate(_other->threadUpdateData_);
    }

    void HistoryControlPageItem::onSizeChanged()
    {
        if (threadPlate_)
            threadPlate_->updatePosition();

        if (reactions_)
            reactions_->onMessageSizeChanged();

        positionCornerMenu();
    }

    ReactionsPlateType HistoryControlPageItem::reactionsPlateType() const
    {
        return ReactionsPlateType::Regular;
    }

    bool HistoryControlPageItem::hasReactions() const
    {
        return msg_.reactions_ && msg_.reactions_->exist_;
    }

    QRect HistoryControlPageItem::reactionsPlateRect() const
    {
        if (reactions_)
            return reactions_->plateRect();

        return QRect();
    }

    QRect HistoryControlPageItem::threadPlateRect() const
    {
        if (threadPlate_)
            return threadPlate_->geometry();

        return QRect();
    }

    bool HistoryControlPageItem::hasHeads() const noexcept
    {
        return !heads_.isEmpty();
    }

    bool HistoryControlPageItem::headsAtBottom() const
    {
        return isOutgoing() || width() < MessageStyle::getHeadsWidth(3);
    }

    QMap<QString, QVariant> HistoryControlPageItem::makeData(const QString& _command, const QString& _arg)
    {
        QMap<QString, QVariant> result;
        result[qsl("command")] = _command;

        if (!_arg.isEmpty())
            result[qsl("arg")] = _arg;

        return result;
    }

    bool HistoryControlPageItem::isChainedToPrevMessage() const
    {
        return isChainedToPrev_;
    }

    bool HistoryControlPageItem::isChainedToNextMessage() const
    {
        return isChainedToNext_;
    }

    bool HistoryControlPageItem::isContextMenuReplyOnly() const noexcept
    {
        return Utils::InterConnector::instance().isRecordingPtt(aimId_);
    }

    void HistoryControlPageItem::setSelectionCenter(int _center)
    {
        selectionCenter_ = _center;
    }

    void HistoryControlPageItem::setIsUnsupported(bool _unsupported)
    {
        isUnsupported_ = _unsupported;
        setContextMenuEnabled(!isUnsupported_);
    }

    bool HistoryControlPageItem::isUnsupported() const
    {
        return isUnsupported_;
    }

    bool HistoryControlPageItem::isEditable() const
    {
        const auto isChannel = Logic::getContactListModel()->isChannel(aimId_);
        const auto channelAdmin = isChannel && Logic::getContactListModel()->areYouAdmin(aimId_);
        const auto readOnlyChannel = isChannel && Logic::getContactListModel()->isReadonly(aimId_);
        return !readOnlyChannel && (isOutgoing() || channelAdmin);
    }

    void HistoryControlPageItem::setNextHasSenderName(bool _nextHasSenderName)
    {
        if (std::exchange(nextHasSenderName_, _nextHasSenderName) != _nextHasSenderName)
            updateSize();
    }

    void HistoryControlPageItem::setBuddy(const Data::MessageBuddy& _msg)
    {
        const auto reactionsAdded = (!msg_.reactions_ || !msg_.reactions_->exist_) && _msg.reactions_ && _msg.reactions_->exist_;
        auto threadData = msg_.threadData_;
        const auto threadIdChanged = getThreadId() != _msg.threadData_.id_ && !_msg.threadData_.id_.isEmpty();

        msg_ = _msg;
        if (!threadIdChanged && threadData.threadFeedMessage_ == msg_.threadData_.threadFeedMessage_)
            msg_.threadData_ = threadData;

        if (reactions_ && initialized_ && reactionsAdded)
        {
            reactions_->subscribe();
            updateSize();
        }

        if (threadIdChanged)
            updateThreadSubscription();
    }

    const Data::MessageBuddy& HistoryControlPageItem::buddy() const
    {
        return msg_;
    }

    Data::MessageBuddy& HistoryControlPageItem::buddy()
    {
        return msg_;
    }

    bool HistoryControlPageItem::nextHasSenderName() const
    {
        return nextHasSenderName_;
    }

    void HistoryControlPageItem::setNextIsOutgoing(bool _nextIsOutgoing)
    {
        if (std::exchange(nextIsOutgoing_, _nextIsOutgoing) != _nextIsOutgoing)
            updateSize();
    }

    bool HistoryControlPageItem::nextIsOutgoing() const
    {
        return nextIsOutgoing_;
    }

    QRect HistoryControlPageItem::messageRect() const
    {
        return QRect();
    }

    QRect HistoryControlPageItem::cornerMenuContentRect() const
    {
        return QRect();
    }

    void HistoryControlPageItem::setReactions(const Data::Reactions& _reactions)
    {
        if (reactions_)
        {
            reactions_->setReactions(_reactions);
            update();
        }
    }

    void HistoryControlPageItem::drawSelection(QPainter& _p, const QRect& _rect)
    {
        static auto size = QSize(Utils::scale_value(MULTISELECT_MARK_SIZE), Utils::scale_value(MULTISELECT_MARK_SIZE));

        static auto on = Utils::renderSvgLayered(qsl(":/history/multiselect_on"),
            {
                {qsl("circle"), Styling::getParameters().getColor(Styling::StyleVariable::PRIMARY)},
                {qsl("check"), Styling::getParameters().getColor(Styling::StyleVariable::TEXT_SOLID_PERMANENT)}
            },
            size);

        static auto off = Utils::renderSvg(qsl(":/history/multiselect_off"), size, Styling::getParameters().getColor(Styling::StyleVariable::BASE_SECONDARY));

        selectionMarkRect_ = QRect(QPoint(width() - Utils::scale_value(MULTISELECT_MARK_OFFSET) - size.width(), _rect.center().y() - size.height() / 2), size);

        double current = Utils::InterConnector::instance().multiselectAnimationCurrent() / 100.0;

        {
            Utils::PainterSaver ps(_p);
            _p.setOpacity(current);
            _p.drawPixmap(selectionMarkRect_.x(), selectionMarkRect_.y(), off);
            if (isSelected())
            {
                auto min = Utils::scale_value(8);
                auto w = (int)(((size.width() - min) * current + min) / 2) * 2;
                auto h = (int)(((size.height() - min) * current + min) / 2) * 2;
                _p.drawPixmap(selectionMarkRect_.x() + (size.width() - w) / 2.0, selectionMarkRect_.y() + (size.width() - h) / 2.0, w, h, on);
            }
        }

        const auto hoverColor = Styling::getParameters().getColor(Styling::StyleVariable::PRIMARY_PASTEL, 0.05 * current);
        const auto hoverAndSelectedColor = Styling::getParameters().getColor(Styling::StyleVariable::PRIMARY_PASTEL, 0.15 * current);
        const auto selectionColor = Styling::getParameters().getColor(Styling::StyleVariable::PRIMARY_PASTEL, 0.10 * current);

        auto r = rect();
        if (HasTopMargin_ && !selectedTop_ && !hoveredTop_)
            r.setY(r.y() + (MessageStyle::getTopMargin(true) - MessageStyle::getTopMargin(false)));
        if (!selectedBottom_ && !hoveredBottom_ && bottomOffset() != Utils::getShadowMargin())
            r.setHeight(r.height() - (bottomOffset() - Utils::getShadowMargin()));

        const auto hovered = (Utils::InterConnector::instance().currentMultiselectElement() == Utils::MultiselectCurrentElement::Message && Utils::InterConnector::instance().currentMultiselectMessage() == getId());

        if (selectedTop_ || hoveredTop_)
        {
            auto topRect = rect();
            topRect.setHeight(topRect.height() - r.height());
            auto color = selectedTop_ ? (hoveredTop_ ? hoverAndSelectedColor : selectionColor) : hoverColor;
            if ((hovered || isSelected()) && !topRect.intersects(r))
                _p.fillRect(topRect, color);
        }

        if (selectedBottom_ || hoveredBottom_)
        {
            auto bottomRect = rect();
            bottomRect.setY(r.y() + r.height());
            auto color = selectedBottom_ ? (hoveredBottom_ ? hoverAndSelectedColor : selectionColor) : hoverColor;
            if ((hovered || isSelected()) && !bottomRect.intersects(r))
                _p.fillRect(bottomRect, color);
        }

        if (hovered)
        {
            if (isSelected())
            {
                _p.fillRect(r, hoverAndSelectedColor);
                return;
            }
            else
            {
                _p.fillRect(r, hoverColor);
            }
        }

        if (!isSelected())
            return;

        _p.fillRect(r, selectionColor);
    }

    void HistoryControlPageItem::startSpellChecking()
    {
    }

    void HistoryControlPageItem::drawHeads(QPainter& _p) const
    {
        if (heads_.isEmpty() || Utils::InterConnector::instance().isMultiselect(aimId_) || Favorites::isFavorites(aimId_))
            return;

        const QRect rc = rect();

        const int avatarSize = MessageStyle::getLastReadAvatarSize();
        const int size = Utils::scale_bitmap(avatarSize);

        auto xMargin = avatarSize + MessageStyle::getLastReadAvatarMargin() - Utils::scale_value(1);
        auto addMargin = 0, addingCount = 0, delCount = 0;
        const auto isAddAnimationRunning = addAnimation_ && addAnimation_->state() == QAbstractAnimation::State::Running;
        const auto curAdd = isAddAnimationRunning ? addAnimation_->currentValue().toDouble() / ANIM_MAX_VALUE : 0.0;
        const auto isRemoveAnimationRunning = removeAnimation_ && removeAnimation_->state() == QAbstractAnimation::State::Running;
        const auto curDel = isRemoveAnimationRunning ? removeAnimation_->currentValue().toDouble() / ANIM_MAX_VALUE : static_cast<double>(ANIM_MAX_VALUE);
        for (const auto& h : heads_)
        {
            if (h.adding_)
            {
                addMargin += Utils::scale_value(ADD_OFFSET) - (Utils::scale_value(ADD_OFFSET) * curAdd);
                addingCount++;
            }
            if (h.removing_)
            {
                delCount++;
            }
        }

        auto maxCount = maxHeadsCount();
        const auto addPhAnimated = addMargin != 0 && heads_.size() - addingCount == maxCount;
        const auto delPhAnimated = curDel != ANIM_MAX_VALUE && heads_.size() - delCount <= maxCount;

        auto i = heads_.size() > maxCount ? maxCount : heads_.size() - 1;
        if (addPhAnimated && i < (heads_.size() - 1))
            i++;

        static const auto morePlaceholder = Utils::renderSvg(qsl(":/history/i_more_seens"), QSize(avatarSize, avatarSize));

        for (; i >= 0; --i)
        {
            Utils::PainterSaver ps(_p);

            bool isDefault = false;
            QPixmap avatar;
            if (i == maxCount && !addPhAnimated && heads_.size() > maxCount + 1)
                avatar = morePlaceholder;
            else
                avatar = Logic::GetAvatarStorage()->GetRounded(heads_[i].aimid_, heads_[i].friendly_, size, isDefault, false, false);

            auto margin = xMargin;
            if (heads_[i].adding_ && isAddAnimationRunning && i != maxCount)
            {
                _p.setOpacity(curAdd);
                margin += addMargin;
            }
            else if (heads_[i].removing_ || (i == maxCount && delPhAnimated))
            {
                _p.setOpacity(curDel);
            }

            _p.drawPixmap(
                rc.right() - margin,
                rc.bottom() + 1 - avatarSize - MessageStyle::getLastReadAvatarBottomMargin(),
                avatarSize,
                avatarSize,
                avatar);

            if (i == (maxCount) && addPhAnimated)
            {
                avatar = morePlaceholder;
                _p.drawPixmap(
                    rc.right() - xMargin,
                    rc.bottom() + 1 - avatarSize - MessageStyle::getLastReadAvatarBottomMargin() - (avatarSize - avatarSize * curAdd),
                    avatarSize,
                    avatarSize,
                    avatar);
            }

            xMargin += (avatarSize + MessageStyle::getLastReadAvatarOffset());
        }
    }

    qint64 HistoryControlPageItem::getId() const
    {
        return -1;
    }

    qint64 HistoryControlPageItem::getPrevId() const
    {
        return -1;
    }

    const QString& HistoryControlPageItem::getInternalId() const
    {
        static const QString _id;
        return _id;
    }

    QString HistoryControlPageItem::getSourceText() const
    {
        return QString();
    }

    void HistoryControlPageItem::setDeleted(const bool _isDeleted)
    {
        isDeleted_ = _isDeleted;
    }

    bool HistoryControlPageItem::isDeleted() const
    {
        return isDeleted_;
    }

    void HistoryControlPageItem::setEdited(const bool _isEdited)
    {
        isEdited_ = _isEdited;
    }

    bool HistoryControlPageItem::isEdited() const
    {
        return isEdited_;
    }

    void HistoryControlPageItem::setLastStatus(LastStatus _lastStatus)
    {
        if (lastStatus_ == _lastStatus)
            return;

        lastStatus_ = _lastStatus;

        if (Favorites::isFavorites(aimId_) && lastStatus_ == LastStatus::Read)
            lastStatus_ = LastStatus::DeliveredToPeer;

        if (lastStatus_ != LastStatus::None)
        {
            if (!lastStatusAnimation_)
                lastStatusAnimation_ = new LastStatusAnimation(this);
        }

        if (lastStatusAnimation_)
        {
            lastStatusAnimation_->setLastStatus(lastStatus_);
            showMessageStatus();
        }

        if (!isChat_)
        {
            if (lastStatus_ == LastStatus::Read)
            {
                Data::ChatHead h;
                h.aimid_ = aimId_;
                h.friendly_ = Logic::GetFriendlyContainer()->getFriendly(aimId_);
                setHeads({ std::move(h) });
            }
            else
            {
                setHeads({});
            }
        }
    }

    LastStatus HistoryControlPageItem::getLastStatus() const
    {
        return lastStatus_;
    }

    int HistoryControlPageItem::bottomOffset() const
    {
        auto margin =  Utils::getShadowMargin();

        if (isChat() && hasHeads() && headsAtBottom())
        {
            margin += MessageStyle::getLastReadAvatarSize() + MessageStyle::getLastReadAvatarOffset();
        }
        else if (isChat() && isOutgoingPosition() && !nextIsOutgoing() && !isThreadFeedMessage())
        {
            const auto single = width() < MessageStyle::getHeadsWidth(3);
            margin += single ? MessageStyle::getTopMargin(true) : (MessageStyle::getLastReadAvatarSize() + MessageStyle::getLastReadAvatarOffset());
        }

        auto plateMargin = 0;

        const auto emptyReactions = reactions_ && !reactions_->hasReactions();
        const auto hasPlate = reactions_ && !reactions_->getReactions().isEmpty();

        const auto hasPlateMargin =
            (Features::isThreadsEnabled() && hasThreadWithReplies()) ||
            (MessageReactions::enabled() && hasPlate && !emptyReactions);

        if (hasPlateMargin)
        {
            plateMargin += MessageStyle::Plates::plateHeight();
            if (reactionsPlateType() == ReactionsPlateType::Media)
                plateMargin += MessageStyle::Plates::mediaPlateYOffset();
            else
                plateMargin += MessageStyle::Plates::plateYOffset() + MessageStyle::Plates::shadowHeight();
        }

        return margin + plateMargin;
    }

    void HistoryControlPageItem::setHeads(const Data::HeadsVector& _heads)
    {
        if (!isChat_)
        {
            if (heads_.empty() && !_heads.empty())
                return addHeads(_heads);
            else if (!heads_.empty() && _heads.empty())
                return removeHeads(heads_);
        }

        if (areTheSameHeads(_heads))
            return;

        heads_.clear();
        for (const auto& h : _heads)
        {
            if (!heads_.contains(h))
                heads_.push_back(h);
        }

        updateSize();
    }

    void HistoryControlPageItem::addHeads(const Data::HeadsVector& _heads)
    {
        if (areTheSameHeads(_heads))
            return;

        for (const auto& h : _heads)
            heads_.removeAll(h);

        const auto empty = heads_.isEmpty();

        for (const auto& h : _heads)
        {
            heads_.push_front(h);
            heads_.front().adding_ = !empty;
        }

        if (empty)
        {
            if (headsAtBottom())
            {
                ensureHeightAnimationInitialized();
                if (heightAnimation_->state() != QAbstractAnimation::State::Running)
                {
                    heightAnimation_->stop();
                    heightAnimation_->setDirection(QAbstractAnimation::Forward);
                    heightAnimation_->start();
                }
            }
        }
        else
        {
            ensureAddAnimationInitialized();
            if (addAnimation_->state() != QAbstractAnimation::State::Running)
                addAnimation_->start();
        }


        updateSize();
        update();
    }

    void HistoryControlPageItem::removeHeads(const Data::HeadsVector& _heads)
    {
        auto delCount = 0;
        for (const auto& h : _heads)
        {
            const auto ind = heads_.indexOf(h);
            if (ind == -1)
                continue;

            heads_[ind].removing_ = true;
            ++delCount;
        }

        ensureRemoveAnimationInitialized();
        if (removeAnimation_->state() != QAbstractAnimation::State::Running)
        {
            removeAnimation_->setDuration(isChat_ ? 200 : 0);
            removeAnimation_->start();
        }

        if (delCount && delCount == heads_.size())
        {
            if (headsAtBottom())
            {
                ensureHeightAnimationInitialized();
                if (heightAnimation_->state() != QAbstractAnimation::State::Running)
                {
                    heightAnimation_->stop();
                    heightAnimation_->setDirection(QAbstractAnimation::Backward);
                    heightAnimation_->start();
                }
            }
        }

        updateSize();
        update();
    }

    bool HistoryControlPageItem::areTheSameHeads(const QVector<Data::ChatHead>& _heads) const
    {
        return Data::isSameHeads(heads_, _heads);
    }

    void HistoryControlPageItem::initSize()
    {
        updateSize();
    }

    void HistoryControlPageItem::updateSize()
    {
        updateGeometry();
        resize(size());
        update();
    }

    void HistoryControlPageItem::setIsChat(bool _isChat)
    {
        isChat_ = _isChat;
    }

    void HistoryControlPageItem::showMessageStatus()
    {
        if (lastStatus_ == LastStatus::DeliveredToPeer || lastStatus_ == LastStatus::DeliveredToServer)
        {
            if (lastStatusAnimation_)
                lastStatusAnimation_->showStatus();
        }
    }

    void HistoryControlPageItem::hideMessageStatus()
    {
        if (lastStatus_ == LastStatus::DeliveredToPeer || lastStatus_ == LastStatus::DeliveredToServer)
        {
            if (lastStatusAnimation_)
                lastStatusAnimation_->hideStatus();
        }
    }

    void HistoryControlPageItem::hideCornerMenuForce()
    {
        if (cornerMenu_)
            cornerMenu_->forceHide();
    }

    void HistoryControlPageItem::setCornerMenuVisible(bool _visible)
    {
        if (_visible)
        {
            initializeCornerMenu();
            if (cornerMenu_)
                cornerMenu_->showAnimated();
        }
        else if (cornerMenu_)
        {
            cornerMenu_->hideAnimated();
        }
    }

    void HistoryControlPageItem::checkCornerMenuNeeded(const QPoint& _pos)
    {
        if (isInCornerMenuHoverArea(mapFromGlobal(_pos)))
        {
            if (!cornerMenu_)
            {
                if (!cornerMenuHoverTimer_)
                {
                    cornerMenuHoverTimer_ = new QTimer(this);
                    cornerMenuHoverTimer_->setSingleShot(true);
                    cornerMenuHoverTimer_->setInterval(cornerMenuHoverDelay);
                    connect(cornerMenuHoverTimer_, &QTimer::timeout, this, [this]()
                    {
                        if (isInCornerMenuHoverArea(mapFromGlobal(QCursor::pos())) && !cornerMenu_)
                            setCornerMenuVisible(true);
                    });
                }

                if (!cornerMenuHoverTimer_->isActive())
                    cornerMenuHoverTimer_->start();
            }
        }
        else
        {
            setCornerMenuVisible(false);
        }
    }

    QString AccessibleHistoryControlPageItem::text(QAccessible::Text _type) const
    {
        if (item_ && _type == QAccessible::Text::Name)
        {
            if (const auto id = item_->getId(); id == -1)
            {
                if (auto serviceItem = qobject_cast<ServiceMessageItem*>(item_))
                {
                    if (serviceItem->isNew())
                        return qsl("AS HistoryPage messageNew");
                    else if (serviceItem->isDate())
                        return u"AS HistoryPage messageDate " % serviceItem->formatRecentsText() % (serviceItem->isFloating() ? qsl(" (floating)") : QString());
                    else if (serviceItem->isOverlay())
                        return u"AS HistoryPage messageOverlay " % serviceItem->formatRecentsText();
                }
            }
            else
            {
                return u"AS HistoryPage message " % QString::number(id);
            }
        }
        return {};
    }
}
