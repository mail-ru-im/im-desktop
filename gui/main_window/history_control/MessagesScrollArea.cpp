#include "stdafx.h"

#include "../ContactDialog.h"

#include "../smiles_menu/SmilesMenu.h"

#include "../../utils/InterConnector.h"
#include "../../utils/log/log.h"
#include "../../utils/utils.h"
#include "../../my_info.h"
#include "../../controls/TooltipWidget.h"

#include "../../main_window/contact_list/ContactListModel.h"
#include "main_window/MainPage.h"

#include "complex_message/ComplexMessageItem.h"

#include "VoipEventItem.h"
#include "MessagesScrollbar.h"
#include "MessagesScrollAreaLayout.h"

#include "MessagesScrollArea.h"
#include "../../gui_settings.h"

#include "MentionCompleter.h"

namespace
{
    double getMaximumScrollDistance();

    double getMinimumSpeed();

    double getMaximumSpeed();

    template<class T>
    T sign(const T value)
    {
        static_assert(
            std::is_arithmetic<T>::value,
            "std::is_arithmetic<T>::value");

        static_assert(
            std::numeric_limits<T>::is_signed,
            "std::numeric_limits<T>::is_signed");

        if (value == 0)
        {
            return 0;
        }

        return ((value > 0) ? 1 : -1);
    }

    Ui::HistoryControlPageItem* getSelectableItem(QWidget* _w)
    {
        if (qobject_cast<Ui::ComplexMessage::ComplexMessageItem*>(_w) || qobject_cast<Ui::VoipEventItem*>(_w))
            return qobject_cast<Ui::HistoryControlPageItem*>(_w);

        return nullptr;
    }

    double cubicOut(const double scrollDistance, const double maxScrollDistance, const double minSpeed, const double maxSpeed);

    double expoOut(const double scrollDistance, const double maxScrollDistance, const double minSpeed, const double maxSpeed);

    const auto MOMENT_UNINITIALIZED = std::numeric_limits<int64_t>::min();

    const auto scrollEasing = expoOut;

    const auto scrollAnimationInterval = 10;
    const auto wheelEventsBufferInterval = 300;
    constexpr std::chrono::milliseconds hideScrollBarTimeoutMs = std::chrono::seconds(2);
}

namespace Ui
{
    ScrollAreaContainer::ScrollAreaContainer(QWidget* _parent)
        : QAbstractScrollArea(_parent)
    {
        setFocusPolicy(Qt::NoFocus);
        setFrameShape(QFrame::NoFrame);
        setStyleSheet(qsl("background:transparent"));
        setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
        setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);

