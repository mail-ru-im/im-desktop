#include "stdafx.h"
#include "controls/StableScrollArea.h"
#include "main_window/input_widget/InputWidget.h"
#include "main_window/contact_list/ContactListModel.h"
#include "../HistoryControlPageItem.h"
#include "utils/utils.h"
#include "utils/InterConnector.h"
#include "ThreadsContainer.h"
#include "ThreadFeedItem.h"
#include "ThreadsFeedLoader.h"
#include "styles/ThemeParameters.h"
#include "../MessageStyle.h"
#include "core_dispatcher.h"
#include "controls/LoaderSpinner.h"
#include "main_window/MainPage.h"

namespace
{
    constexpr auto preloadDistance() noexcept { return 0.5; }

    QSize getSpinnerSize()
    {
        return Utils::scale_value(QSize(64, 64));
    }

    constexpr std::chrono::milliseconds getLoaderDelay() noexcept { return std::chrono::milliseconds(300); }

    int threadsTopMargin() noexcept
    {
        return Utils::scale_value(12);
    }

    int threadItemsSeparatorHeight() noexcept
    {
        return Utils::scale_value(12);
    }

    QWidget* createThreadItemsSeparator(QWidget* _parent)
    {
        auto separator = new QWidget(_parent);
        separator->setFixedHeight(threadItemsSeparatorHeight());
        separator->setAttribute(Qt::WA_NoSystemBackground);
        separator->setAttribute(Qt::WA_TranslucentBackground);
        separator->setAttribute(Qt::WA_TransparentForMouseEvents);
        return separator;
    }
}

namespace Ui
{

class ThreadsContainer_p
{
public:
    ThreadsContainer_p(QObject* _q) : q(_q) {}

    void loadIfNeccessary()
    {
        if (!loader_->isLoading() && scrollArea_->isVisible() && (scrollArea_->widget()->height() - scrollArea_->widgetPosition() - scrollArea_->height()) < preloadDistance() * scrollArea_->height())
            loader_->loadNext();
    }

    void loadIfNeccessaryAsync()
    {
        QTimer::singleShot(0, q, [this](){ loadIfNeccessary(); });
    }

    void clearItems()
    {
        QLayoutItem* item = nullptr;
        while (item = contentLayout_->takeAt(0))
        {
            item->widget()->disconnect();
            item->widget()->deleteLater();
            delete item;
        }

        items_.clear();
        index_.clear();
        anchorWatcher_->clear();
        scrollArea_->setAnchor(nullptr);
    }

    int viewTop() const
    {
        return -scrollArea_->widget()->geometry().top();
    }

    int viewBottom() const
    {
        return viewTop() + scrollArea_->height();
    }

    void updateItemsVisibleRects(int _width)
    {
        const auto max = MessageStyle::getHistoryWidgetMaxWidth();
        const auto w = std::min(_width, max);
        for (auto item : items_)
        {
            item->updateVisibleRects();
            item->setFixedWidth(w);
        }
    }

    void showSpinner()
    {
        spinner_->show();
        spinner_->startAnimation();
    }

    void hideSpinner()
    {
        spinner_->hide();
        spinner_->stopAnimation();
        QTimer::singleShot(getLoaderDelay(), [this]()
        {
            scrollArea_->setScrollLocked(false);
        });
    }

    void updateSpinnerPosition(const QRect& _rect)
    {
        auto spinnerRect = spinner_->rect();
        spinnerRect.moveCenter(_rect.center());
        spinner_->setGeometry(spinnerRect);
    }

