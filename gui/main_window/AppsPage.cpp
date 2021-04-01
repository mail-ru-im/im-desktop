#include "stdafx.h"
#include "AppsPage.h"
#include "MainPage.h"
#include "MainWindow.h"
#include "TabBar.h"

#include "../my_info.h"
#include "../utils/features.h"
#include "../controls/ContactAvatarWidget.h"
#include "../controls/SemitransparentWindowAnimated.h"
#include "../styles/ThemeParameters.h"

namespace
{
    QColor appsNavigationBarBackgroundColor()
    {
        return Styling::getParameters().getColor(Styling::StyleVariable::BASE_GLOBALWHITE);
    }
    QColor appsNavigationBarBorderColor()
    {
        return Styling::getParameters().getColor(Styling::StyleVariable::BASE_BRIGHT);
    }
}

Ui::AppsNavigationBar::AppsNavigationBar(QWidget* _parent)
    : QWidget(_parent)
    , appsBar_(new AppsBar(this))
{
    Testing::setAccessibleName(appsBar_, qsl("AS AppsNavigationBar AppsBar"));
    auto rootLayout = Utils::emptyVLayout(this);
    rootLayout->setAlignment(Qt::AlignHCenter);

    if (Features::isStatusInAppsNavigationBar())
    {
        auto statusWidget_ = new ContactAvatarWidget(this, QString(), QString(), Utils::scale_value(24), true);
        statusWidget_->SetMode(ContactAvatarWidget::Mode::ChangeStatus);
        statusWidget_->setStatusTooltipEnabled(true);
        statusWidget_->setFixedSize(Utils::scale_value(32), Utils::scale_value(32));
        Testing::setAccessibleName(statusWidget_, qsl("AS AppsNavigationBar statusButton"));
        rootLayout->addWidget(statusWidget_);

        connect(MyInfo(), &my_info::received, this, [statusWidget_]()
        {
            statusWidget_->UpdateParams(MyInfo()->aimId(), MyInfo()->friendly());
        });
    }

    appsBar_->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Preferred);
    rootLayout->addWidget(appsBar_);
    rootLayout->addStretch();

    connect(appsBar_, &AppsBar::clicked, this, &AppsNavigationBar::selectTab);

}

Ui::AppBarItem* Ui::AppsNavigationBar::addTab(const QString& _name, const QString& _icon, const QString& _iconActive)
{
    auto item = new AppBarItem(_icon, _iconActive, this);
    item->setName(_name);
    appsBar_->addItem(item);
    if (appsBar_->count() == 1)
        appsBar_->setCurrentIndex(0);
    return item;
}

int Ui::AppsNavigationBar::defaultWidth()
{
    return Utils::scale_value(68);
}

void Ui::AppsNavigationBar::selectTab(int _index)
{
    auto index = appsBar_->getCurrentIndex();
    if (index != _index)
    {
        appsBar_->setCurrentIndex(_index);
        Q_EMIT tabSelected(_index, QPrivateSignal());
    }
}

void Ui::AppsNavigationBar::paintEvent(QPaintEvent* _event)
{
    QPainter p(this);
    Utils::drawBackgroundWithBorders(p, rect(), appsNavigationBarBackgroundColor(), appsNavigationBarBorderColor(), Qt::AlignRight);
}

