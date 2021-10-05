#include "stdafx.h"
#include "AppsPage.h"
#include "MainPage.h"
#include "MainWindow.h"
#include "TabBar.h"
#ifdef HAS_WEB_ENGINE
#include "WebAppPage.h"
#endif
#include "gui_settings.h"

#include "../my_info.h"
#include "../utils/features.h"
#include "../utils/keyboard.h"
#include "../utils/InterConnector.h"
#include "../utils/ObjectFactory.h"
#include "../controls/ContactAvatarWidget.h"
#include "../controls/SemitransparentWindowAnimated.h"
#include "../controls/ContextMenu.h"
#include "../styles/ThemeParameters.h"
#include "../core_dispatcher.h"

#include "contact_list/RecentsModel.h"
#include "contact_list/ContactListModel.h"
#include "contact_list/UnknownsModel.h"

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

    constexpr auto getMessengerPageIndex() noexcept { return 0; }

    constexpr auto appTabsOrder = std::array{
        Ui::AppPageType::messenger,
        Ui::AppPageType::tasks,
        Ui::AppPageType::org_structure,
        Ui::AppPageType::calendar,
        Ui::AppPageType::calls,
        Ui::AppPageType::settings,
    };

    auto getByTypePred(Ui::AppPageType _type)
    {
        return [_type](const Ui::SimpleListItem* _item)
        {
            auto i = qobject_cast<const Ui::AppBarItem*>(_item);
            return i && i->getType() == _type;
        };
    }

    bool isWebApp(Ui::AppPageType _type) noexcept
    {
        switch (_type)
        {
        case Ui::AppPageType::tasks:
        case Ui::AppPageType::org_structure:
        case Ui::AppPageType::calendar:
            return true;

        case Ui::AppPageType::messenger:
        case Ui::AppPageType::settings:
        case Ui::AppPageType::calls:
        case Ui::AppPageType::contacts:
            return false;
        }

        im_assert(!"Unknown app page type");
        return false;
    }

    bool isMessengerApp(Ui::AppPageType _type) noexcept
    {
        return !isWebApp(_type);
    }

    std::pair<QUrl, bool> makeWebAppUrl(Ui::AppPageType _type)
    {
#ifdef HAS_WEB_ENGINE
        const auto appUrl = Ui::WebAppPage::webAppUrl(_type);
        const auto id = Ui::WebAppPage::webAppId(_type);
        return Utils::InterConnector::instance().signUrl(id, appUrl);
#else
        return { QUrl(), false };
#endif
    }

    void requestMiniAppAuthParams(const QString& id)
    {
        Ui::gui_coll_helper coll(Ui::GetDispatcher()->create_collection(), true);
        coll.set_value_as_qstring("id", id);
        Ui::GetDispatcher()->post_message_to_core("miniapp/start_session", coll.get());
    }

    void startMiniAppSession(Ui::AppPageType _type)
    {
#ifdef HAS_WEB_ENGINE
        auto id = Ui::WebAppPage::webAppId(_type);
        if (!id.isEmpty() && !Utils::InterConnector::instance().isMiniAppAuthParamsValid(id))
            requestMiniAppAuthParams(id);
#endif
    }

    Ui::TabType appPageTypeToMessengerTabType(Ui::AppPageType _pageType)
    {
        im_assert(isMessengerApp(_pageType));

        switch (_pageType)
        {
        case Ui::AppPageType::messenger:
            return Ui::TabType::Recents;
        case Ui::AppPageType::settings:
            return Ui::TabType::Settings;
        case Ui::AppPageType::calls:
            return Ui::TabType::Calls;
        case Ui::AppPageType::contacts:
            return Ui::TabType::Contacts;
        default:
            im_assert(false);
            return Ui::TabType::Recents;
        }
    }

    using Factory = Utils::ObjectFactory<Ui::AppBarItem, Ui::AppPageType, Ui::AppPageType, const QString&, const QString&, QWidget*>;
    std::unique_ptr<Factory> itemFactory = []()
    {
        auto factory = std::make_unique<Factory>();
        factory->registerClass<Ui::CalendarItem>(Ui::AppPageType::calendar);
        factory->registerDefaultClass<Ui::AppBarItem>();
        return factory;
    }();
}


