#include "stdafx.h"
#include "MainPage.h"
#include "Common.h"

#include "../common.shared/config/config.h"

#include "Splitter.h"
#include "TabWidget.h"
#include "TabBar.h"
#include "ExpandButton.h"
#include "ContactsTab.h"
#include "CallsTab.h"
#include "ContactDialog.h"
#include "GroupChatOperations.h"
#include "UpdaterButton.h"
#include "contact_list/ChatMembersModel.h"
#include "contact_list/Common.h"
#include "contact_list/RecentsTab.h"
#include "contact_list/ContactListModel.h"
#include "contact_list/ContactListWidget.h"
#include "contact_list/ContactListUtils.h"
#include "contact_list/RecentsModel.h"
#include "contact_list/SearchWidget.h"
#include "contact_list/SearchModel.h"
#include "contact_list/UnknownsModel.h"
#include "contact_list/TopPanel.h"
#include "contact_list/SelectionContactsForGroupChat.h"
#include "contact_list/AddContactDialogs.h"
#include "contact_list/FavoritesUtils.h"
#include "contact_list/ServiceContacts.h"
#include "settings/GeneralSettingsWidget.h"
#include "settings/SettingsTab.h"
#include "settings/SettingsHeader.h"
#include "sidebar/Sidebar.h"
#include "containers/FriendlyContainer.h"
#include "containers/LastseenContainer.h"
#include "containers/InputStateContainer.h"
#include "../core_dispatcher.h"
#include "../gui_settings.h"
#include "../my_info.h"

#include "../controls/CustomButton.h"
#include "../controls/FlatMenu.h"
#include "../controls/TextEmojiWidget.h"
#include "../controls/WidgetsNavigator.h"
#include "../controls/SemitransparentWindowAnimated.h"
#include "../controls/TransparentScrollBar.h"
#include "../controls/LineLayoutSeparator.h"
#include "../controls/LoaderOverlay.h"
#include "../controls/ContextMenu.h"
#include "../controls/GeneralDialog.h"
#include "../utils/InterConnector.h"
#include "../utils/utils.h"
#include "../utils/features.h"
#include "../utils/log/log.h"
#include "../utils/gui_metrics.h"
#ifndef STRIP_VOIP
#include "../voip/IncomingCallWindow.h"
#include "../voip/VideoWindow.h"
#include "../voip/MicroAlert.h"
#endif
#include "../main_window/settings/SettingsForTesters.h"
#include "MainWindow.h"
#include "smiles_menu/Store.h"
#include "smiles_menu/SuggestsWidget.h"
#include "../main_window/sidebar/MyProfilePage.h"
#include "../main_window/input_widget/AttachFilePopup.h"
#include "../main_window/input_widget/InputWidget.h"
#include "../main_window/history_control/HistoryControlPage.h"
#include "../main_window/history_control/HistoryControl.h"

#include "styles/ThemeParameters.h"

#include "url_config.h"

#include "previewer/toast.h"
#include "AppsPage.h"
#ifdef __APPLE__
#   include "../utils/macos/mac_support.h"
#endif

namespace
{
    constexpr int balloon_size = 20;
    constexpr int BACK_HEIGHT = 52;
    constexpr int ANIMATION_DURATION = 200;

    int getRecentsMinWidth() noexcept
    {
        return Utils::scale_value(280);
    }

    int getRecentsMaxWidth() noexcept
    {
        return Utils::scale_value(400);
    }

    int getRecentsDefaultWidth() noexcept
    {
        return Utils::scale_value(328);
    }

    int getRecentMiniModeWidth() noexcept
    {
        return Utils::scale_value(68);
    }

    int getFrame2MiniModeMinWidth() noexcept
    {
        return getRecentMiniModeWidth() + Ui::MainPage::pageMinWidth();
    }

    QSize expandButtonSize() noexcept
    {
        return { getRecentMiniModeWidth(), Utils::scale_value(BACK_HEIGHT) };
    }

    int splitterHandleWidth() noexcept
    {
        return Utils::scale_value(1);
    }

    template<typename Functor, typename...T>
    void for_each(Functor functor, T&&... args)
    {
        (functor(std::forward<T>(args)), ...);
    }

    constexpr std::chrono::milliseconds getLoaderOverlayDelay()
    {
        return std::chrono::milliseconds(100);
    }

    bool fastDropSearchEnabled()
    {
        return Ui::get_gui_settings()->get_value<bool>(settings_fast_drop_search_results, settings_fast_drop_search_default());
    }

    const QSize getHeaderIconSizeUnscaled() noexcept
    {
        return QSize(24, 24);
    }
}

namespace Ui
{
    UnreadsCounter::UnreadsCounter(QWidget* _parent)
        : QWidget(_parent)
    {
        connect(Logic::getRecentsModel(), &Logic::RecentsModel::updated, this, static_cast<void(UnreadsCounter::*)()>(&UnreadsCounter::update));
    }

    void UnreadsCounter::paintEvent(QPaintEvent* e)
    {
        QPainter p(this);
        const auto borderColor = QColor(Qt::transparent);
        const auto bgColor = Styling::getParameters().getColor(Styling::StyleVariable::PRIMARY);
        static const auto textColor = Styling::getParameters().getColor(Styling::StyleVariable::BASE_GLOBALWHITE);
        Utils::drawUnreads(
            &p,
            Fonts::appFontScaled(13, Fonts::FontWeight::SemiBold),
            bgColor,
            textColor,
            borderColor,
            Logic::getRecentsModel()->totalUnreads(),
            Utils::scale_value(balloon_size), 0, 0
            );
    }

    QObjectUniquePtr<MainPage> MainPage::_instance;

    const QString& MainPage::callsTabAccessibleName()
    {
        static const auto name = qsl("AS CallsTab");
        return name;
    }

    const QString& MainPage::settingsTabAccessibleName()
    {
        static const auto name = qsl("AS SettingsTab");
        return name;
    }

    MainPage* MainPage::instance(QWidget* _parent)
    {
        im_assert(_instance || _parent);

        if (!_instance)
            _instance.reset(new MainPage(_parent));

        return _instance.get();
    }

    bool MainPage::hasInstance()
    {
        return !!_instance;
    }

    void MainPage::reset()
    {
        _instance.reset();
    }

