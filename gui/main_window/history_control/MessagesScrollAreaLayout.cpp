#include "stdafx.h"

#include "../../utils/log/log.h"
#include "../../utils/utils.h"

#include "complex_message/ComplexMessageItem.h"

#include "ChatEventItem.h"
#include "MessageStyle.h"
#include "ServiceMessageItem.h"
#include "VoipEventItem.h"

#include "MessagesScrollArea.h"
#include "MessagesScrollbar.h"

#include "MessagesScrollAreaLayout.h"

#include "history/DateInserter.h"
#include "history/Heads.h"

#include "../../utils/InterConnector.h"
#include "../MainWindow.h"
#include "../../gui_settings.h"
#include "complex_message/ComplexMessageItem.h"

#include <boost/range/adaptor/reversed.hpp>

namespace
{
    constexpr std::chrono::milliseconds scrollActivityTimeout = std::chrono::seconds(1);
}

namespace Ui
{
    namespace
    {
        void applyWidgetWidth(const int32_t viewportWidth, QWidget *widget, bool traverseLayout);

        int32_t evaluateWidgetHeight(QWidget *widget);

        HistoryControlPageItem* getSelectableItem(QWidget* _w)
        {
            if (qobject_cast<ComplexMessage::ComplexMessageItem*>(_w) || qobject_cast<VoipEventItem*>(_w))
                return qobject_cast<HistoryControlPageItem*>(_w);

            return nullptr;
        }
    }

    enum class MessagesScrollAreaLayout::SlideOp
    {
        Min,

        NoSlide,
        SlideUp,
        SlideDown,

        Max,
    };

    QTextStream& operator <<(QTextStream &lhs, const MessagesScrollAreaLayout::SlideOp slideOp)
    {
        assert(slideOp > MessagesScrollAreaLayout::SlideOp::Min);
        assert(slideOp < MessagesScrollAreaLayout::SlideOp::Max);

        switch(slideOp)
        {
            case MessagesScrollAreaLayout::SlideOp::NoSlide: lhs << ql1s("no-slide"); break;
            case MessagesScrollAreaLayout::SlideOp::SlideUp: lhs << ql1s("slide-up"); break;
            case MessagesScrollAreaLayout::SlideOp::SlideDown: lhs << ql1s("slide-down"); break;
            default:
                ;
        }

        return lhs;
    }

    MessagesScrollAreaLayout::ItemInfo::ItemInfo(QWidget *widget, const Logic::MessageKey &key)
        : Widget_(widget)
        , Key_(key)
        , IsGeometrySet_(false)
        , IsHovered_(false)
        , IsActive_(false)
        , isVisibleEnoughForPlay_(false)
        , isVisibleEnoughForRead_(false)
    {
        assert(Widget_);
    }

    MessagesScrollAreaLayout::MessagesScrollAreaLayout(
        MessagesScrollArea *scrollArea,
        MessagesScrollbar *messagesScrollbar,
        QWidget *typingWidget,
        hist::DateInserter* dateInserter)
        : QLayout(scrollArea)
        , Scrollbar_(messagesScrollbar)
        , ScrollArea_(scrollArea)
        , TypingWidget_(typingWidget)
        , dateInserter_(dateInserter)
        , ViewportSize_(0, 0)
        , ViewportAbsY_(0)
        , IsDirty_(false)
        , ShiftingViewportEnabled_(true)
        , isInitState_(true)
        , scrollActivityFlag_(false)
        , heads_(nullptr)
    {
        assert(ScrollArea_);
        assert(Scrollbar_);
        assert(TypingWidget_);

        ScrollArea_->installEventFilter(this);
        Utils::InterConnector::instance().getMainWindow()->installEventFilter(this);

        connect(scrollArea, &MessagesScrollArea::buttonDownClicked, this, &MessagesScrollAreaLayout::onButtonDownClicked);

        connect(this, &MessagesScrollAreaLayout::moveViewportUpByScrollingItem, this, &MessagesScrollAreaLayout::onMoveViewportUpByScrollingItem, Qt::QueuedConnection);

        resetScrollActivityTimer_.setInterval(scrollActivityTimeout.count());
        connect(&resetScrollActivityTimer_, &QTimer::timeout, this, [this]() {
            scrollActivityFlag_ = false;
            resetScrollActivityTimer_.stop();
        });
    }

    MessagesScrollAreaLayout::~MessagesScrollAreaLayout()
    {
        ScrollArea_->removeEventFilter(this);
        Utils::InterConnector::instance().getMainWindow()->removeEventFilter(this);
    }

    void MessagesScrollAreaLayout::setGeometry(const QRect &r)
    {
        QLayout::setGeometry(r);

        if (updatesLocker_.isLocked())
        {
            return;
        }

        // -----------------------------------------------------------------------
        // setup initial viewport position if needed

        if (LayoutRect_.isEmpty())
        {
            setViewportAbsYImpl(-r.height() + getTypingWidgetHeight());
        }

        // ------------------------------------------------------------------------
        // check if the height of some item had been changed

        if (IsDirty_)
        {
            updateItemsGeometry();

            IsDirty_ = false;
        }

        // ------------------------------------------------------------------------
        // update button down in history
        updateBounds();

        // -----------------------------------------------------------------------
        // check if layout rectangle changed

        const auto layoutRectChanged = (r != LayoutRect_);
        if (!layoutRectChanged)
        {
            return;
        }

        const auto widthChanged = (r.width() != LayoutRect_.width());

        LayoutRect_ = r;

        // -----------------------------------------------------------------------
        // set scrollbar geometry

        const auto scrollbarWidth = Scrollbar_->sizeHint().width();
        assert(scrollbarWidth > 0);

        const QRect scrollbarGeometry(
            LayoutRect_.right() - scrollbarWidth,
            LayoutRect_.top(),
            scrollbarWidth,
            LayoutRect_.height()
        );

        Scrollbar_->setGeometry(scrollbarGeometry);

        // -----------------------------------------------------------------------
        // set viewport geometry

        const QSize newViewportSize(
            LayoutRect_.width(),
            LayoutRect_.height()
        );

        __TRACE(
            "geometry",
            "    old-viewport-size=<" << ViewportSize_ << ">\n"
            "    new-viewport-size=<" << newViewportSize << ">\n"
            "    new-layout-rect=<" << LayoutRect_ << ">"
        );

        const auto isAtBottom = isViewportAtBottom();

        ViewportSize_ = newViewportSize;

        // -----------------------------------------------------------------------
        // update items width

        if (widthChanged)
        {
            std::scoped_lock locker(*this);
            updateItemsWidth();
        }

        // -----------------------------------------------------------------------
        // lock scroll at bottom (if needed)

        if (isAtBottom)
        {
            moveViewportToBottom();

            std::scoped_lock locker(*this);

            if (!ShiftingViewportEnabled_)
            {
                applyShiftingParams();
            }

            applyItemsGeometry();

            applyTypingWidgetGeometry();
        }
        else
        {
            if (!ShiftingViewportEnabled_)
            {
                std::scoped_lock locker(*this);
                applyShiftingParams();
            }
        }
    }

    void MessagesScrollAreaLayout::addItem(QLayoutItem* /*item*/)
    {
    }

    QLayoutItem* MessagesScrollAreaLayout::itemAt(int /*index*/) const
    {
        return nullptr;
    }

    QLayoutItem* MessagesScrollAreaLayout::takeAt(int /*index*/)
    {
        return nullptr;
    }

    int MessagesScrollAreaLayout::count() const
    {
        return 0;
    }

    QSize MessagesScrollAreaLayout::sizeHint() const
    {
        return QSize();
    }

    void MessagesScrollAreaLayout::invalidate()
    {
        IsDirty_ = true;

        QLayout::invalidate();
    }

    QPoint MessagesScrollAreaLayout::absolute2Viewport(const QPoint absPos) const
    {
        auto result = absPos;

        // apply the viewport transformation
        result.ry() -= ViewportAbsY_;

        return result;
    }

    bool MessagesScrollAreaLayout::containsWidget(QWidget *widget) const
    {
        assert(widget);

        return Widgets_.find(widget) != Widgets_.end();
    }

    QWidget* MessagesScrollAreaLayout::getItemByPos(const int32_t pos) const
    {
        assert(pos >= 0);
        assert(pos < (int32_t)LayoutItems_.size());

        if (pos < 0 || pos >= (int32_t)LayoutItems_.size())
            return nullptr;

        return LayoutItems_[pos]->Widget_;
    }

    QWidget* MessagesScrollAreaLayout::getItemByKey(const Logic::MessageKey &key) const
    {
        const auto it = std::find_if(LayoutItems_.cbegin(), LayoutItems_.cend(), [&key](const auto& item) { return item->Key_ == key; });
        if (it != LayoutItems_.cend())
            return (*it)->Widget_;

        return nullptr;
    }

    QWidget* MessagesScrollAreaLayout::extractItemByKey(const Logic::MessageKey& _key)
    {
        if (auto w = getItemByKey(_key))
        {
            extractItemImpl(w, SuspendAfterExtract::no);
            return w;
        }
        return nullptr;
    }

    QWidget* MessagesScrollAreaLayout::getPrevItem(QWidget* _w) const
    {
        auto it = std::find_if(LayoutItems_.cbegin(), LayoutItems_.cend(), [_w](const auto& item) { return item->Widget_ == _w; });
        if (it != LayoutItems_.cend())
        {
            if (++it != LayoutItems_.cend())
                return (*it)->Widget_;
        }

        return nullptr;
    }

    int32_t MessagesScrollAreaLayout::getItemsCount() const
    {
        return (int32_t)LayoutItems_.size();
    }