        verticalScrollBar()->setRange(-std::numeric_limits<int>::max(), std::numeric_limits<int>::max());
        verticalScrollBar()->setValue(0);
        verticalScrollBar()->setSingleStep(Utils::scale_value(42));
        verticalScrollBar()->setPageStep(Utils::scale_value(1000));
    }

    void ScrollAreaContainer::wheelEvent(QWheelEvent* _e)
    {
        // stub for too fast mouse scrolling
        if (_e->source() == Qt::MouseEventSynthesizedBySystem) //touchpad, magic mouse, etc
        {
            QAbstractScrollArea::wheelEvent(_e);
        }
        else // usual mouse
        {
            QWheelEvent e(_e->pos(), _e->angleDelta().y() / qApp->wheelScrollLines(), _e->buttons(), _e->modifiers(), _e->orientation());
            QAbstractScrollArea::wheelEvent(&e);
            _e->setAccepted(e.isAccepted());
        }
    }

    void ScrollAreaContainer::scrollContentsBy(const int, const int _dy)
    {
        emit scrollViewport(-_dy, QPrivateSignal());
    }

    enum class MessagesScrollArea::ScrollingMode
    {
        Min,

        Plain,
        Selection,

        Max
    };

    MessagesScrollArea::MessagesScrollArea(QWidget *parent, QWidget *typingWidget, hist::DateInserter* dateInserter)
        : QWidget(parent)
        , IsSelecting_(false)
        , TouchScrollInProgress_(false)
        , LastAnimationMoment_(MOMENT_UNINITIALIZED)
        , Mode_(ScrollingMode::Plain)
        , Scrollbar_(new MessagesScrollbar(this))
        , Layout_(new MessagesScrollAreaLayout(this, Scrollbar_, typingWidget, dateInserter))
        , ScrollAnimationTimer_(this)
        , HideScrollbarTimer_(this)
        , ScrollDistance_(0)
        , IsUserActive_(false)
        , IsSearching_(false)
        , scrollValue_(-1)
        , messageId_(-1)
        , recvLastMessage_(false)
    {
        assert(parent);
        assert(Layout_);
        assert(typingWidget);

        setContentsMargins(0, 0, 0, 0);
        setLayout(Layout_);

        // initially invisible, will became visible when there will be enough content
        setScrollbarVisible(false);

        ScrollAnimationTimer_.setInterval(scrollAnimationInterval);
        connect(&ScrollAnimationTimer_, &QTimer::timeout, this, &MessagesScrollArea::onAnimationTimer);

        HideScrollbarTimer_.setSingleShot(true);
        HideScrollbarTimer_.setInterval(hideScrollBarTimeoutMs.count());
        connect(&HideScrollbarTimer_, &QTimer::timeout, this, &MessagesScrollArea::onHideScrollbarTimer);

        WheelEventsBufferResetTimer_.setInterval(wheelEventsBufferInterval);

        connect(&WheelEventsBufferResetTimer_, &QTimer::timeout, this, &MessagesScrollArea::onWheelEventResetTimer);

        connect(this, &MessagesScrollArea::buttonDownClicked, this, [this]() { enableViewportShifting(true); });

        connect(Scrollbar_, &MessagesScrollbar::sliderPressed, this, &MessagesScrollArea::onSliderPressed);
        connect(Scrollbar_, &MessagesScrollbar::valueChanged, this, &MessagesScrollArea::onSliderValue);
        connect(Scrollbar_, &MessagesScrollbar::sliderMoved, this, &MessagesScrollArea::onSliderMoved);
        connect(Scrollbar_, &MessagesScrollbar::hoverChanged, this, &MessagesScrollArea::onScrollHoverChanged);

        connect(Layout_, &MessagesScrollAreaLayout::updateHistoryPosition, this, &MessagesScrollArea::onUpdateHistoryPosition);
        connect(Layout_, &MessagesScrollAreaLayout::recreateAvatarRect, this, &MessagesScrollArea::recreateAvatarRect);
        connect(Layout_, &MessagesScrollAreaLayout::itemRead, this, &MessagesScrollArea::itemRead);
        connect(&Utils::InterConnector::instance(), &Utils::InterConnector::clearSelecting, this, [this]() { IsSelecting_ = false; });
        Utils::grabTouchWidget(this);

        setMouseTracking(true);
    }

    void MessagesScrollArea::onUpdateHistoryPosition(int32_t position, int32_t offset)
    {
        emit updateHistoryPosition(position, offset);
    }

    void MessagesScrollArea::onHideScrollbarTimer()
    {
        if (!isScrolling() && !Scrollbar_->isHovered())
            Scrollbar_->fadeOut();
    }

    void MessagesScrollArea::onWheelEvent(QWheelEvent* e)
    {
        QCoreApplication::sendEvent(this, e);
    }

    bool MessagesScrollArea::isHidingScrollbar() const
    {
        return HideScrollbarTimer_.isActive();
    }

    void MessagesScrollArea::hideScrollbar()
    {
        if (!Scrollbar_->isVisible() || Scrollbar_->isHovered())
            return;

        HideScrollbarTimer_.start(); // restarts if running
    }

    void MessagesScrollArea::setScrollbarVisible(bool _visible)
    {
        Scrollbar_->setVisible(_visible);
        Scrollbar_->raise();

        if (_visible)
            hideScrollbar();
    }

    int MessagesScrollArea::getSelectedCount() const
    {
        int selectedItems = 0;

        enumerateWidgets(
            [&selectedItems] (QWidget* _item, const bool)
            {
                if (auto complexItem = qobject_cast<Ui::ComplexMessage::ComplexMessageItem*>(_item))
                {
                    if (complexItem->isSelected())
                        ++selectedItems;
                }
                else if (auto voipItem = qobject_cast<Ui::VoipEventItem*>(_item))
                {
                    if (voipItem->isSelected())
                        ++selectedItems;
                }
                return true;
            },
            false);

        return selectedItems;
    }

    void MessagesScrollArea::onSelectionChanged(int _selectedCount)
    {
        if (_selectedCount > 0)
            emit messagesSelected();
        else
            emit messagesDeselected();
    }

    void MessagesScrollArea::insertWidget(const Logic::MessageKey &key, std::unique_ptr<QWidget> widget)
    {
        InsertHistMessagesParams params;
        params.widgets.emplace_back(key, std::move(widget));
        params.scrollOnInsert = std::make_optional(ScrollOnInsert::ScrollToBottomIfNeeded);
        insertWidgets(std::move(params));
    }

    void MessagesScrollArea::insertWidgets(InsertHistMessagesParams&& _params)
    {
        for (const auto& iter : _params.widgets)
        {
            if (auto complexItem = qobject_cast<Ui::ComplexMessage::ComplexMessageItem*>(iter.second.get()))
            {
                contacts_.insert(complexItem->getSenderAimid());
                continue;
            }
            else if (auto voipItem = qobject_cast<Ui::VoipEventItem*>(iter.second.get()))
            {
                contacts_.insert(voipItem->getContact());
                continue;
            }
        }

        Layout_->lock();
        Layout_->insertWidgets(std::move(_params));
        Layout_->unlock();

        updateScrollbar();
    }

    bool MessagesScrollArea::isScrolling() const
    {
        return ScrollAnimationTimer_.isActive();
    }

    bool MessagesScrollArea::isSelecting() const
    {
        return IsSelecting_;
    }

    bool MessagesScrollArea::isViewportFull() const
    {
        const auto scrollBounds = Layout_->getViewportScrollBounds();

        const auto scrollRange = (scrollBounds.second - scrollBounds.first);
        assert(scrollRange >= 0);

        return (scrollRange > 0);
    }

    bool MessagesScrollArea::containsWidget(QWidget *widget) const
    {
        assert(widget);

        return Layout_->containsWidget(widget);
    }

    void MessagesScrollArea::eraseContact(QWidget* widget)
    {
        if (auto complexItem = qobject_cast<Ui::ComplexMessage::ComplexMessageItem*>(widget))
            contacts_.erase(complexItem->getSenderAimid());
        else if (auto voipItem = qobject_cast<Ui::VoipEventItem*>(widget))
            contacts_.insert(voipItem->getContact());
    }

    void MessagesScrollArea::removeWidget(QWidget *widget)
    {
        assert(widget);

        eraseContact(widget);

        Layout_->removeWidget(widget);
#ifdef IM_AUTO_TESTING
        widget->setParent(nullptr);
#endif //IM_AUTO_TESTING
        widget->deleteLater();

        updateScrollbar();

        emit widgetRemoved();
    }

    void MessagesScrollArea::cancelWidgetRequests(const QVector<Logic::MessageKey>& keys)
    {
        for (const auto& key : keys)
        {
            if (auto w = getItemByKey(key); w)
            {
                auto item = qobject_cast<HistoryControlPageItem*>(w);
                if (item)
                    item->cancelRequests();
            }
        }
    }

    void MessagesScrollArea::removeWidgets(const QVector<Logic::MessageKey>& keys)
    {
        std::vector<QWidget*> widgets;
        widgets.reserve(keys.size());
        for (const auto& key : keys)
        {
            if (auto w = getItemByKey(key); w)
                widgets.push_back(w);
        }
        Layout_->removeWidgets(widgets);
        std::for_each(widgets.begin(), widgets.end(), [](auto w)
        {
#ifdef IM_AUTO_TESTING
            w->setParent(nullptr);
#endif //IM_AUTO_TESTING
            w->deleteLater();
        });

        emit widgetRemoved();
    }

    void MessagesScrollArea::removeAll()
    {
        const auto keys = getItemsKeys();
        cancelWidgetRequests(keys);
        removeWidgets(keys);
    }

    void MessagesScrollArea::replaceWidget(const Logic::MessageKey &key, std::unique_ptr<QWidget> widget)
    {
        assert(widget);

        auto existingWidget = getItemByKey(key);
        if (!existingWidget)
        {
            return;
        }

        removeWidget(existingWidget);

        insertWidget(key, std::move(widget));
    }

    bool MessagesScrollArea::touchScrollInProgress() const
    {
        return TouchScrollInProgress_ || isScrolling();
    }

    void MessagesScrollArea::scrollToBottom()
    {
        setIsSearch(false);
        setMessageId(-1);
        stopScrollAnimation();
        enableViewportShifting(true);
        resetShiftingParams();
        Layout_->resetInitState();

        if (!Layout_->isViewportAtBottom())
            Layout_->setViewportByOffset(0);

        QSignalBlocker blocker(Scrollbar_);
        updateScrollbar();
        assert(!getIsSearch());
    }

    void MessagesScrollArea::updateItemKey(const Logic::MessageKey &key)
    {
        Layout_->updateItemKey(key);
    }

    void MessagesScrollArea::scrollTo(const Logic::MessageKey& key, hist::scroll_mode_type _scrollMode)
    {
        Layout_->scrollTo(key, _scrollMode);
    }

    void MessagesScrollArea::scrollContent(const int _dy)
    {
        Layout_->shiftViewportAbsY(_dy);
        updateScrollbar();
        setScrollbarVisible(true);
    }

    void MessagesScrollArea::updateScrollbar()
    {
        const auto viewportScrollBounds = Layout_->getViewportScrollBounds();

        auto scrollbarMaximum = (viewportScrollBounds.second - viewportScrollBounds.first);
        assert(scrollbarMaximum >= 0);
        const auto canScroll = scrollbarMaximum > 0;
        const auto scrollbarVisible = canScroll;

        if (ScrollShowOnOpen_ && canScroll && Layout_->widgetsCount())
        {
            setScrollbarVisible(true);
            ScrollShowOnOpen_ = false;
        }
        else
        {
            if (!scrollbarVisible && !isHidingScrollbar())
                hideScrollbar();
            else
                setScrollbarVisible(scrollbarVisible);
        }

        if (Scrollbar_->maximum() != scrollbarMaximum && Scrollbar_->value() > scrollbarMaximum)
            scrollValue_ = -1;

        Scrollbar_->setMaximum(scrollbarMaximum);
        Scrollbar_->raise();

        if (!scrollbarVisible)
        {
            scrollValue_ = -1;
            return;
        }

        const auto viewportAbsY = Layout_->getViewportAbsY();
        const auto scrollPos = (viewportAbsY - viewportScrollBounds.first);

        Scrollbar_->setValue(scrollPos);
    }

    void MessagesScrollArea::cancelSelection()
    {
        IsSelecting_ = false;
    }

    void MessagesScrollArea::cancelWheelBufferReset()
    {
        WheelEventsBufferResetTimer_.stop();
    }

    void MessagesScrollArea::enumerateWidgets(const WidgetVisitor& visitor, const bool reversed) const
    {
        assert(visitor);
        Layout_->enumerateWidgets(visitor, reversed);
    }

    QWidget* MessagesScrollArea::getItemByPos(const int32_t pos) const
    {
        return Layout_->getItemByPos(pos);
    }

    QWidget* MessagesScrollArea::getItemByKey(const Logic::MessageKey &key) const
    {
        return Layout_->getItemByKey(key);
    }

    QWidget* MessagesScrollArea::extractItemByKey(const Logic::MessageKey & _key)
    {
        return Layout_->extractItemByKey(_key);
    }

    QWidget* MessagesScrollArea::getPrevItem(QWidget* _w) const
    {
        return Layout_->getPrevItem(_w);
    }

    int32_t MessagesScrollArea::getItemsCount() const
    {
        return Layout_->getItemsCount();
    }

    Logic::MessageKeyVector MessagesScrollArea::getItemsKeys() const
    {
        return Layout_->getItemsKeys();
    }

    QString MessagesScrollArea::getSelectedText() const
    {
        QString selection_text;
        QString first_message_text_only;
        QString first_message_full;

        enumerateWidgets(
            [&selection_text, &first_message_text_only, &first_message_full]
            (QWidget* _item, const bool)
            {
                QString selected_full;

                if (auto complexItem = qobject_cast<Ui::ComplexMessage::ComplexMessageItem*>(_item))
                {
                    selected_full = complexItem->getSelectedText(true);

                    if (first_message_text_only.isEmpty())
                    {
                        first_message_text_only = complexItem->getSelectedText(false);
                        first_message_full = std::move(selected_full);

                        return true;
                    }
                }
                else if (auto voipItem = qobject_cast<Ui::VoipEventItem*>(_item))
                {
                    selected_full = voipItem->isSelected() ? voipItem->formatCopyText() : QString();

                    if (first_message_text_only.isEmpty())
                    {
                        first_message_text_only = selected_full;
                        first_message_full = selected_full;

                        return true;
                    }
                }

                if (selected_full.isEmpty())
                    return true;

                if (!selection_text.isEmpty())
                    selection_text += QChar::LineFeed % QChar::LineFeed;

                selection_text += selected_full;

                return true;
            }, true
        );

        QString result = (
            selection_text.isEmpty() ?
                first_message_text_only :
                (first_message_full % QChar::LineFeed % QChar::LineFeed % selection_text));

        if (!result.isEmpty() && result.endsWith(ql1s("\n\n")))
            result.chop(1);

        return result;
    }

    Data::QuotesVec MessagesScrollArea::getQuotes() const
    {
        Data::QuotesVec result;
        enumerateWidgets(
        [&result] (QWidget* _item, const bool)
        {
            if (auto complexItem = qobject_cast<Ui::ComplexMessage::ComplexMessageItem*>(_item))
            {
                QString selectedText = complexItem->getSelectedText(false);
                if (selectedText.isEmpty())
                    return true;

                result += complexItem->getQuotes();
                return true;
            }
            else if (auto voipItem = qobject_cast<Ui::VoipEventItem*>(_item))
            {
                if (voipItem->isSelected())
                    result += voipItem->getQuote();
                return true;
            }

            return true;
        }, true
            );

        return result;
    }

    static constexpr double triggerThreshold()
    {
        return 2.8;
    }

    static constexpr double unloadThreshold()
    {
        return 2.5;
    }

    QVector<Logic::MessageKey> MessagesScrollArea::getKeysToUnloadTop() const
    {
        const auto bounds = Layout_->getItemsAbsBounds();
        if (bounds.first == 0 && bounds.second == 0)
            return QVector<Logic::MessageKey>();
        const auto viewportTop = Layout_->getViewportAbsY() + Layout_->getTypingWidgetHeight();
        const auto heightFromTopBoundToViewportTop = viewportTop - bounds.first;
        //assert(heightFromTopBoundToViewportTop > 0);

        constexpr auto triggerThresholdK = triggerThreshold();
        const auto triggerThreshold = int32_t(minViewPortHeightForUnload() * triggerThresholdK);
        assert(triggerThreshold > 0);

        const auto triggerThreasholdNotReached = (heightFromTopBoundToViewportTop < triggerThreshold);
        if (triggerThreasholdNotReached)
            return QVector<Logic::MessageKey>();

        constexpr auto unloadThresholdK = unloadThreshold();
        const auto unloadThreshold = int32_t(minViewPortHeightForUnload() * unloadThresholdK);
        assert(unloadThreshold > 0);

        return Layout_->getWidgetsOverBottomOffset(heightFromTopBoundToViewportTop - unloadThreshold);
    }

    QVector<Logic::MessageKey> MessagesScrollArea::getKeysToUnloadBottom() const
    {
        const auto bounds = Layout_->getItemsAbsBounds();
        if (bounds.first == 0 && bounds.second == 0)
            return QVector<Logic::MessageKey>();
        const auto viewportBottom = Layout_->getViewportAbsY() + minViewPortHeightForUnload() + Layout_->getTypingWidgetHeight();
        const auto heightFromBoundToViewportBottom = bounds.second - viewportBottom;

        constexpr auto triggerThresholdK = triggerThreshold();
        const auto triggerThreshold = int32_t(minViewPortHeightForUnload() * triggerThresholdK);
        assert(triggerThreshold > 0);

        const auto triggerThreasholdNotReached = (heightFromBoundToViewportBottom < triggerThreshold);
        if (triggerThreasholdNotReached)
            return QVector<Logic::MessageKey>();

        constexpr auto unloadThresholdK = unloadThreshold();
        const auto unloadThreshold = (int32_t)(minViewPortHeightForUnload() * unloadThresholdK);
        assert(unloadThreshold > 0);

        return Layout_->getWidgetsUnderBottomOffset(heightFromBoundToViewportBottom - unloadThreshold);
    }


    void MessagesScrollArea::mouseMoveEvent(QMouseEvent *e)
    {
        QWidget::mouseMoveEvent(e);

        const auto isLeftButton = ((e->buttons() & Qt::LeftButton) != 0);
        const auto isGenuine = (e->source() == Qt::MouseEventNotSynthesized);

        if (!isLeftButton || !isGenuine)
        {
            if (isScrolling())
                return;
        }

        if (Utils::clicked(LastMouseGlobalPos_, e->globalPos()))
            return;

        LastMouseGlobalPos_ = e->globalPos();

        if (IsSelecting_)
        {
            const auto mouseAbsPos = Layout_->viewport2Absolute(e->pos());

            if (SelectionBeginAbsPos_.isNull())
            {
                SelectionBeginAbsPos_ = mouseAbsPos;
            }

            SelectionEndAbsPos_ = mouseAbsPos;

            applySelection();
        }

        auto mouseY = e->pos().y();

        const auto topScrollStartPosition = 0;

        const auto scrollUp = (mouseY <= topScrollStartPosition);
        if (scrollUp)
        {
            ScrollDistance_ = mouseY;
            startScrollAnimation(ScrollingMode::Selection);
            return;
        }

        const auto bottomScrollStartPosition = (height());
        assert(bottomScrollStartPosition > 0);

        const auto scrollDown = (mouseY >= bottomScrollStartPosition);
        if (scrollDown)
        {
            assert(!scrollUp);

            ScrollDistance_ = (mouseY - bottomScrollStartPosition);
            startScrollAnimation(ScrollingMode::Selection);

            return;
        }

        stopScrollAnimation();
    }

    void MessagesScrollArea::mousePressEvent(QMouseEvent *e)
    {
        QWidget::mousePressEvent(e);

        auto contactDialog = Utils::InterConnector::instance().getContactDialog();

        const auto smilesMenu = contactDialog->getSmilesMenu();
        const auto isSmilesHidden = !smilesMenu || smilesMenu->isHidden();

        const auto isLeftButton = ((e->buttons() & Qt::LeftButton) != 0);
        const auto isGenuine = (e->source() == Qt::MouseEventNotSynthesized);
        const auto isSelectionStart = (isLeftButton && isGenuine && isSmilesHidden);

        if (isLeftButton)
        {
            emit Utils::InterConnector::instance().hideMentionCompleter();
            emit Utils::InterConnector::instance().searchEnd();
        }

        if (!isSelectionStart)
        {
            return;
        }

        const auto mouseAbsPos = Layout_->viewport2Absolute(e->pos());

        if (!(e->modifiers() & Qt::ShiftModifier))
        {
            if (isLeftButton)
                clearSelection();

            LastMouseGlobalPos_ = e->globalPos();

            SelectionBeginAbsPos_ = mouseAbsPos;
            SelectionEndAbsPos_ = mouseAbsPos;

            applySelection(true);
        }
        else
        {
            if (SelectionBeginAbsPos_.isNull())
            {
                SelectionBeginAbsPos_ = mouseAbsPos;
                SelectionEndAbsPos_ = mouseAbsPos;
            }

            auto old = LastMouseGlobalPos_;
            LastMouseGlobalPos_ = e->globalPos();
            applySelection();
            LastMouseGlobalPos_ = old;
        }

        IsSelecting_ = true;
    }

    void MessagesScrollArea::mouseReleaseEvent(QMouseEvent *e)
    {
        const auto selectedCount = getSelectedCount();

        if (IsSelecting_)
            onSelectionChanged(selectedCount);

        QWidget::mouseReleaseEvent(e);

        IsSelecting_ = false;

        if (e->source() == Qt::MouseEventNotSynthesized)
            stopScrollAnimation();

        if (selectedCount == 0)
            MainPage::instance()->setFocusOnInput();
    }

    void MessagesScrollArea::enterEvent(QEvent *_event)
    {
        if constexpr (!platform::is_apple())
            setScrollbarVisible(true);

        QWidget::enterEvent(_event);
    }

    void MessagesScrollArea::leaveEvent(QEvent *event)
    {
        QApplication::restoreOverrideCursor();
        return QWidget::leaveEvent(event);
    }

    void MessagesScrollArea::notifySelectionChanges()
    {
        onSelectionChanged(getSelectedCount());
    }

    void MessagesScrollArea::wheelEvent(QWheelEvent *e)
    {
        if (Tooltip::canWheel())
        {
            Tooltip::wheel(e);
            return;
        }

        if constexpr (platform::is_apple())
            return;

        e->accept();

        cancelWheelBufferReset();

        startScrollAnimation(ScrollingMode::Plain);

        const QPoint pxDelta = e->pixelDelta();
        const auto isHorMove = !pxDelta.isNull() && std::abs(pxDelta.x()) >= std::abs(pxDelta.y());
        if (isHorMove)
            return;

        const auto delta = -e->delta();
        if (delta == 0)
            return;

        const auto directionChanged = enqueWheelEvent(delta);
        if (directionChanged)
        {
            ScrollDistance_ = 0;
            WheelEventsBuffer_.clear();
        }

        ScrollDistance_ = std::clamp(ScrollDistance_ + delta, -getMaximumScrollDistance(), getMaximumScrollDistance());
    }

    void MessagesScrollArea::hideEvent(QHideEvent*)
    {
        if constexpr (platform::is_apple())
            setScrollbarVisible(false);
    }

    void MessagesScrollArea::scroll(ScrollDirection direction, int delta)
    {
        if (delta == 0)
            return;

        cancelWheelBufferReset();

        startScrollAnimation(ScrollingMode::Plain);

        delta = direction == ScrollDirection::UP ? -delta : delta;
        ScrollDistance_ = std::clamp(ScrollDistance_ + delta, -getMaximumScrollDistance(), getMaximumScrollDistance());
    }

    bool MessagesScrollArea::contains(const QString& _aimId) const
    {
        return contacts_.find(_aimId) != contacts_.end();
    }

    void MessagesScrollArea::resumeVisibleItems()
    {
        assert(Layout_);

        Layout_->resumeVisibleItems();
    }

    void MessagesScrollArea::suspendVisibleItems()
    {
        assert(Layout_);

        Layout_->suspendVisibleItems();
    }

    bool MessagesScrollArea::event(QEvent *e)
    {
        if (e->type() == QEvent::Resize)
        {
            auto resizeEvent = static_cast<QResizeEvent*>(e);
            if (resizeEvent->oldSize().height() != resizeEvent->size().height())
                updateScrollbar();
        }
        else if (e->type() == QEvent::TouchBegin)
        {
            TouchScrollInProgress_ = true;
            QTouchEvent* te = static_cast<QTouchEvent*>(e);
            PrevTouchPoint_ = te->touchPoints().first().pos();
            e->accept();

            return true;
        }
        else if (e->type() == QEvent::TouchUpdate)
        {
            QTouchEvent* te = static_cast<QTouchEvent*>(e);
            if (te->touchPointStates() & Qt::TouchPointMoved)
            {
                QPointF newPoint = te->touchPoints().first().pos();
                if (!newPoint.isNull())
                {
                    int touchDelta = newPoint.y() - PrevTouchPoint_.y();
                    PrevTouchPoint_ = newPoint;

                    startScrollAnimation(ScrollingMode::Plain);

                    const auto wheelMultiplier = -1;
                    const auto delta = (touchDelta * wheelMultiplier);
                    assert(delta != 0);

                    const auto wheelDirection = sign(delta);

                    const auto currentDirection = sign(ScrollDistance_);

                    const auto directionChanged = (wheelDirection != currentDirection);
                    if (directionChanged)
                    {
                        ScrollDistance_ = 0;
                    }

                    ScrollDistance_ += delta;
                }
            }

            e->accept();
            return true;
        }
        else if (e->type() == QEvent::TouchEnd || e->type() == QEvent::TouchCancel)
        {
            TouchScrollInProgress_ = false;
            e->accept();

            return true;
        }

        return QWidget::event(e);
    }

    void MessagesScrollArea::showEvent(QShowEvent *_event)
    {
        if constexpr (!platform::is_apple())
            ScrollShowOnOpen_ = true;

        QWidget::showEvent(_event);
    }

    void MessagesScrollArea::onAnimationTimer()
    {
        if (!ScrollAnimationTimer_.isActive())
            return;

        if (ScrollDistance_ == 0)
        {
            stopScrollAnimation();
            return;
        }

        const auto now = QDateTime::currentMSecsSinceEpoch();

        const auto step = evaluateScrollingStep(now);

        const auto emptyStep = ((int32_t)step == 0);
        if (!emptyStep && !Layout_->shiftViewportAbsY(step))
        {
            stopScrollAnimation();
            return;
        }

        updateScrollbar();

        const auto isPlainMode = (Mode_ == ScrollingMode::Plain);
        if (isPlainMode)
        {
            const auto newScrollDistance = (ScrollDistance_ - step);

            const auto stop = (sign(newScrollDistance) != sign(ScrollDistance_));
            if (stop)
            {
                stopScrollAnimation();
                return;
            }

            ScrollDistance_ = newScrollDistance;
        }

        LastAnimationMoment_ = now;
    }

    void MessagesScrollArea::onMessageHeightChanged(QSize oldSize, QSize newSize)
    {
        if (oldSize == newSize)
        {
            assert(false);
        }
    }

    void MessagesScrollArea::onSliderMoved(int value)
    {
        stopScrollAnimation();

        Layout_->setViewportByOffset(Scrollbar_->maximum() - value);
    }

    void MessagesScrollArea::onSliderPressed()
    {
        stopScrollAnimation();
    }

    void MessagesScrollArea::onSliderValue(int value)
    {
        assert(value >= 0);

        Tooltip::hide();

        const bool isAtBottom = isScrollAtBottom();

        if ((scrollValue_ == -1 || scrollValue_ < value) && Scrollbar_->isInFetchRangeToBottom(value))
        {
            // to newer
            emit fetchRequestedEvent(false /* don't move to bottom */);
        }
        else if ((scrollValue_ == -1 || scrollValue_ > value) && Scrollbar_->isInFetchRangeToTop(value))
        {
            // to older
            emit fetchRequestedEvent(true);
        }

        if (Mode_ == ScrollingMode::Selection)
        {
            applySelection();
        }

        if (isAtBottom)
        {
            emit scrollMovedToBottom();
        }

        // Check that value changed due to mouse press
        if (Scrollbar_->isHovered() && !isScrolling())
            onSliderMoved(value);

        scrollValue_ = value;
    }

    bool MessagesScrollArea::needFetchMoreToTop() const
    {
        //return Scrollbar_->isInFetchRangeToTop(scrollValue_);

        const auto viewportScrollBounds = Layout_->getViewportScrollBounds();

        const auto scrollbarMaximum = (viewportScrollBounds.second - viewportScrollBounds.first);

        if (scrollbarMaximum <= 0)
            return true;

        const auto viewportAbsY = Layout_->getViewportAbsY();
        const auto scrollPos = (viewportAbsY - viewportScrollBounds.first);

        return scrollPos >= 0 && scrollPos < int(0.1 * scrollbarMaximum);
    }

    bool MessagesScrollArea::needFetchMoreToBottom() const
    {
        //return Scrollbar_->isInFetchRangeToBottom(scrollValue_);

        const auto viewportScrollBounds = Layout_->getViewportScrollBounds();

        const auto scrollbarMaximum = (viewportScrollBounds.second - viewportScrollBounds.first);

        if (scrollbarMaximum <= 0)
            return true;

        const auto viewportAbsY = Layout_->getViewportAbsY();
        const auto scrollPos = (viewportAbsY - viewportScrollBounds.first);

        return scrollPos >= 0 && (scrollbarMaximum - scrollPos) < int(0.1 * scrollbarMaximum);
    }

    void MessagesScrollArea::updateMode()
    {
        if (recvLastMessage_)
        {
            if (getIsSearch())
                setIsSearch(false);
        }
        else if (getMessageId() > 0)
        {
            setIsSearch(true);
        }
    }

    void MessagesScrollArea::applySelection(const bool forShift)
    {
        assert(!LastMouseGlobalPos_.isNull());

        const auto selectionBegin = Layout_->absolute2Viewport(SelectionBeginAbsPos_);
        const auto selectionEnd = Layout_->absolute2Viewport(SelectionEndAbsPos_);
        const auto selectionBeginGlobal = mapToGlobal(selectionBegin);
        const auto selectionEndGlobal = mapToGlobal(selectionEnd);

        if (selectionBeginGlobal == selectionEndGlobal)
            return;

        enumerateWidgets(
            [selectionBeginGlobal, selectionEndGlobal](QWidget *w, const bool)
            {
                if (auto complexItem = qobject_cast<Ui::ComplexMessage::ComplexMessageItem*>(w))
                    complexItem->selectByPos(selectionBeginGlobal, selectionEndGlobal);
                else if (auto voipItem = qobject_cast<Ui::VoipEventItem*>(w))
                    voipItem->selectByPos(selectionBeginGlobal, selectionEndGlobal);

                return true;
            },
            false
        );
    }

    void MessagesScrollArea::clearSelection()
    {
        SelectionBeginAbsPos_ = QPoint();
        SelectionEndAbsPos_ = QPoint();

        enumerateWidgets(
            [](QWidget* _w, const bool)
            {
                if (auto w = getSelectableItem(_w))
                    w->clearSelection();

                return true;
            },
            false
        );

        emit messagesDeselected();
    }

    void MessagesScrollArea::pressedDestroyed()
    {
        if (IsSelecting_)
        {
            stopScrollAnimation();
            IsSelecting_ = false;
        }
    }

    void MessagesScrollArea::continueSelection(const QPoint& _pos)
    {
        const auto mouseAbsPos = Layout_->viewport2Absolute(_pos);

        LastMouseGlobalPos_ = mapToGlobal(_pos);

        SelectionBeginAbsPos_ = mouseAbsPos;
        SelectionEndAbsPos_ = mouseAbsPos;

        applySelection(true);

        IsSelecting_ = true;
    }

    bool MessagesScrollArea::enqueWheelEvent(const int32_t delta)
    {
        const auto WHEEL_HISTORY_LIMIT = 3;

        const auto directionBefore = evaluateWheelHistorySign();

        WheelEventsBuffer_.emplace_back(delta);

        const auto isHistoryOverwhelmed = (WheelEventsBuffer_.size() > WHEEL_HISTORY_LIMIT);
        if (isHistoryOverwhelmed)
        {
            WheelEventsBuffer_.pop_front();
        }

        const auto directionAfter = evaluateWheelHistorySign();

        assert(WheelEventsBuffer_.size() <= WHEEL_HISTORY_LIMIT);
        const auto isHistoryFilled = (WheelEventsBuffer_.size() == WHEEL_HISTORY_LIMIT);

        const auto isSignChanged = (
            (directionAfter != 0) &&
            (directionBefore != 0) &&
            (directionAfter != directionBefore));

        return (isHistoryFilled && isSignChanged);
    }

    double MessagesScrollArea::evaluateScrollingVelocity() const
    {
        assert(ScrollDistance_ != 0);
        assert(std::abs(ScrollDistance_) <= getMaximumScrollDistance());

        auto speed = scrollEasing(
            std::abs(ScrollDistance_),
            getMaximumScrollDistance(),
            getMinimumSpeed(),
            getMaximumSpeed());

        speed *= sign(ScrollDistance_);

        return speed;
    }

    double MessagesScrollArea::evaluateScrollingStep(const int64_t now) const
    {
        assert(now > 0);

        const auto timeDiff = (now - LastAnimationMoment_);

        const auto velocity = evaluateScrollingVelocity();

        auto step = velocity;
        step *= timeDiff;
        step /= 1000;

        if (step == 0)
        {
            step = sign(velocity);
        }

        return step;
    }

    int32_t MessagesScrollArea::evaluateWheelHistorySign() const
    {
        const auto sum = std::accumulate(
            WheelEventsBuffer_.cbegin(),
            WheelEventsBuffer_.cend(),
            0);

        return sign(sum);
    }

    void MessagesScrollArea::scheduleWheelBufferReset()
    {
        WheelEventsBufferResetTimer_.start();
    }

    void MessagesScrollArea::startScrollAnimation(const ScrollingMode mode)
    {
        assert(mode > ScrollingMode::Min);
        assert(mode < ScrollingMode::Max);

        Mode_ = mode;

        if (isScrolling())
        {
            return;
        }

        ScrollAnimationTimer_.start();

        assert(LastAnimationMoment_ == MOMENT_UNINITIALIZED);
        LastAnimationMoment_ = QDateTime::currentMSecsSinceEpoch();
    }

    void MessagesScrollArea::stopScrollAnimation()
    {
        auto prevScrollDistance = ScrollDistance_;
        LastAnimationMoment_ = MOMENT_UNINITIALIZED;
        ScrollDistance_ = 0;

        scheduleWheelBufferReset();

        if (!isScrolling())
            return;

        ScrollAnimationTimer_.stop();
        if (prevScrollDistance == 0)
            return;

        hideScrollbar();
    }

    int MessagesScrollArea::minViewPortHeightForUnload() const
    {
        return std::max(Layout_->getViewportHeight(), Utils::scale_value(600));
    }

    void MessagesScrollArea::onWheelEventResetTimer()
    {
        WheelEventsBuffer_.clear();

        WheelEventsBufferResetTimer_.stop();
    }

    void MessagesScrollArea::onScrollHoverChanged(bool _nowHovered)
    {
        if (!_nowHovered)
            hideScrollbar();
    }

    bool MessagesScrollArea::isScrollAtBottom() const
    {
        return Layout_->isViewportAtBottom();
    }

    void MessagesScrollArea::setIsSearch(bool _isSearch)
    {
        IsSearching_ = _isSearch;
    }

    bool MessagesScrollArea::getIsSearch() const
    {
        return IsSearching_;
    }

    void MessagesScrollArea::setMessageId(qint64 _id)
    {
        messageId_ = _id;
    }

    qint64 MessagesScrollArea::getMessageId() const
    {
        return messageId_;
    }

    void MessagesScrollArea::setRecvLastMessage(bool _value)
    {
        recvLastMessage_ = _value;
    }

    void MessagesScrollArea::updateItems()
    {
        Layout_->updateItemsWidth();
        if (Ui::get_gui_settings()->get_value<bool>(settings_partial_read, settings_partial_read_deafult()))
            Layout_->readVisibleItems();
    }

    void MessagesScrollArea::enableViewportShifting(bool enable)
    {
        Layout_->enableViewportShifting(enable);
    }

    void MessagesScrollArea::resetShiftingParams()
    {
        Layout_->resetShiftingParams();
    }

    bool MessagesScrollArea::isInitState() const noexcept
    {
        return Layout_->isInitState();
    }

    bool MessagesScrollArea::tryEditLastMessage()
    {
        auto editCalled = false;

        enumerateWidgets([&editCalled](QWidget* _item, const bool)
        {
            if (auto messageItem = qobject_cast<Ui::MessageItemBase*>(_item))
            {
                if (!messageItem->isOutgoing())
                    return true; // skip

                if (!messageItem->isEditable())
                    return false; // abort enumerating

                messageItem->callEditing();
                editCalled = true;
                return false; // abort enumerating
            }
            return true; // skip
        }, false);

        return editCalled;
    }

    void MessagesScrollArea::invalidateLayout()
    {
        Layout_->invalidate();
        Layout_->updateItemsGeometry();
    }

    void MessagesScrollArea::checkVisibilityForRead()
    {
        Layout_->checkVisibilityForRead();
    }
}

