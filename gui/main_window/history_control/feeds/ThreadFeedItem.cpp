#include "stdafx.h"

#include "../HistoryControlPageItem.h"
#include "../ServiceMessageItem.h"
#include "../ThreadRepliesItem.h"
#include "../history/MessageBuilder.h"
#include "../history/DateInserter.h"
#include "main_window/contact_list/ContactListModel.h"
#include "main_window/contact_list/Common.h"
#include "styles/ThemeParameters.h"
#include "controls/TextUnit.h"
#include "controls/TooltipWidget.h"
#include "utils/utils.h"
#include "utils/InterConnector.h"
#include "utils/PolygonRounder.h"
#include "utils/MimeDataUtils.h"
#include "previewer/toast.h"
#include "ThreadFeedItem.h"
#include "ThreadFeedItemHeader.h"
#include "MessagesLayout.h"
#include "main_window/DragOverlayWindow.h"
#include "main_window/input_widget/InputWidget.h"
#include "main_window/input_widget/InputWidgetUtils.h"
#include "main_window/contact_list/ServiceContacts.h"
#include "main_window/history_control/MessageStyle.h"
#include "main_window/history_control/MentionCompleter.h"
#include "main_window/history_control/complex_message/ComplexMessageItem.h"
#include "main_window/history_control/complex_message/FileSharingUtils.h"
#include "core_dispatcher.h"
#include "main_window/smiles_menu/SmilesMenu.h"
#include "main_window/smiles_menu/SuggestsWidget.h"
#include "main_window/MainWindow.h"
#include "main_window/GroupChatOperations.h"
#include "main_window/contact_list/FavoritesUtils.h"

namespace
{
    constexpr int headerAvatarSize() { return 32; }

    int headerHeight() noexcept
    {
        return Utils::scale_value(headerAvatarSize() + 2 * 12);
    }

    int topMargin() noexcept
    {
        return Utils::scale_value(4);
    }

    int bottomMargin() noexcept
    {
        return Utils::scale_value(8);
    }

    double chatThreadBubbleBorderRadius() noexcept
    {
        return (double)Utils::scale_value(8);
    }

    int minimumDragOverlayHeight() noexcept
    {
        return 2 * headerHeight() + Utils::scale_value(16);
    }

    int newMessagesPlateTopMargin() noexcept
    {
        return Utils::scale_value(4);
    }

    int newMessagesPlateBottomMargin() noexcept
    {
        return Utils::scale_value(16);
    }

    int itemHorizontalMargin() noexcept
    {
        return Utils::scale_value(12);
    }

    QMargins itemMargins() noexcept
    {
        return QMargins(itemHorizontalMargin(), 0, itemHorizontalMargin(), 0);
    }

    QMargins messagesLayoutMargins() noexcept
    {
        return QMargins(itemHorizontalMargin(), Utils::scale_value(10), itemHorizontalMargin(), Utils::scale_value(8));
    }

    QMargins pinnedMessageLayoutMargins() noexcept
    {
        return QMargins(itemHorizontalMargin(), 0, itemHorizontalMargin(), Utils::scale_value(6));
    }

    using ThreadFeedItemUptr = QObjectUniquePtr<Ui::HistoryControlPageItem>;

    ThreadFeedItemUptr createItem(Data::MessageBuddySptr _msg, Ui::ThreadFeedItem* _parent, int _width, int64_t _topicStarter)
    {
        ThreadFeedItemUptr item;
        if (!_msg)
            return item;

        bool isTS = _msg->Id_ == _topicStarter;
        if (isTS)
            _msg->SetOutgoing(false);//show topicstarter as incoming message

        auto pageItem = hist::MessageBuilder::makePageItem(*_msg, _width, _parent);
        item.reset(pageItem.release());
        if (item)
        {
            item->setHasAvatar(true);
            item->setHasSenderName(true);
            item->setChainedToNext(false);
            item->setChainedToPrev(false);
            item->onActivityChanged(true);
            item->initSize();
            item->setTopicStarter(isTS);
            item->setMultiselectEnabled(false);
            QObject::connect(item.get(), &Ui::HistoryControlPageItem::edit, _parent, &Ui::ThreadFeedItem::edit);
            QObject::connect(item.get(), &Ui::HistoryControlPageItem::quote, _parent, &Ui::ThreadFeedItem::quote);
            if (const auto msgItem = dynamic_cast<Ui::ComplexMessage::ComplexMessageItem*>(item.get()))
            {
                QObject::connect(msgItem, &Ui::ComplexMessage::ComplexMessageItem::copy, _parent, &Ui::ThreadFeedItem::copy, Qt::UniqueConnection);
                QObject::connect(msgItem, &Ui::ComplexMessage::ComplexMessageItem::forward, _parent, &Ui::ThreadFeedItem::onForward, Qt::UniqueConnection);
                QObject::connect(msgItem, &Ui::ComplexMessage::ComplexMessageItem::addToFavorites, _parent, &Ui::ThreadFeedItem::addToFavorites, Qt::UniqueConnection);
            }

            QObject::connect(item.get(), &Ui::HistoryControlPageItem::createTask, _parent, &Ui::ThreadFeedItem::createTask);
        }
        return item;
    }

    constexpr int64_t loadMoreMaxCount() noexcept
    {
        return 50;
    }

    int pickerHorMargin() noexcept
    {
        return Utils::scale_value(16);
    }
}

namespace Ui
{

//////////////////////////////////////////////////////////////////////////
// ThreadFeedItem_p
//////////////////////////////////////////////////////////////////////////

class ThreadFeedItem_p
{
public:
    ThreadFeedItem_p(QWidget* _q)
        : q(_q)
        , dateInserter_(new hist::DateInserter(threadId_, q))
    {}