    int32_t MessagesScrollAreaLayout::getItemsHeight() const
    {
        const auto bounds = getItemsAbsBounds();
        const auto height = bounds.second - bounds.first;
        assert(height >= 0);

        return height;
    }

    Logic::MessageKeyVector MessagesScrollAreaLayout::getItemsKeys() const
    {
        Logic::MessageKeyVector result;
        result.reserve(LayoutItems_.size());

        for (const auto &layoutItem : boost::adaptors::reverse(LayoutItems_))
            result.push_back(layoutItem->Key_);

        return result;
    }

    int32_t MessagesScrollAreaLayout::getViewportAbsY() const
    {
        return ViewportAbsY_;
    }

    MessagesScrollAreaLayout::Interval MessagesScrollAreaLayout::getViewportScrollBounds() const
    {
        if (LayoutItems_.empty())
        {
            const auto top = (-ViewportSize_.height() + getTypingWidgetHeight());

            return Interval(top, top);
        }

        const auto overallContentHeight = (getItemsHeight() + getTypingWidgetHeight());
        const auto scrollHeight = std::max(
            0,
            overallContentHeight - ViewportSize_.height()
        );

        if (scrollHeight == 0)
        {
            const auto bottom = getItemsAbsBounds().second;
            const auto top = (bottom + getTypingWidgetHeight() - ViewportSize_.height());

            return Interval(top, top);
        }

        const auto &topItem = *LayoutItems_.crbegin();
        const auto top = topItem->AbsGeometry_.top();
        const auto bottom = (top + scrollHeight);

        return Interval(top, bottom);
    }

    int32_t MessagesScrollAreaLayout::getViewportHeight() const
    {
        assert(!ViewportSize_.isEmpty());
        return ViewportSize_.height();
    }

    QVector<Logic::MessageKey> MessagesScrollAreaLayout::getWidgetsOverBottomOffset(const int32_t offset) const
    {
        assert(offset >= 0);

        QVector<Logic::MessageKey> result;
        if (LayoutItems_.empty())
            return result;

        const auto itemsAbsBounds = getItemsAbsBounds();
        if (itemsAbsBounds.first == 0 && itemsAbsBounds.second == 0)
            return result;
        const auto absTopY = itemsAbsBounds.first;
        const auto absThresholdY = (absTopY + offset);

        for (const auto& layoutItemPtr : boost::adaptors::reverse(LayoutItems_))
        {
            const auto &layoutItem = *layoutItemPtr;

            const auto &itemAbsGeometry = layoutItem.AbsGeometry_;

            const auto isItemUnderThreshold = (itemAbsGeometry.bottom() > absThresholdY) && !layoutItem.Key_.isDate();
            if (isItemUnderThreshold)
            {
                break;
            }

            //if (!layoutItem.Key_.isDate())
                result.push_back(layoutItem.Key_);
        }

        return result;
    }

    QVector<Logic::MessageKey> MessagesScrollAreaLayout::getWidgetsUnderBottomOffset(const int32_t offset) const
    {
        assert(offset >= 0);

        //qDebug() << "LayoutItems_ size" << LayoutItems_.size();

        QVector<Logic::MessageKey> result;
        if (LayoutItems_.empty())
            return result;

        const auto itemsAbsBounds = getItemsAbsBounds();
        if (itemsAbsBounds.first == 0 && itemsAbsBounds.second == 0)
            return result;
        const auto absBottomY = itemsAbsBounds.second;
        const auto absThresholdY = (absBottomY - offset);

        for (const auto& layoutItemPtr : LayoutItems_)
        {
            const auto &layoutItem = *layoutItemPtr;

            const auto &itemAbsGeometry = layoutItem.AbsGeometry_;

            const auto isItemUnderThreshold = (itemAbsGeometry.top() < absThresholdY) && !layoutItem.Key_.isDate();
            if (isItemUnderThreshold)
            {
                break;
            }

            //if (!layoutItem.Key_.isDate())
                result.push_back(layoutItem.Key_);
        }

        return result;
    }

    void MessagesScrollAreaLayout::insertWidgets(InsertHistMessagesParams&& _params)
    {
        //assert(!_params.widgets.empty());
        //UpdatesLocked_ = true;

        const auto isAtBottom = isViewportAtBottom();

        QRect lastReadMessageGeometry;

        if (_params.isBackgroundMode && _params.lastReadMessageId.has_value())
        {
            const auto it = std::find_if(LayoutItems_.cbegin(), LayoutItems_.cend(), [mess_id = *_params.lastReadMessageId](const auto& x) {
                const auto id = x->Key_.getId();
                return id > 0 && (id <= mess_id); // use less to find first message near needle
            });

            if (it != LayoutItems_.cend())
                lastReadMessageGeometry = (*it)->AbsGeometry_;
        }

        const auto bottom = LayoutItems_.empty() ? 0 : LayoutItems_.front()->AbsGeometry_.bottomLeft().y();

        int addedAfterLastReadHeight = 0;

        removeDatesImpl();

        addedAfterLastReadHeight += insertWidgetsImpl(_params);

        _params.widgets = makeDates(getKeysForDates());
        addedAfterLastReadHeight += insertWidgetsImpl(_params);

        const auto lastReadMessageBottom = lastReadMessageGeometry.bottomLeft().y();
        const auto hasAvailableViewport = (bottom - lastReadMessageBottom + addedAfterLastReadHeight) < (ViewportSize_.height() - getTypingWidgetHeight());

        const auto needScrollToBottom = _params.scrollOnInsert.has_value() && ((isAtBottom && *_params.scrollOnInsert == ScrollOnInsert::ScrollToBottomIfNeeded) || *_params.scrollOnInsert == ScrollOnInsert::ForceScrollToBottom);

        const auto canScrollToBottom = !_params.isBackgroundMode;

        updateItemsProps();

        for (auto w : Widgets_)
            w->update();

        if (needScrollToBottom && canScrollToBottom)
        {
            moveViewportToBottom();
        }

        if (_params.scrollToMesssageId.has_value())
        {
            isInitState_ = true;
            ShiftingParams_ = { _params.scrollToMesssageId , _params.scrollMode, 0 };
            ShiftingViewportEnabled_ = false;

            applyShiftingParams();
        }
        else if (needScrollToBottom && !canScrollToBottom && _params.lastReadMessageId.has_value())
        {
            if (hasAvailableViewport)
            {
                isInitState_ = true;
                ShiftingParams_ = { _params.lastReadMessageId , hist::scroll_mode_type::background, 0 };
                ShiftingViewportEnabled_ = false;

                applyShiftingParams();
            }
        }
        else if (!ShiftingViewportEnabled_)
        {
            applyShiftingParams();
        }

        const bool needCheckVisibility = Ui::get_gui_settings()->get_value<bool>(settings_partial_read, settings_partial_read_deafult());

        applyItemsGeometry(needCheckVisibility);

        applyTypingWidgetGeometry();

        //UpdatesLocked_ = false;


        /// check new messages or quotes Later->Early
        for (const auto& item : boost::adaptors::reverse(LayoutItems_))
        {
            auto widget = item->Widget_;
            //const auto& key = item->Key_;
            /// new message item
            //const auto bMoveHistory = Ui::get_gui_settings()->get_value<bool>(settings_auto_scroll_new_messages, false);
            //if (bMoveHistory)
            {
                ComplexMessage::ComplexMessageItem* complex_item = qobject_cast<ComplexMessage::ComplexMessageItem*>(widget);
                if (complex_item && complex_item->isObserveToSize())
                    installEventFilter(this);

                /// quote
                /*if (key.getId() == QuoteId_)
                {
                    if (auto w = getSelectableItem(widget))
                        w->setQuoteSelection();

                    if (complex_item)
                    {
                        complex_item->installEventFilter(this);

                        ScrollingItems_.push_back(complex_item);
                        connect(complex_item, &QObject::destroyed, this, &MessagesScrollAreaLayout::onDeactivateScrollingItems, Qt::UniqueConnection);
                    }
                }*/
            }
        }

        if (!ScrollingItems_.empty())
        {
            moveViewportUpByScrollingItems();
        }
    }

    void MessagesScrollAreaLayout::removeAllExcluded(const InsertHistMessagesParams& _params)
    {
        auto pred = [&_params](const auto& x)
        {
            return std::any_of(_params.widgets.begin(), _params.widgets.end(), [&x](const auto& keyAndWidget) { return x->Key_.hasId() == keyAndWidget.first.hasId() && x->Key_ == keyAndWidget.first; });
        };

        std::vector<QWidget*> toRemove;

        for (const auto& x : LayoutItems_)
        {
            if (!pred(x))
                toRemove.push_back(x->Widget_);
        }

        removeWidgets(toRemove);
        std::for_each(toRemove.begin(), toRemove.end(), [](auto x) { x->deleteLater(); });
    }

    void MessagesScrollAreaLayout::removeItemsByType(Logic::control_type _type)
    {
        std::vector<QWidget*> toRemove;

        for (const auto& x : LayoutItems_)
        {
            if (x->Key_.getControlType() == _type)
                toRemove.push_back(x->Widget_);
        }

        removeWidgets(toRemove);
        std::for_each(toRemove.begin(), toRemove.end(), [](auto x) { x->deleteLater(); });
    }

    bool MessagesScrollAreaLayout::removeItemAtEnd(Logic::control_type _type)
    {
        const auto it = std::find_if(LayoutItems_.begin(), LayoutItems_.end(), [](const auto& x) { return x->Key_.getControlType() != Logic::control_type::ct_date; }); // find first non-date item
        if (it != LayoutItems_.end() && (*it)->Key_.getControlType() == _type)
        {
            auto w = (*it)->Widget_;
            removeWidget(w);
            w->deleteLater();
            return true;
        }
        return false;
    }

