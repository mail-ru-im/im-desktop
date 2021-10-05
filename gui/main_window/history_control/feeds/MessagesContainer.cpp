#include "stdafx.h"
#include "MessagesContainer.h"
#include "utils/utils.h"
#include "../history/MessageBuilder.h"
#include "../HistoryControlPageItem.h"
#include "controls/TransparentScrollBar.h"
#include "MessagesLayout.h"

namespace
{
    int preloadDistance() noexcept
    {
        return Utils::scale_value(200);
    }

    int marginV() noexcept
    {
        return Utils::scale_value(20);
    }
}

namespace Ui
{

class MessagesContainer_p
{
public:
    MessagesContainer_p(QObject* _q) : q(_q) {}
    void loadIfNeccessary()
    {
        auto scrollBar = scrollArea_->verticalScrollBar();
        if (!loader_->isLoading() && scrollBar->maximum() - scrollBar->value() < preloadDistance())
            loader_->loadNext();
    }
    void loadIfNeccessaryAsync()
    {
        QTimer::singleShot(0, q, [this](){ loadIfNeccessary(); });
    }

    MessagesFeedLoader* loader_ = nullptr;
    QVBoxLayout* layout_ = nullptr;
    MessagesLayout* messagesLayout_ = nullptr;
    QVBoxLayout* placeholderLayout_ = nullptr;
    QWidget* content_ = nullptr;
    QWidget* placeholder_ = nullptr;
    QWidget* placeholderContainer_ = nullptr;
    ScrollAreaWithTrScrollBar* scrollArea_ = nullptr;
    std::vector<std::unique_ptr<Ui::HistoryControlPageItem>> items_;
    QObject *q;
};

MessagesContainer::MessagesContainer(QWidget* _parent)
    : FeedDataContainer(_parent),
      d(std::make_unique<MessagesContainer_p>(this))
{
    d->layout_ = Utils::emptyVLayout(this);
    d->scrollArea_ = CreateScrollAreaAndSetTrScrollBarV(this);
    d->scrollArea_->installEventFilter(this);
    d->layout_->addWidget(d->scrollArea_);

    d->placeholderContainer_ = new QWidget(this);
    d->placeholderContainer_->setVisible(false);
    auto placeholderHLayout = Utils::emptyHLayout();
    placeholderHLayout->addStretch();
    placeholderHLayout->addWidget(d->placeholderContainer_);
    placeholderHLayout->addStretch();
    d->placeholderLayout_ = Utils::emptyVLayout(d->placeholderContainer_);
    d->layout_->addLayout(placeholderHLayout);

    d->content_ = new QWidget(this);
    d->content_->installEventFilter(this);
    d->scrollArea_->setWidget(d->content_);
    d->scrollArea_->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    d->scrollArea_->setFocusPolicy(Qt::NoFocus);
    d->scrollArea_->setWidgetResizable(true);
    d->scrollArea_->setStyleSheet(qsl("background-color: transparent; border: none"));
    connect(d->scrollArea_->verticalScrollBar(), &QScrollBar::valueChanged, this, &MessagesContainer::onScrolled);

    d->messagesLayout_ = new MessagesLayout(d->content_);
    d->messagesLayout_->setSpacing(Utils::scale_value(8));
    d->messagesLayout_->setContentsMargins(0, marginV(), 0, marginV());
}

MessagesContainer::~MessagesContainer() = default;

void MessagesContainer::setLoader(FeedDataLoader* _loader)
{
    if (auto loader = qobject_cast<MessagesFeedLoader*>(_loader))
    {
        d->loader_ = loader;
        connect(loader, &MessagesFeedLoader::pageLoaded, this, &MessagesContainer::onPageLoaded);
    }
}

void MessagesContainer::setPlaceholder(QWidget* _placeholder)
{
    if (d->placeholder_)
        delete d->placeholder_;

    d->placeholder_ = _placeholder;
    d->placeholderLayout_->addWidget(d->placeholder_, Qt::AlignVCenter);
}

void MessagesContainer::onPageOpened()
{
    d->messagesLayout_->clear();
    d->items_.clear();
    d->loader_->reset();
    d->loader_->loadNext();
    d->scrollArea_->verticalScrollBar()->setValue(0);
}

bool MessagesContainer::eventFilter(QObject* _o, QEvent* _e)
{
    if (_e->type() == QEvent::Resize && (_o == d->scrollArea_))
       d->loadIfNeccessaryAsync();

    return false;
}

void MessagesContainer::onPageLoaded(const Data::MessageBuddies& _messages)
{
    for (auto& msg : _messages)
    {
        if (auto item = hist::MessageBuilder::makePageItem(*msg, width(), d->content_))
        {
            item->setHasAvatar(true);
            item->setHasSenderName(true);
            item->setChainedToNext(false);
            item->setChainedToPrev(false);
            item->onActivityChanged(true);
            d->messagesLayout_->addWidget(item.get());
            item->show();
            d->items_.push_back(std::move(item));
        }
    }

    d->placeholderContainer_->setVisible(d->items_.empty());
    d->scrollArea_->setVisible(!d->items_.empty());

    d->loadIfNeccessaryAsync();
}

void MessagesContainer::onScrolled(int _val)
{
    d->loadIfNeccessary();
}

}