    void initStickerPicker()
    {
        if (stickerPicker_)
            return;

        stickerPicker_ = new Smiles::SmilesMenu(q, threadId_);

        stickerPicker_->setCurrentHeight(0);
        stickerPicker_->setHorizontalMargins(pickerHorMargin());
        stickerPicker_->setAddButtonVisible(false);
        Testing::setAccessibleName(stickerPicker_, qsl("AS EmojiAndStickerPicker"));

        layout_->insertWidget(layout_->indexOf(input_), stickerPicker_);

        QObject::connect(stickerPicker_, &Smiles::SmilesMenu::emojiSelected, input_, &InputWidget::insertEmoji);
        QObject::connect(stickerPicker_, &Smiles::SmilesMenu::visibilityChanged, input_, &InputWidget::onSmilesVisibilityChanged);
        QObject::connect(stickerPicker_, &Smiles::SmilesMenu::stickerSelected, input_, &InputWidget::sendSticker);
    }

    int nextRepliesToLoadCount() const
    {
        if (unreadCount_ > tailSize_ && unreadCount_ - tailSize_ <= loadMoreMaxCount())
            return std::min(repliesToLoad_, std::min(loadMoreMaxCount() + 1, static_cast<int64_t>(unreadCount_ - tailSize_ + 1)));

        return std::min(repliesToLoad_, loadMoreMaxCount());
    }

    void updateLoadMoreButton()
    {
        auto repliesToLoad = nextRepliesToLoadCount();

        if (unreadCount_ > tailSize_)
        {
            if (unreadCount_ - tailSize_ <= loadMoreMaxCount())
                --repliesToLoad; // substract one already read message

            repliesPlate_->setThreadReplies(repliesToLoad, 0);
        }
        else
        {
            repliesPlate_->setThreadReplies(0, repliesToLoad);
        }
    }

    void addNewMessagesPlate()
    {
        newMessagesPlate_ = hist::MessageBuilder::createNew(threadId_, q->width(), q);
        const auto insertIndex = tailSize_ - unreadCount_;
        messagesLayout_->insertSpace(newMessagesPlateBottomMargin(), insertIndex);
        messagesLayout_->insertWidget(newMessagesPlate_.get(), insertIndex);
        messagesLayout_->insertSpace(newMessagesPlateTopMargin(), insertIndex);
        newMessagesPlate_->show();
    }

    void addDateMessage(const Data::MessageBuddySptr& _msg, bool _front = false, bool _withoutCheck = false)
    {
        addDateMessage(*(_msg.get()), _front, _withoutCheck);
    }

    Logic::MessageKey getDateKey(const Data::MessageBuddy& _msg) const
    {
        return dateInserter_->makeDateKey(_msg.ToKey());
    }

    bool isDateItem(const ThreadFeedItemUptr& _item) const
    {
        return isDateItem(_item.get());
    }

    bool isDateMessage(const Data::MessageBuddy& _msg) const
    {
        return _msg.GetType() == core::message_type::undefined;
    }

    bool isDateMessage(std::shared_ptr<Data::MessageBuddy> _msg)
    {
        auto msg = _msg.get();
        return isDateMessage(*msg);
    }

    bool isDateItem(const Ui::HistoryControlPageItem* _item) const
    {
        return _item && isDateMessage(_item->buddy());
    }

    void insertAndShow(ThreadFeedItemUptr&& _w, const Logic::MessageKey& _key, bool _front = false, bool _needUpdIndex = true,
                       bool _needUpdLastMsgId = false, bool _needUpdPendings = false)
    {
        if (!_w)
            return;

        auto wPtr = _w.get();
        messagesLayout_->insertWidget(wPtr, _front ? 0 : -1);
        _w->show();

        if (_needUpdIndex)
            index_[_key] = wPtr;

        const auto& msg = _w->buddy();
        if (_needUpdLastMsgId)
            lastMsgId_ = std::max(msg.Id_, lastMsgId_);
        if (_needUpdPendings)
            pendings_[msg.InternalId_] = wPtr;

        addItem(std::move(_w));
    }

    void addDateMessage(const Data::MessageBuddy& _msg, bool _front = false, bool _withoutCheck = false)
    {
        auto dateKey = getDateKey(_msg);
        auto noDateItems = (items_.size() > 0 && std::find_if(items_.begin(), items_.end(), [this](const auto& _item){ return isDateItem(_item); }) == items_.end());

        QDate maxDate;
        for (auto& item : items_)
        {
            if (isDateItem(item))
                if (maxDate < item->buddy().GetDate())
                    maxDate = item->buddy().GetDate();
        }

        if (items_.size() == 0 || noDateItems || _withoutCheck || maxDate < _msg.GetDate())
        {
            if (auto w = dateInserter_->makeDateItem(dateKey, q->width(), q))
                insertAndShow(ThreadFeedItemUptr(w.release()), dateKey, _front, false);
        }
    }

    void removeFirstDate(const Logic::MessageKey& _dateKey)
    {
        for (auto iter = items_.begin(); iter != items_.end(); ++iter)
        {
            auto& item = *iter;
            if (isDateItem(item))
            {
                auto itemDateKey = getDateKey(item->buddy());
                if (_dateKey.getDate() == itemDateKey.getDate())
                {
                    index_.erase(itemDateKey);
                    items_.erase(iter);
                }
                break;
            }
        }
    }

    void removeAllDates()
    {
        auto newEnd = std::remove_if(items_.begin(), items_.end(), [this](const auto& _item)
        {
            bool isDate = isDateItem(_item);
            if (isDate)
                index_.erase(getDateKey(_item->buddy()));
            return isDate;
        });
        items_.erase(newEnd, items_.end());
    }

    bool isSameDateKey(const Data::MessageBuddySptr& _msg, const ThreadFeedItemUptr& _item) const
    {
        if (!_msg || !_item)
            return false;
        auto dateKey = getDateKey(*_msg.get());
        return _item ? getDateKey(_item->buddy()).getDate() == _msg->GetDate() : false;
    }

    void deleteDates(const std::vector<QDate>& _deletedMsgDates)
    {
        for (const auto& date : _deletedMsgDates)
        {
            auto messagesWithSameDate = (std::find_if(items_.begin(), items_.end(), [this, date](const auto& _item)
                                             {
                                                 return !isDateItem(_item) && (date == _item->buddy().GetDate()) && (_item->getId() != pinnedMessageId_);
                                             }) != items_.end());

            if (!messagesWithSameDate)
            {
                items_.erase(std::remove_if(items_.begin(), items_.end(), [this, date](const auto& _item)
                {
                    auto needDelete = isDateItem(_item) && (date == _item->buddy().GetDate());
                    if (needDelete)
                        index_.erase(getDateKey(_item->buddy()));
                    return needDelete;
                }), items_.end());
            }
        }
    }

