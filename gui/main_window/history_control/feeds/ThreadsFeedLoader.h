#pragma once
#include "FeedPage.h"
#include "types/thread_feed_data.h"

namespace Ui
{

class ThreadsFeedLoader_p;

class ThreadsFeedLoader : public FeedDataLoader
{
    Q_OBJECT
Q_SIGNALS:
    void pageLoaded(const std::vector<Data::ThreadFeedItemData>& _items);
    void resetPage();

public:
    ThreadsFeedLoader(QObject* _parent);
    ~ThreadsFeedLoader();
    void loadNext() override;
    bool isLoading() const override;
    bool isExhausted() const override;
    void reset() override;

private Q_SLOTS:
    void onThreadFeedGetResult(const std::vector<Data::ThreadFeedItemData>& _items, const QString& _cursor, bool _resetPage);

private:
    std::unique_ptr<ThreadsFeedLoader_p> d;
};

}