    MainPage::MainPage(QWidget* _parent)
        : QWidget(_parent)
        , recentsTab_(new RecentsTab(this))
        , settingsTab_(nullptr)
        , contactsTab_(nullptr)
        , callsTab_(nullptr)
        , videoWindow_(nullptr)
        , pages_(new WidgetsNavigator(this))
        , contactDialog_(new ContactDialog(this))
        , callsHistoryPage_(new EmptyConnectionInfoPage(this))
        , generalSettings_(new GeneralSettingsWidget(this))
        , stickersStore_(nullptr)
        , settingsTimer_(new QTimer(this))
        , animCLWidth_(new QPropertyAnimation(this, QByteArrayLiteral("clWidth"), this))
        , clSpacer_(new QWidget(this))
        , clHostLayout_(new QVBoxLayout())
        , leftPanelState_(LeftPanelState::spreaded)
        , spacerBetweenHeaderAndRecents_(new QWidget(this))
        , semiWindow_(new SemitransparentWindowAnimated(this, ANIMATION_DURATION))
        , settingsHeader_(new SettingsHeader(this))
        , profilePage_(nullptr)
        , splitter_(new Splitter(Qt::Horizontal, this))
        , clContainer_(nullptr)
        , pagesContainer_(nullptr)
        , sidebarPlaceholderWidget_(nullptr)
        , sidebar_(new Sidebar(_parent))
        , sidebarVisibilityOnRecentsFrame_(false)
        , tabWidget_(nullptr)
        , spreadedModeLine_(new LineLayoutSeparator(Utils::scale_value(1), LineLayoutSeparator::Direction::Ver, Styling::getParameters().getColor(Styling::StyleVariable::BASE_BRIGHT),this))
        , updaterButton_(nullptr)
        , NeedShowUnknownsHeader_(false)
        , currentTab_(RECENTS)
        , isManualRecentsMiniMode_(get_gui_settings()->get_value(settings_recents_mini_mode, false) && !Features::isAppsNavigationBarVisible())
        , frameCountMode_(FrameCountMode::_1)
        , oneFrameMode_(OneFrameMode::Tab)
        , prevOneFrameMode_(OneFrameMode::Tab)
        , moreMenu_(nullptr)
        , callLinkCreator_(nullptr)
    {
        if (isManualRecentsMiniMode_)
            leftPanelState_ = LeftPanelState::picture_only;

        connect(&Utils::InterConnector::instance(), &Utils::InterConnector::compactModeChanged, this, &MainPage::compactModeChanged);
        connect(&Utils::InterConnector::instance(), &Utils::InterConnector::contacts, this, &MainPage::openContacts);
        connect(&Utils::InterConnector::instance(), &Utils::InterConnector::openDialogOrProfileById, this, &MainPage::openDialogOrProfileById);
        connect(&Utils::InterConnector::instance(), &Utils::InterConnector::loaderOverlayShow, this, &MainPage::showLoaderOverlay);
        connect(&Utils::InterConnector::instance(), &Utils::InterConnector::loaderOverlayHide, this, &MainPage::hideLoaderOverlay);
        connect(&Utils::InterConnector::instance(), &Utils::InterConnector::sidebarVisibilityChanged, this, &MainPage::onSidebarVisibilityChanged);
        connect(&Utils::InterConnector::instance(), &Utils::InterConnector::scrollToNewsFeedMesssage, this, &MainPage::onScrollToNewsFeedMesssage);

        sidebar_->escCancel_->setCancelCallback([this]() { setSidebarState(SidebarStateFlag::CloseAnimated); });
        sidebar_->escCancel_->setCancelPriority(EscPriority::high);

        connect(sidebar_, &Sidebar::hidden, this, [this]()
        {
            updateSplitterHandle(width());
        });
        sidebar_->hide();

        semiWindow_->setCloseWindowInfo({ Utils::CloseWindowInfo::Initiator::Unknown, Utils::CloseWindowInfo::Reason::Keep_Sidebar });

        auto horizontalLayout = Utils::emptyHLayout(this);

        contactsWidget_ = new QWidget(this);

        if (isManualRecentsMiniMode_)
            contactsWidget_->resize(getRecentMiniModeWidth(), contactsWidget_->height());

        contactsLayout_ = Utils::emptyVLayout(contactsWidget_);

        topWidget_ = new TopPanelWidget(this);
        Testing::setAccessibleName(topWidget_, qsl("AS RecentsTab topWidget"));

        if (!Features::isStatusInAppsNavigationBar())
        {
            recentsHeaderButtons_.more_ = new HeaderTitleBarButton(this);
            recentsHeaderButtons_.more_->setDefaultImage(qsl(":/controls/more_icon"), Styling::ColorParameter{}, getHeaderIconSizeUnscaled());
            Testing::setAccessibleName(recentsHeaderButtons_.more_, qsl("AS RecentsTab moreMenu"));
            topWidget_->getRecentsHeader()->addButtonToLeft(recentsHeaderButtons_.more_);
            connect(recentsHeaderButtons_.more_, &HeaderTitleBarButton::clicked, this, &MainPage::onMoreClicked);
        }

        recentsHeaderButtons_.newContacts_ = new HeaderTitleBarButton(this);
        recentsHeaderButtons_.newContacts_->setDefaultImage(qsl(":/header/new_user"), Styling::ColorParameter{}, getHeaderIconSizeUnscaled());
        recentsHeaderButtons_.newContacts_->setCustomToolTip(QT_TRANSLATE_NOOP("tab header", "New Contacts"));
        Testing::setAccessibleName(recentsHeaderButtons_.newContacts_, qsl("AS RecentsTab newContacts"));
        recentsHeaderButtons_.newContacts_->setVisibility(Logic::getUnknownsModel()->itemsCount() > 0);
        topWidget_->getRecentsHeader()->addButtonToLeft(recentsHeaderButtons_.newContacts_);
        recentsHeaderButtons_.newContacts_->hide();
        QObject::connect(recentsHeaderButtons_.newContacts_, &HeaderTitleBarButton::clicked, this, [this]()
        {
            prevSearchInput_ = topWidget_->getSearchWidget()->getText();
            if (!prevSearchInput_.isEmpty())
                recentsTab_->setSearchMode(false);
            Q_EMIT Utils::InterConnector::instance().unknownsGoSeeThem();
        });
        QObject::connect(Logic::getUnknownsModel(), &Logic::UnknownsModel::updatedSize, this, [this]()
        {
            const auto count = Logic::getUnknownsModel()->itemsCount();
            recentsHeaderButtons_.newContacts_->setVisibility(count > 0);
            recentsHeaderButtons_.newContacts_->setBadgeText(Utils::getUnreadsBadgeStr(Logic::getUnknownsModel()->totalUnreads()));
            topWidget_->getRecentsHeader()->refresh();
        });


        recentsHeaderButtons_.newEmails_ = new EmailTitlebarButton(this);
        recentsHeaderButtons_.newEmails_->setDefaultImage(qsl(":/header/mail"), Styling::ColorParameter{}, getHeaderIconSizeUnscaled());
        recentsHeaderButtons_.newEmails_->setCustomToolTip(QT_TRANSLATE_NOOP("tab header", "Emails"));
        Testing::setAccessibleName(recentsHeaderButtons_.newEmails_, qsl("AS RecentsTab newEmails"));
        topWidget_->getRecentsHeader()->addButtonToLeft(recentsHeaderButtons_.newEmails_);
        recentsHeaderButtons_.newEmails_->hide();
        recentsHeaderButtons_.newEmails_->setVisibility(false);
        connect(MyInfo(), &my_info::received, this, &MainPage::updateNewMailButton);

        contactsLayout_->addWidget(topWidget_);
        spacerBetweenHeaderAndRecents_->setFixedHeight(Utils::scale_value(8));
        contactsLayout_->addWidget(spacerBetweenHeaderAndRecents_);

        if (isManualRecentsMiniMode_)
            spacerBetweenHeaderAndRecents_->hide();

        hideSemiWindow();

        connect(recentsTab_, &RecentsTab::createGroupChatClicked, this, [this]() { createGroupChat(CreateChatSource::pencil); });
        connect(recentsTab_, &RecentsTab::createChannelClicked, this, [this]() { createChannel(CreateChatSource::pencil); });
        connect(&Utils::InterConnector::instance(), &Utils::InterConnector::createGroupChat, this, [this]() { createGroupChat(CreateChatSource::pencil); });
        connect(&Utils::InterConnector::instance(), &Utils::InterConnector::createChannel, this, [this]() { createChannel(CreateChatSource::pencil); });
        connect(recentsTab_, &RecentsTab::addContactClicked, this, &MainPage::addContactClicked);
        connect(recentsTab_, &RecentsTab::tabChanged, this, &MainPage::tabChanged);

        connect(&Utils::InterConnector::instance(), &Utils::InterConnector::unknownsGoSeeThem, this, &MainPage::changeCLHeadToUnknownSlot);
        connect(&Utils::InterConnector::instance(), &Utils::InterConnector::unknownsGoBack, this, &MainPage::changeCLHeadToSearchSlot);
        connect(&Utils::InterConnector::instance(), &Utils::InterConnector::addByNick, this, &MainPage::addByNick);

        connect(&Utils::InterConnector::instance(), &Utils::InterConnector::createGroupCall, this, &MainPage::createGroupCall);
        connect(&Utils::InterConnector::instance(), &Utils::InterConnector::createCallByLink, this, &MainPage::createCallByLink);
        connect(&Utils::InterConnector::instance(), &Utils::InterConnector::createWebinar, this, &MainPage::createWebinar);
        connect(&Utils::InterConnector::instance(), &Utils::InterConnector::callContact, this, &MainPage::callContact);

        if (isManualRecentsMiniMode_)
        {
            recentsTab_->setItemWidth(getRecentMiniModeWidth());
            recentsTab_->resize(getRecentMiniModeWidth(), recentsTab_->height());
            recentsTab_->setPictureOnlyView(true);
            clSpacer_->setFixedWidth(0);
        }

        Testing::setAccessibleName(recentsTab_, qsl("AS RecentsTab chatListWidget"));
        contactsLayout_->addWidget(recentsTab_);

        connect(topWidget_, &TopPanelWidget::back, this, &MainPage::recentsTopPanelBack);
        connect(topWidget_, &TopPanelWidget::needSwitchToRecents, this, &MainPage::openRecents);
        connect(topWidget_, &TopPanelWidget::searchCancelled, this, &MainPage::searchCancelled);

        connect(Logic::getContactListModel(), &Logic::ContactListModel::needSwitchToRecents, this, &MainPage::openRecents);
        connect(Logic::getContactListModel(), &Logic::ContactListModel::contact_removed, this, &MainPage::onContactRemoved);

        contactsLayout_->addSpacerItem(new QSpacerItem(0, 0, QSizePolicy::Minimum));

        pagesLayout_ = Utils::emptyVLayout();
        Testing::setAccessibleName(settingsHeader_, qsl("AS SettingsTab headerWidget"));
        pagesLayout_->addWidget(settingsHeader_);
        settingsHeader_->setFixedHeight(Utils::getTopPanelHeight());
        settingsHeader_->setObjectName(qsl("header"));
        connect(settingsHeader_, &Ui::SettingsHeader::backClicked, this, &MainPage::settingsHeaderBack);
        settingsHeader_->hide();

        Testing::setAccessibleName(pages_, qsl("AS Pages"));
        pagesLayout_->addWidget(pages_);

        {
            Testing::setAccessibleName(contactDialog_, qsl("AS Dialog"));
            pages_->addWidget(contactDialog_);

            Testing::setAccessibleName(generalSettings_, qsl("AS Settings"));
            pages_->addWidget(generalSettings_);

            Testing::setAccessibleName(callsHistoryPage_, qsl("AS CallsHistoryPage"));
            callsHistoryPage_->setPageType(EmptyConnectionInfoPage::OnlineWidgetType::Calls);
            pages_->addWidget(callsHistoryPage_);

            pages_->push(contactDialog_);
        }

        Testing::setAccessibleName(contactsWidget_, qsl("AS RecentsTab"));
        tabWidget_ = new TabWidget(this);

        callsTab_ = new CallsTab();
        Testing::setAccessibleName(callsTab_, callsTabAccessibleName());
        auto tab = tabWidget_->addTab(callsTab_, QT_TRANSLATE_NOOP("tab", "Calls"), qsl(":/tab/calls"), qsl(":/tab/calls_active"));
        indexToTabs_.emplace_back(tab.first, TabType::Calls);

        contactsTab_ = new ContactsTab();
        Testing::setAccessibleName(contactsTab_, qsl("AS ContactsTab"));
        indexToTabs_.emplace_back(tabWidget_->addTab(contactsTab_, QT_TRANSLATE_NOOP("tab", "Contacts"), qsl(":/tab/contacts"), qsl(":/tab/contacts_active")).first, TabType::Contacts);

        indexToTabs_.emplace_back(tabWidget_->addTab(contactsWidget_, QT_TRANSLATE_NOOP("tab", "Chats"), qsl(":/tab/chats"), qsl(":/tab/chats_active")).first, TabType::Recents);

        settingsTab_ = new SettingsTab();
        Testing::setAccessibleName(settingsTab_, settingsTabAccessibleName());
        indexToTabs_.emplace_back(tabWidget_->addTab(settingsTab_, QT_TRANSLATE_NOOP("tab", "Settings"), qsl(":/tab/settings"), qsl(":/tab/settings_active")).first, TabType::Settings);

        selectRecentsTab();

        clHostLayout_->setSpacing(0);
        Testing::setAccessibleName(tabWidget_, qsl("AS Tab widget"));
        clHostLayout_->addWidget(tabWidget_);

        expandButton_ = new ExpandButton();
        expandButton_->setFixedSize(expandButtonSize());
        expandButton_->hide();
        Testing::setAccessibleName(expandButton_, qsl("AS Tab expandButton"));
        clHostLayout_->addWidget(expandButton_);

        connect(Ui::GetDispatcher(), &Ui::core_dispatcher::updateReady, this, &MainPage::initUpdateButton);
        connect(&Utils::InterConnector::instance(), &Utils::InterConnector::onMacUpdateInfo, this, [this](Utils::MacUpdateState _state)
        {
            if (_state == Utils::MacUpdateState::Ready)
            {
                Log::write_network_log("Update is downloaded and extracted by sparkle\n");
                initUpdateButton();
            }
        });

        clHostLayout_->setContentsMargins(0, 0, 0, 0);

        clContainer_ = new QWidget();

        auto clContainerLayout = Utils::emptyHLayout();
        clContainerLayout->setSpacing(0);
        clContainerLayout->setContentsMargins(0, 0, 0, 0);
        clContainerLayout->addLayout(clHostLayout_);
        clContainerLayout->addWidget(spreadedModeLine_);
        spreadedModeLine_->hide();
        clContainer_->setLayout(clContainerLayout);
        clContainer_->setMaximumWidth(getRecentsMaxWidth());
        Testing::setAccessibleName(clContainer_, qsl("AS MainWindow rowContainer"));
        splitter_->addWidget(clContainer_);
        clContainer_->installEventFilter(this);

        if (isManualRecentsMiniMode_)
        {
            settingsTab_->setCompactMode(true);
            tabWidget_->hideTabbar();
            expandButton_->show();
            clContainer_->setFixedWidth(getRecentMiniModeWidth());
        }

        if (!Features::isMessengerTabBarVisible())
            tabWidget_->hideTabbar();

        splitter_->setChildrenCollapsible(false);

        QObject::connect(splitter_, &Splitter::moved, this, [this, _recentIdx = splitter_->count()](int _desiredPos, int _resultPos, int _index)
        {
            if (_index == _recentIdx)
            {
                const auto wasManualMiniMode = isManualRecentsMiniMode_;
                if ((_desiredPos < getRecentMiniModeWidth() * 2) && !Features::isAppsNavigationBarVisible())
                {
                    isManualRecentsMiniMode_ = true;
                    setLeftPanelState(LeftPanelState::picture_only, false);
                    for_each([width = getRecentMiniModeWidth()](auto w) { w->setFixedWidth(width); }, /*topWidget_,*/ clContainer_);
                }
                else if ((width() > getFrame2MinWidth() && !sidebar_->isVisible())
                    || (width() > getFrame3MinWidth() && sidebar_->isVisible()))
                {
                    isManualRecentsMiniMode_ = false;

                    saveContactListWidth();
                    setLeftPanelState(LeftPanelState::normal, false);
                    for_each([width = getRecentsMaxWidth()](auto w) { w->setMaximumWidth(width); }, /*topWidget_,*/ clContainer_);
                    for_each([width = getRecentsMinWidth()](auto w) { w->setMinimumWidth(width); }, /*topWidget_,*/ clContainer_);

                    if (wasManualMiniMode && sidebar_->isVisible())
                    {
                        const auto newWidth = std::min(sidebar_->getCurrentWidth(), width() - (clContainer_->width() + pagesContainer_->width() + splitterHandleWidth()));
                        updateSidebarWidth(newWidth);
                        sidebar_->saveWidth();
                    }
                }
            }
            else if (_index > _recentIdx && sidebar_->isVisible())
            {
                const auto newDesidedWidth = width() - _desiredPos;
                const auto recentsCheckableWidth = isManualRecentsMiniMode_ ? getRecentMiniModeWidth() : getRecentsMinWidth();
                if (newDesidedWidth > Sidebar::getDefaultMaximumWidth() || newDesidedWidth + recentsCheckableWidth + pageMinWidth() + splitterHandleWidth() > width())
                    return;

                updateSidebarWidth(newDesidedWidth);
                if (frameCountMode_ != FrameCountMode::_1)
                    sidebar_->saveWidth();
                if (!isManualRecentsMiniMode_)
                {
                    if (width() >= currentCLWidth_ + pagesContainer_->width() + sidebar_->width() + splitterHandleWidth())
                    {
                        restoreContactListWidth();
                    }
                    else
                    {
                        const auto newCLWidth = std::clamp(width() - (sidebar_->getCurrentWidth() + pagesContainer_->width() + splitterHandleWidth()), getRecentsMinWidth(), getRecentsMaxWidth());
                        for_each([&newCLWidth](auto w) { w->layout()->invalidate(); w->setFixedWidth(newCLWidth); w->updateGeometry(); }, clContainer_);
                    }
                }
                else
                {
                    for_each([width = getRecentMiniModeWidth()](auto w) { w->setFixedWidth(width); }, clContainer_);
                }
            }

            // save after splitter internally updates state
            QTimer::singleShot(0, this, &MainPage::saveSplitterState);
        });

        pagesLayout_->setContentsMargins(0, 0, 0, 0);
        pagesContainer_ = new QWidget;
        pagesContainer_->setLayout(pagesLayout_);
        pagesContainer_->setMinimumWidth(pageMinWidth());
        Testing::setAccessibleName(pagesContainer_, qsl("AS MainWindow pageContainer"));
        splitter_->addWidget(pagesContainer_);

        splitter_->setOpaqueResize(true);
        splitter_->setHandleWidth(splitterHandleWidth());
        splitter_->setMouseTracking(true);

        insertSidebarToSplitter();

        updateSplitterStretchFactors();

        Testing::setAccessibleName(splitter_, qsl("AS MainWindow splitView"));
        horizontalLayout->addWidget(splitter_);

        setFocus();

        connect(recentsTab_, &RecentsTab::itemSelected, this, &MainPage::onContactSelected);
        connect(recentsTab_, &RecentsTab::itemSelected, this, &MainPage::hideRecentsPopup);
        connect(recentsTab_, &RecentsTab::itemSelected, this, &MainPage::onItemSelected);

        connect(contactsTab_->getContactListWidget(), &ContactListWidget::itemSelected, this, &MainPage::onContactSelected);
        connect(contactsTab_->getContactListWidget(), &ContactListWidget::itemSelected, this, &MainPage::onItemSelected);

        connect(&Utils::InterConnector::instance(), &Utils::InterConnector::messageSent, this, &MainPage::onMessageSent);
        connect(&Utils::InterConnector::instance(), &Utils::InterConnector::showSettingsHeader, this, &MainPage::showSettingsHeader);
        connect(&Utils::InterConnector::instance(), &Utils::InterConnector::hideSettingsHeader, this, &MainPage::hideSettingsHeader);
        connect(&Utils::InterConnector::instance(), &Utils::InterConnector::myProfileBack, this, &MainPage::settingsHeaderBack);

        connect(&Utils::InterConnector::instance(), &Utils::InterConnector::closeLastDialog, this, &MainPage::dialogBackClicked);

        connect(&Utils::InterConnector::instance(), &Utils::InterConnector::dialogClosed, this, &MainPage::onDialogClosed);
        connect(Logic::getContactListModel(), &Logic::ContactListModel::selectedContactChanged, this, &MainPage::onSelectedContactChanged);

        connect(&Utils::InterConnector::instance(), &Utils::InterConnector::openThread, this, &MainPage::openThread);
        connect(&Utils::InterConnector::instance(), &Utils::InterConnector::profileSettingsShow, this, &MainPage::onProfileSettingsShow);
        connect(&Utils::InterConnector::instance(), &Utils::InterConnector::sharedProfileShow, this, &MainPage::onSharedProfileShow);
        connect(&Utils::InterConnector::instance(), &Utils::InterConnector::generalSettingsShow, this, &MainPage::onGeneralSettingsShow);

        connect(&Utils::InterConnector::instance(), &Utils::InterConnector::profileSettingsBack, pages_, &Ui::WidgetsNavigator::pop);

        connect(&Utils::InterConnector::instance(), &Utils::InterConnector::attachPhoneBack, pages_, &Ui::WidgetsNavigator::pop);
        connect(&Utils::InterConnector::instance(), &Utils::InterConnector::attachUinBack, pages_, &Ui::WidgetsNavigator::pop);

        connect(&Utils::InterConnector::instance(), &Utils::InterConnector::popPagesToRoot, this, &MainPage::popPagesToRoot);
        connect(&Utils::InterConnector::instance(), &Utils::InterConnector::startSearchInDialog, this, &MainPage::startSearchInDialog);
        connect(&Utils::InterConnector::instance(), &Utils::InterConnector::setSearchFocus, this, &MainPage::setSearchFocus);

        connect(&Utils::InterConnector::instance(), &Utils::InterConnector::searchEnd, this, &MainPage::onSearchEnd);
        connect(&Utils::InterConnector::instance(), &Utils::InterConnector::contactFilterRemoved, this, &MainPage::onContactFilterRemoved);

        connect(topWidget_->getSearchWidget(), &SearchWidget::searchBegin, this, &MainPage::searchBegin);
        connect(topWidget_->getSearchWidget(), &SearchWidget::inputEmpty, this, &MainPage::onSearchInputEmpty);
        connect(topWidget_->getSearchWidget(), &SearchWidget::searchIconClicked, this, &MainPage::spreadCL);
        connect(topWidget_->getSearchWidget(), &SearchWidget::activeChanged, this, &MainPage::searchActivityChanged);

        if (isManualRecentsMiniMode_)
            topWidget_->setState(LeftPanelState::picture_only);

        recentsTab_->connectSearchWidget(topWidget_->getSearchWidget());

#ifndef STRIP_VOIP
        Ui::GetDispatcher()->getVoipController().loadSettings([](voip_proxy::device_desc& desc){});
        connect(&Ui::GetDispatcher()->getVoipController(), &voip_proxy::VoipController::onVoipShowVideoWindow, this, &MainPage::onVoipShowVideoWindow, Qt::DirectConnection);
        connect(&Ui::GetDispatcher()->getVoipController(), &voip_proxy::VoipController::onVoipCallIncoming, this, &MainPage::onVoipCallIncoming, Qt::DirectConnection);
        connect(&Ui::GetDispatcher()->getVoipController(), &voip_proxy::VoipController::onVoipCallIncomingAccepted, this, &MainPage::onVoipCallIncomingAccepted, Qt::DirectConnection);
        connect(&Ui::GetDispatcher()->getVoipController(), &voip_proxy::VoipController::onVoipCallDestroyed, this, &MainPage::onVoipCallDestroyed, Qt::DirectConnection);
#endif

        connect(Ui::GetDispatcher(), &core_dispatcher::chatJoinResultBlocked, this, &MainPage::onJoinChatResultBlocked);

        QTimer::singleShot(Ui::period_for_start_stats_settings_ms, this, &MainPage::post_stats_with_settings);
        QObject::connect(settingsTimer_, &QTimer::timeout, this, &MainPage::post_stats_with_settings);
        settingsTimer_->start(Ui::period_for_stats_settings_ms);

        connect(Ui::GetDispatcher(), &core_dispatcher::idInfo, this, &MainPage::onIdInfo);

        connect(pages_, &QStackedWidget::currentChanged, this, &MainPage::currentPageChanged);

        connect(&Utils::InterConnector::instance(), &Utils::InterConnector::showStickersStore, this, &MainPage::showStickersStore);

        connect(tabWidget_, &TabWidget::currentChanged, this, &MainPage::tabBarCurrentChanged);
        connect(tabWidget_, &TabWidget::tabBarClicked, this, qOverload<int>(&MainPage::onTabClicked));
        connect(tabWidget_->tabBar(), &TabBar::rightClicked, this, &MainPage::tabBarRightClicked);
        connect(expandButton_, &ExpandButton::clicked, this, &MainPage::expandButtonClicked);

        connect(&Utils::InterConnector::instance(), &Utils::InterConnector::closeAnySemitransparentWindow, this, [this](const Utils::CloseWindowInfo& _info)
        {
            if (leftPanelState_ == LeftPanelState::spreaded)
                hideRecentsPopup();
            if (_info.reason_ != Utils::CloseWindowInfo::Reason::Keep_Sidebar)
                setSidebarState(SidebarStateFlag::CloseAnimated);
        });

        connect(&Utils::InterConnector::instance(), &Utils::InterConnector::applicationLocked, this, [this]()
        {
            setSidebarState(SidebarStateFlag::CloseAnimated);
        });

        connect(get_gui_settings(), &Ui::qt_gui_settings::changed, this, [this](const QString& _key)
        {
            if (_key == ql1s(settings_notify_new_mail_messages))
                updateNewMailButton();
        });

        const auto scale = get_gui_settings()->get_value(settings_scale_coefficient, Utils::getBasicScaleCoefficient());
        const auto splitterScale = get_gui_settings()->get_value(settings_splitter_state_scale, 0.0);
        if (const auto val = QByteArray::fromStdString(get_gui_settings()->get_value<std::string>(settings_splitter_state, std::string())); !val.isEmpty() && scale == splitterScale)
            splitter_->restoreState(val);
        else
            clContainer_->resize(getRecentsDefaultWidth(), clContainer_->height());

        saveContactListWidth();
        chatUnreadChanged();
    }