    void deleteDates(const Data::MessageBuddies& _messages)
    {
        for (const auto& msg : _messages)
        {
            if (!msg)
                continue;
            auto messagesWithSameDate = (std::find_if(items_.begin(), items_.end(), [this, msg](const auto& _item)
            { return !isDateItem(_item) && isSameDateKey(msg, _item); }) != items_.end());
            if (!messagesWithSameDate)
            {
                items_.erase(std::remove_if(items_.begin(), items_.end(), [this, msg](const auto& _item)
                { return isDateItem(_item) && isSameDateKey(msg, _item); }), items_.end());
                index_.erase(getDateKey(*msg.get()));
            }
        }
    }

    bool addMessage(ThreadFeedItemUptr&& _newItem, bool _needUpdIndex = true, bool _needUpdPendings = false, bool _front = false)
    {
        if (!_newItem)
            return false;
        const auto key = _newItem->buddy().ToKey();
        if (index_.find(key) != index_.end())
            return false;
        insertAndShow(std::move(_newItem), key, _front, _needUpdIndex, _needUpdIndex, _needUpdPendings);
        return true;
    }

    void addItem(ThreadFeedItemUptr&& _item)
    {
        items_.push_back(std::move(_item));
        QObject::connect(items_.back().get(), &Ui::HistoryControlPageItem::sizeUpdated, q, [this]()
        {
            layout_->invalidate();
            messagesLayout_->invalidate();
            q->updateGeometry();
        }, Qt::UniqueConnection);
    }

    void setInputAllowed(bool _isAllowed)
    {
        isInputEnabled_ = _isAllowed;

        if (!_isAllowed)
        {
            input_->setVisible(false);
            inputWasVisible_ = false;
        }
    }

    void tryHideEmojiPicker(const QPoint& _pos)
    {
        if (stickerPicker_ && !(input_->geometry().contains(_pos) || stickerPicker_->geometry().contains(_pos)))
            stickerPicker_->hideAnimated();
    }

    void showDragOverlay()
    {
        if (dragOverlayWindow_ && dragOverlayWindow_->isVisible())
            return;

        if (!dragOverlayWindow_)
        {
            dragOverlayWindow_ = new DragOverlayWindow(threadId_, q);
            dragOverlayWindow_->setAttribute(Qt::WA_TransparentForMouseEvents);
        }

        updateDragOverlay();
        dragOverlayWindow_->raise();
        dragOverlayWindow_->show();

        Utils::InterConnector::instance().setDragOverlay(true);
    }

    void hideDragOverlay()
    {
        if (dragOverlayWindow_)
            dragOverlayWindow_->hide();

        Utils::InterConnector::instance().setDragOverlay(false);
    }

    void updateDragOverlay()
    {
        Utils::PolygonRounder rounder;
        const double radius = chatThreadBubbleBorderRadius();
        const QRect visibleRect = q->visibleRegion().boundingRect().marginsRemoved(itemMargins());
        const QRect windowRect = q->contentsRect().adjusted(0, headerHeight(), 0, -1);
        const QRect rect = windowRect.intersected(visibleRect);
        if (dragOverlayWindow_)
        {
            dragOverlayWindow_->setGeometry(rect);
            dragOverlayWindow_->setClipPath(rounder(dragOverlayWindow_->rect(), { 0.0, 0.0, radius, radius }));
        }
    }

    void updateBubblePath()
    {
        const double r = chatThreadBubbleBorderRadius();

        Utils::PolygonRounder rounder;
        const auto bubbleRect = q->contentsRect().adjusted(0, headerHeight(), 0, -1);
        bubblePath_ = rounder(bubbleRect, { 0, 0, r, r });
    }

    bool canDragDrop() const
    {
        const QRect visibleRect = q->visibleRegion().boundingRect();
        const QRect windowRect = q->rect().adjusted(0, 0, 0, -1);
        return (windowRect.intersected(visibleRect).height() >= minimumDragOverlayHeight());
    }

    QString threadId_;
    QBoxLayout* layout_ = nullptr;
    MessagesLayout* messagesLayout_ = nullptr;
    MessagesLayout* pinnedMessageLayout_ = nullptr;
    InputWidget* input_ = nullptr;
    ThreadFeedItemHeader* header_ = nullptr;
    QPointer<DragOverlayWindow> dragOverlayWindow_ = nullptr;
    bool isInputEnabled_ = false;
    Stickers::StickersSuggest* suggestsWidget_ = nullptr;
    MentionCompleter* mentionCompleter_ = nullptr;
    std::shared_ptr<Data::MessageParentTopic> parentTopic_;
    std::vector<ThreadFeedItemUptr> items_;
    std::map<Logic::MessageKey, Ui::HistoryControlPageItem*> index_;
    std::unordered_map<QString, Ui::HistoryControlPageItem*> pendings_;

    QTimer* groupSubscribeTimer_ = nullptr;
    bool needToRepeatRequest_ = false;
    bool firstRequest_ = true;
    bool suggestWidgetShown_ = false;
    bool inputWasVisible_ = false;