Ui::AppsNavigationBar::AppsNavigationBar(QWidget* _parent)
    : QWidget(_parent)
    , appsBar_(new AppsBar(this))
    , bottomAppsBar_(new AppsBar(this))
{
    Testing::setAccessibleName(appsBar_, qsl("AS AppsNavigationBar AppsBar"));
    auto rootLayout = Utils::emptyVLayout(this);

    if (Features::isStatusInAppsNavigationBar())
    {
        auto statusWidget = new ContactAvatarWidget(this, QString(), QString(), Utils::scale_value(24), true);
        statusWidget->SetMode(ContactAvatarWidget::Mode::ChangeStatus);
        statusWidget->setStatusTooltipEnabled(true);
        Testing::setAccessibleName(statusWidget, qsl("AS AppsNavigationBar statusButton"));
        auto statusLayout = Utils::emptyHLayout();
        statusLayout->setContentsMargins(Utils::scale_value(6), Utils::scale_value(8), 0, 0);
        statusLayout->addStretch();
        statusLayout->addWidget(statusWidget);
        statusLayout->addStretch();
        rootLayout->addLayout(statusLayout);

        connect(MyInfo(), &my_info::received, this, [statusWidget]()
        {
            statusWidget->UpdateParams(MyInfo()->aimId(), MyInfo()->friendly());
        });
    }

    // See https://jira.mail.ru/browse/IMDESKTOP-17256?focusedCommentId=12423296&page=com.atlassian.jira.plugin.system.issuetabpanels%3Acomment-tabpanel#comment-12423296
    appsBar_->layout()->setSizeConstraint(QLayout::SizeConstraint::SetFixedSize);

    rootLayout->addWidget(appsBar_);
    rootLayout->addStretch();
    rootLayout->addWidget(bottomAppsBar_);

    connect(appsBar_, &AppsBar::clicked, this, [this](int _index) { selectTab(appsBar_, _index); });
    connect(appsBar_, &AppsBar::rightClicked, this, [this](int _index) { onRightClick(appsBar_, _index); });
    connect(bottomAppsBar_, &AppsBar::clicked, this, [this](int _index) { selectTab(bottomAppsBar_, _index); });
    connect(bottomAppsBar_, &AppsBar::rightClicked, this, [this](int _index) { onRightClick(bottomAppsBar_, _index); });
}

Ui::AppBarItem* Ui::AppsNavigationBar::addTab(AppPageType _type, const QString& _name, const QString& _icon, const QString& _iconActive)
{
    auto findTabIndexAccordingToTabsOrder = [this](AppPageType _type)
    {
        auto targetIndex = 0;
        auto precedingTypeIt = std::find(appTabsOrder.cbegin(), appTabsOrder.cend(), _type);
        im_assert(precedingTypeIt != appTabsOrder.cend());
        while (precedingTypeIt != appTabsOrder.cbegin() && targetIndex == 0)
        {
            precedingTypeIt = std::prev(precedingTypeIt);
            for (int i = 0; i < appsBar_->count(); ++i)
            {
                if (auto existingItem = qobject_cast<AppBarItem*>(appsBar_->itemAt(i)))
                {
                    if (existingItem->getType() == *precedingTypeIt)
                    {
                        targetIndex = i + 1;
                        break;
                    }
                }
            }
        }
        return targetIndex;
    };

    return addTab(appsBar_, findTabIndexAccordingToTabsOrder(_type), _type, _name, _icon, _iconActive);
}

Ui::AppBarItem* Ui::AppsNavigationBar::addTab(AppsBar* _bar, int _index, AppPageType _type, const QString& _name, const QString& _icon, const QString& _iconActive)
{
    auto index = _bar->indexOf(getByTypePred(_type));

    if (_bar->isValidIndex(index))
        return qobject_cast<AppBarItem*>(_bar->itemAt(index));

    auto item = itemFactory->create(_type, _type, _icon, _iconActive, this);
    item->setName(_name);
    index = _bar->addItem(item, _index);
    if (_bar->count() == 1)
        _bar->setCurrentIndex(0);
    return item;
}

std::pair<Ui::AppsBar*, int> Ui::AppsNavigationBar::getBarAndIndexByType(AppPageType _type)
{
    for (const auto bar : std::array{ appsBar_, bottomAppsBar_ })
    {
        for (auto index = 0; index < bar->count(); ++index)
        {
            if (_type == getTabType(bar, index))
                return {bar, index};
        }
    }
    return {nullptr, -1};
}