    MainPage::~MainPage()
    {
#ifndef STRIP_VOIP
        if (videoWindow_)
        {
            delete videoWindow_;
            videoWindow_ = nullptr;
        }
#endif
    }

    int MainPage::splitterHandleWidth() noexcept
    {
        return Utils::scale_value(1);
    }

    int MainPage::pageMinWidth() noexcept
    {
        return Utils::scale_value(380);
    }

    int MainPage::getFrame2MinWidth() noexcept
    {
        return Utils::scale_value(660);
    }

    int MainPage::getFrame3MinWidth() noexcept
    {
        return getFrame2MinWidth() + Ui::Sidebar::getDefaultWidth();
    }

    bool MainPage::isUnknownsOpen() const
    {
        return
            pages_->currentWidget() == contactDialog_ &&
            recentsTab_->currentTab() == RECENTS &&
            recentsTab_->isUnknownsOpen();
    }

    bool MainPage::isPictureOnlyView() const
    {
        return clContainer_->width() == getRecentMiniModeWidth();
    }

    void MainPage::setFrameCountMode(FrameCountMode _mode)
    {
        if (frameCountMode_ != _mode)
        {
            frameCountMode_ = _mode;
            for_each([_mode](auto w) { if (w) w->setFrameCountMode(_mode); }, contactDialog_, sidebar_, stickersStore_);

            if (isOneFrameMode() && pages_->currentWidget() == callsHistoryPage_)
                switchToTabFrame();
        }
    }

    void MainPage::setOneFrameMode(OneFrameMode _mode)
    {
        if (_mode == oneFrameMode_)
            return;
        prevOneFrameMode_ = std::exchange(oneFrameMode_, _mode);
    }

    void MainPage::updateSettingHeader()
    {
        if (isOneFrameMode() && oneFrameMode_ == OneFrameMode::Content && getCurrentTab() == TabType::Settings)
            settingsHeader_->show();
        else
            settingsHeader_->hide();
    }

    void MainPage::updateNewMailButton()
    {
        const auto isMailVisible =
            get_gui_settings()->get_value<bool>(settings_notify_new_mail_messages, true) &&
            MyInfo()->haveConnectedEmail() &&
            getUrlConfig().isMailConfigPresent();

        recentsHeaderButtons_.newEmails_->setVisibility(isMailVisible);
        topWidget_->getRecentsHeader()->refresh();
    }

    void MainPage::switchToContentFrame()
    {
        setOneFrameMode(OneFrameMode::Content);
        if (getCurrentTab() == TabType::Settings && settingsHeader_->hasText())
            settingsHeader_->show();
        pagesContainer_->show();
        clContainer_->hide();
        sidebar_->hideAnimated();
        removeSidebarPlaceholder();
        Q_EMIT Utils::InterConnector::instance().currentPageChanged();
    }

    void MainPage::switchToTabFrame()
    {
        if (isOneFrameMode() && getCurrentTab() == TabType::Settings && generalSettings_->isWebViewShown())
        {
            switchToContentFrame();
            return;
        }

        setOneFrameMode(OneFrameMode::Tab);
        if (canShowRecents())
            clContainer_->show();
        pagesContainer_->hide();
        sidebar_->hideAnimated();
        removeSidebarPlaceholder();
        Q_EMIT Utils::InterConnector::instance().currentPageChanged();
        Q_EMIT Utils::InterConnector::instance().stopPttRecord();
    }

    void MainPage::switchToSidebarFrame()
    {
        setOneFrameMode(OneFrameMode::Sidebar);
        pagesContainer_->hide();
        clContainer_->hide();
        insertSidebarToSplitter();
        sidebar_->show();
        Q_EMIT Utils::InterConnector::instance().currentPageChanged();
        Q_EMIT Utils::InterConnector::instance().stopPttRecord();
    }

    void MainPage::onIdInfo(const int64_t _seq, const Data::IdInfo& _info)
    {
        if (dialogIdLoader_.idInfoSeq_ == _seq)
        {
            if (!_info.isValid())
            {
                QString text;
                if (dialogIdLoader_.id_.contains(u'@') && !Utils::isChat(dialogIdLoader_.id_))
                    text = QT_TRANSLATE_NOOP("toast", "There are no profiles with this email");
                else
                    text = QT_TRANSLATE_NOOP("toast", "There are no profiles or groups with this nickname");

                Utils::showTextToastOverContactDialog(text);
            }
            else
            {
                if (dialogIdLoader_.botParams_)
                {
                    im_assert(_info.isBot_);
                    if (_info.isBot_)
                    {
                        Logic::GetLastseenContainer()->setContactBot(_info.sn_);
                        Utils::openDialogWithContact(_info.sn_, -1, true, [params = std::move(dialogIdLoader_.botParams_)](PageBase* page)
                        {
                            if (auto historyPage = qobject_cast<HistoryControlPage*>(page))
                                historyPage->setBotParameters(params);
                        });
                    }
                    else
                    {
                        if (dialogIdLoader_.forceDialogOpen_)
                            Utils::openDialogWithContact(_info.sn_);
                        else
                            Utils::openDialogOrProfile(_info.sn_);
                    }
                }
                else
                {
                    if (_info.isChatInfo() && !_info.stamp_.isEmpty())
                    {
                        Utils::openDialogOrProfile(_info.stamp_, Utils::OpenDOPParam::stamp);
                    }
                    else
                    {
                        if (dialogIdLoader_.forceDialogOpen_)
                            Utils::openDialogWithContact(_info.sn_);
                        else
                            Utils::openDialogOrProfile(_info.sn_);
                    }
                }
            }

            onIdLoadingCancelled();
        }
    }

    void MainPage::onIdLoadingCancelled()
    {
        dialogIdLoader_.clear();
        hideLoaderOverlay();

        Q_EMIT Utils::InterConnector::instance().loaderOverlayCancelled();
    }

    void MainPage::updateLoaderOverlayPosition()
    {
        if (!dialogIdLoader_.loaderOverlay_)
            return;

        QRect loaderRect(QPoint(clContainer_->isVisible() ? clContainer_->width() : 0, 0), pagesContainer_->size());
        dialogIdLoader_.loaderOverlay_->setCenteringRect(loaderRect);
        dialogIdLoader_.loaderOverlay_->setFixedSize(size());
    }

    bool MainPage::canSetSearchFocus() const
    {
        return !isSemiWindowVisible() &&
            getCurrentTab() == TabType::Recents &&
            recentsTab_->currentTab() == RECENTS;
    }

    void MainPage::onMoreClicked()
    {
        if (!moreMenu_)
        {
            moreMenu_ = new ContextMenu(this, 24);
            Testing::setAccessibleName(moreMenu_, qsl("AS MainPage moreMenu"));
            if (config::get().is_on(config::features::add_contact))
            {
                moreMenu_->addActionWithIcon(qsl(":/header/add_user"), QT_TRANSLATE_NOOP("contact_list", "Add contact"), this, []
                {
                    Utils::InterConnector::instance().getMainWindow()->onShowAddContactDialog({ Utils::AddContactDialogs::Initiator::From::ScrChatsTab });
                });
            }

            moreMenu_->addActionWithIcon(qsl(":/add_groupchat"), QT_TRANSLATE_NOOP("contact_list", "Create group"), this, [this]() { createGroupChat(CreateChatSource::dots); });
            moreMenu_->addActionWithIcon(qsl(":/add_channel"), QT_TRANSLATE_NOOP("contact_list", "Create channel"), this, [this]() { createChannel(CreateChatSource::dots); });
            moreMenu_->addSeparator();
            moreMenu_->addActionWithIcon(qsl(":/header/check"), QT_TRANSLATE_NOOP("tab header", "Read all"), this, &MainPage::readAllClicked);
        }

        const auto p = mapToGlobal(rect().topLeft()) + Utils::scale_value(QPoint(8, 8));
        moreMenu_->popup(p);
    }

    void MainPage::setCLWidth(int _val)
    {
        setCLWidthNew(_val);
    }

    void MainPage::setCLWidthNew(int _val)
    {
        auto compact_width = ::Ui::ItemWidth(false, true);
        contactsWidget_->resize(_val, contactsWidget_->height());
        recentsTab_->setItemWidth(_val);
        contactsTab_->setClWidth(_val);
        callsTab_->setClWidth(_val);
        recentsTab_->resize(_val, recentsTab_->height());
        const bool isCompact = (_val == compact_width);
        recentsTab_->setPictureOnlyView(isCompact);
        callsTab_->setPictureOnlyView(isCompact);

        if (leftPanelState_ == LeftPanelState::picture_only && _val == compact_width && splitter_->indexOf(clContainer_) == -1)
        {
            clSpacer_->setFixedWidth(0);
            splitter_->replaceWidget(0, clContainer_);
            updateSplitterStretchFactors();
            Testing::setAccessibleName(contactsWidget_, qsl("AS RecentsTab"));
        }
    }

    int MainPage::getCLWidth() const
    {
        return recentsTab_->width();
    }

    void MainPage::animateVisibilityCL(int _newWidth, bool _withAnimation)
    {
        int startValue = getCLWidth();
        int endValue = _newWidth;

        int duration = _withAnimation ? 200 : 0;

        animCLWidth_->stop();
        animCLWidth_->setDuration(duration);
        animCLWidth_->setStartValue(startValue);
        animCLWidth_->setEndValue(endValue);
        animCLWidth_->setEasingCurve(QEasingCurve::InQuad);
        animCLWidth_->start();
    }

    LeftPanelState MainPage::getLeftPanelState() const
    {
        return leftPanelState_;
    }

    bool MainPage::isOneFrameMode() const
    {
        return frameCountMode_ == FrameCountMode::_1;
    }

    bool MainPage::isOneFrameTab() const
    {
        return isOneFrameMode() && oneFrameMode_ == OneFrameMode::Tab;
    }

    bool MainPage::isOneFrameSidebar() const
    {
        return isOneFrameMode() && oneFrameMode_ == OneFrameMode::Sidebar;
    }

    bool MainPage::isInSettingsTab() const
    {
        return getCurrentTab() == TabType::Settings;
    }

    bool MainPage::isInRecentsTab() const
    {
        return getCurrentTab() == TabType::Recents;
    }

    bool MainPage::isInContactsTab() const
    {
        return getCurrentTab() == TabType::Contacts;
    }

    void MainPage::openDialogOrProfileById(const QString& _id, bool _forceDialogOpen, std::optional<QString> _botParams)
    {
        im_assert(!_id.isEmpty());

        if (fastDropSearchEnabled())
            Q_EMIT Utils::InterConnector::instance().searchEnd();

        Q_EMIT Utils::InterConnector::instance().closeAnySemitransparentWindow({ Utils::CloseWindowInfo::Initiator::Unknown, Utils::CloseWindowInfo::Reason::Keep_Sidebar });

        dialogIdLoader_.id_ = _id.startsWith(u'@') ? _id.mid(1) : _id;
        dialogIdLoader_.botParams_ = std::move(_botParams);
        dialogIdLoader_.idInfoSeq_ = GetDispatcher()->getIdInfo(dialogIdLoader_.id_);
        dialogIdLoader_.forceDialogOpen_ = _forceDialogOpen;
        QTimer::singleShot(getLoaderOverlayDelay(), this, [this, idSeq = dialogIdLoader_.idInfoSeq_]()
        {
            if (dialogIdLoader_.idInfoSeq_ == idSeq)
                showLoaderOverlay();
        });
    }

    FrameCountMode MainPage::getFrameCount() const
    {
        return frameCountMode_;
    }

    void MainPage::closeAndHighlightDialog()
    {
        if (const auto aimId = Logic::getContactListModel()->selectedContact(); !aimId.isEmpty())
        {
            if (isSearchActive() && getFrameCount() != FrameCountMode::_1)
            {
                openRecents();
                stopNavigatingInRecents();
                Q_EMIT Utils::InterConnector::instance().setFocusOnInput(aimId);
            }
            else
            {
                if (!isOpenedAsMain() && recentsTab_->isSearchMode())
                {
                    switchToTabFrame();
                }
                else if (isOpenedAsMain())
                {
                    window()->setFocus();
                    Utils::InterConnector::instance().closeDialog();
                    recentsTab_->highlightContact(aimId);
                }
            }
        }
    }