    qint64 lastMsgId_ = 0;
    int64_t olderMsgId_ = 0;
    int64_t repliesToLoad_ = 0;
    int64_t unreadCount_ = 0;
    int64_t tailSize_ = 0;
    int64_t groupSubscribeSeq_ = -1;
    int64_t seq_ = -1;
    int lastRequestedCount_ = 0;
    core_dispatcher::GetHistoryParams requestToRepeat_;
    std::unique_ptr<Ui::ServiceMessageItem> newMessagesPlate_;
    Ui::ThreadRepliesItem* repliesPlate_ = nullptr;
    Smiles::SmilesMenu* stickerPicker_ = nullptr;
    QWidget* q;
    QWidget* parentForTooltips_ = nullptr;
    int64_t pinnedMessageId_ = -1;
    QPoint selectFrom_;
    QPainterPath bubblePath_;
    hist::DateInserter* dateInserter_;
    std::vector<Logic::MessageKey> lastDeletedCache_;
};

//////////////////////////////////////////////////////////////////////////
// ThreadFeedItem
//////////////////////////////////////////////////////////////////////////

ThreadFeedItem::ThreadFeedItem(const Data::ThreadFeedItemData& _data, QWidget* _parent, QWidget* _parentForTooltips)
    : QWidget(_parent)
    , d(std::make_unique<ThreadFeedItem_p>(this))

{
    setAcceptDrops(true);
    setContentsMargins(itemHorizontalMargin(), 0, itemHorizontalMargin(), 0);

    d->threadId_ = _data.threadId_;
    d->olderMsgId_ = _data.olderMsgId_;
    d->repliesToLoad_ = _data.repliesCount_ - _data.messages_.size();
    d->unreadCount_ = _data.unreadCount_;
    d->parentTopic_ = _data.parent_;
    d->layout_ = Utils::emptyVLayout(this);

    d->header_ = new ThreadFeedItemHeader(_data.parent_->chat_, this);
    d->layout_->addWidget(d->header_);

    d->pinnedMessageLayout_ = new MessagesLayout();
    d->pinnedMessageLayout_->setContentsMargins(pinnedMessageLayoutMargins());

    if (auto item = createItem(_data.parentMessage_, this, width(), _data.parentMessage_->Id_))
    {
        d->pinnedMessageId_ = item->getId();
        d->pinnedMessageLayout_->addWidget(item.get());
        d->addItem(std::move(item));
    }

    d->repliesPlate_ = new ThreadRepliesItem(this);

    auto buttonLayout = Utils::emptyHLayout();
    buttonLayout->addStretch();
    buttonLayout->addWidget(d->repliesPlate_);
    buttonLayout->addStretch();

    d->messagesLayout_ = new MessagesLayout;
    d->messagesLayout_->setContentsMargins(messagesLayoutMargins());

    d->layout_->addSpacing(topMargin());
    d->layout_->addLayout(d->pinnedMessageLayout_);
    d->layout_->addLayout(buttonLayout);
    d->layout_->addLayout(d->messagesLayout_);
    d->layout_->addStretch(1);

    if (!d->threadId_.isEmpty())
    {
        Logic::getContactListModel()->markAsThread(d->threadId_, d->parentTopic_.get());
        d->input_ = new InputWidget(d->threadId_, this, Ui::defaultInputFeatures() | Ui::InputFeature::LocalState, nullptr);
        d->input_->setThreadFeedItem(this);
        d->input_->setParentForPopup(_parentForTooltips);
        connect(d->input_, &InputWidget::heightChanged, this, &ThreadFeedItem::inputHeightChanged);
        connect(d->input_, &InputWidget::needSuggets, this, &ThreadFeedItem::onSuggestShow);
        connect(d->input_, &InputWidget::hideSuggets, this, &ThreadFeedItem::onSuggestHide);
        connect(this, &ThreadFeedItem::quote, d->input_, &InputWidget::quote);
        connect(this, &ThreadFeedItem::createTask, d->input_, &InputWidget::createTask);
        d->layout_->addWidget(d->input_);
        d->layout_->addSpacing(bottomMargin());

        const auto mainWindow = Utils::InterConnector::instance().getMainWindow();
        connect(
            d->input_,
            &InputWidget::startPttRecording,
            mainWindow,
            [mainWindow, _inputWidget = d->input_]() { mainWindow->setLastPttInput(_inputWidget); });

        GetDispatcher()->setDialogOpen(d->threadId_, true);
        d->groupSubscribeSeq_ = GetDispatcher()->groupSubscribe(d->threadId_);

        d->mentionCompleter_ = new MentionCompleter(this);
        d->mentionCompleter_->setDialogAimId(d->threadId_);
        d->mentionCompleter_->hideAnimated();

        d->input_->setMentionCompleter(d->mentionCompleter_);
        connect(&Utils::InterConnector::instance(), &Utils::InterConnector::hideMentionCompleter, this, &ThreadFeedItem::onHideMentionCompleter);

        updateInputVisibilityAccordingToYourRoleInThread();

        auto hideStickersAndSuggests = [this]()
        {
            if (d->stickerPicker_)
                d->stickerPicker_->hideAnimated();
            onSuggestHide();
            onHideMentionCompleter();
        };

        connect(d->input_, &InputWidget::sendMessage, this, hideStickersAndSuggests);
        connect(d->input_, &InputWidget::editFocusOut, this, hideStickersAndSuggests);
        connect(d->input_, &InputWidget::editFocusOut, this, [this]() { if (d->input_) d->input_->stopPttRecording(); });
    }

    d->groupSubscribeTimer_ = new QTimer(this);
    d->groupSubscribeTimer_->setSingleShot(true);
    d->parentForTooltips_ = _parentForTooltips;

    addMessages(_data.messages_);
    d->updateLoadMoreButton();

    connect(d->repliesPlate_, &ThreadRepliesItem::clicked, this, &ThreadFeedItem::onLoadMore);
    connect(d->groupSubscribeTimer_, &QTimer::timeout, this, [this](){ d->groupSubscribeSeq_ = GetDispatcher()->groupSubscribe(d->threadId_); });
    connect(d->header_, &ThreadFeedItemHeader::clicked, this, &ThreadFeedItem::onHeaderClicked);

    connect(d->input_, &InputWidget::editFocusIn, this, &ThreadFeedItem::focusedOnInput);
    connect(d->input_, &InputWidget::smilesMenuSignal, this, [this](const bool _fromKeyboard)
    {
        d->initStickerPicker();
        onSuggestHide();
        onHideMentionCompleter();
        d->stickerPicker_->showHideAnimated();
    }, Qt::QueuedConnection);

    connect(GetDispatcher(), &core_dispatcher::messageBuddies, this, &ThreadFeedItem::onMessageBuddies);
    connect(GetDispatcher(), &core_dispatcher::messagesDeleted, this, &ThreadFeedItem::onMessagesDeleted);
    connect(GetDispatcher(), &core_dispatcher::messageIdsFromServer, this, &ThreadFeedItem::onMessageIdsFromServer);
    connect(GetDispatcher(), &core_dispatcher::removePending, this, &ThreadFeedItem::onRemovePending);
    connect(GetDispatcher(), &core_dispatcher::subscribeResult, this, &ThreadFeedItem::onGroupSubscribeResult);
    connect(GetDispatcher(), &core_dispatcher::reactions, this, &ThreadFeedItem::onReactions);
    connect(&Utils::InterConnector::instance(), &Utils::InterConnector::selectionStateChanged, this, &ThreadFeedItem::selectionStateChanged);

    connect(&Utils::InterConnector::instance(), &Utils::InterConnector::clearSelection, this, &ThreadFeedItem::clearSelection);
}

ThreadFeedItem::~ThreadFeedItem()
{
    GetDispatcher()->setDialogOpen(d->threadId_, false);
    GetDispatcher()->cancelGroupSubscription(d->threadId_);
}

void ThreadFeedItem::copy()
{
    Utils::showCopiedToast();
}

void ThreadFeedItem::onForward(const Data::QuotesVec& _q)
{
    Q_EMIT Utils::InterConnector::instance().stopPttRecord();

    // disable author setting for single favorites messages which are not forwards
    const auto disableAuthor = Favorites::isFavorites(d->threadId_) && _q.size() == 1 && Favorites::isFavorites(_q.front().senderId_);

    if (forwardMessage(_q, true, QString(), QString(), !disableAuthor) != 0)
        Utils::InterConnector::instance().setMultiselect(false, d->threadId_);

    d->input_->setFocusOnInput();
}

void ThreadFeedItem::addToFavorites(const Data::QuotesVec& _q)
{
    Favorites::addToFavorites(_q);
    Favorites::showSavedToast(_q.size());
}

void ThreadFeedItem::onLoadMore()
{
    core_dispatcher::GetHistoryParams params;
    params.from_ = d->olderMsgId_;
    params.early_ = d->nextRepliesToLoadCount();
    params.firstRequest_ = std::exchange(d->firstRequest_, false);
    d->lastRequestedCount_ = params.early_;
    d->seq_ = GetDispatcher()->getHistory(d->threadId_, params);
}

void ThreadFeedItem::onMessageBuddies(const Data::MessageBuddies& _messages, const QString& _aimId, MessagesBuddiesOpt _type, bool _havePending, qint64 _seq, int64_t _lastMsgId)
{
    if (_aimId != d->threadId_)
        return;

    if (_messageBuddiesCache._messages == _messages
        && _messageBuddiesCache._aimId == _aimId
        && _messageBuddiesCache._type == _type
        && _messageBuddiesCache._havePending == _havePending
        && _messageBuddiesCache._seq == _seq
        && _messageBuddiesCache._lastMsgId == _lastMsgId)
        return;

    for (auto msg : _messages)
    {
        _messageBuddiesCache._messages.clear();
        _messageBuddiesCache._messages.push_back(msg);
    }
    _messageBuddiesCache._aimId = _aimId;
    _messageBuddiesCache._type = _type;
    _messageBuddiesCache._havePending = _havePending;
    _messageBuddiesCache._seq = _seq;
    _messageBuddiesCache._lastMsgId = _lastMsgId;

    auto replaceItem = [this](ThreadFeedItemUptr& _current, ThreadFeedItemUptr& _new, MessagesLayout* _layout, int _index)
    {
        auto updateIndex = [this](ThreadFeedItemUptr& _current)
        {
            d->index_[_current->buddy().ToKey()] = _current.get();
            d->lastMsgId_ = std::max(_current->getId(), d->lastMsgId_);
        };

        if (auto currentItem = qobject_cast<Ui::ComplexMessage::ComplexMessageItem*>(_current.get()))
        {
            if (auto newItem = qobject_cast<Ui::ComplexMessage::ComplexMessageItem*>(_new.get()))
            {
                if (currentItem->canBeUpdatedWith(*newItem))
                {
                    currentItem->updateWith(*newItem);
                    updateIndex(_current);
                    return;
                }
            }
        }

        _new->updateGeometry();
        d->addDateMessage(_new->buddy());
        _layout->replaceWidget(_new.get(), _index);
        _current = std::move(_new);
        updateIndex(_current);
    };

    if (d->seq_ == _seq)
    {
        auto messages = _messages;

        for (auto msg : messages)
            msg->threadData_.threadFeedMessage_ = true;

        const auto loadedLessThanRequested = messages.size() < d->lastRequestedCount_;

        if (!messages.empty() && d->olderMsgId_ >= messages.back()->Id_)
        {
            d->olderMsgId_ = messages.front()->Prev_;
            d->repliesToLoad_ -= messages.size();
            d->removeFirstDate(d->getDateKey(*messages.back().get()));
            addMessages(messages, true);
        }

        if (loadedLessThanRequested && d->olderMsgId_ > 0)
        {
            core_dispatcher::GetHistoryParams params;
            params.from_ = d->olderMsgId_;
            params.early_ = std::min(d->repliesToLoad_, loadMoreMaxCount() - messages.size());
            d->requestToRepeat_ = params;
            d->needToRepeatRequest_ = true;
        }

        d->updateLoadMoreButton();
    }
    else if (_type == MessagesBuddiesOpt::Pending)
    {
        for (auto msg : _messages)
        {
            msg->threadData_.threadFeedMessage_ = true;
            auto it = d->pendings_.find(msg->InternalId_);
            if (it == d->pendings_.end())
            {
                if (auto newItem = createItem(msg, this, width(), d->pinnedMessageId_))
                {
                    if (auto itMsg = d->index_.find(msg->ToKey()); itMsg != d->index_.end())
                    {
                        auto existing = itMsg->second;
                        auto index = d->messagesLayout_->indexOf(existing);
                        for (auto& item : d->items_)
                        {
                            if (item->getId() == msg->Id_)
                                replaceItem(item, newItem, d->messagesLayout_, index);
                        }
                    }
                    else if (d->pinnedMessageId_ == msg->Id_)
                    {
                        for (auto& item : d->items_)
                        {
                            if (item->getId() == msg->Id_)
                                replaceItem(item, newItem, d->pinnedMessageLayout_, d->pinnedMessageLayout_->indexOf(item.get()));
                        }
                    }
                    else if (d->index_.size() > 0 && std::find_if(d->index_.cbegin(), d->index_.cend(), [msg](const auto& _item)
                                                         { return _item.second && msg && (_item.second->getInternalId() == msg->InternalId_); }) == d->index_.cend())
                    {
                        if (newItem)
                        {
                            d->addDateMessage(newItem->buddy());
                            d->addMessage(std::move(newItem), true, true);
                        }
                    }
                }
            }

            if (msg->IsOutgoing() && msg->IsFileSharing())
                Q_EMIT activated();
        }
    }
    else if (_type == MessagesBuddiesOpt::MessageStatus || _type == MessagesBuddiesOpt::DlgState)
    {
        for (auto msg : _messages)
        {
            msg->threadData_.threadFeedMessage_ = true;
            auto it = d->pendings_.find(msg->InternalId_);
            if (it != d->pendings_.end())
            {
                auto pendingItem = it->second;
                if (auto newItem = createItem(msg, this, width(), d->pinnedMessageId_))
                {
                    auto index = d->messagesLayout_->indexOf(pendingItem);
                    for (auto& item : d->items_)
                    {
                        if (item->getInternalId() == msg->InternalId_)
                        {
                            replaceItem(item, newItem, d->messagesLayout_, index);
                            d->pendings_.erase(it);
                            break;
                        }
                    }
                }
            }
            else if (auto it = d->index_.find(msg->ToKey()); it != d->index_.end())
            {
                auto existing = it->second;
                if (auto newItem = createItem(msg, this, width(), d->pinnedMessageId_))
                {
                    auto layout = d->pinnedMessageId_ == msg->Id_ ? d->pinnedMessageLayout_ : d->messagesLayout_;
                    auto index = layout->indexOf(existing);
                    for (auto& item : d->items_)
                    {
                        if (item->getId() == msg->Id_)
                            replaceItem(item, newItem, layout, index);
                    }
                }
            }
            else if (d->index_.find(msg->ToKey()) == d->index_.end() && msg->Id_ > d->lastMsgId_)
            {
                if (auto item = createItem(msg, this, width(), d->pinnedMessageId_))
                {
                    d->addDateMessage(msg);
                    d->addMessage(std::move(item));
                }
            }
            if (msg->IsOutgoing() && msg->IsFileSharing())
                Q_EMIT activated();
        }
    }

    readLastMessage();
}

void ThreadFeedItem::onMessagesDeleted(const QString& _aimid, const Data::MessageBuddies& _messages)
{
    if (_aimid != d->threadId_)
        return;

    std::vector<Logic::MessageKey> msgKeys;
    for (auto msg : _messages)
        msgKeys.push_back(msg->ToKey());


    if (msgKeys == d->lastDeletedCache_)
        return;

    d->lastDeletedCache_.clear();
    for (auto msg : _messages)
        d->lastDeletedCache_.push_back(msg->ToKey());

    std::vector<QDate> deletedDates;
    for (auto msg : _messages)
    {
        if (auto it = d->index_.find(msg->ToKey()); it != d->index_.end())
        {
            auto index = d->messagesLayout_->indexOf(it->second);
            auto w = d->messagesLayout_->takeAt(index);
            d->index_.erase(it);
            d->items_.erase(std::remove_if(d->items_.begin(), d->items_.end(), [&w, &deletedDates](const auto& _item)
            {
                auto ret = _item.get() == w->widget();
                if (ret)
                    deletedDates.push_back(_item->buddy().GetDate());
                return ret;
            }), d->items_.end());
            d->messagesLayout_->invalidate();
        }
    }

    d->deleteDates(deletedDates);
}

void ThreadFeedItem::onRemovePending(const QString& _aimId, const Logic::MessageKey& _key)
{
    d->lastDeletedCache_.push_back(_key);

    std::vector<QDate> deletedDates;
    if (auto it = d->index_.find(_key); it != d->index_.end())
    {
        auto index = d->messagesLayout_->indexOf(it->second);
        auto w = d->messagesLayout_->takeAt(index);
        d->index_.erase(it);
        d->items_.erase(std::remove_if(d->items_.begin(), d->items_.end(), [&w, &deletedDates](const auto& _item)
        {
            auto ret = _item.get() == w->widget();
            if (ret)
                deletedDates.push_back(_item->buddy().GetDate());
            return ret;
        }), d->items_.end());
        d->messagesLayout_->invalidate();
    }
    d->deleteDates(deletedDates);
}

void ThreadFeedItem::onMessageIdsFromServer(const QVector<qint64>& _ids, const QString& _aimId, qint64 _seq)
{
    if (_aimId == d->threadId_ && std::exchange(d->needToRepeatRequest_, false))
        d->seq_ = GetDispatcher()->getHistory(d->threadId_, d->requestToRepeat_);
}

void ThreadFeedItem::onHeaderClicked()
{
    Q_EMIT Utils::InterConnector::instance().addPageToDialogHistory(ServiceContacts::contactId(ServiceContacts::ContactType::ThreadsFeed));
    Utils::InterConnector::instance().openDialog(d->parentTopic_->chat_, d->parentTopic_->msgId_);
}

void ThreadFeedItem::onGroupSubscribeResult(const int64_t _seq, int _error, int _resubscribeIn)
{
    if (_seq != d->groupSubscribeSeq_ || _error)
        return;

    if (!d->groupSubscribeTimer_)
    {
        d->groupSubscribeTimer_ = new QTimer(this);
        d->groupSubscribeTimer_->setSingleShot(true);
    }

    d->groupSubscribeTimer_->start(std::chrono::seconds(_resubscribeIn));
}

void ThreadFeedItem::onSuggestShow(const QString& _text, const QPoint& _pos)
{
    showStickersSuggest(_text, _pos);
}

void ThreadFeedItem::onSuggestHide()
{
    if (d->suggestsWidget_)
        d->suggestsWidget_->hideAnimated();
    d->input_->clearLastSuggests();
    d->suggestWidgetShown_ = false;
}

void ThreadFeedItem::onSuggestedStickerSelected(const Utils::FileSharingId& _stickerId)
{
    if (d->suggestsWidget_)
        d->suggestsWidget_->hideAnimated();

    d->initStickerPicker();
    d->stickerPicker_->addStickerToRecents(-1, _stickerId);
    d->input_->sendSticker(_stickerId);
    d->input_->clearInputText();
}

void ThreadFeedItem::onHideMentionCompleter()
{
    if (d->mentionCompleter_)
        d->mentionCompleter_->hideAnimated();
}

void ThreadFeedItem::selectionStateChanged(const QString& _aimid, qint64, const QString&, bool _selected)
{
    if (_aimid == d->threadId_ && _selected && hasFocus())
    {
        if (d->input_)
            d->input_->setFocusOnInput();
        Q_EMIT selected();
    }
}

void ThreadFeedItem::onReactions(const QString& _contact, const std::vector<Data::Reactions> _reactionsData)
{
    if (_contact != threadId())
        return;

    for (const auto& reactions : _reactionsData)
    {
        for (auto& item : d->items_)
        {
            if (item && item->getId() == reactions.msgId_)
                item->setReactions(reactions);
        }
    }
}

void ThreadFeedItem::onNavigationKeyPressed(int _key, Qt::KeyboardModifiers _modifiers)
{
    const bool applePageUp = isApplePageUp(_key, _modifiers);

    if (_key == Qt::Key_Up && !applePageUp && d->input_->isInputEmpty())
        d->messagesLayout_->tryToEditLastMessage();
}

void ThreadFeedItem::edit(const Data::MessageBuddySptr& _msg, MediaType _mediaType)
{
    if (d->input_)
    {
        Q_EMIT editing();
        d->input_->edit(_msg, _mediaType);
    }
}

bool ThreadFeedItem::containsMessage(const Logic::MessageKey& _key)
{
    return d->index_.count(_key) != 0;
}

int64_t ThreadFeedItem::parentMsgId() const
{
    return d->parentTopic_->msgId_;
}

const QString& ThreadFeedItem::parentChatId() const
{
    return d->parentTopic_->chat_;
}

const QString& ThreadFeedItem::threadId() const
{
    return d->threadId_;
}

int64_t ThreadFeedItem::lastMsgId() const
{
    return d->lastMsgId_;
}

void ThreadFeedItem::updateVisibleRects()
{
    for (auto& item : d->items_)
        item->onVisibleRectChanged(item->visibleRegion().boundingRect());

    d->updateDragOverlay();
}

QRect ThreadFeedItem::inputGeometry() const
{
    if (!d->input_)
        return {};

    return d->input_->geometry();
}

InputWidget* ThreadFeedItem::inputWidget() const
{
    return d->input_;
}

bool ThreadFeedItem::isInputActive() const
{
    if (d->input_)
        return d->input_->isActive();

    return false;
}

void ThreadFeedItem::positionMentionCompleter(const QPoint& _atPos)
{
    if (d->mentionCompleter_)
    {
        d->mentionCompleter_->setFixedWidth(std::min(width(), Tooltip::getMaxMentionTooltipWidth()));
        d->mentionCompleter_->recalcHeight();

        const auto x = (width() - d->mentionCompleter_->width()) / 2;
        const auto y = d->mentionCompleter_->parentWidget()->mapFromGlobal(_atPos).y() - d->mentionCompleter_->height() + Tooltip::getArrowHeight();
        d->mentionCompleter_->move(x, y);
    }
}

void ThreadFeedItem::setInputVisible(bool _visible)
{
    if (!d->isInputEnabled_)
        return;

    if (d->inputWasVisible_ != _visible)
    {
        if (_visible && !d->input_->isVisible())
            d->input_->setVisible(true);

        if (_visible)
            readLastMessage(true);
        else
            d->input_->stopPttRecording();

        d->inputWasVisible_ = _visible;
    }
}

void ThreadFeedItem::updateInputVisibilityAccordingToYourRoleInThread()
{
    const auto areYouAllowed = Logic::getContactListModel()->areYouAllowedToWriteInThreads(parentChatId());
    d->setInputAllowed(areYouAllowed);
    setInputVisible(areYouAllowed);
}

void ThreadFeedItem::clearSelection()
{
    if (d->input_ && !d->input_->isActive())
    {
        d->messagesLayout_->clearSelection();
        d->pinnedMessageLayout_->clearSelection();
    }
}

void ThreadFeedItem::resizeEvent(QResizeEvent* _event)
{
    d->updateBubblePath();
    d->updateDragOverlay();
    if (d->mentionCompleter_ && d->mentionCompleter_->isVisible())
    {
        d->input_->updateGeometry();
        positionMentionCompleter(d->input_->tooltipArrowPosition());
    }
    if (d->suggestWidgetShown_)
    {
        const auto text = d->input_->getInputText().string();
        const auto pos = d->input_->suggestPosition();
        showStickersSuggest(text, pos);
    }
    Q_EMIT sizeUpdated();
    QWidget::resizeEvent(_event);
}

void ThreadFeedItem::mousePressEvent(QMouseEvent* _event)
{
    if (_event->button() == Qt::LeftButton)
    {
        Q_EMIT Utils::InterConnector::instance().clearSelection();
        d->messagesLayout_->clearSelection();
        d->pinnedMessageLayout_->clearSelection();
        d->selectFrom_ = _event->pos();
        if (d->input_)
            d->input_->setFocusOnInput();
    }
    QWidget::mousePressEvent(_event);
}

void ThreadFeedItem::mouseReleaseEvent(QMouseEvent* _event)
{
    if (Utils::clicked(d->selectFrom_, _event->pos()))
        d->tryHideEmojiPicker(_event->pos());
    d->selectFrom_ = {};
}

void ThreadFeedItem::mouseMoveEvent(QMouseEvent* _event)
{
    if (!d->selectFrom_.isNull())
    {
        clearSelection();
        d->pinnedMessageLayout_->selectByPos(mapToGlobal(d->selectFrom_), mapToGlobal(_event->pos()),
                                             mapToGlobal(rect().topLeft()), mapToGlobal(rect().bottomRight()));
        d->messagesLayout_->selectByPos(mapToGlobal(d->selectFrom_), mapToGlobal(_event->pos()),
                                        mapToGlobal(rect().topLeft()), mapToGlobal(rect().bottomRight()));
    }
    QWidget::mouseMoveEvent(_event);
}

void ThreadFeedItem::keyPressEvent(QKeyEvent* _event)
{
    if (_event->type() == QEvent::KeyPress)
    {
        QKeyEvent* keyEvent = static_cast<QKeyEvent*>(_event);
        if (keyEvent->matches(QKeySequence::Copy))
        {
            auto text = d->pinnedMessageLayout_->getSelectedText();
            if (text.isEmpty())
                text = d->messagesLayout_->getSelectedText();

            if (!text.isEmpty())
            {
                QApplication::clipboard()->setMimeData(MimeData::toMimeData(std::move(text), {}, {}));
                Utils::showCopiedToast();
            }
        }
    }
    QWidget::keyPressEvent(_event);
}

void ThreadFeedItem::paintEvent(QPaintEvent* _event)
{
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);

