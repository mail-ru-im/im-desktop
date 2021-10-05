#include "stdafx.h"
#include "ThreadsFeedLoader.h"
#include "main_window/containers/FriendlyContainer.h"
#include "main_window/contact_list/ServiceContacts.h"
#include "core_dispatcher.h"
#include "my_info.h"

namespace Ui
{

class ThreadsFeedLoader_p
{
public:
    QString cursor_;
    bool loading_ = false;
    bool exhausted_ = false;
};

ThreadsFeedLoader::ThreadsFeedLoader(QObject* _parent)
    : FeedDataLoader(_parent),
      d(std::make_unique<ThreadsFeedLoader_p>())
{
    connect(GetDispatcher(), &core_dispatcher::threadsFeedGetResult, this, &ThreadsFeedLoader::onThreadFeedGetResult);
}

ThreadsFeedLoader::~ThreadsFeedLoader() = default;

void ThreadsFeedLoader::loadNext()
{
    if (d->exhausted_)
        return;

    d->loading_ = true;
    GetDispatcher()->getThreadsFeed(d->cursor_);
}

bool ThreadsFeedLoader::isLoading() const
{
    return d->loading_;
}

bool ThreadsFeedLoader::isExhausted() const
{
    return d->exhausted_;
}

void ThreadsFeedLoader::reset()
{
    d->cursor_.clear();
    d->exhausted_ = false;
    d->loading_ = false;
}

void ThreadsFeedLoader::onThreadFeedGetResult(const std::vector<Data::ThreadFeedItemData>& _items, const QString& _cursor, bool _resetPage)
{
    if (!d->loading_)
        return;

    d->cursor_ = _cursor;
    d->exhausted_ = _cursor.isEmpty();
    d->loading_ = false;

    if (_resetPage)
        Q_EMIT resetPage();

    Q_EMIT pageLoaded(_items);
}

}
