#pragma once
#include "FeedPage.h"

namespace Ui
{
class MessagesContainer_p;
class MessagesContainer : public FeedDataContainer
{
    Q_OBJECT
public:
    MessagesContainer(QWidget* _parent);
    ~MessagesContainer();
    void setLoader(FeedDataLoader* _loader) override;
    void setPlaceholder(QWidget* _placeholder) override;
    void onPageOpened() override;

protected:
    bool eventFilter(QObject* _o, QEvent* _e) override;

private Q_SLOTS:
    void onPageLoaded(const Data::MessageBuddies& _messages);
    void onScrolled(int _val);

private:
    std::unique_ptr<MessagesContainer_p> d;
};
}