    const auto color = Styling::getParameters().getColor(Styling::StyleVariable::CHAT_ENVIRONMENT);
    Utils::drawBubbleShadow(p, d->bubblePath_);
    p.fillPath(d->bubblePath_, color);
}

void ThreadFeedItem::dragEnterEvent(QDragEnterEvent* _event)
{
    const auto ignoreDrop =
        !d->isInputEnabled_ ||
        !(_event->mimeData() && (_event->mimeData()->hasUrls() || _event->mimeData()->hasImage())) ||
        _event->mimeData()->property(mimetype_marker()).toBool() ||
        QApplication::activeModalWidget() != nullptr ||
        Logic::getContactListModel()->isReadonly(d->threadId_) ||
        Logic::getContactListModel()->isDeleted(d->threadId_);

    if (ignoreDrop || !d->canDragDrop())
    {
        _event->setDropAction(Qt::IgnoreAction);
        return;
    }

    d->showDragOverlay();
    d->input_->setAttribute(Qt::WA_TransparentForMouseEvents, true);
    _event->acceptProposedAction();
}

void ThreadFeedItem::dragLeaveEvent(QDragLeaveEvent* _event)
{
    _event->accept();
    d->hideDragOverlay();
    d->input_->setAttribute(Qt::WA_TransparentForMouseEvents, false);
}