    void MessagesScrollAreaLayout::lock()
    {
        updatesLocker_.lock();
    }

    void MessagesScrollAreaLayout::unlock()
    {
        updatesLocker_.unlock();
    }

    void MessagesScrollAreaLayout::removeWidget(QWidget* widget)
    {
        extractItemImpl(widget, SuspendAfterExtract::yes);
    }

    void MessagesScrollAreaLayout::removeWidgets(const std::vector<QWidget*>& widgets)
    {
        std::scoped_lock locker(*this);
        bool isAtBottom = false;
        bool isItemsDirty = false;
        for (auto widget : widgets)
        {
            assert(widget);
            assert(widget->parent() == ScrollArea_);

            //dumpGeometry(QString().sprintf("before removal of %p", widget));

            // remove the widget from the widgets collection

            const auto widgetIt = Widgets_.find(widget);

            if (widgetIt == Widgets_.end())
            {
                assert(!"the widget is not in the layout");
                continue;
            }

            Widgets_.erase(widgetIt);

            // find the widget in the layout items

            const auto iter = std::find_if(LayoutItems_.begin(), LayoutItems_.end(), [widget](const auto& item) { return item->Widget_ == widget; });
            if (iter == LayoutItems_.end())
            {
                assert(!"can not find widget in layout");
                continue;
            }

            suspendVisibleItem(*iter);

            isAtBottom |= isViewportAtBottom();

            // determine slide operation type

            const auto &layoutItemGeometry = (*iter)->AbsGeometry_;
            assert(layoutItemGeometry.height() >= 0);
            assert(layoutItemGeometry.width() > 0);

            const auto insertBeforeViewportMiddle = (layoutItemGeometry.top() < evalViewportAbsMiddleY());

            auto slideOp = SlideOp::NoSlide;
            if (!layoutItemGeometry.isEmpty())
            {
                slideOp = (insertBeforeViewportMiddle ? SlideOp::SlideUp : SlideOp::SlideDown);
            }

            const auto itemsSlided = slideItemsApart(iter, -layoutItemGeometry.height(), slideOp);

            widget->hide();

            LayoutItems_.erase(iter);
            isItemsDirty |= (isAtBottom || itemsSlided);

        }

        if (isAtBottom)
            moveViewportToBottom();

        if (isItemsDirty)
            applyItemsGeometry();

        applyTypingWidgetGeometry();
    }

    size_t MessagesScrollAreaLayout::widgetsCount() const noexcept
    {
        return Widgets_.size();
    }

    int32_t MessagesScrollAreaLayout::shiftViewportAbsY(const int32_t delta)
    {
        if (delta != 0)
        {
            const auto newViewportAbsY_ = (ViewportAbsY_ + delta);

            if (ShiftingViewportEnabled_)
            {
                setViewportAbsY(newViewportAbsY_);
            }
            else
            {
                if (applyShiftingParams())
                {
                    std::scoped_lock locker(*this);

                    applyItemsGeometry();

                    applyTypingWidgetGeometry();
                }
            }
        }

        return ViewportAbsY_;
    }

    void MessagesScrollAreaLayout::enableViewportShifting(bool enable)
    {
        if (ShiftingViewportEnabled_ != enable)
            ShiftingViewportEnabled_ = enable;
    }

    void MessagesScrollAreaLayout::resetShiftingParams()
    {
        ShiftingParams_ = {};
    }

    bool MessagesScrollAreaLayout::applyShiftingParams()
    {
        if (hist::scroll_mode_type::none == ShiftingParams_.type)
            return false;

        const auto begin = LayoutItems_.cbegin();
        const auto end = LayoutItems_.cend();

        if (begin == end)
            return false;

        int newViewport = 0;

        if (ShiftingParams_.type == hist::scroll_mode_type::unread || ShiftingParams_.type == hist::scroll_mode_type::background)
        {
            int delta = 0;
            auto it = std::find_if(begin, end, [](const auto& x) { return x->Key_.getControlType() == Logic::control_type::ct_new_messages; });
            if (it != end)
            {
                delta = Utils::scale_value(50); // 50 px by design
            }
            else
            {
                it = std::find_if(begin, end, [mess_id = ShiftingParams_.messageId](const auto& x) {
                    const auto id = x->Key_.getId();
                    return id > 0 && (id <= mess_id); // use less to find first message near needle
                });
                if (it == end || it == begin)
                    return false;
                std::advance(it, -1); // first new message
                delta = Utils::scale_value(50 + 32); // 50 px by design; 32 px - height of 'new' plate
            }

            const QRect rect = (*it)->AbsGeometry_;
            const QRect lastMess = (*begin)->AbsGeometry_;
            if (lastMess.bottom() - rect.y() + delta <= (ViewportSize_.height() - getTypingWidgetHeight()))
                newViewport = getViewportScrollBounds().second;
            else
                newViewport = (rect.y() - delta);
        }
        else
        {
            auto it = std::find_if(begin, end, [mess_id = ShiftingParams_.messageId](const auto& x) { return x->Key_.getId() == mess_id; });

            assert(it != end);

            if (it == end)
            {
                it = begin;
            }
            else if (ShiftingParams_.type == hist::scroll_mode_type::search && ShiftingParams_.counter == 0)
            {
                if (auto w = getSelectableItem((*it)->Widget_))
                    w->setQuoteSelection();
            }

            const auto message_item = qobject_cast<HistoryControlPageItem*>((*it)->Widget_);

            const auto itemsbounds = getItemsAbsBounds();
            const auto halfViewportHeight = ViewportSize_.height() / 2;
            const QRect rect = (*it)->AbsGeometry_;

            if ((itemsbounds.second - halfViewportHeight) < rect.y() && rect.height() < halfViewportHeight)
            {
                newViewport = getViewportScrollBounds().second;
            }
            else if (std::abs(itemsbounds.first - rect.y()) > halfViewportHeight)
            {
                const int delta = (message_item && message_item->hasPictureContent()) ? Utils::scale_value(40) : halfViewportHeight;
                newViewport = (rect.y() - delta);
            }
            else
            {
                newViewport = itemsbounds.first;
            }
        }

        ++ShiftingParams_.counter;

        if (newViewport == ViewportAbsY_)
            return false;
        setViewportAbsYImpl(newViewport);
        return true;
    }

    void MessagesScrollAreaLayout::setViewportAbsYImpl(int32_t _y)
    {
        ViewportAbsY_ = _y;
        /*const auto viewportbounds = getViewportScrollBounds();
        const auto itemsbounds = getItemsAbsBounds();
        qDebug() << "set ViewportAbsY_ " << _y << "viewport_h " << ViewportSize_.height()
            << "  viewportbounds " << viewportbounds.first << viewportbounds.second
            << " itemsbounds " << itemsbounds.first << itemsbounds.second
            << " first item " << (!LayoutItems_.empty() ? LayoutItems_[0]->AbsGeometry_.bottomLeft() : QPoint());
            */
    }

    QRect MessagesScrollAreaLayout::absolute2Viewport(QRect absolute) const
    {
        // apply the viewport transformation
        absolute.translate(0, -ViewportAbsY_);

        absolute.setX(getXForItem());
        absolute.setWidth(getWidthForItem());

        return absolute;
    }

    void MessagesScrollAreaLayout::applyItemsGeometry(const bool _checkVisibility)
    {
        const bool isWindowActive = Utils::InterConnector::instance().getMainWindow()->isActiveWindow();
        const bool isUIActive = Utils::InterConnector::instance().getMainWindow()->isUIActive();

        const auto globalMousePos = QCursor::pos();
        const auto localMousePos = ScrollArea_->mapFromGlobal(globalMousePos);

        const auto viewportAbsRect = evalViewportAbsRect();

        const auto preloadMargin = Utils::scale_value(1900);
        const QMargins preloadMargins(0, preloadMargin, 0, preloadMargin);
        const auto viewportActivityAbsRect = viewportAbsRect.marginsAdded(preloadMargins);

        const auto visibilityMargin = Utils::scale_value(0);
        const QMargins visibilityMargins(0, visibilityMargin, 0, visibilityMargin);
        const auto viewportVisibilityAbsRect = viewportAbsRect.marginsAdded(visibilityMargins);

        const auto isPartialReadEnabled = scrollActivityFlag_ && Ui::get_gui_settings()->get_value<bool>(settings_partial_read, settings_partial_read_deafult());

        for (auto &item : LayoutItems_)
        {
            const auto &widgetAbsGeometry = item->AbsGeometry_;

            const auto isGeometryActive = viewportActivityAbsRect.intersects(widgetAbsGeometry);

            const auto isActivityChanged = (item->IsActive_ != isGeometryActive);
            if (isActivityChanged)
            {
                item->IsActive_ = isGeometryActive;

                onItemActivityChanged(item->Widget_, isGeometryActive);
            }

            const auto isVisibleForPlay = isVisibleEnoughForPlay(item->Widget_, widgetAbsGeometry, viewportVisibilityAbsRect) && isWindowActive;
            const auto isVisibleForRead = isVisibleEnoughForRead(item->Widget_, widgetAbsGeometry, viewportVisibilityAbsRect) && (isUIActive || isPartialReadEnabled);

            if (_checkVisibility)
            {
                auto isVisibilityChanged = (item->isVisibleEnoughForPlay_ != isVisibleForPlay);

                if (isVisibilityChanged)
                {
                    item->isVisibleEnoughForPlay_ = isVisibleForPlay;

                    onItemVisibilityChanged(item->Widget_, isVisibleForPlay);
                }

                isVisibilityChanged = (item->isVisibleEnoughForRead_ != isVisibleForRead);

                if (isVisibilityChanged)
                {
                    item->isVisibleEnoughForRead_ = isVisibleForRead;

                    onItemRead(item->Widget_, isVisibleForRead);
                }
            }

            if (!widgetAbsGeometry.isEmpty())
            {
                const auto widgetGeometry = absolute2Viewport(widgetAbsGeometry);

                const auto geometryChanged = (item->Widget_->geometry() != widgetGeometry);
                if (geometryChanged)
                {
                    item->Widget_->setGeometry(widgetGeometry);

                    simulateMouseEvents(*item, widgetGeometry, globalMousePos, localMousePos);
                }

            }

            item->IsGeometrySet_ = true;
        }
    }