    ThreadsFeedLoader* loader_ = nullptr;
    LoaderSpinner* spinner_ = nullptr;
    QTimer* loaderDelayTimer_ = nullptr;
    QVBoxLayout* layout_ = nullptr;
    QVBoxLayout* placeholderLayout_ = nullptr;
    QVBoxLayout* contentLayout_ = nullptr;
    QHBoxLayout* contentContainerLayoutH_ = nullptr;
    QWidget* content_ = nullptr;
    QWidget* placeholder_ = nullptr;
    QWidget* placeholderContainer_ = nullptr;
    StableScrollArea* scrollArea_ = nullptr;
    std::vector<ThreadFeedItem*> items_;
    std::unordered_map<QString, ThreadFeedItem*> index_;
    AnchorWatcher* anchorWatcher_ = nullptr;
    QObject* q;
};

ThreadsContainer::ThreadsContainer(QWidget* _parent)
    : FeedDataContainer(_parent),
      d(std::make_unique<ThreadsContainer_p>(this))
{
    d->layout_ = Utils::emptyVLayout(this);
    d->scrollArea_ = new StableScrollArea(this);
    d->scrollArea_->installEventFilter(this);
    d->layout_->addWidget(d->scrollArea_);

    d->placeholderContainer_ = new QWidget(this);
    d->placeholderContainer_->setVisible(false);
    Testing::setAccessibleName(this, qsl("AS ThreadsContainer placeholder"));

    auto placeholderHLayout = Utils::emptyHLayout();
    placeholderHLayout->addStretch();
    placeholderHLayout->addWidget(d->placeholderContainer_);
    placeholderHLayout->addStretch();
    d->placeholderLayout_ = Utils::emptyVLayout(d->placeholderContainer_);
    d->layout_->addLayout(placeholderHLayout);

    auto contentContainer = new QWidget(d->scrollArea_);
    d->contentContainerLayoutH_ = Utils::emptyHLayout(contentContainer);
    d->content_ = new QWidget(contentContainer);
    d->content_->installEventFilter(this);
    d->content_->setMaximumWidth(MessageStyle::getHistoryWidgetMaxWidth());
    d->contentLayout_ = Utils::emptyVLayout(d->content_);
    d->contentContainerLayoutH_->addWidget(d->content_);
    d->contentContainerLayoutH_->setContentsMargins(0, threadsTopMargin(), 0, 0);
    d->contentContainerLayoutH_->setAlignment(Qt::AlignTop|Qt::AlignHCenter);

    d->scrollArea_->setWidget(contentContainer);
    d->anchorWatcher_ = new AnchorWatcher(d->scrollArea_);
    connect(d->scrollArea_, &StableScrollArea::scrolled, this, &ThreadsContainer::onScrolled);
    connect(&Utils::InterConnector::instance(), &Utils::InterConnector::scrollThreadFeedToMsg, this, &ThreadsContainer::onScrollTo);
    connect(&Utils::InterConnector::instance(), &Utils::InterConnector::setFocusOnInput, this, [this]() { if (d->anchorWatcher_) d->anchorWatcher_->setFocusOnInput(); });
    connect(Logic::getContactListModel(), &Logic::ContactListModel::yourRoleChanged, this, [this](const QString& _aimId)
    {
        for (auto item : d->items_)
        {
            if (_aimId == item->parentChatId())
                item->updateInputVisibilityAccordingToYourRoleInThread();
        }
    });

    d->spinner_ = new LoaderSpinner(this, getSpinnerSize(), LoaderSpinner::Option::Rounded | LoaderSpinner::Option::Shadowed);
    d->spinner_->hide();

    d->loaderDelayTimer_ = new QTimer(this);
    d->loaderDelayTimer_->setSingleShot(true);
    d->loaderDelayTimer_->setInterval(getLoaderDelay());
    connect(d->loaderDelayTimer_, &QTimer::timeout, this, [this]()
    {
        if (d->items_.empty() && !d->loader_->isExhausted())
            d->showSpinner();
    });
}

ThreadsContainer::~ThreadsContainer() = default;

void ThreadsContainer::setLoader(FeedDataLoader* _loader)
{
    if (auto loader = qobject_cast<ThreadsFeedLoader*>(_loader))
    {
        d->loader_ = loader;
        connect(loader, &ThreadsFeedLoader::pageLoaded, this, &ThreadsContainer::onPageLoaded);
        connect(loader, &ThreadsFeedLoader::resetPage, this, &ThreadsContainer::onResetPage);
    }
}

void ThreadsContainer::setPlaceholder(QWidget* _placeholder)
{
    if (auto prev = std::exchange(d->placeholder_, _placeholder))
        delete prev;

    d->placeholderLayout_->addWidget(d->placeholder_, Qt::AlignVCenter);
}

void ThreadsContainer::onPageOpened()
{
    d->loader_->loadNext();
    d->loaderDelayTimer_->start();
    d->scrollArea_->setScrollLocked(true);
}

void ThreadsContainer::onPageLeft()
{
    QTimer::singleShot(0, this, [this]()
    {
        canReopenSidebar_ = true;
        d->clearItems();
        d->loaderDelayTimer_->stop();
        d->hideSpinner();
        d->loader_->reset();
        d->placeholderContainer_->setVisible(false);
    });
}

void ThreadsContainer::scrollToInput(const QString& _id)
{
    if (auto [idx, item] = d->anchorWatcher_->getItem(_id); item)
    {
        d->scrollArea_->scrollToWidget(item->inputWidget());
        d->anchorWatcher_->setCurrent(idx);
        d->anchorWatcher_->setFocusOnInput();
    }
}

void ThreadsContainer::scrollToTop()
{
    d->scrollArea_->ensureVisible(0);
    d->anchorWatcher_->setCurrent(0);
    d->anchorWatcher_->setFocusOnInput();
}

bool ThreadsContainer::eventFilter(QObject* _o, QEvent* _e)
{
    if (_e->type() == QEvent::Resize && (_o == d->scrollArea_ || _o == d->content_))
    {
       updateVisibleRects();
       d->loadIfNeccessaryAsync();
       d->anchorWatcher_->setViewBounds(d->viewTop(), d->viewBottom());
    }

    return false;
}

void ThreadsContainer::resizeEvent(QResizeEvent* _event)
{
    d->updateItemsVisibleRects(_event->size().width());
    FeedDataContainer::resizeEvent(_event);
}

void ThreadsContainer::showEvent(QShowEvent* _event)
{
    d->updateItemsVisibleRects(width());
    FeedDataContainer::showEvent(_event);
}

void ThreadsContainer::onPageLoaded(const std::vector<Data::ThreadFeedItemData>& _items)
{
    const auto wereSomeItemsLoadedBefore = !d->items_.empty();
    if (wereSomeItemsLoadedBefore)
        d->contentLayout_->addWidget(createThreadItemsSeparator(this));

    for (auto& item : _items)
    {
        auto threadItem = new ThreadFeedItem(item, d->content_, d->scrollArea_->widget());
        threadItem->setFixedWidth(std::min(width(), MessageStyle::getHistoryWidgetMaxWidth()));
        d->contentLayout_->addWidget(threadItem);
        if (&item != &_items.back())
            d->contentLayout_->addWidget(createThreadItemsSeparator(this));
        threadItem->show();
        auto index = d->items_.size();
        d->items_.push_back(threadItem);
        d->index_[item.parentMessage_->threadData_.id_] = threadItem;
        d->anchorWatcher_->addThreadItem(threadItem);
        connect(threadItem, &ThreadFeedItem::focusedOnInput, this, [this, threadItem, index]()
        {
            d->scrollArea_->setAnchor(threadItem->inputWidget());
            d->anchorWatcher_->setCurrent(index);
        });

        connect(&Utils::InterConnector::instance(), &Utils::InterConnector::selectionStateChanged, this, [this]()
        {
            for (auto& i : d->items_)
                i->clearSelection();
        });

        connect(threadItem, &ThreadFeedItem::sizeUpdated, d->scrollArea_, &StableScrollArea::updateSize);
        connect(threadItem, &ThreadFeedItem::inputHeightChanged, d->scrollArea_, &StableScrollArea::updateSize);
        connect(threadItem, &ThreadFeedItem::quote, this, &ThreadsContainer::onItemActivated);
        connect(threadItem, &ThreadFeedItem::editing, this, &ThreadsContainer::onItemActivated);
        connect(threadItem, &ThreadFeedItem::activated, this, &ThreadsContainer::onItemActivated);
    }

    if (!d->items_.empty())
    {
        d->anchorWatcher_->setCurrent(0);
        d->anchorWatcher_->setFocusOnInput();
    }

    const auto showPlaceholder = d->items_.empty() && d->loader_->isExhausted();
    d->placeholderContainer_->setVisible(showPlaceholder);
    d->scrollArea_->setVisible(!showPlaceholder);

    if (!d->items_.empty() || showPlaceholder)
    {
        d->loaderDelayTimer_->stop();
        d->hideSpinner();
    }

    updateVisibleRects();
    d->loadIfNeccessaryAsync();
    if (canReopenSidebar_ && !wereSomeItemsLoadedBefore && !d->items_.empty())
    {
        tryUpdateSidebar();
        canReopenSidebar_ = false;
    }
    else if (showPlaceholder)
    {
        Q_EMIT Utils::InterConnector::instance().closeAnySemitransparentWindow({});
    }
}

void ThreadsContainer::onScrolled(int _val)
{
    updateVisibleRects();
    d->loadIfNeccessary();
    d->anchorWatcher_->setViewBounds(d->viewTop(), d->viewBottom(), AnchorWatcher::UpdateFocus::Yes);
}

void ThreadsContainer::onScrollTo(const QString& _threadId, int64_t _msgId)
{
    if (const auto it = d->index_.find(_threadId); it != d->index_.end())
    {
        if (auto item = it->second)
        {
            Q_EMIT Utils::InterConnector::instance().openThread(_threadId, item->parentMsgId(), item->parentChatId());
            Q_EMIT Utils::InterConnector::instance().scrollThreadToMsg(_threadId, _msgId);
        }
    }
    else
    {
        if (const auto& selected = Logic::getContactListModel()->selectedContact(); selected != _threadId)
            Q_EMIT Utils::InterConnector::instance().addPageToDialogHistory(selected);
        Utils::InterConnector::instance().openDialog(_threadId, _msgId);
    }
}

void ThreadsContainer::onResetPage()
{
    canReopenSidebar_ = true;
    d->clearItems();
    d->loaderDelayTimer_->stop();
    d->hideSpinner();
}

void ThreadsContainer::onItemActivated()
{
    if (const auto threadItem = qobject_cast<ThreadFeedItem*>(sender()))
        scrollToInput(threadItem->threadId());
}

void ThreadsContainer::updateVisibleRects()
{
    d->updateItemsVisibleRects(width());
    d->updateSpinnerPosition(rect());
}

void ThreadsContainer::tryUpdateSidebar()
{
    if (Utils::InterConnector::instance().getMessengerPage()->isSidebarVisible())
    {
        if (const auto top = d->items_.front(); top && !top->threadId().isEmpty())
            Q_EMIT Utils::InterConnector::instance().openThread(top->threadId(), top->parentMsgId(), top->threadId());
        else
            Q_EMIT Utils::InterConnector::instance().closeAnySemitransparentWindow({});
    }
    d->anchorWatcher_->setFocusOnInput();
}

class AnchorWatcher_p
{
public:
    std::vector<ThreadFeedItem*> items_;
    size_t current_ = 0;
    StableScrollArea* scrollArea_ = nullptr;
};

AnchorWatcher::AnchorWatcher(StableScrollArea* _scrollArea)
    : QObject(_scrollArea)
    , d(std::make_unique<AnchorWatcher_p>())
{
    d->scrollArea_ = _scrollArea;
}

AnchorWatcher::~AnchorWatcher() = default;

void AnchorWatcher::addThreadItem(ThreadFeedItem* _item)
{
    d->items_.push_back(_item);
}

void AnchorWatcher::setViewBounds(int _top, int _bottom, UpdateFocus _updateFocus)
{
    if (d->items_.empty() || _top == 0 && d->scrollArea_->anchor() == nullptr)
        return;

    auto item = d->items_[d->current_];
    auto inputGeometry = item->inputGeometry().translated(0, item->geometry().top() + threadsTopMargin());

    if (inputGeometry.bottom() <= _top)
    {
        if (d->current_ < d->items_.size() - 1)
        {
            auto next = d->current_ + 1;
            auto item = d->items_[next];
            auto inputGeometry = item->inputGeometry().translated(0, item->geometry().top() + threadsTopMargin());
            if (inputGeometry.top() < _bottom)
                d->current_ = next;
        }
    }
    else if (inputGeometry.top() >= _bottom)
    {
        if (d->current_ > 0)
        {
            auto next = d->current_ - 1;
            auto item = d->items_[next];
            auto inputGeometry = item->inputGeometry().translated(0, item->geometry().top() + threadsTopMargin());
            if (inputGeometry.bottom() > _top)
                d->current_ = next;
        }
    }

    if (_updateFocus == UpdateFocus::Yes)
        setFocusOnInput();

    d->scrollArea_->setAnchor(d->items_[d->current_]->inputWidget());

    for (auto i = 0u; i < d->items_.size(); ++i)
    {
        auto item = d->items_[i];
        auto inputGeometry = item->inputGeometry().translated(0, item->geometry().top() + threadsTopMargin());
        item->setInputVisible(inputGeometry.top() >= _top && inputGeometry.bottom() <= _bottom);
    }
}

void AnchorWatcher::setCurrent(int _index)
{
    d->current_ = _index;
}

void AnchorWatcher::clear()
{
    d->items_.clear();
    d->current_ = 0;
}

void AnchorWatcher::setFocusOnInput()
{
    if (!d->items_.empty() && d->current_ >= 0)
        d->items_[d->current_]->inputWidget()->setFocusOnInput();
}

std::pair<int, ThreadFeedItem*> AnchorWatcher::getItem(const QString& _id) const
{
    if (!d->items_.empty() && d->current_ >= 0)
    {
        auto it = std::find_if(d->items_.begin(), d->items_.end(), [_id](auto item) { return item->threadId() == _id; });
        if (it != d->items_.end())
            return { std::distance(d->items_.begin(), it), *it };
    }
    return { -1, nullptr };
}

}
