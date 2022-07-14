#include "stdafx.h"

#include "main_window/input_widget/InputWidgetUtils.h"
#include "../smiles_menu/SmilesMenu.h"

#include "../../utils/InterConnector.h"
#include "../../utils/log/log.h"
#include "../../utils/utils.h"
#include "../../my_info.h"
#include "../../controls/TooltipWidget.h"
#include "statuses/StatusTooltip.h"

#include "../../main_window/contact_list/ContactListModel.h"
#include "../../main_window/contact_list/ServiceContacts.h"
#include "main_window/history_control/complex_message/FileSharingUtils.h"
#include "main_window/MainPage.h"

#include "complex_message/ComplexMessageItem.h"

#include "VoipEventItem.h"
#include "MessagesScrollbar.h"
#include "MessagesScrollAreaLayout.h"
#include "HistoryControlPage.h"

#include "MessagesScrollArea.h"
#include "../../gui_settings.h"
#include "../../core_dispatcher.h"

#include "MentionCompleter.h"
#include "../MainWindow.h"

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
    constexpr auto hideScrollBarTimeout = std::chrono::seconds(2);
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
        Q_EMIT scrollViewport(-_dy, QPrivateSignal());
    }

    enum class MessagesScrollArea::ScrollingMode
    {
        Min,

        Plain,
        Selection,

        Max
    };

    MessagesScrollArea::MessagesScrollArea(QWidget *parent, QWidget *typingWidget, hist::DateInserter* dateInserter, const QString& _aimid)
        : QWidget(parent)
        , LastAnimationMoment_(MOMENT_UNINITIALIZED)
        , Mode_(ScrollingMode::Plain)
        , Scrollbar_(new MessagesScrollbar(this))
        , Layout_(new MessagesScrollAreaLayout(this, Scrollbar_, typingWidget, dateInserter))
        , ScrollAnimationTimer_(this)
        , HideScrollbarTimer_(this)
        , ScrollDistance_(0)
        , scrollValue_(-1)
        , messageId_(-1)
        , aimid_(_aimid)
    {
        im_assert(parent);
        im_assert(Layout_);
        im_assert(typingWidget);

        setContentsMargins(0, 0, 0, 0);
        setLayout(Layout_);

        // initially invisible, will became visible when there will be enough content
        setScrollbarVisible(false);

        ScrollAnimationTimer_.setInterval(scrollAnimationInterval);
        connect(&ScrollAnimationTimer_, &QTimer::timeout, this, &MessagesScrollArea::onAnimationTimer);

        HideScrollbarTimer_.setSingleShot(true);
        HideScrollbarTimer_.setInterval(hideScrollBarTimeout);
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
        connect(&Utils::InterConnector::instance(), &Utils::InterConnector::clearSelecting, this, [this]() { updateStateFlag(State::IsSelecting_, false); });
        connect(&Utils::InterConnector::instance(), &Utils::InterConnector::selectionStateChanged, this, &MessagesScrollArea::updateSelected);
        connect(&Utils::InterConnector::instance(), &Utils::InterConnector::pttProgressChanged, this, &MessagesScrollArea::updatePttProgress);
        connect(&Utils::InterConnector::instance(), &Utils::InterConnector::updateSelectedCount, this, &MessagesScrollArea::updateSelectedCount);
        connect(&Utils::InterConnector::instance(), &Utils::InterConnector::multiselectChanged, this, &MessagesScrollArea::multiselectChanged);
        connect(&Utils::InterConnector::instance(), &Utils::InterConnector::multiSelectCurrentElementChanged, this, &MessagesScrollArea::multiSelectCurrentElementChanged);
        connect(&Utils::InterConnector::instance(), &Utils::InterConnector::multiSelectCurrentMessageUp, this, &MessagesScrollArea::multiSelectCurrentMessageUp);
        connect(&Utils::InterConnector::instance(), &Utils::InterConnector::multiSelectCurrentMessageDown, this, &MessagesScrollArea::multiSelectCurrentMessageDown);
        connect(&Utils::InterConnector::instance(), &Utils::InterConnector::messageSelected, this, &MessagesScrollArea::messageSelected);
        connect(&Utils::InterConnector::instance(), &Utils::InterConnector::clearSelection, this, [this]() { clearSelection(); });
        Utils::grabTouchWidget(this);

        setMouseTracking(true);
    }

    void MessagesScrollArea::onUpdateHistoryPosition(int32_t position, int32_t offset)
    {
        Q_EMIT updateHistoryPosition(position, offset);
    }

    void MessagesScrollArea::onHideScrollbarTimer()
    {
        if (!isScrolling() && !Scrollbar_->isHovered())
            Scrollbar_->fadeOut();
    }

    void MessagesScrollArea::multiSelectCurrentElementChanged()
    {
        if (!Utils::InterConnector::instance().isMultiselect(aimid_))
            return;

        if (Utils::InterConnector::instance().currentMultiselectElement() == Utils::MultiselectCurrentElement::Message)
            Utils::InterConnector::instance().setCurrentMultiselectMessage(Layout_->firstAvailableToSelect().getId());
        else
            Utils::InterConnector::instance().setCurrentMultiselectMessage(-1);
    }

    void MessagesScrollArea::multiSelectCurrentMessageUp(bool _shift)
    {
        if (!Utils::InterConnector::instance().isMultiselect(aimid_))
            return;

        if (Utils::InterConnector::instance().currentMultiselectElement() == Utils::MultiselectCurrentElement::Message)
        {
            auto current = Utils::InterConnector::instance().currentMultiselectMessage();
            if (current == -1)
                current = lastSelectedMessageId_.getId();

            auto prev = Layout_->prevAvailableToSelect(current);
            bool prevSelected = false;
            if (prev.getId() != -1)
            {
                Utils::InterConnector::instance().setCurrentMultiselectMessage(prev.getId());
                Layout_->scrollTo(prev, hist::scroll_mode_type::multiselect);
                if (_shift)
                    prevSelected = switchMessageSelection(prev);
            }

            if (_shift)
                switchMessageSelection(Logic::MessageKey(current, QString()), prevSelected);
        }
    }

    void MessagesScrollArea::multiSelectCurrentMessageDown(bool _shift)
    {
        if (!Utils::InterConnector::instance().isMultiselect(aimid_))
            return;

        if (Utils::InterConnector::instance().currentMultiselectElement() == Utils::MultiselectCurrentElement::Message)
        {
            auto current = Utils::InterConnector::instance().currentMultiselectMessage();
            if (current == -1)
                current = lastSelectedMessageId_.getId();

            auto next = Layout_->nextAvailableToSelect(current);
            bool nextSelected = true;
            if (next.getId() != -1)
            {
                Utils::InterConnector::instance().setCurrentMultiselectMessage(next.getId());
                Layout_->scrollTo(next, hist::scroll_mode_type::multiselect);
                if (_shift)
                    nextSelected = switchMessageSelection(next);
            }

            if (_shift)
                switchMessageSelection(Logic::MessageKey(current, QString()), nextSelected);
        }
    }

    void MessagesScrollArea::messageSelected(qint64 _id, const QString& _internalId)
    {
        if (Utils::InterConnector::instance().isMultiselect(aimid_))
            lastSelectedMessageId_ = Logic::MessageKey(_id, _internalId);
        //scrollTo(Logic::MessageKey(_id, _internalId), hist::scroll_mode_type::multiselect);
    }

    bool MessagesScrollArea::switchMessageSelection(Logic::MessageKey _key, std::optional<bool> _prevSelection)
    {
        auto w = getItemByKey(_key);
        bool prevSelected = false;
        if (auto item = qobject_cast<HistoryControlPageItem*>(w))
        {
            prevSelected = item->isSelected();
            if (!_prevSelection || *_prevSelection == prevSelected)
                item->setSelected(!prevSelected);
        }
        return prevSelected;
    }

    void MessagesScrollArea::onWheelEvent(QWheelEvent* e)
    {
        QCoreApplication::sendEvent(this, e);
    }

    void MessagesScrollArea::updateSelected(const QString& _aimId, qint64 _id, const QString& _internalId, bool _selected)
    {
        if (aimid_ == _aimId)
        {
            const auto isMultiselect = Utils::InterConnector::instance().isMultiselect(aimid_);
            Logic::MessageKey key(_id, _internalId);
            if (!_selected)
            {
                selected_.erase(key);
            }
            else if (auto item = qobject_cast<Ui::HistoryControlPageItem*>(Layout_->getItemByKey(key)))
            {
                SelectedMessageInfo info;
                info.isOutgoing_ = item->isOutgoing();
                info.isDisabled_ = ((_id == -1) || item->isUnsupported());
                if (!isMultiselect)
                {
                    auto selectionBegin = Layout_->absolute2Viewport(SelectionBeginAbsPos_);
                    auto selectionEnd = Layout_->absolute2Viewport(SelectionEndAbsPos_);
                    auto selectionBeginGlobal = mapToGlobal(selectionBegin);
                    auto selectionEndGlobal = mapToGlobal(selectionEnd);
                    info.from_ = item->mapFromGlobal(selectionBeginGlobal);
                    info.to_ = item->mapFromGlobal(selectionEndGlobal);
                }
                else
                {
                    info.from_ = QPoint();
                    info.to_ = QPoint();
                }

                if (auto complexItem = qobject_cast<Ui::ComplexMessage::ComplexMessageItem*>(item))
                {
                    info.text_ = complexItem->getSelectedText(isMultiselect, Ui::ComplexMessage::ComplexMessageItem::TextFormat::Raw);
                    info.quotes_ = complexItem->getQuotes(!(Utils::InterConnector::instance().isMultiselect(aimid_)), true);
                    info.filesPlaceholders_ = complexItem->getFilesPlaceholders();
                    info.mentions_ = complexItem->getMentions();
                }
                else if (auto voipItem = qobject_cast<Ui::VoipEventItem*>(item))
                {
                    info.text_ = voipItem->formatCopyText();
                    info.quotes_ += voipItem->getQuote();
                }

                const auto tmp = selected_.find(key);
                if (tmp != selected_.end() && tmp->first.getId() != key.getId())
                    selected_.erase(key);

                selected_[key] = info;
            }

            if (selected_.empty())
                Utils::InterConnector::instance().setMultiselect(false, aimid_);
        }
    }

    void MessagesScrollArea::updateSelectedCount(const QString& _aimId)
    {
        if (aimid_ == _aimId)
            Q_EMIT Utils::InterConnector::instance().selectedCount(aimid_, getSelectedCount(), getSelectedUnsupportedCount(), getSelectedPlainFilesCount(), messagesCanBeDeleted());
    }

    void MessagesScrollArea::multiselectChanged()
    {
        if (!Utils::InterConnector::instance().isMultiselect(aimid_))
            clearSelection();
        else
            Q_EMIT Utils::InterConnector::instance().setFocusOnInput(aimid_);
        Layout_->invalidate();
        Layout_->updateItemsGeometry();
    }

    void MessagesScrollArea::updatePttProgress(qint64 _id, const Utils::FileSharingId& _fsId, int _progress)
    {
        if (Utils::InterConnector::instance().isHistoryPageVisible(aimid_))
        {

            auto iter = std::find_if(pttProgress_.begin(), pttProgress_.end(), [&_id, &_fsId](const auto& p)
            {
                return p.id_ == _id && p.fsId_ == _fsId;
            });

            if (_progress != 0)
            {
                if (iter == pttProgress_.end())
                {
                    PttProgress p(_id, _fsId, _progress);
                    pttProgress_.push_back(std::move(p));
                }
                else
                {
                    iter->progress_ = _progress;
                }
            }
            else if (iter != pttProgress_.end())
            {
                pttProgress_.erase(iter);
            }
        }
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
        return selected_.size();
    }

    int MessagesScrollArea::getSelectedUnsupportedCount() const
    {
        return std::count_if(selected_.begin(), selected_.end(), [](const auto& _p) { return _p.second.isDisabled_; });
    }

    int MessagesScrollArea::getSelectedPlainFilesCount() const
    {
        return std::count_if(selected_.begin(), selected_.end(), [](const auto& _p)
        {
            return std::any_of(_p.second.filesPlaceholders_.begin(), _p.second.filesPlaceholders_.end(), [](const auto& _fp)
            {
                return ComplexMessage::getFileSharingContentType(_fp.first).is_plain_file();
            });
        });
    }

    void MessagesScrollArea::setSmartreplyWidget(SmartReplyWidget* _widget)
    {
        Layout_->setSmartreplyWidget(_widget);
    }

    void MessagesScrollArea::showSmartReplies()
    {
        Layout_->showSmartReplyWidgetAnimated();
    }

    void MessagesScrollArea::hideSmartReplies()
    {
        Layout_->hideSmartReplyWidgetAnimated();
    }

    void MessagesScrollArea::setSmartreplyButton(QWidget* _button)
    {
        Layout_->setSmartreplyButton(_button);
    }

    void MessagesScrollArea::setSmartrepliesSemitransparent(bool _semi)
    {
        Layout_->setSmartrepliesSemitransparent(_semi);
    }

    void MessagesScrollArea::notifyQuotesResize()
    {
        Layout_->notifyQuotesResize();
    }

    bool MessagesScrollArea::hasItems() const noexcept
    {
        return Layout_->hasItems();
    }

    QString MessagesScrollArea::getAimid() const
    {
        return aimid_;
    }

    void MessagesScrollArea::clearPttProgress()
    {
        pttProgress_.clear();
    }

    void MessagesScrollArea::onSelectionChanged(int _selectedCount)
    {
        if (_selectedCount > 0)
            Q_EMIT messagesSelected();
        else
            Q_EMIT messagesDeselected();
    }

    void MessagesScrollArea::insertWidget(const Logic::MessageKey &key, std::unique_ptr<QWidget> widget)
    {
        InsertHistMessagesParams params;
        params.widgets.emplace_back(key, std::move(widget));
        params.scrollOnInsert = std::make_optional(ScrollOnInsert::ScrollToBottomIfNeeded);
        insertWidgets(std::move(params));
    }

    std::map<Logic::MessageKey, QWidget*> MessagesScrollArea::getWidgetsMapFromParams(const InsertHistMessagesParams& _params)
    {
        std::map<Logic::MessageKey, QWidget*> widgets;
        for (const auto& iter : _params.widgets)
            widgets[iter.first] = (iter.second.get());
        return widgets;
    }

    void MessagesScrollArea::insertWidgets(InsertHistMessagesParams&& _params)
    {
        std::map<Logic::MessageKey, QWidget*> widgets = getWidgetsMapFromParams(_params);
        _params.isThread_ = Logic::getContactListModel()->isThread(aimid_);
        {
            std::scoped_lock locker(*Layout_);
            Layout_->insertWidgets(std::move(_params));
        }

        updateScrollbar();

        if (selected_.empty() && pttProgress_.empty())
            return;

        std::vector<Logic::MessageKey> toEmit;
        for (const auto& [key, widget] : widgets)
        {
            if (auto complexItem = qobject_cast<Ui::ComplexMessage::ComplexMessageItem*>(widget))
                contacts_.insert(complexItem->getSenderAimid());
            else if (auto voipItem = qobject_cast<Ui::VoipEventItem*>(widget))
                contacts_.insert(voipItem->getContact());

            if (const auto selectedInfo = std::find_if(selected_.begin(), selected_.end(), [id = key.getId()](const auto& _info) { return id == _info.first.getId(); });
                selectedInfo != selected_.end())
            {
                if (auto item = qobject_cast<Ui::HistoryControlPageItem*>(widget))
                {
                    if (selectedInfo->second.from_.isNull())
                    {
                        item->setSelected(true);
                    }
                    else
                    {
                        auto begin = item->mapToGlobal(selectedInfo->second.from_);
                        auto end = item->mapToGlobal(selectedInfo->second.to_);
                        item->selectByPos(begin, end, SelectionBeginAbsPos_, SelectionEndAbsPos_);
                    }
                    toEmit.push_back(key);
                }
            }


            for (const auto& p : pttProgress_)
            {
                if (p.id_ == key.getId())
                {
                    if (auto pttItem = qobject_cast<Ui::ComplexMessage::ComplexMessageItem*>(widget))
                        pttItem->setProgress(p.fsId_, p.progress_);
                }
            }
        }

        for (const auto& key : toEmit)
            Q_EMIT Utils::InterConnector::instance().selectionStateChanged(aimid_, key.getId(), key.getInternalId(), true);
    }

    bool MessagesScrollArea::isScrolling() const
    {
        return ScrollAnimationTimer_.isActive();
    }

    bool MessagesScrollArea::isSelecting() const
    {
        return (areaState_ & State::IsSelecting_);
    }

    bool MessagesScrollArea::isViewportFull() const
    {
        const auto scrollBounds = Layout_->getViewportScrollBounds();

        const auto scrollRange = (scrollBounds.second - scrollBounds.first);
        im_assert(scrollRange >= 0);

        return (scrollRange > 0);
    }

    bool MessagesScrollArea::containsWidget(QWidget *widget) const
    {
        im_assert(widget);

        return Layout_->containsWidget(widget);
    }

    void MessagesScrollArea::eraseContact(QWidget* widget)
    {
        if (auto complexItem = qobject_cast<Ui::ComplexMessage::ComplexMessageItem*>(widget))
            contacts_.erase(complexItem->getSenderAimid());
        else if (auto voipItem = qobject_cast<Ui::VoipEventItem*>(widget))
            contacts_.erase(voipItem->getContact());
    }

    void MessagesScrollArea::removeWidget(QWidget *widget)
    {
        im_assert(widget);

        eraseContact(widget);

        Layout_->removeWidget(widget);
#ifdef IM_AUTO_TESTING
        widget->setParent(nullptr);
#endif //IM_AUTO_TESTING
        widget->deleteLater();

        updateScrollbar();

        Q_EMIT widgetRemoved();
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
            {
                widgets.push_back(w);
                eraseContact(w);
            }
        }
        Layout_->removeWidgets(widgets);
        std::for_each(widgets.begin(), widgets.end(), [](auto w)
        {
#ifdef IM_AUTO_TESTING
            w->setParent(nullptr);
#endif //IM_AUTO_TESTING
            w->deleteLater();
        });

        Q_EMIT widgetRemoved();
    }

    void MessagesScrollArea::removeAll()
    {
        const auto keys = getItemsKeys();
        cancelWidgetRequests(keys);
        removeWidgets(keys);
    }

    void MessagesScrollArea::replaceWidget(const Logic::MessageKey &key, std::unique_ptr<QWidget> widget)
    {
        im_assert(widget);

        auto existingWidget = getItemByKey(key);
        if (!existingWidget)
            return;
        removeWidget(existingWidget);
        insertWidget(key, std::move(widget));
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
        im_assert(!getIsSearch());
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
        im_assert(scrollbarMaximum >= 0);
        const auto canScroll = scrollbarMaximum > 0;
        const auto scrollbarVisible = canScroll;

        if ((areaState_ & State::IsScrollShowOnOpen_) && canScroll && Layout_->widgetsCount())
        {
            setScrollbarVisible(true);
            updateStateFlag(State::IsScrollShowOnOpen_, false);
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
        updateStateFlag(State::IsSelecting_, false);
    }

    void MessagesScrollArea::cancelWheelBufferReset()
    {
        WheelEventsBufferResetTimer_.stop();
    }

    void MessagesScrollArea::enumerateWidgets(const WidgetVisitor& visitor, const bool reversed) const
    {
        im_assert(visitor);
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

    Data::FString MessagesScrollArea::getSelectedText(TextFormat _format) const
    {
        Data::FString result;
        size_t i = 1;
        const auto mentions = _format == TextFormat::Raw ? Data::MentionMap() : getMentions();
        for (const auto& [_, info] : selected_)
        {
            auto text = info.text_;
            if (!mentions.empty())
                Utils::convertMentions(text, mentions);
            result += std::move(text);
            if (i != selected_.size())
                result += u"\n\n";

            ++i;
        }

        return result;
    }

    Data::FilesPlaceholderMap MessagesScrollArea::getFilesPlaceholders() const
    {
        Data::FilesPlaceholderMap ph;
        for (const auto&[_, info] : selected_)
            ph.insert(info.filesPlaceholders_.begin(), info.filesPlaceholders_.end());
        return ph;
    }

    Data::MentionMap MessagesScrollArea::getMentions() const
    {
        Data::MentionMap mentions;
        for (const auto& [_, info] : selected_)
            mentions.insert(info.mentions_.begin(), info.mentions_.end());
        return mentions;
    }

    Data::QuotesVec MessagesScrollArea::getQuotes(MessagesScrollArea::IsForward _isForward) const
    {
        Data::QuotesVec result;

        auto isQuote = [](const Data::Quote& _quote) { return _quote.type_ == Data::Quote::Type::quote && _quote.hasReply_; };

        for (const auto&[_, info] : selected_)
        {
            if (!info.isDisabled_)
                result += info.quotes_;
        }

        if (_isForward == IsForward::No)
        {
            const auto allQuotes = std::all_of(result.begin(), result.end(), isQuote);

            if (!allQuotes)
                result.erase(std::remove_if(result.begin(), result.end(), isQuote), result.end());
        }

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
        const auto viewportTop = Layout_->getViewportAbsY() + Layout_->getBottomWidgetsHeight();
        const auto heightFromTopBoundToViewportTop = viewportTop - bounds.first;
        //im_assert(heightFromTopBoundToViewportTop > 0);

        constexpr auto triggerThresholdK = triggerThreshold();
        const auto triggerThreshold = int32_t(minViewPortHeightForUnload() * triggerThresholdK);
        im_assert(triggerThreshold > 0);

        const auto triggerThreasholdNotReached = (heightFromTopBoundToViewportTop < triggerThreshold);
        if (triggerThreasholdNotReached)
            return QVector<Logic::MessageKey>();

        constexpr auto unloadThresholdK = unloadThreshold();
        const auto unloadThreshold = int32_t(minViewPortHeightForUnload() * unloadThresholdK);
        im_assert(unloadThreshold > 0);

        return Layout_->getWidgetsOverBottomOffset(heightFromTopBoundToViewportTop - unloadThreshold);
    }

    QVector<Logic::MessageKey> MessagesScrollArea::getKeysToUnloadBottom() const
    {
        const auto bounds = Layout_->getItemsAbsBounds();
        if (bounds.first == 0 && bounds.second == 0)
            return QVector<Logic::MessageKey>();
        const auto viewportBottom = Layout_->getViewportAbsY() + minViewPortHeightForUnload() + Layout_->getBottomWidgetsHeight();
        const auto heightFromBoundToViewportBottom = bounds.second - viewportBottom;

        constexpr auto triggerThresholdK = triggerThreshold();
        const auto triggerThreshold = int32_t(minViewPortHeightForUnload() * triggerThresholdK);
        im_assert(triggerThreshold > 0);

        const auto triggerThreasholdNotReached = (heightFromBoundToViewportBottom < triggerThreshold);
        if (triggerThreasholdNotReached)
            return QVector<Logic::MessageKey>();

        constexpr auto unloadThresholdK = unloadThreshold();
        const auto unloadThreshold = (int32_t)(minViewPortHeightForUnload() * unloadThresholdK);
        im_assert(unloadThreshold > 0);

        return Layout_->getWidgetsUnderBottomOffset(heightFromBoundToViewportBottom - unloadThreshold);
    }


    void MessagesScrollArea::mouseMoveEvent(QMouseEvent *e)
    {
        QWidget::mouseMoveEvent(e);

        const auto isLeftButton = ((e->buttons() & Qt::LeftButton) != 0);
        const auto isGenuine = (e->source() == Qt::MouseEventNotSynthesized);

        if (!isLeftButton || !isGenuine)
        {
            if (isSelecting())
            {
                stopScrollAnimation();
                updateStateFlag(State::IsSelecting_, false);
            }
            return;
        }

        if (Utils::clicked(LastMouseGlobalPos_, e->globalPos()))
            return;

        LastMouseGlobalPos_ = e->globalPos();

        if (isSelecting())
        {
            const auto mouseAbsPos = Layout_->viewport2Absolute(e->pos());

            if (SelectionBeginAbsPos_.isNull())
                SelectionBeginAbsPos_ = mouseAbsPos;

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

        const auto bottomScrollStartPosition = height();
        im_assert(bottomScrollStartPosition > 0);

        const auto scrollDown = (mouseY >= bottomScrollStartPosition);
        if (scrollDown)
        {
            im_assert(!scrollUp);

            ScrollDistance_ = mouseY - bottomScrollStartPosition;
            startScrollAnimation(ScrollingMode::Selection);

            return;
        }

        stopScrollAnimation();
    }

    void MessagesScrollArea::mousePressEvent(QMouseEvent *e)
    {
        QWidget::mousePressEvent(e);

        const auto page = Utils::InterConnector::instance().getHistoryPage(aimid_);
        const auto isSmilesHidden = getWidgetIfNot<Smiles::SmilesMenu>(page, &Smiles::SmilesMenu::isHidden) == nullptr;

        const auto isLeftButton = ((e->buttons() & Qt::LeftButton) != 0);
        const auto isGenuine = (e->source() == Qt::MouseEventNotSynthesized);
        const auto isSelectionStart = (isLeftButton && isGenuine && isSmilesHidden);

        if (isLeftButton)
        {
            Q_EMIT Utils::InterConnector::instance().hideMentionCompleter();
            if (Ui::get_gui_settings()->get_value<bool>(settings_fast_drop_search_results, settings_fast_drop_search_default()))
                Q_EMIT Utils::InterConnector::instance().searchEnd();
        }

        if (!isSelectionStart)
            return;

        const auto mouseAbsPos = Layout_->viewport2Absolute(e->pos());

        if (!(e->modifiers() & Qt::ShiftModifier))
        {
            if (isLeftButton && !Utils::InterConnector::instance().isMultiselect(aimid_))
                Q_EMIT Utils::InterConnector::instance().clearSelection();

            LastMouseGlobalPos_ = e->globalPos();

            SelectionBeginAbsPos_ = mouseAbsPos;
            SelectionEndAbsPos_ = mouseAbsPos;

            applySelection();
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

        updateStateFlag(State::IsSelecting_, true);
    }

    void MessagesScrollArea::mouseReleaseEvent(QMouseEvent* e)
    {
        const auto selectedCount = getSelectedCount();

        if (isSelecting())
            onSelectionChanged(selectedCount);

        QWidget::mouseReleaseEvent(e);

        updateStateFlag(State::IsSelecting_, false);

        if (e->source() == Qt::MouseEventNotSynthesized)
            stopScrollAnimation();

        auto hasNoSelectionActually = false;
        if (selectedCount <= 2)
        {
            // After press on link selected_ gets one dummy item, we do not care about
            hasNoSelectionActually = true;
            for (const auto& v: selected_)
            {
                if (!v.second.text_.isEmpty())
                {
                    hasNoSelectionActually = false;
                    break;
                }
            }
        }

        if (hasNoSelectionActually)
        {
            auto w = Utils::InterConnector::instance().getMainWindow()->getPreviousFocusedWidget();
            if (w && w != this && w->isVisible())
                w->setFocus();
            else
                Q_EMIT Utils::InterConnector::instance().setFocusOnInput(aimid_);
        }
        else
        {
            setFocus();
        }
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

        if (e->isAccepted())
            return;

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
        im_assert(Layout_);

        Layout_->resumeVisibleItems();
    }

    void MessagesScrollArea::suspendVisibleItems()
    {
        im_assert(Layout_);

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
            updateStateFlag(State::IsTouchScrollInProgress_, true);
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
                    im_assert(delta != 0);

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
            updateStateFlag(State::IsTouchScrollInProgress_, false);
            e->accept();

            return true;
        }

        return QWidget::event(e);
    }

    void MessagesScrollArea::showEvent(QShowEvent *_event)
    {
        if constexpr (!platform::is_apple())
            updateStateFlag(State::IsScrollShowOnOpen_, true);

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
        StatusTooltip::instance()->updatePosition();

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
            im_assert(false);
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
        im_assert(value >= 0);

        const bool isAtBottom = isScrollAtBottom();

        if ((scrollValue_ == -1 || scrollValue_ < value) && Scrollbar_->isInFetchRangeToBottom(value))
        {
            // to newer
            Q_EMIT fetchRequestedEvent(false /* don't move to bottom */);
        }
        else if ((scrollValue_ == -1 || scrollValue_ > value) && Scrollbar_->isInFetchRangeToTop(value))
        {
            // to older
            Q_EMIT fetchRequestedEvent(true);
        }

        if (Mode_ == ScrollingMode::Selection && ScrollAnimationTimer_.isActive())
        {
            SelectionEndAbsPos_ = Layout_->viewport2Absolute(mapFromGlobal(QCursor::pos()));
            applySelection();
        }

        if (isAtBottom)
        {
            Q_EMIT scrollMovedToBottom();
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
        if (viewportAbsY <= viewportScrollBounds.first)
            return true;

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
        setIsSearch(getMessageId() > 0);
    }

    void MessagesScrollArea::applySelection()
    {
        im_assert(!LastMouseGlobalPos_.isNull());

        auto selectionBegin = Layout_->absolute2Viewport(SelectionBeginAbsPos_);
        auto selectionEnd = Layout_->absolute2Viewport(SelectionEndAbsPos_);
        auto selectionBeginGlobal = mapToGlobal(selectionBegin);
        auto selectionEndGlobal = mapToGlobal(selectionEnd);
        auto selectionGlobalRect = QRect(selectionBeginGlobal, selectionEndGlobal);

        if (selectionBeginGlobal == selectionEndGlobal)
            return;

        if (!Utils::InterConnector::instance().isMultiselect(aimid_))
        {
            enumerateWidgets([this, selectionBeginGlobal, selectionEndGlobal](QWidget* _w, const bool)
                {
                    if (getSelectableItem(_w))
                    {
                        auto item = qobject_cast<Ui::ComplexMessage::ComplexMessageItem*>(_w);
                        if (!item)
                            return true;

                        const auto itemRect = _w->rect();
                        const auto globalRect = QRect(_w->mapToGlobal(itemRect.topLeft()), itemRect.size());
                        const auto setSelectionBeginAbsPos = [this, selectionBeginGlobal, globalRect](const int& dy = 0){ SelectionBeginAbsPos_.setY(SelectionBeginAbsPos_.y() + globalRect.y() - selectionBeginGlobal.y() + (dy ? 1 : -1) + dy); };
                        if (globalRect.contains(selectionBeginGlobal) && !globalRect.contains(selectionEndGlobal))
                        {
                            auto center = globalRect.center();
                            const auto directOrder = (selectionBeginGlobal.y() < selectionEndGlobal.y());
                            const auto leftSelection = (selectionBeginGlobal.y() < center.y());

                            if (directOrder && !leftSelection && item->isSelected())
                                setSelectionBeginAbsPos();
                            if ((directOrder && !leftSelection && !item->isSelected()) ||
                                !directOrder && (leftSelection || !leftSelection && item->isSelected()))
                                setSelectionBeginAbsPos(globalRect.height());
                            if (item->isSelected() && (directOrder && !leftSelection  || !directOrder))
                                Utils::InterConnector::instance().setMultiselect(true, aimid_);

                            return false;
                        }
                    }
                    return true;
                },
            false
            );

            selectionBegin = Layout_->absolute2Viewport(SelectionBeginAbsPos_);
            selectionEnd = Layout_->absolute2Viewport(SelectionEndAbsPos_);
            selectionBeginGlobal = mapToGlobal(selectionBegin);
            selectionEndGlobal = mapToGlobal(selectionEnd);
            selectionGlobalRect = QRect(selectionBeginGlobal, selectionEndGlobal);
        }

        bool isSomethingSelected = false;
        enumerateWidgets(
            [this, selectionBeginGlobal, selectionEndGlobal, selectionGlobalRect, &isSomethingSelected](QWidget* _w, const bool)
            {
                if (auto item = getSelectableItem(_w))
                {
                    const auto itemRect = _w->rect();
                    const auto globalRect = QRect(_w->mapToGlobal(itemRect.topLeft()), itemRect.size());

                    if (isSomethingSelected && selectionGlobalRect.intersects(globalRect) && !Utils::InterConnector::instance().isMultiselect(aimid_))
                        Utils::InterConnector::instance().setMultiselect(true, aimid_);

                    item->selectByPos(selectionBeginGlobal, selectionEndGlobal, SelectionBeginAbsPos_, SelectionEndAbsPos_);

                    if (isSomethingSelected && item->isSelected())
                        lastSelectedMessageId_ = Logic::MessageKey(item->getId(), item->getInternalId());

                    isSomethingSelected |= item->isSelected();
                }

                return true;
            },
            false
        );
    }

    void MessagesScrollArea::clearSelection(bool _keepSingleSelection)
    {
        for (const auto& key : selected_)
        {
            auto item = Layout_->getItemByKey(key.first);
            if (auto w = getSelectableItem(item))
                w->clearSelection(_keepSingleSelection);
        }

        if (!_keepSingleSelection)
            selected_.clear();

        Utils::InterConnector::instance().setMultiselect(false, aimid_);

        Q_EMIT messagesDeselected();
    }

    void MessagesScrollArea::clearPartialSelection()
    {
        for (const auto& key : selected_)
        {
            auto item = Layout_->getItemByKey(key.first);
            if (auto w = getSelectableItem(item))
                w->clearSelection(false);
        }

        selected_.clear();
    }

    void MessagesScrollArea::continueSelection(const QPoint& _pos)
    {
        const auto mouseAbsPos = Layout_->viewport2Absolute(_pos);

        LastMouseGlobalPos_ = mapToGlobal(_pos);

        SelectionBeginAbsPos_ = mouseAbsPos;
        SelectionEndAbsPos_ = mouseAbsPos;

        applySelection();

        updateStateFlag(State::IsSelecting_, true);
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

        im_assert(WheelEventsBuffer_.size() <= WHEEL_HISTORY_LIMIT);
        const auto isHistoryFilled = (WheelEventsBuffer_.size() == WHEEL_HISTORY_LIMIT);

        const auto isSignChanged = (
            (directionAfter != 0) &&
            (directionBefore != 0) &&
            (directionAfter != directionBefore));

        return (isHistoryFilled && isSignChanged);
    }

    double MessagesScrollArea::evaluateScrollingVelocity() const
    {
        im_assert(ScrollDistance_ != 0);
        im_assert(std::abs(ScrollDistance_) <= getMaximumScrollDistance());

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
        im_assert(now > 0);

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
        im_assert(mode > ScrollingMode::Min);
        im_assert(mode < ScrollingMode::Max);

        Mode_ = mode;

        if (isScrolling())
        {
            return;
        }

        ScrollAnimationTimer_.start();

        im_assert(LastAnimationMoment_ == MOMENT_UNINITIALIZED);
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

    void MessagesScrollArea::updateStateFlag(State _flag, bool _enabled)
    {
        areaState_.setFlag(_flag, _enabled);
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
        updateStateFlag(State::IsSearching_, _isSearch);
    }

    bool MessagesScrollArea::getIsSearch() const
    {
        return (areaState_ & State::IsSearching_);
    }

    void MessagesScrollArea::setMessageId(qint64 _id)
    {
        messageId_ = _id;
    }

    qint64 MessagesScrollArea::getMessageId() const
    {
        return messageId_;
    }

    void MessagesScrollArea::updateItems()
    {
        Layout_->updateItemsWidth();
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
            if (auto messageItem = qobject_cast<Ui::HistoryControlPageItem*>(_item))
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

    void MessagesScrollArea::countSelected(int& _forMe, int& _forAll) const
    {
        _forMe = 0;
        _forAll = 0;

        const auto isAdmin = Logic::getContactListModel()->areYouAdmin(aimid_);
        const auto isMe = aimid_ == MyInfo()->aimId();
        for (const auto& sel : selected_)
        {
            if (!isMe && (sel.second.isOutgoing_ || isAdmin))
                ++_forAll;

            ++_forMe;
        }
    }

    std::vector<DeleteMessageInfo> MessagesScrollArea::getSelectedMessagesForDelete() const
    {
        const auto isAdmin = Logic::getContactListModel()->areYouAdmin(aimid_);
        std::vector<DeleteMessageInfo> result;
        result.reserve(selected_.size());
        for (const auto& [key, info] : selected_)
            result.emplace_back(key.getId(), key.getInternalId(), (info.isOutgoing_ || isAdmin));

        return result;
    }

    bool MessagesScrollArea::messagesCanBeDeleted() const
    {
        bool enabled = true;
        if (Logic::getContactListModel()->isThread(aimid_))
            enabled = std::all_of(selected_.begin(), selected_.end(), [](const auto& sel) { return sel.second.isOutgoing_; });

        return enabled;
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
        im_assert(maxScrollDistance > 0);
        im_assert(std::abs(scrollDistance) <= maxScrollDistance);
        im_assert(minSpeed >= 0);
        im_assert(maxSpeed > 0);
        im_assert(maxSpeed > minSpeed);

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
        im_assert(maxScrollDistance > 0);
        im_assert(std::abs(scrollDistance) <= maxScrollDistance);
        im_assert(minSpeed >= 0);
        im_assert(maxSpeed > 0);
        im_assert(maxSpeed > minSpeed);

        // see http://gizma.com/easing/#expo2

        auto t = scrollDistance;
        const auto b = minSpeed;
        const auto c = (maxSpeed - minSpeed);
        const auto d = maxScrollDistance;

        return (c * (-std::pow(2.0, -10 * t/d) + 1) + b);
    }
}
