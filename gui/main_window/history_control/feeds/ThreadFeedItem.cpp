#include "stdafx.h"

#include "../HistoryControlPageItem.h"
#include "../ServiceMessageItem.h"
#include "../history/MessageBuilder.h"
#include "main_window/containers/FriendlyContainer.h"
#include "main_window/contact_list/ContactListModel.h"
#include "styles/ThemeParameters.h"
#include "cache/avatars/AvatarStorage.h"
#include "controls/TextUnit.h"
#include "controls/TooltipWidget.h"
#include "utils/utils.h"
#include "utils/InterConnector.h"
#include "utils/PainterPath.h"
#include "previewer/toast.h"
#include "fonts.h"
#include "ThreadFeedItem.h"
#include "MessagesLayout.h"
#include "main_window/input_widget/InputWidget.h"
#include "main_window/input_widget/InputWidgetUtils.h"
#include "main_window/contact_list/ServiceContacts.h"
#include "main_window/history_control/MessageStyle.h"
#include "main_window/history_control/MentionCompleter.h"
#include "main_window/history_control/complex_message/ComplexMessageItem.h"
#include "main_window/history_control/complex_message/FileSharingUtils.h"
#include "core_dispatcher.h"
#include "main_window/smiles_menu/SmilesMenu.h"
#include "main_window/smiles_menu/suggests_widget.h"