    void MainPage::showChatMembersFailuresPopup(Utils::ChatMembersOperation _operation, QString _chatAimId, std::map<core::chat_member_failure, std::vector<QString>> _failures)
    {
        if (_failures.empty())
            return;

        const auto isChannel = Logic::getContactListModel()->isChannel(_chatAimId);

        const auto federationError =
            _operation == Utils::ChatMembersOperation::Remove
            ? QT_TRANSLATE_NOOP("popup_window", "Error removing federation user")
            : _operation == Utils::ChatMembersOperation::Block
            ? QT_TRANSLATE_NOOP("popup_window", "Error blocking federation user")
            : _operation == Utils::ChatMembersOperation::Unblock
            ? QT_TRANSLATE_NOOP("popup_window", "Error unblocking federation user")
            : QString();

        auto correctFederationUserName = [](QStringView _user) -> QString
        {
            // i.ivanov@example.com#remote_federation -> i.ivanov@example.com (remote_federation)
            const auto pos = _user.indexOf(u'#');
            if (pos == -1)
                return _user.toString();
            return _user.mid(0, pos) % u" (" % _user.mid(pos + 1) % u')';
        };

        if (_failures.size() == 1 && _failures.begin()->second.size() == 1)
        {
            const auto& [type, contacts] = *_failures.begin();
            const auto friendly = correctFederationUserName(Logic::GetFriendlyContainer()->getFriendly(contacts.front()));
            const auto cannotOperation =
                _operation == Utils::ChatMembersOperation::Add
                ? QT_TRANSLATE_NOOP("popup_window", "Cannot add %1").arg(friendly)
                : _operation == Utils::ChatMembersOperation::Remove
                ? QT_TRANSLATE_NOOP("popup_window", "Cannot remove %1").arg(friendly)
                : _operation == Utils::ChatMembersOperation::Block
                ? QT_TRANSLATE_NOOP("popup_window", "Cannot block %1").arg(friendly)
                : _operation == Utils::ChatMembersOperation::Unblock
                ? QT_TRANSLATE_NOOP("popup_window", "Cannot unblock %1").arg(friendly)
                : QString();

            if (cannotOperation.isEmpty())
                return;

            if (type == core::chat_member_failure::user_waiting_for_approve)
            {
                Utils::GetConfirmationWithOneButton(
                    QT_TRANSLATE_NOOP("popup_window", "OK"),
                    QT_TRANSLATE_NOOP("popup_window", "Please wait until admins approve %1's request").arg(friendly),
                    QT_TRANSLATE_NOOP("popup_window", "Request for %1 was sent").arg(friendly),
                    nullptr);
            }
            else if (type == core::chat_member_failure::user_must_join_by_link)
            {
                const auto stamp = Logic::getContactListModel()->getChatStamp(_chatAimId);
                im_assert(!stamp.isEmpty());

                const auto link = stamp.isEmpty() ? QString() : QString(u"https://" % Utils::getDomainUrl() % u'/' % stamp);
                const auto text = isChannel
                    ? QT_TRANSLATE_NOOP("popup_window", "You cannot add %1 to channel. Send them subscription link %2").arg(friendly, link)
                    : QT_TRANSLATE_NOOP("popup_window", "You cannot add %1 to chat. Send them join link %2").arg(friendly, link);

                const auto confirmed = Utils::GetConfirmationWithTwoButtons(
                    QT_TRANSLATE_NOOP("popup_window", "Cancel"),
                    QT_TRANSLATE_NOOP("popup_window", "Send"),
                    text,
                    cannotOperation,
                    nullptr);

                if (confirmed)
                {
                    const auto text = isChannel
                        ? QT_TRANSLATE_NOOP("popup_window", "Subscribe the channel %1").arg(link)
                        : QT_TRANSLATE_NOOP("popup_window", "Join the group %1").arg(link);

                    Data::MessageBuddy msg;
                    msg.SetText(text);
                    msg.AimId_ = contacts.front();
                    GetDispatcher()->setDraft(msg, QDateTime::currentMSecsSinceEpoch());
                    Q_EMIT Utils::InterConnector::instance().reloadDraft(contacts.front());

                    Utils::openDialogWithContact(contacts.front());
                }
            }
            else if (type == core::chat_member_failure::user_blocked_confirmation_required)
            {
                const auto title = QT_TRANSLATE_NOOP("popup_window", "Add %1?").arg(friendly);
                const auto text = isChannel
                    ? QT_TRANSLATE_NOOP("popup_window", "Previously this contact was removed from channel. Probably, %1 violated the rules. Add anyway?").arg(friendly)
                    : QT_TRANSLATE_NOOP("popup_window", "Previously this contact was removed from chat. Probably, %1 violated the rules. Add anyway?").arg(friendly);

                const auto confirmed = Utils::GetConfirmationWithTwoButtons(
                    QT_TRANSLATE_NOOP("popup_window", "Cancel"),
                    QT_TRANSLATE_NOOP("popup_window", "Add"),
                    text,
                    title,
                    nullptr);

                if (confirmed)
                    postAddChatMembersToCore(_chatAimId, contacts, true);
            }
            else if (type == core::chat_member_failure::user_must_be_added_by_admin)
            {
                const auto text = isChannel
                    ? QT_TRANSLATE_NOOP("popup_window", "You cannot add this contact to channel, please ask admin to do that")
                    : QT_TRANSLATE_NOOP("popup_window", "You cannot add this contact to chat, please ask admin to do that");

                Utils::GetConfirmationWithOneButton(
                    QT_TRANSLATE_NOOP("popup_window", "OK"),
                    text,
                    cannotOperation,
                    nullptr);
            }
            else if (type == core::chat_member_failure::user_captcha)
            {
                Utils::GetConfirmationWithOneButton(
                    QT_TRANSLATE_NOOP("popup_window", "OK"),
                    QT_TRANSLATE_NOOP("popup_window", "Contact is banned in %1").arg(Utils::getAppTitle()),
                    cannotOperation,
                    nullptr);
            }
            else if (type == core::chat_member_failure::user_already_added)
            {
                Utils::GetConfirmationWithOneButton(
                    QT_TRANSLATE_NOOP("popup_window", "OK"),
                    QT_TRANSLATE_NOOP("popup_window", "Contact is already added"),
                    cannotOperation,
                    nullptr);
            }
            else if (type == core::chat_member_failure::bot_setjoingroups_false)
            {
                Utils::GetConfirmationWithOneButton(
                    QT_TRANSLATE_NOOP("popup_window", "OK"),
                    QT_TRANSLATE_NOOP("popup_window", "Bot can be added only by admin and if bot joining the groups was allowed by its creator"),
                    cannotOperation,
                    nullptr);
            }
            else if (type == core::chat_member_failure::user_not_found)
            {
                Utils::GetConfirmationWithOneButton(
                    QT_TRANSLATE_NOOP("popup_window", "OK"),
                    QT_TRANSLATE_NOOP("popup_window", "User not found"),
                    cannotOperation,
                    nullptr);
            }
            else if (type == core::chat_member_failure::not_member)
            {
                Utils::GetConfirmationWithOneButton(
                    QT_TRANSLATE_NOOP("popup_window", "OK"),
                    QT_TRANSLATE_NOOP("popup_window", "User not a member"),
                    cannotOperation,
                    nullptr);
            }
            else if (type == core::chat_member_failure::user_is_blocked_already)
            {
                Utils::GetConfirmationWithOneButton(
                    QT_TRANSLATE_NOOP("popup_window", "OK"),
                    QT_TRANSLATE_NOOP("popup_window", "User is already blocked"),
                    cannotOperation,
                    nullptr);
            }
            else if (type == core::chat_member_failure::user_not_blocked)
            {
                Utils::GetConfirmationWithOneButton(
                    QT_TRANSLATE_NOOP("popup_window", "OK"),
                    QT_TRANSLATE_NOOP("popup_window", "User not blocked"),
                    cannotOperation,
                    nullptr);
            }
            else if (type == core::chat_member_failure::permission_denied)
            {
                Utils::GetConfirmationWithOneButton(
                    QT_TRANSLATE_NOOP("popup_window", "OK"),
                    QT_TRANSLATE_NOOP("popup_window", "Permission denied"),
                    cannotOperation,
                    nullptr);
            }
            else if (type == core::chat_member_failure::remote_federation_error && !federationError.isEmpty())
            {
                Utils::GetConfirmationWithOneButton(
                    QT_TRANSLATE_NOOP("popup_window", "OK"),
                    federationError,
                    cannotOperation,
                    nullptr);
            }
        }
        else
        {
            auto makeText = [](auto _code, const auto& _text) -> QString
            {
                return Emoji::EmojiCode::toQString(Emoji::EmojiCode(_code)) % u' ' % _text;
            };

            QString message;
            int totalCount = 0;
            for (const auto& [type, contacts] : _failures)
            {
                if (contacts.empty())
                    continue;

                if (type == core::chat_member_failure::user_waiting_for_approve)
                    message += makeText(0x23f0, QT_TRANSLATE_NOOP("popup_window", "Wait until admins approve join requests"));
                else if (type == core::chat_member_failure::user_must_join_by_link)
                    message += makeText(0x274c, QT_TRANSLATE_NOOP("popup_window", "Send them invite link"));
                else if (type == core::chat_member_failure::user_must_be_added_by_admin)
                    message += makeText(0x1f44b, QT_TRANSLATE_NOOP("popup_window", "Ask admins to invite"));
                else if (type == core::chat_member_failure::user_captcha)
                    message += makeText(0x1f6ab, QT_TRANSLATE_NOOP("popup_window", "Contacts are banned in %1").arg(Utils::getAppTitle()));
                else if (type == core::chat_member_failure::user_already_added)
                    message += makeText(0x1f914, QT_TRANSLATE_NOOP("popup_window", "Contacts were already added"));
                else if (type == core::chat_member_failure::bot_setjoingroups_false)
                    message += makeText(0x1f916, QT_TRANSLATE_NOOP("popup_window", "Bot joining the groups was forbidden by its creator"));
                else if (type == core::chat_member_failure::user_not_found)
                    message += makeText(0x1f50d, QT_TRANSLATE_NOOP("popup_window", "User not found"));
                else if (type == core::chat_member_failure::not_member)
                    message += makeText(0x1f645, QT_TRANSLATE_NOOP("popup_window", "User not a member"));
                else if (type == core::chat_member_failure::user_is_blocked_already)
                    message += makeText(0x1f512, QT_TRANSLATE_NOOP("popup_window", "User is already blocked"));
                else if (type == core::chat_member_failure::user_not_blocked)
                    message += makeText(0x1f513, QT_TRANSLATE_NOOP("popup_window", "User not blocked"));
                else if (type == core::chat_member_failure::permission_denied)
                    message += makeText(0x1f6b7, QT_TRANSLATE_NOOP("popup_window", "Permission denied"));
                else if (type == core::chat_member_failure::remote_federation_error && !federationError.isEmpty())
                    message += makeText(0x1f6ab, federationError);
                else
                    continue;

                totalCount += contacts.size();

                message += qsl(": ");
                for (const auto& c : contacts)
                    message += correctFederationUserName(Logic::GetFriendlyContainer()->getFriendly(c)) % u", ";

                if (!message.isEmpty())
                    message.chop(2);

                message += QChar::LineFeed % QChar::LineFeed;
            }

            if (!message.isEmpty())
                message.chop(2);

            if (!message.isEmpty())
            {
                QString titleForOneContact;
                QString titleForTwoContacts;
                QString titleForFiveContacts;
                QString titleForTwentyOneContacts;
                if (_operation == Utils::ChatMembersOperation::Add)
                {
                    titleForOneContact = QT_TRANSLATE_NOOP3("popup_window", "Cannot add %1 contact", "1");
                    titleForTwoContacts = QT_TRANSLATE_NOOP3("popup_window", "Cannot add %1 contacts", "2");
                    titleForFiveContacts = QT_TRANSLATE_NOOP3("popup_window", "Cannot add %1 contacts", "5");
                    titleForTwentyOneContacts = QT_TRANSLATE_NOOP3("popup_window", "Cannot add %1 contacts", "21");
                }
                else if (_operation == Utils::ChatMembersOperation::Remove)
                {
                    titleForOneContact = QT_TRANSLATE_NOOP3("popup_window", "Cannot remove %1 contact", "1");
                    titleForTwoContacts = QT_TRANSLATE_NOOP3("popup_window", "Cannot remove %1 contacts", "2");
                    titleForFiveContacts = QT_TRANSLATE_NOOP3("popup_window", "Cannot remove %1 contacts", "5");
                    titleForTwentyOneContacts = QT_TRANSLATE_NOOP3("popup_window", "Cannot remove %1 contacts", "21");
                }
                else if (_operation == Utils::ChatMembersOperation::Block)
                {
                    titleForOneContact = QT_TRANSLATE_NOOP3("popup_window", "Cannot block %1 contact", "1");
                    titleForTwoContacts = QT_TRANSLATE_NOOP3("popup_window", "Cannot block %1 contacts", "2");
                    titleForFiveContacts = QT_TRANSLATE_NOOP3("popup_window", "Cannot block %1 contacts", "5");
                    titleForTwentyOneContacts = QT_TRANSLATE_NOOP3("popup_window", "Cannot block %1 contacts", "21");
                }
                else if (_operation == Utils::ChatMembersOperation::Unblock)
                {
                    titleForOneContact = QT_TRANSLATE_NOOP3("popup_window", "Cannot unblock %1 contact", "1");
                    titleForTwoContacts = QT_TRANSLATE_NOOP3("popup_window", "Cannot unblock %1 contacts", "2");
                    titleForFiveContacts = QT_TRANSLATE_NOOP3("popup_window", "Cannot unblock %1 contacts", "5");
                    titleForTwentyOneContacts = QT_TRANSLATE_NOOP3("popup_window", "Cannot unblock %1 contacts", "21");
                }

                if (titleForOneContact.isEmpty() || titleForTwoContacts.isEmpty() || titleForFiveContacts.isEmpty() || titleForTwentyOneContacts.isEmpty())
                    return;

                const auto title = Utils::GetTranslator()->getNumberString(
                    totalCount,
                    titleForOneContact,
                    titleForTwoContacts,
                    titleForFiveContacts,
                    titleForTwentyOneContacts
                ).arg(totalCount);

                Utils::GetConfirmationWithOneButton(
                    QT_TRANSLATE_NOOP("popup_window", "OK"),
                    message,
                    title,
                    nullptr);
            }
        }
    }

    QString MainPage::getSidebarSelectedText() const //TODO FIXME maybe will not work for threads in sidebar
    {
        if (sidebar_)
            return sidebar_->getSelectedText();
        return QString();
    }

    void MainPage::setOpenedAs(PageOpenedAs _openedAs)
    {
        openedAs_ = _openedAs;
        recentsTab_->setPageOpenedAs(_openedAs);
        contactDialog_->setPageOpenedAs(_openedAs);
        if (!isOpenedAsMain())
        {
            if (!isOneFrameMode() || (isOneFrameMode() && !isSearchActive()))
                clContainer_->hide();
            else
                clContainer_->show();
        }
        onSizeChanged();
    }