Ui::AppsPage::AppsPage(QWidget* _parent)
    : QWidget(_parent)
    , pages_(new QStackedWidget(this))
    , messengerPage_(nullptr)
    , navigationBar_(nullptr)
    , semiWindow_(new SemitransparentWindowAnimated(this))
{
    auto rootLayout = Utils::emptyHLayout(this);
    if (Features::isAppsNavigationBarVisible())
    {
        navigationBar_ = new AppsNavigationBar(this);
        navigationBar_->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Expanding);
        Testing::setAccessibleName(navigationBar_, qsl("AS AppsPage AppsNavigationBar"));
        rootLayout->addWidget(navigationBar_);

        connect(navigationBar_, &AppsNavigationBar::tabSelected, this, &AppsPage::onTabSelected);
        messengerPage_ = MainPage::instance(this);
        Testing::setAccessibleName(messengerPage_, qsl("AS MessengerPage"));

        auto tab1 = new QWidget(this);
        tab1->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
        tab1->setStyleSheet(qsl("background-color: #4444dd"));
        Testing::setAccessibleName(tab1, qsl("AS StructurePage"));
        auto tab2 = new QWidget(this);
        tab2->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
        tab2->setStyleSheet(qsl("background-color: #44dd44"));
        Testing::setAccessibleName(tab2, qsl("AS TasksPage"));

        addTab(messengerPage_, QT_TRANSLATE_NOOP("tab", "Chats"), qsl(":/tab/chats"), qsl(":/tab/chats_active"));
        addTab(tab1, QT_TRANSLATE_NOOP("tab", "Structure"), qsl(":/tab/structure"), qsl(":/tab/structure_active"));
        addTab(tab2, QT_TRANSLATE_NOOP("tab", "Tasks"), qsl(":/tab/tasks"), qsl(":/tab/tasks_active"));
    }
    else
    {
        prepareForShow();
    }

    rootLayout->addWidget(pages_);
    Testing::setAccessibleName(pages_, qsl("AS AppsPage AppsPages"));

    semiWindow_->setCloseWindowInfo({ Utils::CloseWindowInfo::Initiator::Unknown, Utils::CloseWindowInfo::Reason::Keep_Sidebar });
    hideSemiWindow();
}

Ui::AppsPage::~AppsPage()
{
    MainPage::reset();
}

Ui::MainPage* Ui::AppsPage::messengerPage() const
{
    return messengerPage_;
}

bool Ui::AppsPage::isMessengerPage() const
{
    return MainPage::hasInstance() && (MainPage::instance() == pages_->currentWidget());
}

bool Ui::AppsPage::isContactDialog() const
{
    return isMessengerPage() && MainPage::instance()->isContactDialog();
}

void Ui::AppsPage::resetMessengerPage()
{
    pages_->removeWidget(messengerPage_);
    messengerPage_ = nullptr;
    MainPage::reset();
}

void Ui::AppsPage::prepareForShow()
{
    if (!MainPage::hasInstance())
    {
        messengerPage_ = MainPage::instance(this);
        Testing::setAccessibleName(messengerPage_, qsl("AS MessengerPage"));
        pages_->insertWidget(0, messengerPage_);
        pages_->setCurrentIndex(0);
        selectTab(0);
    }
}

void Ui::AppsPage::showMessengerPage()
{
    selectTab(pages_->indexOf(messengerPage_));
}

void Ui::AppsPage::addTab(QWidget* _widget, const QString& _name, const QString& _icon, const QString& _iconActive)
{
    if (!navigationBar_)
    {
        im_assert(!"Can't add tab without navigation bar");
        return;
    }
    int index = pages_->indexOf(_widget);
    if (index == -1)
    {
        index = pages_->addWidget(_widget);
        auto tabItem = navigationBar_->addTab(_name, _icon, _iconActive);
        Testing::setAccessibleName(tabItem, u"AS AppTab " % _widget->accessibleName());
        if (index == 0)
            pages_->setCurrentIndex(index);
    }
    else
    {
        qWarning("TabWidget: widget is already in tabWidget");
    }
}

void Ui::AppsPage::showSemiWindow()
{
    semiWindow_->setFixedSize(size());
    semiWindow_->raise();
    semiWindow_->showAnimated();
}

void Ui::AppsPage::hideSemiWindow()
{
    semiWindow_->forceHide();
}

bool Ui::AppsPage::isSemiWindowVisible() const
{
    return semiWindow_->isSemiWindowVisible();
}

void Ui::AppsPage::resizeEvent(QResizeEvent* _event)
{
    semiWindow_->setFixedSize(size());
}

void Ui::AppsPage::selectTab(int _tabIndex)
{
    if (navigationBar_)
        navigationBar_->selectTab(_tabIndex);
    else
        pages_->setCurrentIndex(_tabIndex);
}

void Ui::AppsPage::onTabSelected(int _tabIndex)
{
    pages_->setCurrentIndex(_tabIndex);
    Utils::InterConnector::instance().getMainWindow()->updateMainMenu();
    Q_EMIT Utils::InterConnector::instance().currentPageChanged();
}