void ThreadFeedItem::dropEvent(QDropEvent* _event)
{
    const QMimeData* mimeData = _event->mimeData();
    d->input_->setAttribute(Qt::WA_TransparentForMouseEvents, false);
    d->dragOverlayWindow_->processMimeData(mimeData);
    d->hideDragOverlay();
}

void ThreadFeedItem::addMessages(const Data::MessageBuddies& _messages, bool _front)
{
    if (_messages.size() == 0)
        return;

    auto messages = _messages;
    Data::MessageBuddies messagesWithDates;
    std::shared_ptr<Data::MessageBuddy> prevMsg = nullptr;
    for (auto& msg : messages)
    {
        auto dateKey = d->getDateKey(*msg.get());
        if (!prevMsg || d->dateInserter_->needDate(d->getDateKey(*prevMsg.get()), dateKey))
            messagesWithDates.push_back(std::make_shared<Data::MessageBuddy>(d->dateInserter_->makeDateMessage(dateKey)));
        messagesWithDates.push_back(msg);
        prevMsg = msg;
    }
    if (_front)
        std::reverse(messagesWithDates.begin(), messagesWithDates.end());

    for (auto& msg : messagesWithDates)
    {
        if (d->isDateMessage(msg))
        {
            d->addDateMessage(msg, _front, true);
        }
        else
        {
            if (auto item = createItem(msg, this, width(), d->pinnedMessageId_))
                d->addMessage(std::move(item), true, false, _front);
        }
    }

    d->tailSize_ += _messages.size();
    if (d->unreadCount_ > 0 && d->unreadCount_ <= d->tailSize_)
        d->addNewMessagesPlate();

    readLastMessage();
}