    void MainPage::onSizeChanged(QSize _oldSize, bool _needUpdateFocus)
    {
        if (const int newWidth = width(); width() != _oldSize.width())
        {
            if ((!isManualRecentsMiniMode_ && newWidth < getFrame2MinWidth())
                || (isManualRecentsMiniMode_ && newWidth < getFrame2MiniModeMinWidth()))
            {
                if (newWidth <= Sidebar::getDefaultMaximumWidth() && frameCountMode_ != FrameCountMode::_1)
                    sidebar_->saveWidth();
                setFrameCountMode(FrameCountMode::_1);

                for_each([width = getFrame2MinWidth() - 1](auto w) { w->setMaximumWidth(width); }, clContainer_);
                for_each([width = pageMinWidth()](auto w) { w->setMinimumWidth(width); }, clContainer_);
                sidebar_->setWidth(newWidth);

                switch (oneFrameMode_)
                {
                case OneFrameMode::Tab:
                    switchToTabFrame();
                    break;
                case OneFrameMode::Content:
                    switchToContentFrame();
                    break;
                case OneFrameMode::Sidebar:
                    switchToSidebarFrame();
                    break;
                }
            }
            else
            {
                if (!isManualRecentsMiniMode_)
                {
                    for_each([width = getRecentsMaxWidth()](auto w) { w->setMaximumWidth(width); }, clContainer_);
                    for_each([width = getRecentsMinWidth()](auto w) { w->setMinimumWidth(width); }, clContainer_);
                }
                else
                {
                    for_each([width = getRecentMiniModeWidth()](auto w) { w->setFixedWidth(width); }, clContainer_);
                }

                const auto has3FrameWidth = newWidth >= getFrame3MinWidth() || (!isOpenedAsMain() && !clContainer_->isVisible());
                setFrameCountMode((sidebar_->isVisible() || sidebar_->wasVisible()) && isSideBarInSplitter() && has3FrameWidth ? FrameCountMode::_3 : FrameCountMode::_2);
                sidebar_->restoreWidth();
                Q_EMIT Utils::InterConnector::instance().currentPageChanged();

                if (pages_->currentWidget() == contactDialog_ && getCurrentTab() == TabType::Settings)
                    selectSettings(settingsTab_->currentSettingsItem());
                pagesContainer_->show();
                if (canShowRecents())
                    clContainer_->show();
            }

            if (leftPanelState_ == LeftPanelState::spreaded
                || (isPictureOnlyView() != (leftPanelState_ == LeftPanelState::picture_only)))
            {
                setLeftPanelState(isPictureOnlyView() ? LeftPanelState::picture_only : LeftPanelState::normal, false);
            }
        }

        sidebar_->updateSize();

        updateLoaderOverlayPosition();

        semiWindow_->setFixedSize(size());

        if (width() != _oldSize.width())
        {
            updateSidebarPos(width());
            updateSplitterStretchFactors();
        }

        saveSplitterState();

        if (_needUpdateFocus)
        {
            QPointer<QWidget> prevFocused = qApp->focusWidget();
            QMetaObject::invokeMethod(this, [prevFocused, this]() { resizeUpdateFocus(prevFocused); }, Qt::QueuedConnection);
        }
    }

    void MainPage::onItemSelected(const QString& _aimId, qint64 _messageId, qint64, const highlightsV& _highlights, bool _ignoreScroll)
    {
        contactDialog_->onContactSelected(_aimId, _messageId, _highlights, _ignoreScroll);
    }

    void MainPage::resizeEvent(QResizeEvent* _event)
    {
        onSizeChanged(_event->oldSize());
    }

    bool MainPage::eventFilter(QObject* _obj, QEvent* _event)
    {
        if (_obj == clContainer_ && _event->type() == QEvent::Resize)
        {
            QResizeEvent *resizeEvent = static_cast<QResizeEvent*>(_event);
            setCLWidthNew(resizeEvent->size().width());
            updateSplitterStretchFactors();
            saveSplitterState();
            return true;
        }
        return QWidget::eventFilter(_obj, _event);
    }

    void MainPage::showSemiWindow()
    {
        semiWindow_->setFixedSize(size());
        semiWindow_->raise();
        semiWindow_->showAnimated();
    }

    void MainPage::hideSemiWindow()
    {
        semiWindow_->forceHide();
    }

    bool MainPage::isSemiWindowVisible() const
    {
        return semiWindow_->isSemiWindowVisible();
    }

    bool MainPage::isSearchActive() const
    {
        return topWidget_->getSearchWidget()->isActive();
    }

    bool MainPage::isNavigatingInRecents() const
    {
        return recentsTab_->isKeyboardFocused();
    }

    void MainPage::stopNavigatingInRecents()
    {
        return recentsTab_->dropKeyboardFocus();
    }

    void MainPage::setSearchFocus()
    {
        if (canSetSearchFocus())
        {
            if (isPictureOnlyView())
                setLeftPanelState(LeftPanelState::spreaded, true, true);

            topWidget_->getSearchWidget()->setFocus();
        }
    }

    void MainPage::setSearchTabFocus()
    {
        if (canSetSearchFocus())
            topWidget_->getSearchWidget()->setTabFocus();
    }

    bool MainPage::hasSearchFocus() const
    {
        return topWidget_->getSearchWidget()->hasFocus();
    }

    void MainPage::onProfileSettingsShow(const QString& _uin)
    {
        if (!isVisible())
            return;

        hideRecentsPopup();
        if (!_uin.isEmpty())
        {
            showSidebar(_uin);
            return;
        }

        if (!profilePage_)
        {
            profilePage_ = new MyProfilePage(this);
            Testing::setAccessibleName(profilePage_, qsl("AS ProfilePage"));
            pages_->addWidget(profilePage_);
        }

        profilePage_->initFor(_uin);
        pages_->push(profilePage_);

        if (isOneFrameMode() && getCurrentTab() == TabType::Settings)
            switchToContentFrame();
    }

    void MainPage::onSharedProfileShow(const QString& _uin)
    {
        hideRecentsPopup();
        showSidebar(_uin);
    }

#ifndef STRIP_VOIP
    void MainPage::raiseVideoWindow(const voip_call_type _call_type)
    {
        bool create_new_window = !videoWindow_;

        if (create_new_window)
        {
            if (_call_type == voip_call_type::audio_call)
                statistic::getGuiMetrics().eventAudiocallStart();
            else
                statistic::getGuiMetrics().eventVideocallStart();

            videoWindow_ = new(std::nothrow) VideoWindow();
        }

        if (!!videoWindow_ && !videoWindow_->isHidden())
        {
            videoWindow_->showNormal();
            videoWindow_->activateWindow();
        }

        if (create_new_window)
        {
            if (_call_type == voip_call_type::audio_call)
                statistic::getGuiMetrics().eventAudiocallWindowCreated();
            else
                statistic::getGuiMetrics().eventVideocallWindowCreated();
        }
        Utils::InterConnector::instance().getMainWindow()->updateMainMenu();
    }
#endif

    void MainPage::nextChat()
    {
        if (const auto& sel = Logic::getContactListModel()->selectedContact(); !sel.isEmpty())
        {
            if (recentsTab_->currentTab() == RECENTS && recentsTab_->isRecentsOpen())
            {
                if (const auto nextContact = Logic::getRecentsModel()->nextAimId(sel); !nextContact.isEmpty())
                {
                    recentsTab_->setNextSelectWithOffset();
                    Q_EMIT Logic::getContactListModel()->select(nextContact, -1);
                }
            }
        }
    }

    void MainPage::prevChat()
    {
        if (const auto& sel = Logic::getContactListModel()->selectedContact(); !sel.isEmpty())
        {
            if (recentsTab_->currentTab() == RECENTS && recentsTab_->isRecentsOpen())
            {
                if (const auto prevContact = Logic::getRecentsModel()->prevAimId(sel); !prevContact.isEmpty())
                {
                    recentsTab_->setNextSelectWithOffset();
                    Q_EMIT Logic::getContactListModel()->select(prevContact, -1);
                }
            }
        }
    }

    void MainPage::saveSidebarState()
    {
        sidebarVisibilityOnRecentsFrame_ = isSidebarVisible();
    }

    void MainPage::restoreSidebarState()
    {
        SidebarStateFlags flags { SidebarStateFlag::Closed };
        flags.setFlag(SidebarStateFlag::Opened, sidebarVisibilityOnRecentsFrame_);
        setSidebarState(flags);
    }

    void MainPage::onGeneralSettingsShow(int _type)
    {
        if (isOneFrameMode() && getCurrentTab() == TabType::Settings)
            switchToContentFrame();

        generalSettings_->setType(_type);
        pages_->push(generalSettings_);
    }

    void MainPage::clearSearchMembers()
    {
        recentsTab_->update();
    }

    void MainPage::cancelSelection()
    {
        im_assert(contactDialog_);
        contactDialog_->cancelSelection();
    }

    void MainPage::createGroupChat(const CreateChatSource _source)
    {
        searchEnd();
        openRecents();

        Ui::createGroupChat({}, _source);

        hideSemiWindow();
    }

    void MainPage::createChannel(const CreateChatSource _source)
    {
        searchEnd();
        openRecents();

        Ui::createChannel(_source);

        hideSemiWindow();
    }

    void MainPage::dialogBackClicked(const bool _needHighlightInRecents)
    {
        if (isOneFrameMode())
        {
            if (_needHighlightInRecents)
            {
                closeAndHighlightDialog();
            }
            else
            {
                window()->setFocus();
                Utils::InterConnector::instance().closeDialog();
            }
            switchToTabFrame();
        }
    }

    void MainPage::sidebarBackClicked()
    {
        if (isOneFrameMode())
        {
            if (oneFrameMode_ == OneFrameMode::Content)
                switchToContentFrame();
            else
                switchToTabFrame();
        }
    }

    ContactDialog* MainPage::getContactDialog() const
    {
        im_assert(contactDialog_);
        return contactDialog_;
    }

    VideoWindow* Ui::MainPage::getVideoWindow() const
    {
        return videoWindow_;
    }

    static bool canOpenSidebarOutsideWithoutMove(int _sidebarWidth)
    {
        const auto availableGeometry = Utils::InterConnector::instance().getMainWindow()->screenGeometry();
        const auto& mainWindowGeometry = Utils::InterConnector::instance().getMainWindow()->geometry();

        return (availableGeometry.width() - mainWindowGeometry.topRight().x()) >= _sidebarWidth;
    }

    static bool canOpenSidebarOutsideWithMove(int _sidebarWidth)
    {
        const auto availableGeometry = Utils::InterConnector::instance().getMainWindow()->screenGeometry();
        const auto& mainWindowGeometry = Utils::InterConnector::instance().getMainWindow()->geometry();

        return (availableGeometry.width() - mainWindowGeometry.width()) >= _sidebarWidth;
    }

    void MainPage::showSidebar(const QString& _aimId, bool _selectionChanged)
    {
        showSidebarWithParams(_aimId, { true, frameCountMode_ }, _selectionChanged);
    }

    void MainPage::showSidebarWithParams(const QString& _aimId, SidebarParams _params, bool _selectionChanged, bool _animate)
    {
        Utils::InterConnector::instance().setSidebarVisible(true);
        updateSidebarPos(width());
        sidebar_->preparePage(_aimId, _params, _selectionChanged);
        SidebarStateFlags flags(SidebarStateFlag::Opened);
        flags.setFlag(SidebarStateFlag::Animated, _animate);
        setSidebarState(flags);
        if (isOneFrameMode())
        {
            clContainer_->hide();
            pagesContainer_->hide();
        }
        setOneFrameMode(OneFrameMode::Sidebar);
        Q_EMIT Utils::InterConnector::instance().currentPageChanged();
        Q_EMIT Utils::InterConnector::instance().stopPttRecord();
    }

    void MainPage::showMembersInSidebar(const QString& _aimId)
    {
        updateSidebarPos(width());
        sidebar_->showMembers(_aimId);
        if (isOneFrameMode())
        {
            clContainer_->hide();
            pagesContainer_->hide();
        }
        setOneFrameMode(OneFrameMode::Sidebar);
        Q_EMIT Utils::InterConnector::instance().currentPageChanged();
        Q_EMIT Utils::InterConnector::instance().setFocusOnInput();
        Q_EMIT Utils::InterConnector::instance().stopPttRecord();

    }

    bool MainPage::isSidebarVisible() const
    {
        return sidebar_->isVisible();
    }

    void MainPage::onSidebarVisibilityChanged(bool _visible)
    {
        if (isVisible())
        {
            SidebarStateFlags flags { SidebarStateFlag::CloseAnimated };
            flags.setFlag(SidebarStateFlag::Opened, _visible);
            setSidebarState(flags);
        }
    }

    void MainPage::setSidebarState(SidebarStateFlags _flags)
    {
        if ((_flags & SidebarStateFlag::KeepState) && pages_->currentWidget() == contactDialog_)
            sidebarVisibilityOnRecentsFrame_ = isSidebarVisible();

        const bool isSBVisible = _flags & SidebarStateFlag::Opened;

        if (isVisible() && !isSBVisible && oneFrameMode_ == OneFrameMode::Sidebar)
        {
            setOneFrameMode(prevOneFrameMode_);
            sidebarBackClicked();
            contactDialog_->escCancel_->removeChild(sidebar_);
        }

        if (sidebar_->isVisible() == isSBVisible)
            return;

        if (isSBVisible)
        {
            if (frameCountMode_ != FrameCountMode::_1)
                sidebar_->saveWidth();

            if (isOneFrameMode())
            {
                sidebar_->setWidth(std::min(width(), getFrame2MinWidth()));
            }
            else
            {
                sidebar_->restoreWidth();
                if (width() >= getRecentsMinWidth() + pageMinWidth() + sidebar_->getCurrentWidth())
                    insertSidebarToSplitter();
                else
                    updateSidebarPos(width());
            }

            if (isOneFrameMode() || !_flags.testFlag(SidebarStateFlag::Animated))
                sidebar_->show();
            else
                sidebar_->showAnimated();

            if (frameCountMode_ == FrameCountMode::_2 && isSideBarInSplitter())
                setFrameCountMode(FrameCountMode::_3);

            setOneFrameMode(OneFrameMode::Sidebar);

            contactDialog_->escCancel_->addChild(sidebar_);
        }
        else
        {
            if (frameCountMode_ == FrameCountMode::_3)
                setFrameCountMode(FrameCountMode::_2);

            sidebar_->saveWidth();
            removeSidebarPlaceholder();

            if (isOneFrameMode() || !_flags.testFlag(SidebarStateFlag::Animated))
                sidebar_->hide();
            else
                sidebar_->hideAnimated();

            splitter_->refresh();

            contactDialog_->escCancel_->removeChild(sidebar_);

            Q_EMIT Utils::InterConnector::instance().setFocusOnInput();
        }

        updateSplitterHandle(width());
    }

    void MainPage::restoreSidebar()
    {
        const auto cont = Logic::getContactListModel()->selectedContact();
        if (sidebar_->wasVisible() && !cont.isEmpty())
            showSidebar(cont);
        else
            setSidebarState(SidebarStateFlag::CloseAnimated);
    }

    int MainPage::getSidebarWidth() const
    {
        return (frameCountMode_ == FrameCountMode::_3 && isSidebarVisible()) ? sidebar_->width() : 0;
    }

    QString MainPage::getSidebarAimid(Sidebar::ResolveThread _resolve) const
    {
        return sidebar_->currentAimId(_resolve);
    }

    bool MainPage::isSidebarWithThreadPage() const
    {
        return sidebar_->isThreadOpen();
    }

    bool MainPage::isSidebarInFocus(bool _checkSearchFocus) const
    {
        return sidebar_->hasInputOrHistoryFocus() || (_checkSearchFocus && sidebar_->hasSearchFocus());
    }