    void MessagesScrollAreaLayout::readVisibleItems()
    {
        for (auto &item : LayoutItems_)
            if (item->isVisibleEnoughForRead_)
                onItemRead(item->Widget_, true);
    }

    void MessagesScrollAreaLayout::applyTypingWidgetGeometry()
    {
        QRect typingWidgetGeometry(
            getXForItem(),
            getItemsAbsBounds().second,
            getWidthForItem(),
            getTypingWidgetHeight()
        );

        assert(TypingWidget_);

        TypingWidget_->setGeometry(absolute2Viewport(typingWidgetGeometry));
    }

    QRect MessagesScrollAreaLayout::calculateInsertionRect(const ItemsInfoIter &itemInfoIter, Out SlideOp &slideOp)
    {
        assert(itemInfoIter != LayoutItems_.end());

        Out slideOp = SlideOp::NoSlide;

        const auto widgetHeight = evaluateWidgetHeight((*itemInfoIter)->Widget_);
        assert(widgetHeight >= 0);

        const auto isInitialInsertion = (LayoutItems_.size() == 1);
        if (isInitialInsertion)
        {
            assert(itemInfoIter == LayoutItems_.begin());

            const QRect result(
                getXForItem(),
                -widgetHeight,
                getWidthForItem(),
                widgetHeight
            );

            __TRACE(
                "geometry",
                "initial insertion\n"
                "    " << result
            );

            return result;
        }

        const auto isAppend = ((itemInfoIter + 1) == LayoutItems_.end());
        if (isAppend)
        {
            // add to geometry top

            const auto &prevItemInfo = *(itemInfoIter - 1);

            const auto &prevItemGeometry = prevItemInfo->AbsGeometry_;

            const QRect result(
                getXForItem(),
                prevItemGeometry.top() - widgetHeight,
                getWidthForItem(),
                widgetHeight
            );

            __TRACE(
                "geometry",
                "added to the top\n"
                "    geometry=<" << result << ">"
            );

            return result;
        }

        const auto isPrepend = (itemInfoIter == LayoutItems_.begin());
        if (isPrepend)
        {
            // add to geometry bottom

            const auto &nextItemInfo = *(itemInfoIter + 1);

            const auto &nextItemGeometry = nextItemInfo->AbsGeometry_;
            assert(nextItemGeometry.width() > 0);
            assert(nextItemGeometry.height() >= 0);

            const QRect result(
                getXForItem(),
                nextItemGeometry.bottom() + 1,
                getWidthForItem(),
                widgetHeight
            );

            __TRACE(
                "geometry",
                "added to the bottom\n"
                "    "<< result
            );

            return result;
        }

        // insert in the middle

        const auto &overItemInfo = **(itemInfoIter + 1);
        const auto &underItemInfo = **(itemInfoIter - 1);

        const auto insertionY = underItemInfo.AbsGeometry_.top();

        const auto insertBeforeViewportMiddle = (insertionY < evalViewportAbsMiddleY());

        if (insertBeforeViewportMiddle)
        {
            if (widgetHeight > 0)
            {
                Out slideOp =  SlideOp::SlideUp;
            }

            const QRect result(
                getXForItem(),
                overItemInfo.AbsGeometry_.bottom() + 1,
                getWidthForItem(),
                widgetHeight
            );

            __TRACE(
                "geometry",
                "inserted before viewport middle\n"
                "    geometry=<" << result << ">\n"
                "    height=<" << widgetHeight << ">\n"
                "    widget-over=<" << overItemInfo.Widget_ << ">\n"
                "    widget-over-geometry=<" << overItemInfo.AbsGeometry_ << ">"
            );

            return result;
        }

        if (widgetHeight > 0)
        {
            Out slideOp = SlideOp::SlideDown;
        }

        const QRect result(
            getXForItem(),
            underItemInfo.AbsGeometry_.top(),
            getWidthForItem(),
            widgetHeight
        );

        __TRACE(
            "geometry",
            "inserted after viewport middle\n"
            "    " << result
        );

        return result;
    }

    void MessagesScrollAreaLayout::debugValidateGeometry()
    {
        if constexpr (!build::is_debug())
        {
            return;
        }

        if (LayoutItems_.empty())
        {
            return;
        }

        auto iter = LayoutItems_.begin();
        for (; ; ++iter)
        {
            const auto next = (iter + 1);

            if (next == LayoutItems_.end())
            {
                break;
            }

            const auto &geometry = (*iter)->AbsGeometry_;
            const auto &nextGeometry = (*next)->AbsGeometry_;

            if (nextGeometry.bottom() > geometry.top())
            {
                __TRACE(
                    "geometry",
                    "    widget-above=<" << (*next)->Widget_ << ">\n"
                    "    widget-below=<" << (*iter)->Widget_ << ">\n"
                    "    widget-above-bottom=<" << nextGeometry.bottom() << ">\n"
                    "    widget-below-top=<" << geometry.top() << ">\n"
                    "    error=<intersection>"
                );
            }
        }
    }

    void MessagesScrollAreaLayout::removeDatesImpl()
    {
        std::vector<QWidget*> dates;
        dates.reserve(LayoutItems_.size());

        for (const auto& x : LayoutItems_)
        {
            if (x->Key_.isDate())
                dates.push_back(x->Widget_);
        }

        removeWidgets(dates);
        std::for_each(dates.begin(), dates.end(), [](auto x) { x->deleteLater(); });
    }

    std::vector<Logic::MessageKey> MessagesScrollAreaLayout::getKeysForDates() const
    {
        std::vector<Logic::MessageKey> result;

        if (LayoutItems_.empty())
            return result;

        auto prev = LayoutItems_.rbegin();
        while (prev != LayoutItems_.rend())
        {
            if ((*prev)->Key_.getControlType() == Logic::control_type::ct_message)
                break;
            ++prev;
        }
        if (prev != LayoutItems_.rend())
        {
            result.push_back((*prev)->Key_);
            for (auto it = std::next(prev), end = LayoutItems_.rend(); it != end; ++it)
            {
                if ((*it)->Key_.getControlType() == Logic::control_type::ct_message)
                {
                    if (dateInserter_->needDate((*prev)->Key_, (*it)->Key_))
                        result.push_back((*it)->Key_);
                    prev = it;
                }
            }
        }
        return result;
    }

    InsertHistMessagesParams::WidgetsList MessagesScrollAreaLayout::makeDates(const std::vector<Logic::MessageKey>& _keys) const
    {
        InsertHistMessagesParams::WidgetsList result;
        result.reserve(_keys.size());
        for (const auto& key : _keys)
        {
            const auto dateKey = dateInserter_->makeDateKey(key);
            auto w = dateInserter_->makeDateItem(dateKey, getWidthForItem(), ScrollArea_);
            result.push_back({ dateKey, std::move(w) });
        }
        return result;
    }

    int MessagesScrollAreaLayout::insertWidgetsImpl(InsertHistMessagesParams& _params)
    {
        int addedAfterLastReadHeight = 0;
        for (auto& _widget_and_position : _params.widgets)
        {
            const auto key = _widget_and_position.first;

            assert(_widget_and_position.second);

            // -----------------------------------------------------------------------
            // check if the widget was already inserted

            if (containsWidget(_widget_and_position.second.get()))
            {
                assert(!"the widget is already in the layout");
                continue;
            }

            const auto widget = _widget_and_position.second.release();

            if (auto complexMessage = qobject_cast<ComplexMessage::ComplexMessageItem*>(widget))
            {
                connect(
                    complexMessage,
                    &ComplexMessage::ComplexMessageItem::selectionChanged,
                    ScrollArea_,
                    &MessagesScrollArea::notifySelectionChanges, Qt::UniqueConnection);
            }

            Widgets_.emplace(widget);

            // -----------------------------------------------------------------------
            // apply widget width (if needed)

            applyWidgetWidth(getWidthForItem(), widget, true);

            // -----------------------------------------------------------------------
            // find a proper place for the widget

            auto itemInfoIter = insertItem(widget, key);

            // -----------------------------------------------------------------------
            // show the widget to enable geometry operations on it

            widget->show();

            // -----------------------------------------------------------------------
            // calculate insertion position and make the space to put the widget in

            auto slideOp = SlideOp::NoSlide;
            const auto absolutePosition = calculateInsertionRect(itemInfoIter, Out slideOp);
            (*itemInfoIter)->AbsGeometry_ = absolutePosition;

            const auto insertedItemHeight = (*itemInfoIter)->AbsGeometry_.height();
            slideItemsApart(itemInfoIter, insertedItemHeight, slideOp);

            if (_params.lastReadMessageId && (*itemInfoIter)->Key_.getId() > *_params.lastReadMessageId)
                addedAfterLastReadHeight += insertedItemHeight;

            if (_params.forceUpdateItems)
                updateItemsProps(); // TODO optimize me: fix jitter in chat: IMDESKTOP-9432. Now it's overhead.
        }

        return addedAfterLastReadHeight;
    }