namespace
{

int headerHeight() noexcept
{
    return Utils::scale_value(32 + 2*12);
}

int topMargin() noexcept
{
    return Utils::scale_value(4);
}

int inputBottomMargin() noexcept
{
    return Utils::scale_value(2);
}

int bottomMargin() noexcept
{
    return Utils::scale_value(8);
}

int chatThreadBubbleBorderRadius() noexcept
{
    return Utils::scale_value(16);
}

constexpr int headerAvatarSize()
{
    return 32;
}

int headerAvatarSizeScaled() noexcept
{
    return Utils::scale_value(headerAvatarSize());
}

int headerAvatarRightMargin() noexcept
{
    return Utils::scale_value(8);
}

int headerAvatarLeftMargin() noexcept
{
    return Utils::scale_value(16);
}

int headerLeftMargin() noexcept
{
    return Utils::scale_value(8);
}

int headerRightMargin() noexcept
{
    return Utils::scale_value(16);
}

int newMessagesPlateTopMargin() noexcept
{
    return Utils::scale_value(4);
}

int newMessagesPlateBottomMargin() noexcept
{
    return Utils::scale_value(16);
}

QMargins messagesLayoutMargins() noexcept
{
    return Utils::scale_value(QMargins(0, 10, 0, 8));
}

QColor headerTextColor()
{
    return Styling::getParameters().getColor(Styling::StyleVariable::TEXT_SOLID);
}

const QColor& headerBackgroundColor(bool _hovered, bool _pressed)
{
    if (_pressed)
    {
        static const auto c = Styling::getParameters().getColor(Styling::StyleVariable::BASE_SECONDARY_ACTIVE, 0.15);
        return c;
    }

    if (_hovered)
    {
        static const auto c = Styling::getParameters().getColor(Styling::StyleVariable::BASE_SECONDARY_HOVER, 0.15);
        return c;
    }

    static const auto c = Styling::getParameters().getColor(Styling::StyleVariable::BASE_SECONDARY, 0.15);
    return c;
}

int headerTextOffsetV() noexcept
{
    return Utils::scale_value(-1);
}
    
std::unique_ptr<Ui::HistoryControlPageItem> createItem(Data::MessageBuddySptr _msg, Ui::ThreadFeedItem* _parent, int _width, int64_t _topicStarter)
{
    if (_msg->Id_ == _topicStarter)
        _msg->SetOutgoing(false);//show topicstarter as incoming message

    std::unique_ptr<Ui::HistoryControlPageItem> item;
    if (item = hist::MessageBuilder::makePageItem(*_msg, _width, _parent))
    {
        item->setHasAvatar(true);
        item->setHasSenderName(true);
        item->setChainedToNext(false);
        item->setChainedToPrev(false);
        item->onActivityChanged(true);
        item->initSize();
        QObject::connect(item.get(), &Ui::HistoryControlPageItem::edit, _parent, &Ui::ThreadFeedItem::edit);
        QObject::connect(item.get(), &Ui::HistoryControlPageItem::quote, _parent, &Ui::ThreadFeedItem::quote);
    }
    return item;
}

constexpr int64_t loadMoreMaxCount() noexcept
{
    return 50;
}

int64_t loadMoreHeight() noexcept
{
    return Utils::scale_value(20);
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
    ThreadFeedItem_p(QWidget* _q) : q(_q) {}

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

    void addItem(std::unique_ptr<Ui::HistoryControlPageItem>&& _item)
    {
        items_.push_back(std::move(_item));
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

    QString threadId_;
    QBoxLayout* layout_ = nullptr;
    MessagesLayout* messagesLayout_ = nullptr;
    MessagesLayout* pinnedMessageLayout_ = nullptr;
    InputWidget* input_ = nullptr;
    bool isInputEnabled_ = false;
    Stickers::StickersSuggest* suggestsWidget_ = nullptr;
    MentionCompleter* mentionCompleter_ = nullptr;
    std::shared_ptr<Data::MessageParentTopic> parentTopic_;
    std::vector<std::unique_ptr<Ui::HistoryControlPageItem>> items_;
    std::unordered_map<int64_t, Ui::HistoryControlPageItem*> index_;
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
    Ui::ServiceMessageItem* repliesPlate_ = nullptr;
    Smiles::SmilesMenu* stickerPicker_ = nullptr;
    QWidget* q;
    QWidget* parentForTooltips_ = nullptr;
    int64_t pinnedMessageId_ = -1;
    QPoint selectFrom_;
    Utils::RenderBubbleFlags renderBubbleFlags_ = Utils::RenderBubbleFlags::AllRounded;
};

//////////////////////////////////////////////////////////////////////////
// ThreadFeedItem
//////////////////////////////////////////////////////////////////////////

ThreadFeedItem::ThreadFeedItem(const Data::ThreadFeedItemData& _data, QWidget* _parent, QWidget* _parentForTooltips)
    : QWidget(_parent),
      d(std::make_unique<ThreadFeedItem_p>(this))
{
    d->threadId_ = _data.threadId_;
    d->olderMsgId_ = _data.olderMsgId_;
    d->repliesToLoad_ = _data.repliesCount_ - _data.messages_.size();
    d->unreadCount_ = _data.unreadCount_;
    d->parentTopic_ = _data.parent_;
    d->layout_ = Utils::emptyVLayout(this);

    auto header = new ThreadFeedItemHeader(_data.parent_->chat_, this);
    d->layout_->addWidget(header);

    d->pinnedMessageLayout_ = new MessagesLayout();
    d->pinnedMessageLayout_->setContentsMargins(0, 0, 0, Utils::scale_value(6));

    if (auto item = createItem(_data.parentMessage_, this, width(), _data.parentMessage_->Id_))
    {
        d->pinnedMessageId_ = item->getId();
        d->pinnedMessageLayout_->addWidget(item.get());
        d->addItem(std::move(item));
    }

    d->repliesPlate_ = new ServiceMessageItem(this);
    d->repliesPlate_->setContact(d->threadId_);

    auto buttonLayout = Utils::emptyHLayout();
    buttonLayout->addStretch();
    buttonLayout->addWidget(d->repliesPlate_);
    buttonLayout->addStretch();

    d->messagesLayout_ = new MessagesLayout();
    d->messagesLayout_->setContentsMargins(messagesLayoutMargins());

    d->layout_->addSpacing(topMargin());
    d->layout_->addLayout(d->pinnedMessageLayout_);
    d->layout_->addLayout(buttonLayout);
    d->layout_->addLayout(d->messagesLayout_);
    d->layout_->addStretch(1);

    if (!d->threadId_.isEmpty())
    {
        Logic::getContactListModel()->markAsThread(d->threadId_, d->parentTopic_->chat_);
        d->input_ = new InputWidget(d->threadId_, this, Ui::defaultInputFeatures() | Ui::InputFeature::LocalState, nullptr);
        d->input_->setThreadFeedItem(this);
        d->input_->setParentForPopup(_parentForTooltips);
        connect(d->input_, &InputWidget::heightChanged, this, &ThreadFeedItem::inputHeightChanged);
        connect(d->input_, &InputWidget::needSuggets, this, &ThreadFeedItem::onSuggestShow);
        connect(d->input_, &InputWidget::hideSuggets, this, &ThreadFeedItem::onSuggestHide);
        connect(this, &ThreadFeedItem::quote, d->input_, &InputWidget::quote);
        d->layout_->addWidget(d->input_);
        d->layout_->addSpacing(bottomMargin());

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
        connect(d->input_, &InputWidget::viewChanged, this, hideStickersAndSuggests);
        connect(d->input_, &InputWidget::editFocusOut, this, [this]() { if (d->input_) d->input_->stopPttRecording(); });
    }

    d->groupSubscribeTimer_ = new QTimer(this);
    d->groupSubscribeTimer_->setSingleShot(true);
    d->parentForTooltips_ = _parentForTooltips;

    addMessages(_data.messages_);
    d->updateLoadMoreButton();

    connect(d->repliesPlate_, &ServiceMessageItem::clicked, this, &ThreadFeedItem::onLoadMore);
    connect(d->groupSubscribeTimer_, &QTimer::timeout, this, [this](){ d->groupSubscribeSeq_ = GetDispatcher()->groupSubscribe(d->threadId_); });
    connect(header, &ThreadFeedItemHeader::clicked, this, &ThreadFeedItem::onHeaderClicked);

    connect(d->input_, &InputWidget::editFocusIn, this, &ThreadFeedItem::focusedOnInput);
    connect(d->input_, &InputWidget::smilesMenuSignal, this, [this]()
    {
        d->initStickerPicker();
        onSuggestHide();
        onHideMentionCompleter();
        d->stickerPicker_->showHideAnimated();
    });

    connect(GetDispatcher(), &core_dispatcher::messageBuddies, this, &ThreadFeedItem::onMessageBuddies);
    connect(GetDispatcher(), &core_dispatcher::messagesDeleted, this, &ThreadFeedItem::onMessagesDeleted);
    connect(GetDispatcher(), &core_dispatcher::messageIdsFromServer, this, &ThreadFeedItem::onMessageIdsFromServer);
    connect(GetDispatcher(), &core_dispatcher::subscribeResult, this, &ThreadFeedItem::onGroupSubscribeResult);
    connect(&Utils::InterConnector::instance(), &Utils::InterConnector::selectionStateChanged, this, &ThreadFeedItem::selectionStateChanged);
}

ThreadFeedItem::~ThreadFeedItem()
{
    GetDispatcher()->setDialogOpen(d->threadId_, false);
    GetDispatcher()->cancelGroupSubscription(d->threadId_);
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

    auto replaceItem = [this](std::unique_ptr<Ui::HistoryControlPageItem>& _current, std::unique_ptr<Ui::HistoryControlPageItem>& _new, MessagesLayout* _layout, int _index)
    {
        auto updateIndex = [this](std::unique_ptr<Ui::HistoryControlPageItem>& _current)
        {
            d->index_[_current->getId()] = _current.get();
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
                    if (auto itMsg = d->index_.find(msg->Id_); itMsg != d->index_.end())
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
                    else if (std::find_if(d->index_.cbegin(), d->index_.cend(), [msg](const auto& i) { return i.second->getInternalId() == msg->InternalId_; }) == d->index_.cend())
                    {
                        d->messagesLayout_->addWidget(newItem.get());
                        newItem->show();
                        d->pendings_[msg->InternalId_] = newItem.get();
                        d->addItem(std::move(newItem));
                    }
                }
            }
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
                if (auto newItem = createItem(msg, this, width(),d->pinnedMessageId_))
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
            else if (auto it = d->index_.find(msg->Id_); it != d->index_.end())
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
            else if (d->index_.find(msg->Id_) == d->index_.end() && msg->Id_ > d->lastMsgId_)
            {
                if (auto item = createItem(msg, this, width(), d->pinnedMessageId_))
                {
                    d->messagesLayout_->addWidget(item.get());
                    item->show();
                    d->index_[msg->Id_] = item.get();
                    d->addItem(std::move(item));
                    d->lastMsgId_ = msg->Id_;
                }
            }
        }
    }