    bool MainPage::isContactDialog() const
    {
        if (!pages_)
            return false;

        return (frameCountMode_ != FrameCountMode::_1 || oneFrameMode_ == OneFrameMode::Content) && pages_->currentWidget() == contactDialog_;
    }

    bool Ui::MainPage::isSearchInDialog() const
    {
        if (!recentsTab_)
            return false;

        return recentsTab_->getSearchInDialog();
    }

    void MainPage::onContactSelected(const QString& _contact)
    {
        setOneFrameMode(OneFrameMode::Content);
        if (isOneFrameMode())
        {
            pagesContainer_->show();
            clContainer_->hide();
        }
        pages_->poproot();
        settingsHeader_->hide();

        hideRecentsPopup();

        if (const auto tab = getCurrentTab(); tab != TabType::Contacts && tab != TabType::Recents)
            selectRecentsTab();

        if (!_contact.isEmpty() && !ServiceContacts::isServiceContact(_contact) && frameCountMode_ == FrameCountMode::_3 && sidebar_->isVisible() && isSideBarInSplitter())
        {
            Q_EMIT Utils::InterConnector::instance().closeAnySemitransparentWindow({ Utils::CloseWindowInfo::Initiator::Unknown, Utils::CloseWindowInfo::Reason::Keep_Sidebar });

            if (!sidebar_->isThreadOpen() || sidebar_->parentAimId() != _contact)
                showSidebar(_contact, true);
        }
        else
        {
            Utils::CloseWindowInfo info;
            if (_contact == ServiceContacts::getThreadsName())
                info.reason_ = Utils::CloseWindowInfo::Reason::Keep_Sidebar;
            Q_EMIT Utils::InterConnector::instance().closeAnySemitransparentWindow(info);
        }

        Q_EMIT Utils::InterConnector::instance().hideSearchDropdown();
    }

    void MainPage::onMessageSent()
    {
        if (!isUnknownsOpen() && fastDropSearchEnabled())
            openRecents();
    }

    void MainPage::showStickersStore()
    {
        if (!stickersStore_)
        {
            stickersStore_ = new Stickers::Store(this);
            connect(stickersStore_, &Stickers::Store::back, this, [this]()
            {
                if (settingsHeader_)
                    settingsHeader_->hide();
                if (pages_)
                    pages_->poproot();
            });
            Testing::setAccessibleName(stickersStore_, qsl("AS StickersPage"));
            if (pages_)
                pages_->addWidget(stickersStore_);
        }
        if (stickersStore_)
        {
            const auto unreadCounters = Logic::getUnreadCounters();
            stickersStore_->setBadgeText(Utils::getUnreadsBadgeStr(unreadCounters.unreadMessages_));
            stickersStore_->setFrameCountMode(frameCountMode_);
            if (pages_)
                pages_->push(stickersStore_);
            stickersStore_->hideSearch();
            if (settingsHeader_)
                settingsHeader_->hide();
        }
    }

    void MainPage::onSearchEnd()
    {
        if (!isUnknownsOpen())
            openRecents();
    }

    void MainPage::chatUnreadChanged()
    {
        const auto unreadCounters = Logic::getUnreadCounters();
        const auto unreadMessagesStr = Utils::getUnreadsBadgeStr(unreadCounters.unreadMessages_);
        if (tabWidget_)
            tabWidget_->updateItemInfo(getIndexByTab(TabType::Recents),
                                       unreadMessagesStr,
                                       unreadCounters.attention_ ? qsl(":/tab/attention") : QString());

        if (auto newContacts = recentsHeaderButtons_.newContacts_)
            newContacts->setBadgeText(Utils::getUnreadsBadgeStr(unreadCounters.unknownUnreads_));

        if (stickersStore_)
            stickersStore_->setBadgeText(unreadMessagesStr);

        if (auto mainWindow = Utils::InterConnector::instance().getMainWindow())
            mainWindow->updateWindowTitle();
    }

    void MainPage::expandButtonClicked()
    {
        expandButton_->hide();
        isManualRecentsMiniMode_ = false;
        setLeftPanelState(LeftPanelState::normal, false);
        const auto w = width();
        if (w < getFrame2MinWidth())
        {
            setFrameCountMode(FrameCountMode::_1);
            switchToTabFrame();

            for_each([width = getFrame2MinWidth() - 1](auto w) { w->setMaximumWidth(width); }, clContainer_);
            for_each([width = pageMinWidth()](auto w) { w->setMinimumWidth(width); }, clContainer_);
        }
        else
        {
            for_each([width = getRecentsMaxWidth()](auto w) { w->setMaximumWidth(width); }, clContainer_);
            for_each([width = getRecentsMinWidth()](auto w) { w->setMinimumWidth(width); }, clContainer_);

            if (sidebar_->isVisible() && w < getFrame3MinWidth())
                setSidebarState(SidebarStateFlag::CloseAnimated);

        }
        updateSplitterStretchFactors();
    }

    void MainPage::onDialogClosed(const QString&, bool _isCurrent)
    {
        if (_isCurrent && isOneFrameMode() && oneFrameMode_ != OneFrameMode::Tab)
            switchToTabFrame();
    }

    void MainPage::onSelectedContactChanged(const QString& _contact)
    {
        if (_contact.isEmpty())
        {
            if (isOneFrameMode() && oneFrameMode_ != OneFrameMode::Tab)
                switchToTabFrame();
            else
                setOneFrameMode(OneFrameMode::Tab);
        }
    }

    void MainPage::onContactRemoved(const QString& _contact)
    {
        if (!_contact.isEmpty() && _contact == recentsTab_->getSearchInDialogContact())
            searchCancelled();
    }

    void MainPage::addContactClicked()
    {
        hideRecentsPopup();
        if (leftPanelState_ == LeftPanelState::normal)
            openRecents();
    }

    void MainPage::readAllClicked()
    {
        const auto [readChats, readCnt] = Logic::getRecentsModel()->markAllRead();
        if (readChats || readCnt)
        {
            core::stats::event_props_type props = {
            { "unread_chats", std::to_string(readChats) },
            { "unread_messages", std::to_string(readCnt) }
            };

            Ui::GetDispatcher()->post_stats_to_core(core::stats::stats_event_names::mark_read_all, props);
        }
    }

    void MainPage::showLoaderOverlay()
    {
        if (!dialogIdLoader_.loaderOverlay_)
        {
            dialogIdLoader_.loaderOverlay_ = new LoaderOverlay(this);
            connect(dialogIdLoader_.loaderOverlay_, &LoaderOverlay::cancelled, this, &MainPage::onIdLoadingCancelled);
        }

        updateLoaderOverlayPosition();
        dialogIdLoader_.loaderOverlay_->raise();
        dialogIdLoader_.loaderOverlay_->show();
    }

    void MainPage::hideLoaderOverlay()
    {
        if (dialogIdLoader_.loaderOverlay_)
            dialogIdLoader_.loaderOverlay_->hide();
    }

    void MainPage::onJoinChatResultBlocked(const QString& _stamp)
    {
        const auto friendly = Logic::GetFriendlyContainer()->getFriendly(Logic::getContactListModel()->getAimidByStamp(_stamp));
        const auto caption = QT_TRANSLATE_NOOP("popup_window", "%1 is now unavailable").arg(friendly);

        Utils::GetConfirmationWithOneButton(QT_TRANSLATE_NOOP("popup_window", "OK"),
                                            QT_TRANSLATE_NOOP("popup_window", "You were blocked by admins"),
                                            caption,
                                            nullptr);
    }

    void MainPage::openThread(const QString& _threadAimId, int64_t _msgId, const QString& _fromPage)
    {
        Q_UNUSED(_msgId);
        if (_threadAimId.isEmpty())
            return;

        if (_fromPage != ServiceContacts::contactId(ServiceContacts::ContactType::ThreadsFeed))
        {
            if (Logic::getContactListModel()->areYouNotAMember(_fromPage))
            {
                Utils::showTextToastOverContactDialog(Logic::getContactListModel()->isChannel(_fromPage)
                    ? QT_TRANSLATE_NOOP("popup_window", "Only channel subscribers can view replies")
                    : QT_TRANSLATE_NOOP("popup_window", "Only chat members can view replies"));
                return;
            }
        }

        hideRecentsPopup();

        SidebarParams params;
        params.parentPageId_ = _fromPage;
        params.frameCountMode_ = frameCountMode_;
        threadParentPageId_ = params.parentPageId_;
        showSidebarWithParams(Sidebar::getThreadPrefix() % _threadAimId, params);

        Q_EMIT Utils::InterConnector::instance().setFocusOnInput(_threadAimId);
    }

    void MainPage::closeThreadInfo(const QString& _threadId)
    {
        SidebarParams params;
        params.parentPageId_ = threadParentPageId_;
        params.frameCountMode_ = frameCountMode_;
        showSidebarWithParams(Sidebar::getThreadPrefix() % _threadId, params, false, false);
    }

    void MainPage::openContacts()
    {
        openRecents();

        SelectContactsWidget contacts(nullptr,
            Logic::MembersWidgetRegim::CONTACT_LIST_POPUP,
            QT_TRANSLATE_NOOP("popup_window", "Contacts"),
            QString(),
            MainPage::instance(),
            SelectContactsWidgetOptions(),
            Logic::DrawIcons::NeedDrawIcons);
        Logic::updatePlaceholders();
        contacts.show();

        hideSemiWindow();
    }

    void MainPage::settingsClicked()
    {
        settingsTab_->selectSettings(Utils::CommonSettingsType::CommonSettingsType_General);

        Q_EMIT Utils::InterConnector::instance().generalSettingsShow((int)Utils::CommonSettingsType::CommonSettingsType_General);
        Q_EMIT Utils::InterConnector::instance().showSettingsHeader(QT_TRANSLATE_NOOP("main_page", "General settings"));
        selectTab(TabType::Settings);
    }

    void MainPage::searchBegin()
    {
        Q_EMIT Utils::InterConnector::instance().hideSearchDropdown();
        recentsTab_->setSearchMode(true);
    }

    void MainPage::searchEnd()
    {
        if (NeedShowUnknownsHeader_)
            topWidget_->setRegime(TopPanelWidget::Regime::Unknowns);

        topWidget_->getSearchWidget()->clearInput();
        topWidget_->getSearchWidget()->setDefaultPlaceholder();
        clearSearchFocus();

        Q_EMIT Utils::InterConnector::instance().disableSearchInDialog();

        if (platform::is_linux() && leftPanelState_ != LeftPanelState::picture_only)
            Q_EMIT Utils::InterConnector::instance().updateTitleButtons();

        recentsTab_->setSearchMode(false);
        recentsTab_->setSearchInAllDialogs();

        if (isOneFrameMode())
        {
            switch (prevOneFrameMode_)
            {
            case OneFrameMode::Tab:
                switchToTabFrame();
                prevOneFrameMode_ = OneFrameMode::Tab;
                break;
            case OneFrameMode::Content:
                switchToContentFrame();
                prevOneFrameMode_ = OneFrameMode::Tab;
                break;
            case OneFrameMode::Sidebar:
                prevOneFrameMode_ = OneFrameMode::Tab;
                switchToSidebarFrame();
                break;
            default:
                break;
            }
        }

        Q_EMIT Utils::InterConnector::instance().setFocusOnInput();
    }

    void MainPage::searchCancelled()
    {
        if (isSearchInDialog())
            GetDispatcher()->post_stats_to_core(core::stats::stats_event_names::chat_search_dialog_cancel, { { "type", "outside" } });

        Q_EMIT Utils::InterConnector::instance().searchEnd();

        hideRecentsPopup();
    }

    void MainPage::onSearchInputEmpty()
    {
        if (!isSearchInDialog())
        {
            if (topWidget_->getSearchWidget()->hasFocus())
                recentsTab_->switchToContactListWithHeaders(leftPanelState_ == LeftPanelState::normal ? RecentsTab::SwichType::Animated : RecentsTab::SwichType::Simple);

            recentsTab_->setSearchMode(false);
        }
    }

#ifndef STRIP_VOIP
    void MainPage::onVoipCallIncoming(const std::string& call_id, const std::string& _contact, const std::string& call_type)
    {
        im_assert(!call_id.empty() && !_contact.empty());
        if (!call_id.empty() && !_contact.empty())
        {
            auto it = incomingCallWindows_.find(call_id);
            if (incomingCallWindows_.end() == it || !it->second)
            {
                std::shared_ptr<IncomingCallWindow> window(new(std::nothrow) IncomingCallWindow(call_id, _contact, call_type));
                im_assert(!!window);
                if (!!window)
                {
                    window->showFrame();
                    incomingCallWindows_[call_id] = window;
                }
            }
            else
            {
                std::shared_ptr<IncomingCallWindow> wnd = it->second;
                wnd->showFrame();
            }

            Utils::InterConnector::instance().getMainWindow()->closeGallery();
        }
        Utils::InterConnector::instance().getMainWindow()->updateMainMenu();
    }

    void MainPage::destroyIncomingCallWindow(const std::string& _callId)
    {
        im_assert(!_callId.empty());
        if (!_callId.empty())
        {
            auto it = incomingCallWindows_.find(_callId);
            if (incomingCallWindows_.end() != it)
            {
                auto window = it->second;
                im_assert(!!window);
                if (!!window)
                {
                    window->hideFrame();
                    window->close();
                    it->second.reset();
                }
                incomingCallWindows_.erase(_callId);
            }
        }
        Utils::InterConnector::instance().getMainWindow()->updateMainMenu();
        update();
    }

    void MainPage::onVoipCallIncomingAccepted(const std::string& _callId)
    {
        //destroy exactly when accept button pressed
        destroyIncomingCallWindow(_callId);
        Utils::InterConnector::instance().getMainWindow()->closeGallery();
    }

    void MainPage::onVoipCallDestroyed(const voip_manager::ContactEx& _contactEx)
    {
        destroyIncomingCallWindow(_contactEx.contact.call_id);
    }

    void MainPage::onVoipShowVideoWindow(bool _enabled)
    {
        if (!videoWindow_)
        {
            videoWindow_ = new(std::nothrow) VideoWindow();
            Ui::GetDispatcher()->getVoipController().updateActivePeerList();
        }

        if (_enabled)
        {
            videoWindow_->showFrame();
        }
        else
        {
            videoWindow_->hideFrame();

            bool wndMinimized = false;
            bool wndHiden = false;
            if (QWidget* parentWnd = window())
            {
                wndHiden = !parentWnd->isVisible();
                wndMinimized = parentWnd->isMinimized();
            }

            if (!Utils::foregroundWndIsFullscreened() && !wndMinimized && !wndHiden)
                raise();
        }

        Utils::InterConnector::instance().getMainWindow()->updateMainMenu();
    }
#endif

    void MainPage::showVideoWindow()
    {
#ifndef STRIP_VOIP
        if (!videoWindow_)
            return;

        if (videoWindow_->isMinimized() || videoWindow_->isMaximized())
        {
            videoWindow_->showNormal();
        }

        videoWindow_->activateWindow();

        if constexpr (!platform::is_windows())
        {
            videoWindow_->raise();
        }
#endif
        Utils::InterConnector::instance().getMainWindow()->updateMainMenu();
    }


    void MainPage::minimizeVideoWindow()
    {
#ifndef STRIP_VOIP
        if (!videoWindow_)
            return;

        videoWindow_->showMinimized();
#endif
    }