    void MessagesScrollAreaLayout::dumpGeometry(const QString &notes)
    {
        assert(!notes.isEmpty());

        if constexpr (!build::is_debug())
        {
            return;
        }

        __INFO(
            "geometry.dump",
            "************************************************************************\n"
            "*** " << notes
        );

        __INFO(
            "geometry.dump",
            "    scroll-bounds=<" << getViewportScrollBounds() << ">\n"
            "    items-bounds=<" << getItemsAbsBounds() << ">"
        );

        for (auto iter = LayoutItems_.crbegin(); iter != LayoutItems_.crend(); ++iter)
        {
            const auto pos = (iter - LayoutItems_.crbegin());

            const auto contentClassName = qsl("no");

            __INFO(
                "geometry.dump",
                "    index=<" << pos << ">\n"
                "    widget=<" << (*iter)->Widget_ << ">\n"
                "    class=<" << (*iter)->Widget_->metaObject()->className() << ">\n"
                "    content=<" << contentClassName << ">\n"
                "    abs-geometry=<" << (*iter)->AbsGeometry_ << ">\n"
                "    rel-y-inclusive=<" << getRelY((*iter)->AbsGeometry_.top()) << "," << getRelY((*iter)->AbsGeometry_.bottom() + 1) << ">"
            );
        }

        __INFO(
            "geometry.dump",
            "*** " << notes << "\n"
            "************************************************************************"
        );
    }

    int32_t MessagesScrollAreaLayout::evalViewportAbsMiddleY() const
    {
        return (
            ViewportAbsY_ +
            (ViewportSize_.height() / 2)
        );
    }

    QRect MessagesScrollAreaLayout::evalViewportAbsRect() const
    {
        QRect result(
            0,
            ViewportAbsY_,
            ViewportSize_.width(),
            ViewportSize_.height());

        return result;
    }

    MessagesScrollAreaLayout::Interval MessagesScrollAreaLayout::getItemsAbsBounds() const
    {
        if (LayoutItems_.empty())
            return Interval(0, 0);

        const auto &topItem = **(LayoutItems_.crbegin());
        const auto &bottomItem = **LayoutItems_.cbegin();

        const auto itemsAbsTop = topItem.AbsGeometry_.top();

        auto offset = 0;
        if (auto w = qobject_cast<HistoryControlPageItem*>(bottomItem.Widget_))
        {
            if (w->isChat())
                offset = w->bottomOffset();
            else
                offset = MessageStyle::getLastReadAvatarSize();
        }

        const auto itemsAbsBottom = (bottomItem.AbsGeometry_.bottom() + 1 - offset + MessageStyle::getHistoryBottomOffset());

        assert(itemsAbsBottom >= itemsAbsTop);
        return Interval(itemsAbsTop, itemsAbsBottom);
    }

    void MessagesScrollAreaLayout::setHeadContainer(Heads::HeadContainer* _heads)
    {
        heads_ = _heads;
    }

    void MessagesScrollAreaLayout::checkVisibilityForRead()
    {
        if (Ui::get_gui_settings()->get_value<bool>(settings_partial_read, settings_partial_read_deafult()))
        {
            applyItemsGeometry(true);
            readVisibleItems();
        }
    }

    int32_t MessagesScrollAreaLayout::getRelY(const int32_t y) const
    {
        const auto itemsRect = getItemsAbsBounds();

        return (itemsRect.second - y);
    }

    int32_t MessagesScrollAreaLayout::getTypingWidgetHeight() const
    {
        assert(TypingWidget_);
        return TypingWidget_->height();
    }

    MessagesScrollAreaLayout::ItemsInfoIter MessagesScrollAreaLayout::insertItem(QWidget *widget, const Logic::MessageKey &key)
    {
        assert(widget);

        auto info = std::make_unique<ItemInfo>(widget, key);

        auto inserted = qobject_cast<MessageItemBase*>(widget);

        const auto isInitialInsertion = LayoutItems_.empty();
        const auto end = LayoutItems_.end();
        if (isInitialInsertion)
        {
            return LayoutItems_.emplace(end, std::move(info));
        }

        const auto begin = LayoutItems_.begin();

        {
            // insert to the geometry bottom

            const auto iterFirst = begin;
            const auto &keyFirst = (*iterFirst)->Key_;
            const auto isPrepend = (keyFirst < key);
            if (isPrepend)
            {
                if (auto first = qobject_cast<MessageItemBase*>((*begin)->Widget_))
                {
                    if (inserted)
                    {
                        first->setNextHasSenderName(inserted->hasSenderName());
                        first->setNextIsOutgoing(inserted->isOutgoing());
                    }
                }

                LayoutItems_.emplace_front(std::move(info));

                return LayoutItems_.begin();
            }
        }

        {
            // insert to the geometry top

            const auto iterLast = std::prev(end);
            const auto &keyLast = (*iterLast)->Key_;
            const auto isAppend = (key < keyLast);
            if (isAppend)
            {
                if (auto last = qobject_cast<MessageItemBase*>((*iterLast)->Widget_))
                {
                    if (inserted)
                    {
                        inserted->setNextHasSenderName(last->hasSenderName());
                        inserted->setNextIsOutgoing(last->isOutgoing());
                    }
                }

                return LayoutItems_.emplace(LayoutItems_.end(), std::move(info));
            }
        }


        // insert into the middle

        auto iter = begin;
        for (; iter != end; ++iter)
        {
            if ((*iter)->Key_ < key)
            {
                break;
            }
        }

        if (iter != end)
        {
            if (auto middle = qobject_cast<MessageItemBase*>((*iter)->Widget_))
            {
                if (inserted)
                {
                    middle->setNextHasSenderName(inserted->hasSenderName());
                    middle->setNextIsOutgoing(inserted->isOutgoing());
                }
            }
        }

        if (iter != begin)
        {
            auto nextItem = std::prev(iter);
            if (auto nextItemW = qobject_cast<MessageItemBase*>((*nextItem)->Widget_))
            {
                if (inserted)
                {
                    inserted->setNextHasSenderName(nextItemW->hasSenderName());
                    inserted->setNextIsOutgoing(nextItemW->isOutgoing());
                }
            }
        }

        return LayoutItems_.emplace(iter, std::move(info));
    }

    bool MessagesScrollAreaLayout::isViewportAtBottom() const
    {
        if (LayoutItems_.empty())
        {
            return true;
        }

        const auto scrollBounds = getViewportScrollBounds();
        const auto isEmpty = (scrollBounds.second == scrollBounds.first);
        if (isEmpty)
        {
            return true;
        }

        const auto isAtBottom = (
            ViewportAbsY_ >= scrollBounds.second
        );

        __TRACE(
            "geometry",
            "    viewport-y=<" << ViewportAbsY_ << ">\n"
            "    scroll-bounds=<" << scrollBounds << ">"
        );

        return isAtBottom;
    }

    void MessagesScrollAreaLayout::resumeVisibleItems()
    {
        const bool isWindowActive = Utils::InterConnector::instance().getMainWindow()->isActiveWindow();

        const auto isPartialReadEnabled = scrollActivityFlag_ && Ui::get_gui_settings()->get_value<bool>(settings_partial_read, settings_partial_read_deafult());

        for (auto &item : LayoutItems_)
        {
            if (item->IsGeometrySet_)
            {
                item->IsActive_ = true;

                onItemActivityChanged(item->Widget_, true);

                const auto &widgetAbsGeometry = item->AbsGeometry_;
                const auto viewportAbsRect = evalViewportAbsRect();
                const auto isGeometryActive = viewportAbsRect.intersects(widgetAbsGeometry);

                const auto isActivityChanged = (item->IsActive_ != isGeometryActive);
                if (isActivityChanged)
                {
                    item->IsActive_ = isGeometryActive;

                    onItemActivityChanged(item->Widget_, isGeometryActive);
                }

                const auto visibilityMargin = Utils::scale_value(0);
                const QMargins visibilityMargins(0, visibilityMargin, 0, visibilityMargin);
                const auto viewportVisibilityAbsRect = viewportAbsRect.marginsAdded(visibilityMargins);

                const auto isVisibleForPlay = isVisibleEnoughForPlay(item->Widget_, widgetAbsGeometry, viewportVisibilityAbsRect) && isWindowActive;
                const auto isVisibleForRead = isVisibleEnoughForRead(item->Widget_, widgetAbsGeometry, viewportVisibilityAbsRect) && (isWindowActive || isPartialReadEnabled);

                auto isVisibilityChanged = (item->isVisibleEnoughForPlay_ != isVisibleForPlay);

                if (isVisibilityChanged)
                {
                    item->isVisibleEnoughForPlay_ = isVisibleForPlay;

                    onItemVisibilityChanged(item->Widget_, isVisibleForPlay);
                }

                isVisibilityChanged = (item->isVisibleEnoughForRead_ != isVisibleForRead);

                if (isVisibilityChanged)
                {
                    item->isVisibleEnoughForRead_ = isVisibleForRead;

                    onItemRead(item->Widget_, isVisibleForRead);
                }
            }
        }
    }

    void MessagesScrollAreaLayout::updateDistanceForViewportItems()
    {
        for (auto &item : LayoutItems_)
        {
            if (item->IsGeometrySet_)
            {
                const auto &widgetAbsGeometry = item->AbsGeometry_;
                const auto viewportAbsRect = evalViewportAbsRect();

                const auto visibilityMargin = Utils::scale_value(0);
                const QMargins visibilityMargins(0, visibilityMargin, 0, visibilityMargin);
                const auto viewportVisibilityAbsRect = viewportAbsRect.marginsAdded(visibilityMargins);

                onItemDistanseToViewPortChanged(item->Widget_, widgetAbsGeometry, viewportVisibilityAbsRect);
            }
        }
    }

    void MessagesScrollAreaLayout::suspendVisibleItems()
    {
        for (const auto &item : LayoutItems_)
            suspendVisibleItem(item);
    }