Ui::AppPageType Ui::AppsNavigationBar::getTabType(AppsBar* _bar, int _index) const
{
    if (const auto item = qobject_cast<AppBarItem*>(_bar->itemAt(_index)))
        return item->getType();

    im_assert(!"tab type cannot be identified");
    return AppPageType();
}

Ui::AppPageType Ui::AppsNavigationBar::currentPage() const
{
    auto index = -1;
    AppsBar* bar = nullptr;

    if (index = appsBar_->getCurrentIndex(); index != -1)
        bar = appsBar_;
    else if (index = bottomAppsBar_->getCurrentIndex(); index != -1)
        bar = bottomAppsBar_;

    if (index == -1 || !bar)
    {
        im_assert(!"no app is selected");
        return AppPageType::messenger;
    }

    return getTabType(bar, index);
}

Ui::AppBarItem* Ui::AppsNavigationBar::addBottomTab(AppPageType _type, const QString& _name, const QString& _icon, const QString& _iconActive)
{
    auto bottomTab = addTab(bottomAppsBar_, -1, _type, _name, _icon, _iconActive);
    bottomAppsBar_->clearSelection();
    return bottomTab;
}

void Ui::AppsNavigationBar::removeTab(AppPageType _type)
{
    if (const auto [bar, index] = getBarAndIndexByType(_type); bar && index != -1)
        bar->removeItemAt(index);
    else
        im_assert(!"Cannot remove tab which does not exist");

}

bool Ui::AppsNavigationBar::contains(AppPageType _type) const
{
    auto pred = getByTypePred(_type);
    return appsBar_->contains(pred) || bottomAppsBar_->contains(pred);
}

int Ui::AppsNavigationBar::defaultWidth()
{
    return Utils::scale_value(68);
}

void Ui::AppsNavigationBar::selectTab(AppsBar* _bar, int _index)
{
    const auto oldTab = currentPage();
    const auto newTab = getTabType(_bar, _index);
    Q_EMIT appTabClicked(newTab, oldTab, QPrivateSignal());

    if (newTab != oldTab)
    {
        (_bar == appsBar_ ? bottomAppsBar_ : appsBar_)->clearSelection();
        _bar->setCurrentIndex(_index);
    }
}

void Ui::AppsNavigationBar::selectTab(AppPageType _pageType)
{
    if (const auto [bar, pageIndex] = getBarAndIndexByType(_pageType); bar && pageIndex != -1)
    {
        if (pageIndex != bar->getCurrentIndex())
            selectTab(bar, pageIndex);
    }
    else
    {
        im_assert(!"attempt to select not existing tab");
    }
}

