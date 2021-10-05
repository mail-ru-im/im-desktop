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
        d->container_->onPageOpened();
}

void FeedPage::pageLeave()
{
    if (std::exchange(d->opened_, false))
        d->container_->onPageLeft();
}

void FeedPage::resizeEvent(QResizeEvent* _event)
{
    d->container_->setFixedWidth(_event->size().width());
    PageBase::resizeEvent(_event);
}

void FeedPage::paintEvent(QPaintEvent* _event)
{
    //! This color is a bit different from the color from figma, but decided it's okay for now https://jira.mail.ru/browse/IMDESKTOP-17303?focusedCommentId=12589705&page=com.atlassian.jira.plugin.system.issuetabpanels%3Acomment-tabpanel#comment-12589705
    const auto backgroundColor = Styling::getParameters().getColorAlphaBlendedHex(Styling::StyleVariable::CHAT_ENVIRONMENT, Styling::StyleVariable::GHOST_LIGHT_INVERSE);
    QPainter p(this);
    p.fillRect(rect(), backgroundColor);
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