    readLastMessage();
}

void ThreadFeedItem::onMessagesDeleted(const QString& _aimid, const Data::MessageBuddies& _messages)
{
    if (_aimid != d->threadId_)
        return;

    for (auto msg : _messages)
    {
        if (auto it = d->index_.find(msg->Id_); it != d->index_.end())
        {
            auto index = d->messagesLayout_->indexOf(it->second);
            auto w = d->messagesLayout_->takeAt(index);
            d->index_.erase(it);
            d->items_.erase(std::remove_if(d->items_.begin(), d->items_.end(), [w](const auto& i) { return i.get() == w->widget(); }), d->items_.end());
            d->messagesLayout_->invalidate();
        }
    }
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
    if (_aimid == d->threadId_ && _selected)
    {
        if (d->input_)
            d->input_->setFocusOnInput();
        Q_EMIT selected();
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
        d->input_->edit(_msg, _mediaType);
}

bool ThreadFeedItem::highlightQuote(int64_t _msgId)
{
    auto it = d->index_.find(_msgId);
    if (it != d->index_.end())
    {
        it->second->setQuoteSelection();
        return true;
    }
    return false;
}

bool ThreadFeedItem::containsMessage(int64_t _msgId)
{
    return d->index_.count(_msgId) != 0;
}

int ThreadFeedItem::messageY(int64_t _msgId)
{
    auto it = d->index_.find(_msgId);
    if (it != d->index_.end())
        return it->second->geometry().top();

    return -1;
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

void ThreadFeedItem::setIsFirstInFeed(bool _isFirstInFeed)
{
    d->renderBubbleFlags_ = _isFirstInFeed
        ? Utils::RenderBubbleFlags::BottomSideRounded
        : Utils::RenderBubbleFlags::AllRounded;
}

void ThreadFeedItem::resizeEvent(QResizeEvent* _event)
{
    if (d->mentionCompleter_ && d->mentionCompleter_->isVisible())
    {
        d->input_->updateGeometry();
        positionMentionCompleter(d->input_->tooltipArrowPosition());
    }
    Q_EMIT sizeUpdated();
    QWidget::resizeEvent(_event);
}

void ThreadFeedItem::mousePressEvent(QMouseEvent* _event)
{
    if (_event->button() == Qt::LeftButton)
    {
        d->messagesLayout_->clearSelection();
        d->pinnedMessageLayout_->clearSelection();
        d->selectFrom_ = _event->pos();
    }
    QWidget::mousePressEvent(_event);
}

void ThreadFeedItem::mouseMoveEvent(QMouseEvent* _event)
{
    d->pinnedMessageLayout_->selectByPos(mapToGlobal(d->selectFrom_), mapToGlobal(_event->pos()), mapToGlobal(rect().topLeft()), mapToGlobal(rect().bottomRight()));
    d->messagesLayout_->selectByPos(mapToGlobal(d->selectFrom_), mapToGlobal(_event->pos()), mapToGlobal(rect().topLeft()), mapToGlobal(rect().bottomRight()));

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
    const auto bubbleRect = rect().adjusted(0, 0, 0, -1);
    const auto bubblePath = Utils::renderMessageBubble(bubbleRect, chatThreadBubbleBorderRadius(), MessageStyle::getBorderRadiusSmall(), d->renderBubbleFlags_);
    Utils::drawBubbleShadow(p, bubblePath);
    p.fillPath(bubblePath, color);
}

void ThreadFeedItem::addMessages(const Data::MessageBuddies& _messages, bool _front)
{
    auto messages = _messages;
    if (_front)
        std::reverse(messages.begin(), messages.end());

    for (auto& msg : messages)
    {
        if (auto item = createItem(msg, this, width(), d->pinnedMessageId_))
        {
            d->messagesLayout_->insertWidget(item.get(), _front ? 0 : -1);
            item->show();
            d->index_[msg->Id_] = item.get();
            d->addItem(std::move(item));
            d->lastMsgId_ = std::max(msg->Id_, d->lastMsgId_);
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

//////////////////////////////////////////////////////////////////////////
// ThreadFeedItemHeader_p
//////////////////////////////////////////////////////////////////////////

class ThreadFeedItemHeader_p
{
public:
    void updateContent(int _width)
    {
        if (!chatName_)
            return;

        const auto availableWidth = _width - headerLeftMargin() - headerAvatarRightMargin() - headerAvatarSizeScaled() - headerRightMargin();
        const auto h = chatName_->getHeight(availableWidth);
        chatName_->getLastLineWidth();

        auto x = headerAvatarLeftMargin();
        avatarRect_ = QRect(x, (headerHeight() - headerAvatarSizeScaled()) / 2, headerAvatarSizeScaled(), headerAvatarSizeScaled());
        x += headerAvatarSizeScaled() + headerAvatarRightMargin();
        chatName_->setOffsets(x, headerHeight() / 2 + headerTextOffsetV());
    }
    QString chatId_;
    QRect avatarRect_;
    bool pressed_ = false;
    TextRendering::TextUnitPtr chatName_;
};

//////////////////////////////////////////////////////////////////////////
// ThreadFeedItemHeader
//////////////////////////////////////////////////////////////////////////

ThreadFeedItemHeader::ThreadFeedItemHeader(const QString& _chatId, QWidget* _parent)
    : ClickableWidget(_parent)
    , d(std::make_unique<ThreadFeedItemHeader_p>())
{
    Testing::setAccessibleName(this, qsl("AS ThreadFeedItemHeader"));

    d->chatId_ = _chatId;
    d->chatName_ = TextRendering::MakeTextUnit(Logic::GetFriendlyContainer()->getFriendly(_chatId), {}, TextRendering::LinksVisible::DONT_SHOW_LINKS, TextRendering::ProcessLineFeeds::REMOVE_LINE_FEEDS);
    d->chatName_->init(Fonts::adjustedAppFont(16, Fonts::FontWeight::Medium), headerTextColor(), {}, {}, {}, TextRendering::HorAligment::LEFT, 1);

    setFixedHeight(headerHeight());
    setCursor(Qt::PointingHandCursor);
    auto rootLayout = Utils::emptyHLayout(this);
    rootLayout->addStretch();

    connect(this, &ThreadFeedItemHeader::pressed, this, [this](){ update(); });
}

ThreadFeedItemHeader::~ThreadFeedItemHeader() = default;

void ThreadFeedItemHeader::paintEvent(QPaintEvent* _event)
{
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);

    bool isDefault;
    const auto avatarSize = Utils::scale_bitmap(headerAvatarSizeScaled());
    p.drawPixmap(d->avatarRect_, Logic::GetAvatarStorage()->GetRounded(d->chatId_, Logic::GetFriendlyContainer()->getFriendly(d->chatId_), avatarSize, isDefault, false, false));
    d->chatName_->draw(p, TextRendering::VerPosition::MIDDLE);
}

void ThreadFeedItemHeader::resizeEvent(QResizeEvent* _event)
{
    d->updateContent(_event->size().width());
}

}
