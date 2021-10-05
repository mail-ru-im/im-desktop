#pragma once
#include "FeedPage.h"
#include "types/thread_feed_data.h"

namespace Ui
{
class ThreadsContainer_p;
class ThreadsContainer : public FeedDataContainer
{
    Q_OBJECT
public:
    ThreadsContainer(QWidget* _parent);
    ~ThreadsContainer();
    void setLoader(FeedDataLoader* _loader) override;
    void setPlaceholder(QWidget* _placeholder) override;
    void onPageOpened() override;
    void onPageLeft() override;

protected:
    bool eventFilter(QObject* _o, QEvent* _e) override;
    void resizeEvent(QResizeEvent* _event) override;

private Q_SLOTS:
    void onPageLoaded(const std::vector<Data::ThreadFeedItemData>& _items);
    void onScrolled(int _val);
    void onScrollTo(const QString& _threadId, int64_t _msgId);
    void onResetPage();
    void updateVisibleRects();

private:
    std::unique_ptr<ThreadsContainer_p> d;
};

class ThreadFeedItem;
class StableScrollArea;
class AnchorWatcher_p;
class AnchorWatcher : public QObject
{
    Q_OBJECT
public:
    AnchorWatcher(StableScrollArea* _scrollArea);
    ~AnchorWatcher();
    void addThreadItem(ThreadFeedItem* _item);    
    enum class UpdateFocus { No, Yes };
    void setViewBounds(int _top, int _bottom, UpdateFocus _updateFocus = UpdateFocus::No);
    void setCurrent(int _index);
    void clear();
    void setFocusOnInput();

private:
    std::unique_ptr<AnchorWatcher_p> d;
};

}