void ThreadFeedItem::readLastMessage(bool _force)
{
    if (_force || d->input_->isActive())
        GetDispatcher()->setLastRead(d->threadId_, d->lastMsgId_, core_dispatcher::ReadAll::Yes);
}

void ThreadFeedItem::showStickersSuggest(const QString& _text, const QPoint& _pos)
{
    if (!d->suggestsWidget_)
    {
        d->suggestsWidget_ = new Stickers::StickersSuggest(d->parentForTooltips_);
        d->suggestsWidget_->setArrowVisible(true);
        connect(d->suggestsWidget_, &Stickers::StickersSuggest::stickerSelected, this, &ThreadFeedItem::onSuggestedStickerSelected);
        connect(d->suggestsWidget_, &Stickers::StickersSuggest::scrolledToLastItem, d->input_, &InputWidget::requestMoreStickerSuggests);
    }
    const auto fromInput = d->input_ && QRect(d->input_->mapToGlobal(d->input_->rect().topLeft()), d->input_->size()).contains(_pos);
    const auto maxSize = QSize(!fromInput ? width() : std::min(width(), Tooltip::getMaxMentionTooltipWidth()), height());

    const auto pt = d->parentForTooltips_->mapFromGlobal(_pos);
    d->suggestsWidget_->setNeedScrollToTop(!d->input_->hasServerSuggests());

    auto areaRect = rect();
    if (areaRect.width() > MessageStyle::getHistoryWidgetMaxWidth())
    {
        const auto center = areaRect.center();
        areaRect.setWidth(MessageStyle::getHistoryWidgetMaxWidth());

        if (fromInput)
        {
            areaRect.moveCenter(center);
        }
        else
        {
            if (pt.x() < width() / 3)
                areaRect.moveLeft(0);
            else if (pt.x() >= (width() * 2) / 3)
                areaRect.moveRight(width());
            else
                areaRect.moveCenter(center);
        }
    }

    if (d->suggestWidgetShown_)
    {
        d->suggestsWidget_->updateStickers(_text, pt, maxSize, areaRect);
    }
    else
    {
        d->suggestsWidget_->showAnimated(_text, pt, maxSize, areaRect);
        d->suggestWidgetShown_ = !d->suggestsWidget_->getStickers().empty();
    }
}

}