    void MainPage::notifyApplicationWindowActive(const bool isActive)
    {
        if (contactDialog_)
            contactDialog_->notifyApplicationWindowActive(isActive);
    }

    void MainPage::notifyUIActive(const bool _isActive)
    {
        if (contactDialog_)
            contactDialog_->notifyUIActive(_isActive);
    }

    void MainPage::recentsTabActivate(bool _selectUnread)
    {
        im_assert(!!recentsTab_);
        if (recentsTab_)
        {
            recentsTab_->changeTab(RECENTS);

            if (_selectUnread)
            {
                QString aimId = Logic::getRecentsModel()->nextUnreadAimId(Logic::getContactListModel()->selectedContact());
                if (aimId.isEmpty())
                    aimId = Logic::getUnknownsModel()->nextUnreadAimId();
                if (!aimId.isEmpty())
                {
                    recentsTab_->select(aimId);
                    recentsTab_->scrollToItem(aimId, QAbstractItemView::ScrollHint::PositionAtCenter);
                }
            }
        }
    }

    void MainPage::selectRecentsTab(bool _needUpdateFocus)
    {
        selectTab(TabType::Recents, _needUpdateFocus);
    }

    void MainPage::selectRecentChat(const QString& _aimId)
    {
        im_assert(recentsTab_);
        if (recentsTab_ && !_aimId.isEmpty())
            recentsTab_->select(_aimId);
    }

    void MainPage::settingsTabActivate(Utils::CommonSettingsType _item)
    {
        Utils::InterConnector::instance().getAppsPage()->onSettingsClicked();

        selectSettings(_item);

        if (isOneFrameMode())
            switchToContentFrame();
    }

    void MainPage::onSwitchedToEmpty()
    {
        setSidebarState(SidebarStateFlag::CloseAnimated);
        contactDialog_->onSwitchedToEmpty();
    }

    void MainPage::post_stats_with_settings()
    {
        Utils::getStatsSender()->trySendStats();
    }

    void MainPage::openCreatedGroupChat()
    {
        auto conn = std::make_shared<QMetaObject::Connection>();
        *conn = connect(GetDispatcher(), &core_dispatcher::openChat, this, [this, conn](const QString& _aimId)
        {
            QTimer::singleShot(500, this, [this, id = _aimId]() { recentsTab_->select(id); });
            disconnect(*conn);
        });
    }

    void MainPage::updateSidebarPos(int _width)
    {
        if (clContainer_->isVisible())
            sidebarPosWithRecents(_width);
        else
            sidebarPosWithoutRecents(_width);
    }

    void MainPage::updateSplitterHandle(int _width)
    {
        if (auto handle = splitter_->handle(1))
        {
            const bool enableHandle = (_width > getFrame2MinWidth() && !sidebar_->isVisible())
                || (_width > (getFrame2MinWidth() + Sidebar::getDefaultWidth()) && sidebar_->isVisible());
            handle->setEnabled(enableHandle);
        }
    }

    bool MainPage::isSideBarInSplitter() const
    {
        return splitter_->indexOf(sidebar_) != -1;
    }

    void MainPage::insertSidebarToSplitter()
    {
        sidebar_->setNeedShadow(false);

        if (frameCountMode_ != FrameCountMode::_2)
            removeSidebarPlaceholder();

        if (!isSideBarInSplitter())
        {
            Testing::setAccessibleName(sidebar_, qsl("AS Sidebar"));
            splitter_->addWidget(sidebar_);
        }
    }

    void MainPage::takeSidebarFromSplitter(int _widthHint)
    {
        if (sidebar_->isVisible())
        {
            if (!sidebarPlaceholderWidget_)
                sidebarPlaceholderWidget_ = new QWidget();

            sidebarPlaceholderWidget_->setFixedWidth(_widthHint);

            if (splitter_->indexOf(sidebarPlaceholderWidget_) == -1)
                splitter_->addWidget(sidebarPlaceholderWidget_);
        }
        sidebar_->setNeedShadow(true);
    }

    void MainPage::removeSidebarPlaceholder()
    {
        if (sidebarPlaceholderWidget_)
        {
            sidebarPlaceholderWidget_->hide();
            sidebarPlaceholderWidget_->deleteLater();
            sidebarPlaceholderWidget_ = nullptr;
        }
    }

    void MainPage::sidebarPosWithRecents(int _width)
    {
        const auto updateSidebarPlaceholder = [this, _width]()
        {
            if (sidebarPlaceholderWidget_)
                sidebarPlaceholderWidget_->setFixedWidth(std::max(0, _width - (clContainer_->width() + pageMinWidth())));
        };

        auto prevFocused = qobject_cast<QWidget*>(qApp->focusObject());

        if (_width >= getFrame3MinWidth() || (clContainer_->width() != getRecentMiniModeWidth() && _width < getFrame2MinWidth()) ||
            (clContainer_->width() == getRecentMiniModeWidth() && _width < getFrame2MiniModeMinWidth()))
        {
            if (frameCountMode_ != FrameCountMode::_1 && _width < clContainer_->width() + sidebar_->width() + pageMinWidth())
            {
                if (sidebar_->width() < Sidebar::getDefaultMaximumWidth())
                {
                    sidebar_->saveWidth();
                    takeSidebarFromSplitter(sidebar_->getCurrentWidth() + splitterHandleWidth());
                    updateSidebarPlaceholder();
                }
                else
                {
                    insertSidebarToSplitter();
                    const auto newWidth =
                        std::max(_width - (clContainer_->width() + pageMinWidth() + splitterHandleWidth()), Sidebar::getDefaultMaximumWidth());
                    updateSidebarWidth(newWidth);
                    updateSidebarPlaceholder();
                }
            }
            else
            {
                removeSidebarPlaceholder();
                insertSidebarToSplitter();
            }
        }
        else if (isSideBarInSplitter())
        {
            if (pagesContainer_->width() < (pageMinWidth() + Sidebar::getDefaultWidth() + splitterHandleWidth()))
            {
                const int width = leftPanelState_ == LeftPanelState::picture_only ? sidebar_->getCurrentWidth() + splitterHandleWidth()
                                                                                  : std::max(0, _width - (clContainer_->width() + pageMinWidth()));
                takeSidebarFromSplitter(width);
                if (_width <= Sidebar::getDefaultMaximumWidth())
                {
                    sidebar_->saveWidth();
                    updateSidebarWidth(std::min(sidebar_->width(), _width));
                }
                else
                {
                    sidebar_->restoreWidth();
                }
            }
        }
        else
        {
            if (pagesContainer_->width() >= (pageMinWidth() + sidebar_->getCurrentWidth() + splitterHandleWidth()))
            {
                sidebar_->restoreWidth();
                insertSidebarToSplitter();
            }
            else if (sidebarPlaceholderWidget_ && _width < clContainer_->width() + pageMinWidth() + sidebarPlaceholderWidget_->width())
            {
                updateSidebarPlaceholder();
            }
        }

        resizeUpdateFocus(prevFocused);

        updateSplitterHandle(_width);
    }

    void MainPage::sidebarPosWithoutRecents(int _width)
    {
        const auto updateSidebarPlaceholder = [this, _width]()
        {
            if (sidebarPlaceholderWidget_)
                sidebarPlaceholderWidget_->setFixedWidth(std::max(0, _width - pageMinWidth()));
        };

        if ((_width >= getFrame3MinWidth() || _width < getFrame2MinWidth())
            || (pagesContainer_->width() != pageMinWidth() && _width < getFrame2MinWidth()))
        {
            if (sidebar_->getFrameCountMode() != FrameCountMode::_1 && _width < pageMinWidth() + sidebar_->width() + splitterHandleWidth())
            {
                if (sidebar_->width() < Sidebar::getDefaultMaximumWidth())
                {
                    sidebar_->saveWidth();
                    takeSidebarFromSplitter(sidebar_->getCurrentWidth() + splitterHandleWidth());
                    updateSidebarPlaceholder();
                }
                else
                {
                    insertSidebarToSplitter();
                    const auto newWidth = std::max(_width - (pageMinWidth() + splitterHandleWidth()), Sidebar::getDefaultMaximumWidth());
                    updateSidebarWidth(newWidth);
                    updateSidebarPlaceholder();
                }
            }
            else
            {
                removeSidebarPlaceholder();
                insertSidebarToSplitter();
            }
        }
        else if (isSideBarInSplitter())
        {
            if (pagesContainer_->width() == pageMinWidth() || _width < pageMinWidth() + sidebar_->width() + splitterHandleWidth())
            {
                takeSidebarFromSplitter(std::max(0, _width - pageMinWidth()));
                if (_width <= Sidebar::getDefaultMaximumWidth())
                {
                    sidebar_->saveWidth();
                    updateSidebarWidth(std::min(sidebar_->width(), _width));
                }
                else
                {
                    sidebar_->restoreWidth();
                }
            }
        }
        else
        {
            sidebar_->updateSize();
            if (_width >= pageMinWidth() + sidebar_->width() + splitterHandleWidth())
            {
                sidebar_->restoreWidth();
                removeSidebarPlaceholder();
                insertSidebarToSplitter();
            }
            else
            {
                takeSidebarFromSplitter(sidebar_->getCurrentWidth() + splitterHandleWidth());
                updateSidebarPlaceholder();
            }
        }
        updateSplitterHandle(_width);
    }

    void MainPage::popPagesToRoot()
    {
        pages_->poproot();
    }

    void MainPage::spreadCL()
    {
        if (isPictureOnlyView())
        {
            setLeftPanelState(LeftPanelState::spreaded, true, true);
            topWidget_->getSearchWidget()->setFocus();
        }
    }

    void MainPage::hideRecentsPopup()
    {
        if (leftPanelState_ == LeftPanelState::spreaded && !NeedShowUnknownsHeader_)
            setLeftPanelState(LeftPanelState::picture_only, true);
    }

    void MainPage::setLeftPanelState(LeftPanelState _newState, bool _withAnimation, bool _for_search, bool _force)
    {
        im_assert(_newState > LeftPanelState::min && _newState < LeftPanelState::max);

        if (leftPanelState_ == _newState && !_force)
            return;

        int newPanelWidth = 0;
        leftPanelState_ = _newState;

        topWidget_->setState(_newState);
        if (leftPanelState_ == LeftPanelState::normal)
        {
            clSpacer_->setFixedWidth(0);
            if (splitter_->indexOf(clContainer_) == -1)
            {
                Testing::setAccessibleName(contactsWidget_, qsl("AS RecentsTab"));
                splitter_->replaceWidget(0, clContainer_);
            }

            newPanelWidth = ::Ui::ItemWidth(false, false);
            recentsTab_->setPictureOnlyView(false);
            callsTab_->setPictureOnlyView(false);
            settingsTab_->setCompactMode(false);
            if (Features::isMessengerTabBarVisible())
                tabWidget_->showTabbar();
            if (updaterButton_)
                updaterButton_->show();
            expandButton_->hide();
            spacerBetweenHeaderAndRecents_->show();
            hideSemiWindow();
        }
        else if (leftPanelState_ == LeftPanelState::picture_only)
        {
            newPanelWidth = getRecentMiniModeWidth();
            clSpacer_->setFixedWidth(0);
            if (splitter_->indexOf(clContainer_) == -1)
                splitter_->replaceWidget(0, clContainer_);
            clContainer_->setFixedWidth(newPanelWidth);
            topWidget_->getSearchWidget()->clearInput();
            settingsTab_->setCompactMode(true);
            tabWidget_->hideTabbar();
            if (updaterButton_)
                updaterButton_->hide();
            expandButton_->show();
            spacerBetweenHeaderAndRecents_->hide();

            if (getCurrentTab() == TabType::Recents)
                openRecents();

            if (currentTab_ != RECENTS)
                recentsTab_->changeTab(RECENTS);

            hideSemiWindow();
        }
        else if (leftPanelState_ == LeftPanelState::spreaded)
        {
            clSpacer_->setFixedWidth(getRecentMiniModeWidth());
            splitter_->replaceWidget(0, clSpacer_);
            //clHostLayout_->removeWidget(contactsWidget_);

            //contactsLayout_->setParent(contactsWidget_);
            newPanelWidth = Utils::scale_value(360);

            showSemiWindow();
            recentsTab_->setItemWidth(newPanelWidth);
            clContainer_->setParent(this);
            clContainer_->setFixedWidth(newPanelWidth);
            clContainer_->raise();
            if (canShowRecents())
                clContainer_->show();
            expandButton_->hide();
            spacerBetweenHeaderAndRecents_->show();
        }
        else
        {
            im_assert(false && "Left Panel state does not exist.");
        }
        contactsTab_->setState(leftPanelState_);
        callsTab_->setState(leftPanelState_);
        spreadedModeLine_->setVisible(leftPanelState_ == LeftPanelState::spreaded);
        updateSplitterStretchFactors();
    }

    void MainPage::saveSplitterState()
    {
        static const auto scale = get_gui_settings()->get_value(settings_scale_coefficient, Utils::getBasicScaleCoefficient());
        get_gui_settings()->set_value(settings_splitter_state, splitter_->saveState().toStdString());
        if (get_gui_settings()->get_value(settings_splitter_state_scale, 0.0) != scale)
            get_gui_settings()->set_value(settings_splitter_state_scale, scale);

        get_gui_settings()->set_value(settings_recents_mini_mode, isManualRecentsMiniMode_);
    }

    void MainPage::updateSplitterStretchFactors()
    {
        for (int i = 0; i < splitter_->count(); ++i)
            splitter_->setStretchFactor(i, i == 0 ? 0 : 1);
    }

    void MainPage::searchActivityChanged(bool _isActive)
    {
        if (_isActive)
        {
            recentsTab_->switchToContactListWithHeaders(leftPanelState_ == LeftPanelState::normal ? RecentsTab::SwichType::Animated : RecentsTab::SwichType::Simple);
            topWidget_->setRegime(TopPanelWidget::Regime::Search);
        }
    }

    bool MainPage::isVideoWindowActive() const
    {
#ifndef STRIP_VOIP
        return videoWindow_ && videoWindow_->isActiveWindow();
#else
        return false;
#endif
    }

    bool MainPage::isVideoWindowMinimized() const
    {
#ifndef STRIP_VOIP
        return videoWindow_ && videoWindow_->isMinimized();
#else
        return false;
#endif
    }

    void MainPage::clearSearchFocus()
    {
        topWidget_->getSearchWidget()->clearFocus();
    }

    void MainPage::startSearchInDialog(const QString& _aimid, const QString& _pattern)
    {
        prevOneFrameMode_ = oneFrameMode_;

        if (isOneFrameMode() && oneFrameMode_ != OneFrameMode::Tab)
            switchToTabFrame();

        selectRecentsTab(false);
        setOneFrameMode(OneFrameMode::Tab);

        topWidget_->setRegime(TopPanelWidget::Regime::Search);
        topWidget_->getSearchWidget()->clearInput();

        recentsTab_->setSearchMode(true);

        if (_aimid.isEmpty())
        {
            if (isOpenedAsMain())
                recentsTab_->setSearchInAllDialogs();
        }
        else
        {
            recentsTab_->setSearchInDialog(_aimid);
        }
        recentsTab_->showSearch();

        setSearchFocus();
        if (!_pattern.isEmpty())
            topWidget_->getSearchWidget()->setText(_pattern);

        GetDispatcher()->post_stats_to_core(core::stats::stats_event_names::cl_search_dialog);
        if (!clContainer_->isVisible())
            clContainer_->show();

        QMetaObject::invokeMethod(this, [this]()
            {
                if (canSetSearchFocus())
                    topWidget_->getSearchWidget()->setFocus();
            }, Qt::QueuedConnection);
    }