    QPoint MessagesScrollAreaLayout::viewport2Absolute(QPoint viewportPos) const
    {
        // apply the absolute transformation
        viewportPos.ry() += ViewportAbsY_;
        return viewportPos;
    }

    void MessagesScrollAreaLayout::moveViewportToBottom()
    {
        const auto scrollBounds = getViewportScrollBounds();

        const auto newViewportAbsY = scrollBounds.second;

        __TRACE(
            "geometry",
            "    old-viewport-y=<" << ViewportAbsY_ << ">\n"
            "    new-viewport-y=<" << newViewportAbsY << ">\n"
            "    viewport-height=<" << ViewportSize_.height() << ">"
        );

        setViewportAbsYImpl(newViewportAbsY);
    }

    void MessagesScrollAreaLayout::setViewportByOffset(const int32_t bottomOffset)
    {
        assert(bottomOffset >= 0);

        const auto viewportBounds = getViewportScrollBounds();

        auto newViewportAbsY = viewportBounds.second;
        newViewportAbsY -= bottomOffset;

        setViewportAbsY(newViewportAbsY);
    }

    void MessagesScrollAreaLayout::onItemActivityChanged(QWidget *widget, const bool isActive)
    {
        auto item = qobject_cast<HistoryControlPageItem*>(widget);

        if (item)
        {
            item->onActivityChanged(isActive);
            return;
        }
    }

    void MessagesScrollAreaLayout::onItemVisibilityChanged(QWidget *widget, const bool isVisible)
    {
        auto item = qobject_cast<HistoryControlPageItem*>(widget);

        if (item)
        {
            item->onVisibilityChanged(isVisible);

            return;
        }
    }

    void MessagesScrollAreaLayout::onItemRead(QWidget *widget, const bool _isVisible)
    {
        auto item = qobject_cast<HistoryControlPageItem*>(widget);

        if (item)
        {
            emit itemRead(item->getId(), _isVisible);
        }
    }


    void MessagesScrollAreaLayout::onItemDistanseToViewPortChanged(QWidget *widget, const QRect& _widgetAbsGeometry, const QRect& _viewportVisibilityAbsRect)
    {
        auto item = qobject_cast<HistoryControlPageItem*>(widget);

        if (item)
        {
            item->onDistanceToViewportChanged(_widgetAbsGeometry, _viewportVisibilityAbsRect);
            return;
        }
    }

    bool MessagesScrollAreaLayout::setViewportAbsY(const int32_t absY)
    {
        const auto viewportBounds = getViewportScrollBounds();

        const auto newViewportAbsY = std::clamp(absY, viewportBounds.first, viewportBounds.second);
        if (newViewportAbsY == ViewportAbsY_)
            return false;

        setViewportAbsYImpl(newViewportAbsY);

        {
            std::scoped_lock locker(*this);

            updateDistanceForViewportItems();

            applyItemsGeometry();

            applyTypingWidgetGeometry();
        }


        /// move new offset to HistporyControlPage
        emit updateHistoryPosition(newViewportAbsY, viewportBounds.second);

        return true;
    }

    void MessagesScrollAreaLayout::simulateMouseEvents(
        ItemInfo &itemInfo,
        const QRect &scrollAreaWidgetGeometry,
        const QPoint &globalMousePos,
        const QPoint &scrollAreaMousePos)
    {
        if (scrollAreaWidgetGeometry.isEmpty())
        {
            return;
        }

        if (!ScrollArea_->isVisible())
        {
            return;
        }

        if (!ScrollArea_->isScrolling())
        {
            return;
        }

        auto widget = itemInfo.Widget_;
        assert(widget);

        const auto oldHoveredState = itemInfo.IsHovered_;
        const auto newHoveredState = scrollAreaWidgetGeometry.contains(scrollAreaMousePos);

        itemInfo.IsHovered_ = newHoveredState;

        const auto isMouseLeftWidget = (oldHoveredState && !newHoveredState);
        if (isMouseLeftWidget)
        {
            QEvent leaveEvent(QEvent::Leave);
            QApplication::sendEvent(widget, &leaveEvent);
            return;
        }

        const auto widgetMousePos = widget->mapFromGlobal(globalMousePos);
        QMouseEvent moveEvent(QEvent::MouseMove, widgetMousePos, Qt::NoButton, Qt::NoButton, Qt::NoModifier);
        QApplication::sendEvent(widget, &moveEvent);
    }

    bool MessagesScrollAreaLayout::slideItemsApart(const ItemsInfoIter &changedItemIter, const int slideY, const SlideOp slideOp)
    {
        assert(slideOp > SlideOp::Min);
        assert(slideOp < SlideOp::Max);
        assert(
            (slideOp == SlideOp::NoSlide) ||
            (slideY != 0));

        __TRACE(
            "geometry",
            "    widget=<" << (*changedItemIter)->Widget_ << ">\n"
            "    slide-y=<" << slideY << ">\n"
            "    slide-op=<" << slideOp << ">"
        );

        const auto nothingToSlide = (
            (slideOp == SlideOp::NoSlide) ||
            (LayoutItems_.size() == 1)
        );
        if (nothingToSlide)
        {
            __TRACE(
                "geometry",
                "    nothing to slide"
            );

            return false;
        }

        if (slideOp == SlideOp::SlideUp)
        {
            (*changedItemIter)->AbsGeometry_.translate(0, -slideY);

            auto iter = std::next(changedItemIter);
            const auto end = LayoutItems_.end();

            if (iter == end)
            {
                __TRACE(
                    "geometry",
                    "    op=<" << slideOp << ">\n"
                    "    nothing to slide except for the changed item"
                );

                // nothing to slide up except for the changed item
                return false;
            }

            for (; iter != end; ++iter)
            {
                const auto &geometry = (*iter)->AbsGeometry_;
                const auto newGeometry = geometry.translated(0, -slideY);

                __TRACE(
                    "geometry",
                    "    op=<" << slideOp << ">\n"
                    "    widget=<" << (*iter)->Widget_ << ">\n"
                    "    old-geometry=<" << geometry << ">\n"
                    "    new-geometry=<" << newGeometry << ">");

                (*iter)->AbsGeometry_ = newGeometry;
            }

            return true;
        }

        assert(slideOp == SlideOp::SlideDown);

        if (changedItemIter == LayoutItems_.begin())
        {
            // nothing to slide down
            return false;
        }

        auto iter = std::prev(changedItemIter);
        const auto begin = LayoutItems_.begin();

        for (;; --iter)
        {
            (*iter)->AbsGeometry_.translate(0, slideY);

            if (iter == begin)
            {
                break;
            }
        }

        return true;
    }

    void MessagesScrollAreaLayout::updateItemsWidth()
    {
        const auto viewportAbsMiddleY = evalViewportAbsMiddleY();

        const bool atBottom = ScrollArea_->isScrollAtBottom();

        for (auto iter = LayoutItems_.begin(); iter != LayoutItems_.end(); ++iter)
        {
            auto &item = *iter;

            auto widget = item->Widget_;

            applyWidgetWidth(getWidthForItem(), widget, true);

            const auto oldGeometry = item->AbsGeometry_;

            const auto newHeight = evaluateWidgetHeight(widget);

            const QRect newGeometry(
                oldGeometry.left(),
                oldGeometry.top(),
                getWidthForItem(),
                newHeight
            );

            item->AbsGeometry_ = newGeometry;

            const auto deltaY = (newGeometry.height() - oldGeometry.height());
            if (deltaY != 0)
            {
                const auto changeAboveViewportMiddle = (oldGeometry.bottom() < viewportAbsMiddleY);

                const auto slideOp = (
                    changeAboveViewportMiddle ? SlideOp::SlideUp : SlideOp::SlideDown
                );

                slideItemsApart(iter, deltaY, slideOp);

                if (atBottom)
                    ScrollArea_->scrollToBottom();
            }
        }

        applyItemsGeometry();

        applyTypingWidgetGeometry();

        ScrollArea_->updateScrollbar();
    }

    void MessagesScrollAreaLayout::updateItemsGeometry()
    {
        assert(IsDirty_);
        assert(!updatesLocker_.isLocked());

        if (LayoutItems_.empty())
        {
            return;
        }

        const auto isAtBottom = isViewportAtBottom();

        auto geometryChanged = false;

        std::scoped_lock locker(*this);

        for (
                auto iter = LayoutItems_.begin();
                iter != LayoutItems_.end();
                ++iter
            )
        {
            auto &item = **iter;

            const auto itemHeight = evaluateWidgetHeight(item.Widget_);

            const auto &storedGeometry = item.AbsGeometry_;

            const auto storedItemHeight = storedGeometry.height();

            const auto deltaY = (itemHeight - storedItemHeight);
            if (deltaY == 0)
            {
                continue;
            }

            __TRACE(
                "geometry",
                "    widget=<" << item.Widget_ << ">\n"
                "    stored-height=<" << storedItemHeight << ">\n"
                "    new-height=<" << itemHeight << ">\n"
                "    old-geometry=<" << item.AbsGeometry_ << ">\n"
                "    rel-y-inclusive=<" << getRelY(item.AbsGeometry_.top()) << "," << getRelY(item.AbsGeometry_.bottom() + 1) << ">"
            );

            geometryChanged = true;

            const auto changeAboveViewportMiddle = (storedGeometry.bottom() < evalViewportAbsMiddleY());

            const auto slideOp = (
                changeAboveViewportMiddle ? SlideOp::SlideUp : SlideOp::SlideDown
            );

            item.AbsGeometry_.setHeight(itemHeight);

            slideItemsApart(iter, deltaY, slideOp);

            __TRACE(
                "geometry",
                "    width=<" << item.Widget_ << ">\n"
                "    fixed-geometry=<" << item.AbsGeometry_ << ">"
            );
        }

        if (isAtBottom)
        {
            moveViewportToBottom();
        }

        if (geometryChanged || isAtBottom)
        {
            applyItemsGeometry();
            ScrollArea_->updateScrollbar();
        }

        applyTypingWidgetGeometry();

        //debugValidateGeometry();
    }