namespace
{
    double getMaximumScrollDistance()
    {
        return Utils::scale_value(10000);
    }

    double getMinimumSpeed()
    {
        return Utils::scale_value(100);
    }

    double getMaximumSpeed()
    {
        return Utils::scale_value(10000);
    }

    double cubicOut(const double scrollDistance, const double maxScrollDistance, const double minSpeed, const double maxSpeed)
    {
        assert(maxScrollDistance > 0);
        assert(std::abs(scrollDistance) <= maxScrollDistance);
        assert(minSpeed >= 0);
        assert(maxSpeed > 0);
        assert(maxSpeed > minSpeed);

        // see http://gizma.com/easing/#cub2

        auto t = scrollDistance;
        const auto b = minSpeed;
        const auto c = (maxSpeed - minSpeed);
        const auto d = maxScrollDistance;

        t /= d;

        t--;

        return (c * (std::pow(t, 3.0) + 1) + b);
    }

    double expoOut(const double scrollDistance, const double maxScrollDistance, const double minSpeed, const double maxSpeed)
    {
        assert(maxScrollDistance > 0);
        assert(std::abs(scrollDistance) <= maxScrollDistance);
        assert(minSpeed >= 0);
        assert(maxSpeed > 0);
        assert(maxSpeed > minSpeed);

        // see http://gizma.com/easing/#expo2

        auto t = scrollDistance;
        const auto b = minSpeed;
        const auto c = (maxSpeed - minSpeed);
        const auto d = maxScrollDistance;

        return (c * (-std::pow(2.0, -10 * t/d) + 1) + b);
    }
}
