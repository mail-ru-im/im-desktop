#pragma once

#include "../PageBase.h"

namespace Ui
{

enum class FeedType
{
    Threads,
};

class FeedDataLoader : public QObject
{
public:
    FeedDataLoader(QObject* _parent) : QObject(_parent) {}
    virtual void loadNext() = 0;
    virtual bool isLoading() const { return false; }
    virtual bool isExhausted() const { return true; }
    virtual void reset() {}
};

class MessagesFeedLoader : public FeedDataLoader
{
    Q_OBJECT
Q_SIGNALS:
    void pageLoaded(const Data::MessageBuddies& _messages);
public:
    MessagesFeedLoader(QObject* _parent) : FeedDataLoader(_parent) {}
};

class FeedDataContainer: public QWidget
{
    Q_OBJECT
public:
    FeedDataContainer(QWidget* _parent) : QWidget(_parent) {}
    virtual void setLoader(FeedDataLoader* _loader) {}
    virtual void setPlaceholder(QWidget* _placeholder) {}
    virtual void onPageOpened() {}
    virtual void onPageLeft() {}
};

class FeedPage_p;

class FeedPage : public PageBase
{
    Q_OBJECT
public:
    FeedPage(const QString& _id, QWidget* _parent);
    ~FeedPage();

    const QString& aimId() const override;
    QWidget* getTopWidget() const override;
    void setUnreadBadgeText(const QString& _text) override;
    void setPrevChatButtonVisible(bool _visible) override;
    void setOverlayTopWidgetVisible(bool _visible) override;
    void pageOpen() override;
    void pageLeave() override;

protected:
    void resizeEvent(QResizeEvent* _event) override;
    void paintEvent(QPaintEvent* _event) override;

private:
    FeedDataLoader* createDataLoader();
    FeedDataContainer* createContainer();
    std::unique_ptr<FeedPage_p> d;
};

}