    void MessagesScrollAreaLayout::updateItemKey(const Logic::MessageKey &key)
    {
        for (auto &layoutItem : LayoutItems_)
        {
            if (layoutItem->Key_ == key)
            {
                layoutItem->Key_ = key;
                break;
            }
        }
    }

    void MessagesScrollAreaLayout::scrollTo(const Logic::MessageKey &key, hist::scroll_mode_type _scrollMode)
    {
        ShiftingParams_ = { key.getId(), _scrollMode, 0 };
        ShiftingViewportEnabled_ = false;
        if (applyShiftingParams())
        {
            {
                std::scoped_lock locker(*this);
                applyItemsGeometry();
                ScrollArea_->updateScrollbar();
            }

            update();
        }
    }

    void MessagesScrollAreaLayout::enumerateWidgets(const WidgetVisitor& visitor, const bool reversed)
    {
        assert(visitor);

        if (LayoutItems_.empty())
        {
            return;
        }

        auto onItemInfo = [this, &visitor, reversed](const ItemInfo& itemInfo) -> bool
        {
            assert(itemInfo.Widget_);
            if (!itemInfo.Widget_)
            {
                return true;
            }

            const auto &itemGeometry = itemInfo.AbsGeometry_;
            assert(itemGeometry.width() >= 0);
            assert(itemGeometry.height() >= 0);

            const auto isAboveViewport = (itemGeometry.bottom() < ViewportAbsY_);

            const auto viewportBottom = (ViewportAbsY_ + ViewportSize_.height());
            const auto isBelowViewport = (itemGeometry.top() > viewportBottom);

            const auto isHidden = (isAboveViewport || isBelowViewport);

            if (!visitor(itemInfo.Widget_, !isHidden))
                return false;

            auto layout = itemInfo.Widget_->layout();
            if (!layout)
            {
                return true;
            }

            int i = (reversed ? (layout->count() - 1) : 0);
            int i_end = (reversed ? -1 : layout->count());

            while (i != i_end)
            {
                auto item_child = layout->itemAt(i);

                auto widget = item_child->widget();

                if (widget)
                {
                    if (!visitor(widget, !isHidden))
                        return false;
                }

                if (reversed)
                    --i;
                else
                    ++i;
            }

            return true;
        };

        if (reversed)
        {
            for (const auto& item : boost::adaptors::reverse(std::as_const(LayoutItems_)))
            {
                if (!onItemInfo(*item))
                    break;
            }
        }
        else
        {
            for (const auto& item : std::as_const(LayoutItems_))
            {
                if (!onItemInfo(*item))
                    break;
            }
        }
    }

    int MessagesScrollAreaLayout::getWidthForItem() const
    {
        const int viewPortWidth = ViewportSize_.width();

        const int maxItemWidth = MessageStyle::getHistoryWidgetMaxWidth();

        return std::min(viewPortWidth, maxItemWidth);
    }

    int MessagesScrollAreaLayout::getXForItem() const
    {
        return std::max((ViewportSize_.width() - getWidthForItem()) / 2, 0);
    }

    void MessagesScrollAreaLayout::updateItemsPropsDirect()
    {
        if (LayoutItems_.empty())
            return;
        const auto isMultichat = dateInserter_->isChat();

        auto messagesIter = LayoutItems_.begin();

        for (;;)
        {
            auto item = qobject_cast<MessageItemBase*>((*messagesIter)->Widget_);
            auto &message = item->buddy();
            const auto &messageKey = (*messagesIter)->Key_;
            const auto isOutgoing = message.IsVoipEvent() ? message.IsOutgoingVoip() : message.IsOutgoing();

            ++messagesIter;

            const auto isFirstElementReached = (messagesIter == LayoutItems_.end());
            if (isFirstElementReached)
            {
                if (message.GetIndentBefore() || item->hasTopMargin())
                {
                    message.SetIndentBefore(false);
                    item->setTopMargin(false);
                }

                if (isMultichat && !isOutgoing && !messageKey.isDate() && !messageKey.isChatEvent())
                {
                    if (!message.HasSenderName() || !item->hasSenderName())
                    {
                        message.SetHasSenderName(true);
                        item->setHasSenderName(true);
                    }
                }

                return;
            }

            auto prevItem = qobject_cast<MessageItemBase*>((*messagesIter)->Widget_);
            auto &prevMessage = prevItem->buddy();

            auto hasHeads = [this](auto id, auto item)
            {
                return heads_ && heads_->hasHeads(id) && item->headsAtBottom();
            };

            const auto newMessageIndent = !hasHeads(prevMessage.Id_, item) && message.GetIndentWith(prevMessage, isMultichat);

            message.SetIndentBefore(newMessageIndent);
            item->setTopMargin(newMessageIndent);

            if (isMultichat && !isOutgoing)
            {
                const auto hasName = message.hasSenderNameWith(prevMessage, isMultichat);
                message.SetHasSenderName(hasName);
                item->setHasSenderName(hasName);
            }
        }
    }

    void MessagesScrollAreaLayout::updateItemsPropsReverse()
    {
        if (LayoutItems_.empty())
            return;
        const auto canHaveAvatar = [](const auto& _key, const auto& _msg)
        {
            return !_key.isChatEvent() && !_key.isDate() && !_msg.IsDeleted();
        };

        const auto updateChain = [](auto prevMsgIter, auto nextMsgIter)
        {
            const auto checkChained = [](const auto& prevMsg, const auto& nextMsg)
            {
                return prevMsg.isSameDirection(nextMsg) &&
                    !prevMsg.IsDeleted() && !nextMsg.IsDeleted() &&
                    !prevMsg.IsChatEvent() && !nextMsg.IsChatEvent() &&
                    !prevMsg.IsServiceMessage() && !nextMsg.IsServiceMessage() &&
                    !prevMsg.isTimeGapWith(nextMsg) &&
                    prevMsg.GetChatSender() == nextMsg.GetChatSender();
            };

            auto prevItem = qobject_cast<MessageItemBase*>((*prevMsgIter)->Widget_);
            auto& prevMsg = prevItem->buddy();
            auto nextItem = qobject_cast<MessageItemBase*>((*nextMsgIter)->Widget_);
            auto& nextMsg = nextItem->buddy();

            const auto isChained = checkChained(prevMsg, nextMsg);
            if (prevMsg.isChainedToNext() != isChained || prevItem->isChainedToNextMessage() != isChained)
            {
                prevMsg.setHasChainToNext(isChained);
                prevItem->setChainedToNext(isChained);
            }

            if (nextMsg.isChainedToPrev() != isChained || nextItem->isChainedToPrevMessage() != isChained)
            {
                nextMsg.setHasChainToPrev(isChained);
                nextItem->setChainedToPrev(isChained);
            }
        };

        const auto isMultichat = dateInserter_->isChat();

        auto prevIt = LayoutItems_.rbegin();
        for (;;)
        {
            auto prevItem = qobject_cast<MessageItemBase*>((*prevIt)->Widget_);
            auto& prevMsg = prevItem->buddy();
            const auto& prevKey = (*prevIt)->Key_;
            const auto prevOutgoing = prevMsg.IsVoipEvent() ? prevMsg.IsOutgoingVoip() : prevMsg.IsOutgoing();
            const auto prevCanHaveAvatar = isMultichat && !prevOutgoing && canHaveAvatar(prevKey, prevMsg);

            auto it = prevIt;
            ++it;

            if (it == LayoutItems_.rend())
            {
                if (prevCanHaveAvatar && !prevMsg.HasAvatar())
                {
                    prevMsg.SetHasAvatar(true);
                    prevItem->setHasAvatar(true);
                }

                if (prevMsg.isChainedToNext())
                {
                    prevMsg.setHasChainToNext(false);
                    prevItem->setChainedToNext(false);
                }
                return;
            }

            updateChain(prevIt, it);

            prevIt = it;

            if (prevCanHaveAvatar)
            {
                auto curItem = qobject_cast<MessageItemBase*>((*it)->Widget_);
                const auto& curMsg = curItem->buddy();
                const auto& curKey = (*it)->Key_;

                const auto prevNeedAvatar = !canHaveAvatar(curKey, curMsg) || curMsg.hasAvatarWith(prevMsg);
                if (prevMsg.HasAvatar() != prevNeedAvatar)
                {
                    prevMsg.SetHasAvatar(prevNeedAvatar);
                    prevItem->setHasAvatar(prevNeedAvatar);
                }
            }
        }
    }

    bool MessagesScrollAreaLayout::isVisibleEnoughForPlay(const QWidget*, const QRect& _widgetAbsGeometry, const QRect& _viewportVisibilityAbsRect) const
    {
        constexpr auto enough_percent = 0.7;
        const auto intersected = _viewportVisibilityAbsRect.intersected(_widgetAbsGeometry);
        return 1.0 * intersected.height() / _widgetAbsGeometry.height() > enough_percent;
    }

    static double getEnoughPercentForMedia(const QRect& _widgetAbsGeometry, const QRect& _viewportVisibilityAbsRect) noexcept
    {
        constexpr auto percentForMedia = 0.7;
        if (_viewportVisibilityAbsRect.height() > _widgetAbsGeometry.height())
            return percentForMedia;
        return double(_viewportVisibilityAbsRect.height()) / _widgetAbsGeometry.height() * 0.9;
    }