    void MainPage::changeCLHeadToSearchSlot()
    {
        NeedShowUnknownsHeader_ = false;
        openRecents();
    }

    void MainPage::changeCLHeadToUnknownSlot()
    {
        NeedShowUnknownsHeader_ = true;
        topWidget_->setRegime(TopPanelWidget::Regime::Unknowns);
        Q_EMIT Utils::InterConnector::instance().hideSearchDropdown();
    }

    void MainPage::recentsTopPanelBack()
    {
        if (!prevSearchInput_.isEmpty())
        {
            NeedShowUnknownsHeader_ = false;
            topWidget_->setRegime(TopPanelWidget::Regime::Recents);
            recentsTab_->switchToRecents();
            topWidget_->getSearchWidget()->setFocus();
            topWidget_->getSearchWidget()->clearInput();
            topWidget_->getSearchWidget()->setText(prevSearchInput_);
        }
        else
        {
            changeCLHeadToSearchSlot();
            hideRecentsPopup();
        }
    }

    void MainPage::addByNick()
    {
        if (isPictureOnlyView())
            setLeftPanelState(LeftPanelState::spreaded, true, true);

        if (isOneFrameMode() && oneFrameMode_ != OneFrameMode::Tab)
            switchToTabFrame();

        if (getCurrentTab() == TabType::Settings)
            selectRecentsTab();

        SearchWidget* widget = nullptr;
        if (getCurrentTab() == TabType::Contacts)
            widget = contactsTab_->getSearchWidget();
        else if (getCurrentTab() == TabType::Calls)
            widget = callsTab_->getSearchWidget();
        else
            widget = topWidget_->getSearchWidget();

        if (widget)
        {
            widget->setFocus();
            widget->setTempPlaceholderText(config::get().is_on(config::features::has_nicknames) ? QT_TRANSLATE_NOOP("add_widget", "Enter nickname") : QT_TRANSLATE_NOOP("add_widget", "Enter email"));
        }
    }

    void MainPage::createGroupCall()
    {
        createGroupVideoCall();
    }

    void MainPage::createCallByLink()
    {
        createCallLink(ConferenceType::Call);
    }

    void MainPage::createWebinar()
    {
        createCallLink(ConferenceType::Webinar);
    }

    void MainPage::createCallLink(ConferenceType _type)
    {
        if (!callLinkCreator_)
            callLinkCreator_ = new Utils::CallLinkCreator(Utils::CallLinkFrom::CallLog);
        callLinkCreator_->createCallLink(_type, QString());
    }

    void MainPage::callContact(const std::vector<QString>& _aimids, const QString& _friendly, bool _video)
    {
#ifndef STRIP_VOIP
        auto w = new Ui::MultipleOptionsWidget(nullptr,
            {{qsl(":/phone_icon"), QT_TRANSLATE_NOOP("tooltips", "Audio call")},
             {qsl(":/video_icon"), QT_TRANSLATE_NOOP("tooltips", "Video call")}});
        Ui::GeneralDialog generalDialog(w, Utils::InterConnector::instance().getMainWindow());
        generalDialog.addLabel(_friendly, Qt::AlignTop | Qt::AlignLeft, 2);
        generalDialog.addCancelButton(QT_TRANSLATE_NOOP("report_widget", "Cancel"), true);

        if (!generalDialog.execute())
            return;

        _video = w->selectedIndex() == 1;
        const auto started = Ui::GetDispatcher()->getVoipController().setStartCall(_aimids, _video, false);
        auto mainPage = MainPage::instance();
        if (mainPage && started)
            mainPage->raiseVideoWindow(voip_call_type::video_call);
#endif
    }

    void MainPage::openRecents()
    {
        if (isOpenedAsMain())
        {
            const auto contactToSelect = recentsTab_->getContactListWidget()->getSearchInDialog()
                ? recentsTab_->getContactListWidget()->getSearchInDialogContact()
                : QString();

            searchEnd();
            Q_EMIT Utils::InterConnector::instance().hideSearchDropdown();
            topWidget_->setRegime(TopPanelWidget::Regime::Recents);

            recentsTab_->switchToRecents(contactToSelect);
        }
        else
        {
            searchEnd();
            clContainer_->hide();
            onSizeChanged();
        }
    }

    void Ui::MainPage::openCalls()
    {
        setSidebarState(SidebarStateFlag::CloseAnimated);
        if (frameCountMode_ != FrameCountMode::_1)
            selectSettings(settingsTab_->currentSettingsItem());
        notifyApplicationWindowActive(true);
    }

    void MainPage::compactModeChanged()
    {
        setLeftPanelState(leftPanelState_, false, false, true);
    }

    void MainPage::tabChanged(int tab)
    {
        if (tab != currentTab_ && leftPanelState_ == LeftPanelState::spreaded)
        {
            setLeftPanelState(LeftPanelState::picture_only, false);
        }

        if (tab != currentTab_)
        {
            topWidget_->setVisible(tab == RECENTS);
            currentTab_ = tab;
        }
    }

    void MainPage::settingsHeaderBack()
    {
        if (generalSettings_->isSessionsShown() || generalSettings_->isWebViewShown())
        {
            if (generalSettings_->isVisible())
                generalSettings_->navigateBack();
            else
                settingsTab_->selectSettings(Utils::CommonSettingsType::CommonSettingsType_Security);
        }
        else if(frameCountMode_ == FrameCountMode::_1)
        {
            switchToTabFrame();
            settingsHeader_->setText(QString());
        }
        else
        {
            settingsHeader_->hide();
            pages_->poproot();

            openRecents();

            selectRecentsTab();

            Q_EMIT Utils::InterConnector::instance().headerBack();
        }
    }

    void MainPage::showSettingsHeader(const QString& text)
    {
        settingsHeader_->setText(text);
        settingsHeader_->show();
    }

    void MainPage::hideSettingsHeader()
    {
        settingsHeader_->setText(QString());
        settingsHeader_->hide();
    }

    void MainPage::currentPageChanged(int _index)
    {
        if (pages_->currentWidget() != contactDialog_)
            Q_EMIT Utils::InterConnector::instance().hideSearchDropdown();

        Q_EMIT Utils::InterConnector::instance().currentPageChanged();
    }

    void MainPage::tabBarCurrentChanged(int _index)
    {
        if (fastDropSearchEnabled())
        {
            topWidget_->getSearchWidget()->setNeedClear(true);
            topWidget_->getSearchWidget()->clearInput();
            Q_EMIT contactsTab_->getContactListWidget()->forceSearchClear(true);
            topWidget_->getSearchWidget()->setNeedClear(false);
        }

        const auto tab = getTabByIndex(_index);
        if (tab != TabType::Settings)
            settingsHeader_->hide();

        const auto handleSidebar = [this](TabType _tab)
        {
            pages_->setCurrentWidget(contactDialog_);
            if (isOneFrameMode() && sidebarVisibilityOnRecentsFrame_)
            {
                setSidebarState(SidebarStateFlag::Opened | SidebarStateFlag::KeepState);
                switchToSidebarFrame();
            }
            else if (!isOneFrameMode() && sidebarVisibilityOnRecentsFrame_)
            {
                setSidebarState(SidebarStateFlag::Opened | SidebarStateFlag::KeepState);
            }
            else if (_tab == TabType::Recents && isOneFrameMode())
            {
                if (contactDialog_->currentAimId().isEmpty())
                    switchToTabFrame();
                else
                    switchToContentFrame();
            }
        };

        switch (tab)
        {
        case TabType::Recents:
        case TabType::Contacts:
            handleSidebar(tab);
            break;
        case TabType::Calls:
            setSidebarState(SidebarStateFlag::KeepState);
            pages_->setCurrentWidget(callsHistoryPage_);
            break;
        case TabType::Settings:
            setSidebarState(SidebarStateFlag::KeepState);
            if (isOneFrameMode())
                switchToTabFrame();
            else
                selectSettings(settingsTab_->currentSettingsItem());
            break;
        default:
            im_assert(!"unsupported tab");
            break;
        }

        notifyApplicationWindowActive(tab != TabType::Settings);

        if (tab == TabType::Settings)
            selectTab(TabType::Settings);
        else if (tab != TabType::Recents && tab != TabType::Calls && topWidget_->getSearchWidget()->isEmpty())
            openRecents();
    }

    void MainPage::onTabClicked(int _index)
    {
        const auto tabType = getTabByIndex(_index);
        onTabClicked(tabType, getCurrentTab() == tabType);
    }

    void MainPage::onTabClicked(TabType _tabType, bool _isSameTabClicked)
    {
        if (_tabType == TabType::Recents)
        {
            if (_isSameTabClicked)
            {
                changeCLHeadToSearchSlot();
                const auto unreadCounters = Logic::getUnreadCounters();
                if (unreadCounters.unreadMessages_ == 0 && !unreadCounters.attention_)
                    return;

                Ui::GetDispatcher()->post_stats_to_core(core::stats::stats_event_names::titlebar_message);

                Q_EMIT Utils::InterConnector::instance().activateNextUnread();
            }
            else
            {
                Ui::GetDispatcher()->post_stats_to_core(core::stats::stats_event_names::chatscr_view);
            }
        }
        else if (_tabType == TabType::Settings)
        {
            Ui::GetDispatcher()->post_stats_to_core(core::stats::stats_event_names::settingsscr_view);
        }
    }

    void MainPage::tabBarRightClicked(int _index)
    {
        if (const auto tab = getTabByIndex(_index); tab == TabType::Settings)
        {
            if (Utils::keyboardModifiersCorrespondToKeyComb(QGuiApplication::keyboardModifiers(), Utils::KeyCombination::Ctrl_or_Cmd_AltShift))
                Q_EMIT Utils::InterConnector::instance().showDebugSettings();
        }
    }

    TabType MainPage::getTabByIndex(int _index) const
    {
        const auto it = std::find_if(indexToTabs_.begin(), indexToTabs_.end(), [_index](auto x) { return x.first == _index; });
        if (it != indexToTabs_.end())
            return it->second;
        im_assert(false);
        return TabType::Recents;
    }

    int MainPage::getIndexByTab(TabType _tab) const
    {
        const auto it = std::find_if(indexToTabs_.begin(), indexToTabs_.end(), [_tab](auto x) { return x.second == _tab; });
        if (it != indexToTabs_.end())
            return it->first;
        im_assert(false);
        return 0;
    }

    TabType MainPage::getCurrentTab() const
    {
        return getTabByIndex(tabWidget_->currentIndex());
    }

    void MainPage::selectSettings(Utils::CommonSettingsType _type)
    {
        settingsTab_->selectSettings(_type);
    }

    void MainPage::selectTab(TabType _tab, bool _needUpdateFocus)
    {
        if (isVisible())
            onSizeChanged({}, _needUpdateFocus);
        tabWidget_->setCurrentIndex(getIndexByTab(_tab));
    }

    bool MainPage::isVideoWindowOn() const
    {
#ifndef STRIP_VOIP
        return videoWindow_ && !videoWindow_->isHidden();
#else
        return false;
#endif
    }

    void MainPage::maximizeVideoWindow()
    {
#ifndef STRIP_VOIP
        if (!videoWindow_)
            return;

        videoWindow_->showMaximized();
#endif
    }

    bool MainPage::isVideoWindowMaximized() const
    {
#ifndef STRIP_VOIP
        return videoWindow_ && videoWindow_->isMaximized();
#else
        return false;
#endif
    }

    bool MainPage::isVideoWindowFullscreen() const
    {
#ifndef STRIP_VOIP
        return videoWindow_ && videoWindow_->isFullScreen();
#else
        return false;
#endif
    }

    void MainPage::initUpdateButton()
    {
        expandButton_->setBadgeText(QT_TRANSLATE_NOOP("tab", "NEW"));
        if (!updaterButton_)
        {
            updaterButton_ = new UpdaterButton(this);
            tabWidget_->insertAdditionalWidget(updaterButton_);
        }
    }

    void MainPage::resizeUpdateFocus(QWidget* _prevFocused)
    {
        if (GeneralDialog::isActive() || getCurrentTab() != TabType::Recents)
            return;

        switch (frameCountMode_)
        {
        case FrameCountMode::_1:
            if (sidebar_->isVisible() && sidebar_->isThreadOpen())
                Q_EMIT Utils::InterConnector::instance().setFocusOnInput(sidebar_->currentAimId(Sidebar::ResolveThread::Yes));
            else if (_prevFocused && sidebar_->isVisible() && sidebar_->isAncestorOf(_prevFocused))
                _prevFocused->setFocus();
            else
                Q_EMIT Utils::InterConnector::instance().setFocusOnInput();
            break;
        case FrameCountMode::_2:
            if (sidebar_->isVisible() && sidebar_->isThreadOpen())
                Q_EMIT Utils::InterConnector::instance().setFocusOnInput(sidebar_->currentAimId(Sidebar::ResolveThread::Yes));
            else if (_prevFocused && sidebar_->isVisible() && sidebar_->isAncestorOf(_prevFocused))
                _prevFocused->setFocus();
            break;
        default:
            break;
        }
    }

    void MainPage::updateSidebarWidth(int _desiredWidth)
    {
        if (sidebar_)
            sidebar_->setFixedWidth(std::clamp(_desiredWidth, Sidebar::getDefaultWidth(), Sidebar::getDefaultMaximumWidth()));
    }

    void MainPage::saveContactListWidth()
    {
        if (clContainer_)
            currentCLWidth_ = clContainer_->width();
    }

    void MainPage::restoreContactListWidth()
    {
        if (clContainer_)
        {
            if (isManualRecentsMiniMode_)
                clContainer_->setFixedWidth(getRecentMiniModeWidth());
            else
                clContainer_->setFixedWidth(std::clamp(currentCLWidth_, getRecentsMinWidth(), getRecentsMaxWidth()));
        }
    }

    bool MainPage::isOpenedAsMain() const
    {
        return openedAs_ == PageOpenedAs::MainPage;
    }

    bool MainPage::canShowRecents() const
    {
        return (isOpenedAsMain() || isSearchActive());
    }

    void MainPage::onContactFilterRemoved(const QString& _aimId)
    {
        if (!isOpenedAsMain() && contactDialog_->currentAimId() == _aimId)
        {
            clContainer_->hide();
            Q_EMIT Utils::InterConnector::instance().searchEnd();
            onSizeChanged();
        }
    }

    void MainPage::onScrollToNewsFeedMesssage(const QString& _aimId, int64_t _msgId, const highlightsV& _highlights)
    {
        if (!isOpenedAsMain() && contactDialog_->currentAimId() == _aimId)
            contactDialog_->onContactSelected(_aimId, _msgId, _highlights);
    }

    void MainPage::scrollRecentsToTop()
    {
        recentsTab_->scrollToTop();
    }
}
