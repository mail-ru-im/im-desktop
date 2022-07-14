#include "stdafx.h"

#include "utils/utils.h"
#include "utils/InterConnector.h"
#include "styles/ThemeParameters.h"
#include "FeedPage.h"
#include "../top_widget/DialogHeaderPanel.h"
#include "../top_widget/TopWidget.h"
#include "../../contact_list/ServiceContacts.h"
#include "MessagesContainer.h"
#include "ThreadsContainer.h"
#include "ThreadsFeedLoader.h"
#include "ThreadsFeedPlaceholder.h"
#include "ThreadFeedItem.h"
#include "../../input_widget/InputWidget.h"

namespace Ui
{

class FeedPage_p
{
public:
    FeedPage_p(FeedPage* _q) : q(_q) {}
    void initHeader()
    {
        header_ = new DialogHeaderPanel(pageId_, q);
        topWidget_ = new TopWidget(q);
        topWidget_->insertWidget(TopWidget::Main, header_);

        QObject::connect(header_, &DialogHeaderPanel::switchToPrevDialog, q, &FeedPage::switchToPrevDialog);
    }

    FeedDataContainer* container_ = nullptr;
    FeedDataLoader* loader_ = nullptr;
    DialogHeaderPanel* header_ = nullptr;
    TopWidget* topWidget_ = nullptr;
    Styling::ThemeChecker themeChecker_;
    QString pageId_;
    bool opened_ = false;
    FeedPage* q;
};

FeedPage::FeedPage(const QString& _id, QWidget* _parent)
    : PageBase(_parent),
      d(std::make_unique<FeedPage_p>(this))
{
    d->pageId_ = _id;
    d->container_ = createContainer();
    d->loader_ = createDataLoader();
    d->container_->setLoader(d->loader_);

    d->initHeader();

    auto layout = Utils::emptyVLayout(this);
    layout->addWidget(d->container_);

    Utils::InterConnector::instance().addPage(this);
}

FeedPage::~FeedPage()
{
    Utils::InterConnector::instance().removePage(this);
}

const QString& FeedPage::aimId() const
{
    return d->pageId_;
}

QWidget* FeedPage::getTopWidget() const
{
    return d->topWidget_;
}

void FeedPage::setUnreadBadgeText(const QString& _text)
{
    d->header_->setUnreadBadgeText(_text);
}

void FeedPage::setPrevChatButtonVisible(bool _visible)
{
    d->header_->setPrevChatButtonVisible(_visible);
}

void FeedPage::setOverlayTopWidgetVisible(bool _visible)
{
    d->header_->setOverlayTopWidgetVisible(_visible);
}

void FeedPage::pageOpen()
{
    if (!std::exchange(d->opened_, true))
    {
        d->container_->onPageOpened();
        updateWidgetsTheme();
    }
}

void FeedPage::pageLeave()
{
    if (std::exchange(d->opened_, false))
        d->container_->onPageLeft();
}

void FeedPage::scrollToInput(const QString& _id)
{
    d->container_->scrollToInput(_id);
}

void FeedPage::scrollToTop()
{
    d->container_->scrollToTop();
}

void FeedPage::updateWidgetsTheme()
{
    if (d->themeChecker_.checkAndUpdateHash())
        d->topWidget_->updateStyle();
}

void FeedPage::resizeEvent(QResizeEvent* _event)
{
    d->container_->setFixedWidth(_event->size().width());
    PageBase::resizeEvent(_event);
}

void FeedPage::paintEvent(QPaintEvent* _event)
{
    QPainter p(this);
    p.fillRect(rect(), Styling::getParameters().getColor(Styling::StyleVariable::CHAT_THREAD_ENVIRONMENT));
}

FeedDataLoader* FeedPage::createDataLoader()
{
    if (d->pageId_ == ServiceContacts::contactId(ServiceContacts::ContactType::ThreadsFeed))
        return new ThreadsFeedLoader(this);

    return nullptr;
}

FeedDataContainer* FeedPage::createContainer()
{
    if (d->pageId_ == ServiceContacts::contactId(ServiceContacts::ContactType::ThreadsFeed))
    {
        auto container =  new ThreadsContainer(this);
        container->setPlaceholder(new ThreadsFeedPlaceholder(container));
        return container;
    }

    return nullptr;
}

}