    bool MessagesScrollAreaLayout::isVisibleEnoughForRead(const QWidget* _widget, const QRect& _widgetAbsGeometry, const QRect& _viewportVisibilityAbsRect) const
    {
        const auto messageItem = qobject_cast<const HistoryControlPageItem*>(_widget);
        const auto enoughPercent = messageItem && messageItem->hasPictureContent() ? getEnoughPercentForMedia(_widgetAbsGeometry, _viewportVisibilityAbsRect) : 0.01;
        const auto intersected = _viewportVisibilityAbsRect.intersected(_widgetAbsGeometry);
        return 1.0 * intersected.height() / _widgetAbsGeometry.height() > enoughPercent;
    }

    void MessagesScrollAreaLayout::suspendVisibleItem(const ItemInfoUptr& _item)
    {
        if (_item)
        {
            if (_item->isVisibleEnoughForPlay_)
            {
                _item->isVisibleEnoughForPlay_ = false;

                onItemVisibilityChanged(_item->Widget_, false);
            }

            if (_item->isVisibleEnoughForRead_)
            {
                _item->isVisibleEnoughForRead_ = false;

                onItemRead(_item->Widget_, false);
            }

            _item->IsActive_ = false;

            onItemActivityChanged(_item->Widget_, false);
        }
    }

    void MessagesScrollAreaLayout::updateBounds()
    {
        if (!ViewportSize_.isEmpty())
        {
            const auto viewportBounds = getViewportScrollBounds();
            emit updateHistoryPosition(ViewportAbsY_, viewportBounds.second);
        }
    }

    void MessagesScrollAreaLayout::updateScrollbar()
    {
        ScrollArea_->updateScrollbar();
    }

    void MessagesScrollAreaLayout::updateItemsProps()
    {
        updateItemsPropsReverse();
        updateItemsPropsDirect();
        updateItemsWidth();
    }

    bool MessagesScrollAreaLayout::isInitState() const noexcept
    {
        return isInitState_;
    }

    void MessagesScrollAreaLayout::resetInitState() noexcept
    {
        isInitState_ = false;
    }

    bool MessagesScrollAreaLayout::eventFilter(QObject* watcher, QEvent* e)
    {
        /// messageScrollArea mainWindow
        if (e->type() == QEvent::KeyPress ||
            e->type() == QEvent::MouseButtonPress ||
            e->type() == QEvent::MouseButtonRelease ||
            e->type() == QEvent::Wheel ||
            e->type() == QEvent::TouchBegin ||
            e->type() == QEvent::TouchUpdate ||
            e->type() == QEvent::TouchEnd ||
            e->type() == QEvent::TouchCancel)
        {
            //ScrollArea_->removeEventFilter(this);
            //Utils::InterConnector::instance().getMainWindow()->removeEventFilter(this);

            resetInitState();
            enableViewportShifting(true);
            resetShiftingParams();

            ScrollingItems_.clear();
            scrollActivityFlag_ = true;
            resetScrollActivityTimer_.start();
        }
        else if (e->type() == QEvent::Resize)
        {
            if (ScrollingItems_.size() == 1 && ScrollingItems_.back() == watcher)
            {
                emit moveViewportUpByScrollingItem((QWidget*)watcher);
            }
        }
        return false;
    }

    void MessagesScrollAreaLayout::onButtonDownClicked()
    {
        ScrollingItems_.clear();
    }

    void MessagesScrollAreaLayout::onMoveViewportUpByScrollingItem(QWidget* widget)
    {
        if (!widget)
            return;

        const auto r = widget->geometry();

        const int new_pos = r.top();
        auto delta = Utils::scale_value(40);

        {
            /// move new_message to position
            int dpos = new_pos - delta;
            setViewportAbsYImpl(ViewportAbsY_ - (dpos - getTypingWidgetHeight()));
            for (auto& val : LayoutItems_)
            {
                val->Widget_->setGeometry(val->Widget_->geometry().translated(0, -dpos));
            }
            TypingWidget_->setGeometry(TypingWidget_->geometry().translated(0, -dpos));
        }

        /// move bottom to edge
        if (TypingWidget_->geometry().bottom() < ViewportSize_.height())
        {
            int dpos = ViewportSize_.height() - TypingWidget_->geometry().bottom();

            for (auto& val : LayoutItems_)
            {
                val->Widget_->setGeometry(val->Widget_->geometry().translated(0, dpos));
            }
            TypingWidget_->setGeometry(TypingWidget_->geometry().translated(0, dpos));

            setViewportAbsYImpl(getViewportScrollBounds().second);
        }

        for (auto& val : LayoutItems_)
        {
            val->AbsGeometry_ = val->Widget_->geometry().translated(0, ViewportAbsY_);
        }

        ///  transfer new position to HistporyControlPage (button down)
        emit updateHistoryPosition(ViewportAbsY_, getViewportScrollBounds().second);
        emit recreateAvatarRect();
   }

    void MessagesScrollAreaLayout::extractItemImpl(QWidget* _widget, SuspendAfterExtract _mode)
    {
        assert(_widget);
        assert(_widget->parent() == ScrollArea_);

        // remove the widget from the widgets collection

        const auto widgetIt = Widgets_.find(_widget);

        if (widgetIt == Widgets_.end())
        {
            assert(!"the widget is not in the layout");
            return;
        }

        Widgets_.erase(widgetIt);

        // find the widget in the layout items

        const auto iter = std::find_if(LayoutItems_.begin(), LayoutItems_.end(), [_widget](const auto& item) { return item->Widget_ == _widget; });
        if (iter == LayoutItems_.end())
        {
            assert(!"can not find widget in layout");
            return;
        }

        std::scoped_lock locker(*this);

        const auto isAtBottom = isViewportAtBottom();

        if (_mode == SuspendAfterExtract::yes)
            suspendVisibleItem(*iter);

        // determine slide operation type

        const auto &layoutItemGeometry = (*iter)->AbsGeometry_;
        assert(layoutItemGeometry.height() >= 0);
        assert(layoutItemGeometry.width() > 0);

        const auto insertBeforeViewportMiddle = (layoutItemGeometry.top() < evalViewportAbsMiddleY());

        auto slideOp = SlideOp::NoSlide;
        if (!layoutItemGeometry.isEmpty())
            slideOp = (insertBeforeViewportMiddle ? SlideOp::SlideUp : SlideOp::SlideDown);

        const auto itemsSlided = slideItemsApart(iter, -layoutItemGeometry.height(), slideOp);

        if (_mode == SuspendAfterExtract::yes)
            _widget->hide();

        auto prev = LayoutItems_.erase(iter);
        if (prev != LayoutItems_.cbegin() && prev != LayoutItems_.cend())
        {
            auto next = prev - 1;
            if (next != LayoutItems_.cend())
            {
                auto prevItem = qobject_cast<MessageItemBase*>((*prev)->Widget_);
                auto nextItem = qobject_cast<MessageItemBase*>((*next)->Widget_);
                if (prevItem && nextItem)
                {
                    prevItem->setNextHasSenderName(nextItem->hasSenderName());
                    prevItem->setNextIsOutgoing(nextItem->isOutgoing());
                }
            }
        }

        if (isAtBottom)
            moveViewportToBottom();

        if (const auto isItemsDirty = isAtBottom || itemsSlided; isItemsDirty)
        {
            applyItemsGeometry();
        }

        applyTypingWidgetGeometry();
    }

    void MessagesScrollAreaLayout::moveViewportUpByScrollingItems()
    {
        if (ScrollingItems_.empty() || ScrollingItems_.size() > 1)
            return;

        QWidget* widget = ScrollingItems_.back();
        onMoveViewportUpByScrollingItem(widget);
    }

    void MessagesScrollAreaLayout::onDeactivateScrollingItems(QObject* obj)
    {
        if (obj)
        {
            auto it = std::remove(ScrollingItems_.begin(), ScrollingItems_.end(), obj);
            ScrollingItems_.erase(it, ScrollingItems_.end());

            moveViewportUpByScrollingItems();
        }
        else
        {
            ScrollingItems_.clear();
        }
    }

    namespace
    {
        void applyWidgetWidth(const int32_t viewportWidth, QWidget *widget, bool traverseLayout)
        {
            assert(widget);

            auto chatEventItem = qobject_cast<ChatEventItem*>(widget);
            if (chatEventItem)
            {
                chatEventItem->setFixedWidth(viewportWidth);

                return;
            }

            auto voipEventItem = qobject_cast<VoipEventItem*>(widget);
            if (voipEventItem)
            {
                voipEventItem->setFixedWidth(viewportWidth);

                return;
            }

            auto serviceMessageItem = qobject_cast<ServiceMessageItem*>(widget);
            if (serviceMessageItem)
            {
                serviceMessageItem->setFixedWidth(viewportWidth);

                return;
            }

            auto complexMessageItem = qobject_cast<ComplexMessage::ComplexMessageItem*>(widget);
            if (complexMessageItem)
            {
                complexMessageItem->setFixedWidth(viewportWidth);

                return;
            }

            if (!traverseLayout)
            {
                return;
            }

            auto layout = widget->layout();
            if (!layout)
            {
                return;
            }

            widget->setFixedWidth(viewportWidth);

            auto itemIndex = 0;
            while (auto itemChild = layout->itemAt(itemIndex++))
            {
                if (auto itemWidget = itemChild->widget())
                    applyWidgetWidth(viewportWidth, itemWidget, false);
            }
        }

        int32_t evaluateWidgetHeight(QWidget *widget)
        {
            assert(widget);

            const auto widgetLayout = widget->layout();
            if (widgetLayout)
            {
                if (const auto height = widgetLayout->sizeHint().height(); height >= 0)
                    return height;
            }

            if (const auto height = widget->sizeHint().height(); height >= 0)
                return height;

            return widget->height();
        }
    }

}