void Ui::AppsNavigationBar::onRightClick(AppsBar* _bar, int _index)
{
    const auto pageType = getTabType(_bar, _index);
    const auto areModifiersPressed = Utils::keyboardModifiersCorrespondToKeyComb(qApp->keyboardModifiers(), Utils::KeyCombination::Ctrl_or_Cmd_AltShift);
    if (!areModifiersPressed)
        return;

    if (pageType == AppPageType::settings)
    {
        Q_EMIT Utils::InterConnector::instance().showDebugSettings();
    }
    else if (isWebApp(pageType))
    {
        auto menu = new ContextMenu(this);
        Testing::setAccessibleName(menu, qsl("AS WebApp menu"));
        menu->addActionWithIcon(qsl(":/controls/reload"), QT_TRANSLATE_NOOP("tooltips", "Refresh"), this, [this, pageType]()
        {
            Q_EMIT reloadWebPage(pageType, QPrivateSignal());
        });
        menu->popup(QCursor::pos());
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
    , messengerItem_(nullptr)
    , semiWindow_(new SemitransparentWindowAnimated(this))
{
    auto rootLayout = Utils::emptyHLayout(this);
    navigationBar_ = new AppsNavigationBar(this);
    navigationBar_->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Expanding);
    Testing::setAccessibleName(navigationBar_, qsl("AS AppsPage AppsNavigationBar"));
    rootLayout->addWidget(navigationBar_);

    connect(navigationBar_, &AppsNavigationBar::appTabClicked, this, &AppsPage::onTabClicked);
    connect(navigationBar_, &AppsNavigationBar::reloadWebPage, this, &AppsPage::reloadWebAppPage);
    messengerPage_ = MainPage::instance(this);
    Testing::setAccessibleName(messengerPage_, qsl("AS MessengerPage"));

    messengerItem_ = addTab(AppPageType::messenger, messengerPage_, QT_TRANSLATE_NOOP("tab", "Chats"), qsl(":/tab/chats"), qsl(":/tab/chats_active"));

    updateNavigationBarVisibility();

    if (get_gui_settings()->get_value<bool>(settings_show_calls_tab, true))
        addCallsTab();

    auto settingsTab = navigationBar_->addBottomTab(AppPageType::settings, QT_TRANSLATE_NOOP("tab", "Settings"), qsl(":/tab/settings"), qsl(":/tab/settings_active"));
    Testing::setAccessibleName(settingsTab, qsl("AS Tab ") % MainPage::settingsTabAccessibleName());

    connect(&Utils::InterConnector::instance(), &Utils::InterConnector::headerBack, this, [this]()
    {
        navigationBar_->selectTab(AppPageType::messenger);
    });
    connect(get_gui_settings(), &Ui::qt_gui_settings::changed, this, &AppsPage::onGuiSettingsChanged);
    connect(&Utils::InterConnector::instance(), &Utils::InterConnector::authParamsChanged, this, &AppsPage::onAuthParamsChanged);
    connect(Ui::GetDispatcher(), &Ui::core_dispatcher::externalUrlConfigUpdated, this, &AppsPage::onConfigChanged);
    connect(&Utils::InterConnector::instance(), &Utils::InterConnector::omicronUpdated, this, &AppsPage::onConfigChanged);
    connect(Ui::GetDispatcher(), &core_dispatcher::im_created, this, &AppsPage::loggedIn);

    loggedIn();

    rootLayout->addWidget(pages_);
    Testing::setAccessibleName(pages_, qsl("AS AppsPage AppsPages"));

    semiWindow_->setCloseWindowInfo({ Utils::CloseWindowInfo::Initiator::Unknown, Utils::CloseWindowInfo::Reason::Keep_Sidebar });
    hideSemiWindow();

#ifdef HAS_WEB_ENGINE
    QWebEngineProfile::defaultProfile()->setPersistentCookiesPolicy(QWebEngineProfile::NoPersistentCookies);
    QWebEngineProfile::defaultProfile()->setHttpCacheType(QWebEngineProfile::MemoryHttpCache);
#endif //HAS_WEB_ENGINE
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

bool Ui::AppsPage::isTasksPage() const
{
    const auto tasksIter = pagesByType_.find(AppPageType::tasks);
    return (tasksIter != pagesByType_.end()) && (tasksIter->second == pages_->currentWidget());
}

void Ui::AppsPage::resetPages()
{
    pages_->removeWidget(messengerPage_);
    pagesByType_.erase(AppPageType::messenger);
    messengerPage_ = nullptr;
    MainPage::reset();

    authDataValid_ = false;

    for (auto type : { AppPageType::org_structure, AppPageType::tasks, AppPageType::calendar })
        removeTab(type, false);
    if (containsTab(AppPageType::calls))
        removeTab(AppPageType::calls, false);
    if (containsTab(AppPageType::settings))
        removeTab(AppPageType::settings, false);
}

void Ui::AppsPage::prepareForShow()
{
    if (!MainPage::hasInstance())
    {
        messengerPage_ = MainPage::instance(this);
        Testing::setAccessibleName(messengerPage_, qsl("AS MessengerPage"));
        pages_->insertWidget(getMessengerPageIndex(), messengerPage_);
        pages_->setCurrentIndex(getMessengerPageIndex());
        pagesByType_[AppPageType::messenger] = messengerPage_;
        selectTab(AppPageType::messenger);
    }
}

void Ui::AppsPage::showMessengerPage()
{
    selectTab(AppPageType::messenger);
}

Ui::AppBarItem* Ui::AppsPage::addTab(AppPageType _type, QWidget* _widget, const QString& _name, const QString& _icon, const QString& _iconActive)
{
    if (!navigationBar_)
    {
        im_assert(!"Can't add tab without navigation bar");
        return nullptr;
    }

    if (int index = pages_->indexOf(_widget); index == -1)
    {
        index = pages_->addWidget(_widget);
        im_assert(_type != AppPageType::messenger || index == getMessengerPageIndex());
        im_assert(pagesByType_.count(_type) == 0);
        pagesByType_[_type] = _widget;

        auto tabItem = navigationBar_->addTab(_type, _name, _icon, _iconActive);
        Testing::setAccessibleName(tabItem, u"AS AppTab " % _widget->accessibleName());
        if (index == 0)
            pages_->setCurrentIndex(index);

        return tabItem;
    }
    else
    {
        im_assert(pagesByType_.count(_type) == 1);
        qWarning("TabWidget: widget is already in tabWidget");
    }

    return nullptr;
}

void Ui::AppsPage::removeTab(AppPageType _type, bool _showDefaultPage)
{
    if (_type == AppPageType::messenger || _type == AppPageType::contacts || _type == AppPageType::calls)
    {
        im_assert(!"Can't remove messenger tab");
        return;
    }

    if (const auto it = pagesByType_.find(_type); it != pagesByType_.end())
    {
        auto page = it->second;
        im_assert(page);
        if (const int index = pages_->indexOf(page); index != -1)
        {
            if (pages_->currentIndex() == index && _showDefaultPage)
                showMessengerPage();
            pages_->removeWidget(page);
            page->deleteLater();
        }
        else
        {
            im_assert(!"pages are supposed to have page of this type");
        }
        pagesByType_.erase(it);
    }

    if (navigationBar_)
        navigationBar_->removeTab(_type);
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

void Ui::AppsPage::selectTab(AppPageType _pageType)
{
    if (navigationBar_)
    {
        navigationBar_->selectTab(_pageType);
    }
    else
    {
        if (auto it = pagesByType_.find(_pageType); it != pagesByType_.end())
            pages_->setCurrentWidget(it->second);
        else
            im_assert(false);
    }
}

void Ui::AppsPage::addCallsTab()
{
    auto callsTab = navigationBar_->addTab(AppPageType::calls, QT_TRANSLATE_NOOP("tab", "Calls"), qsl(":/tab/calls"), qsl(":/tab/calls_active"));
    Testing::setAccessibleName(callsTab, qsl("AS Tab ") % MainPage::callsTabAccessibleName());
}

void Ui::AppsPage::updateWebPages()
{
    auto makeWebPage = [this](AppPageType _type, const QString& _accessibleName)
    {
#ifdef HAS_WEB_ENGINE
        auto w = new WebAppPage(_type, this);

        const auto [url, isSigned] = makeWebAppUrl(_type);
        w->setUrl(url);

        if (authDataValid_ && !isSigned)
            startMiniAppSession(_type);
#else
        auto w = new QWidget(this);
#endif
        w->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
        Testing::setAccessibleName(w, _accessibleName);
        return w;
    };

    auto updateTab = [this, makeWebPage](bool _enabled, auto _type, const QString& _accName, auto... _params)
    {
        const auto hasTab = containsTab(_type);
        if (!_enabled && hasTab)
            removeTab(_type);
        else if (_enabled && !hasTab)
            addTab(_type, makeWebPage(_type, _accName), _params...);
    };

    updateNavigationBarVisibility();
    updateTab(Features::isTasksEnabled(), AppPageType::tasks, qsl("AS TasksPage"), QT_TRANSLATE_NOOP("tab", "Tasks"), qsl(":/tab/tasks"), qsl(":/tab/tasks_active"));
    updateTab(Features::isOrganizationStructureEnabled(), AppPageType::org_structure, qsl("AS StructurePage"), QT_TRANSLATE_NOOP("tab", "Contacts"), qsl(":/tab/structure"), qsl(":/tab/structure_active"));
    updateTab(Features::isCalendarEnabled(), AppPageType::calendar, qsl("AS CalendarPage"), QT_TRANSLATE_NOOP("tab", "Calendar"), qsl(":/tab/calendar"), qsl(":/tab/calendar_active"));
}

void Ui::AppsPage::updatePagesAimsids()
{
#ifdef HAS_WEB_ENGINE
    for (auto [type, widget] : pagesByType_)
    {
        auto id = Ui::WebAppPage::webAppId(type);
        if (!id.isEmpty() && !Utils::InterConnector::instance().isMiniAppAuthParamsValid(id))
        {
            requestMiniAppAuthParams(id);
        }
        else
        {
            if (auto page = qobject_cast<WebAppPage*>(widget))
                page->setUrl(makeWebAppUrl(type).first);
        }

    }
#endif
}

void Ui::AppsPage::updateNavigationBarVisibility()
{
    if (navigationBar_)
        navigationBar_->setVisible(Features::isAppsNavigationBarVisible());
}

void Ui::AppsPage::onAuthParamsChanged(bool _aimsidChanged)
{
    const auto isValid = Utils::InterConnector::instance().isAuthParamsValid();
    if (!isValid)
        return;

    const auto oldValid = std::exchange(authDataValid_, isValid);
    if (!oldValid) // first time receiving auth params
        updateWebPages();
    else if (_aimsidChanged)
        updatePagesAimsids();
}

void Ui::AppsPage::onConfigChanged()
{
    if (authDataValid_)
        updateWebPages();
}

void Ui::AppsPage::onGuiSettingsChanged(const QString& _key)
{
    if (_key != ql1s(settings_show_calls_tab))
        return;

    if (get_gui_settings()->get_value<bool>(settings_show_calls_tab, true))
        addCallsTab();
    else
        navigationBar_->removeTab(AppPageType::calls);
}

bool Ui::AppsPage::containsTab(AppPageType _type) const
{
    return pagesByType_.find(_type) != pagesByType_.end();
}

void Ui::AppsPage::onTabClicked(AppPageType _newTab, AppPageType _oldTab)
{
    const auto isSameTabClicked = _newTab == _oldTab;

    if (isMessengerApp(_newTab))
        messengerPage_->onTabClicked(appPageTypeToMessengerTabType(_newTab), isSameTabClicked);

    if (!isSameTabClicked)
        onTabChanged(_newTab);
}

void Ui::AppsPage::onTabChanged(AppPageType _pageType)
{
    if (isWebApp(_pageType))
    {
        if (auto it = pagesByType_.find(_pageType); it != pagesByType_.end())
            pages_->setCurrentWidget(it->second);
        else
            im_assert(false);
    }
    else
    {
        pages_->setCurrentIndex(getMessengerPageIndex());
        messengerPage_->selectTab(appPageTypeToMessengerTabType(_pageType));
    }

    Utils::InterConnector::instance().getMainWindow()->updateMainMenu();
#ifdef HAS_WEB_ENGINE
    if (auto page = qobject_cast<WebAppPage*>(pages_->currentWidget()))
        page->refreshPageIfNeeded();
#endif
    Q_EMIT Utils::InterConnector::instance().currentPageChanged();
}

void Ui::AppsPage::loggedIn()
{
    connect(Logic::getRecentsModel(), &Logic::RecentsModel::dlgStatesHandled, this, &AppsPage::chatUnreadChanged, Qt::UniqueConnection);
    connect(Logic::getRecentsModel(), &Logic::RecentsModel::updated, this, &AppsPage::chatUnreadChanged, Qt::UniqueConnection);
    connect(Logic::getUnknownsModel(), &Logic::UnknownsModel::dlgStatesHandled, this, &AppsPage::chatUnreadChanged, Qt::UniqueConnection);
    connect(Logic::getUnknownsModel(), &Logic::UnknownsModel::updatedMessages, this, &AppsPage::chatUnreadChanged, Qt::UniqueConnection);
    connect(Logic::getUnknownsModel(), &Logic::UnknownsModel::updatedSize, this, &AppsPage::chatUnreadChanged, Qt::UniqueConnection);
    connect(Logic::getContactListModel(), &Logic::ContactListModel::contactChanged, this, &AppsPage::chatUnreadChanged, Qt::UniqueConnection);
    connect(Logic::getContactListModel(), &Logic::ContactListModel::contact_removed, this, &AppsPage::chatUnreadChanged, Qt::UniqueConnection);
}

void Ui::AppsPage::chatUnreadChanged()
{
    messengerPage_->chatUnreadChanged();
    if (messengerItem_)
    {
        const auto totalUnreadsStr = Utils::getUnreadsBadgeStr(Logic::getRecentsModel()->totalUnreads() + Logic::getUnknownsModel()->totalUnreads());
        messengerItem_->setBadgeText(totalUnreadsStr);
    }
}

void Ui::AppsPage::reloadWebAppPage(AppPageType _pageType)
{
#ifdef HAS_WEB_ENGINE
    im_assert(isWebApp(_pageType));
    if (auto it = pagesByType_.find(_pageType); it != pagesByType_.end())
    {
        if (auto page = qobject_cast<WebAppPage*>(it->second))
            page->reloadPage();
    }
#endif
}
