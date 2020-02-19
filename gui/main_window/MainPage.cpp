#include "stdafx.h"
#include "MainPage.h"

#include "../common.shared/config/config.h"

#include "Splitter.h"
#include "TabWidget.h"
#include "TabBar.h"
#include "ExpandButton.h"
#include "ContactsTab.h"
#include "ContactDialog.h"
#include "GroupChatOperations.h"
#include "IntroduceYourself.h"
#include "UpdaterButton.h"
#include "livechats/LiveChatsHome.h"
#include "livechats/LiveChatProfile.h"
#include "contact_list/ChatMembersModel.h"
#include "contact_list/Common.h"
#include "contact_list/ContactList.h"
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
#include "settings/GeneralSettingsWidget.h"
#include "settings/SettingsTab.h"
#include "settings/SettingsHeader.h"
#include "sidebar/Sidebar.h"
#include "../core_dispatcher.h"
#include "../gui_settings.h"
#include "../my_info.h"

#include "../controls/CustomButton.h"
#include "../controls/FlatMenu.h"
#include "../controls/TextEmojiWidget.h"
#include "../controls/WidgetsNavigator.h"
#include "../controls/SemitransparentWindowAnimated.h"
#include "../controls/HorScrollableView.h"
#include "../controls/TransparentScrollBar.h"
#include "../controls/LineLayoutSeparator.h"
#include "../controls/LoaderOverlay.h"
#include "../controls/ContextMenu.h"
#include "../utils/InterConnector.h"
#include "../utils/utils.h"
#include "../utils/log/log.h"
#include "../utils/gui_metrics.h"
#include "../voip/IncomingCallWindow.h"
#include "../voip/VideoWindow.h"
#include "../main_window/settings/SettingsForTesters.h"
#include "MainWindow.h"
#include "smiles_menu/Store.h"
#include "smiles_menu/suggests_widget.h"
#include "../main_window/sidebar/MyProfilePage.h"
#include "../main_window/input_widget/AttachFilePopup.h"
#include "../main_window/input_widget/InputWidget.h"

#include "styles/ThemeParameters.h"

#include "url_config.h"

#include "previewer/toast.h"

#ifdef __APPLE__
#   include "../utils/macos/mac_support.h"
#endif

namespace
{
    constexpr int balloon_size = 20;
    constexpr int unreads_padding = 6;
    constexpr int counter_padding = 4;
    constexpr int back_spacing = 16;
    constexpr int unreads_minimum_extent = balloon_size;
    constexpr int BACK_HEIGHT = 52;
    constexpr int MENU_WIDTH = 240;
    constexpr int ANIMATION_DURATION = 200;

    int getChatMinWidth() noexcept
    {
        return Utils::scale_value(380);
    }

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

    int getFrame2MiniModeMinWith() noexcept
    {
        return getRecentMiniModeWidth() + getChatMinWidth();
    }

    int getFrame2MinWith() noexcept
    {
        return Utils::scale_value(660);
    }

    int getFrame3MinWith() noexcept
    {
        return Utils::scale_value(990);
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
}

namespace Ui
{
    enum class Tabs
    {
        Calls,
        Contacts,
        Recents,
        Settings
    };

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
        static const auto textColor = Styling::getParameters().getColor(Styling::StyleVariable::TEXT_SOLID_PERMANENT);
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

    std::unique_ptr<MainPage> MainPage::_instance;
    MainPage* MainPage::instance(QWidget* _parent)
    {
        assert(_instance || _parent);

        if (!_instance)
            _instance.reset(new MainPage(_parent));

        return _instance.get();
    }

    void MainPage::reset()
    {
        _instance.reset();
    }

    MainPage::MainPage(QWidget* _parent)
        : QWidget(_parent)
        , contactListWidget_(new ContactList(this))
        , videoWindow_(nullptr)
        , pages_(new WidgetsNavigator(this))
        , contactDialog_(new ContactDialog(this))
        , generalSettings_(new GeneralSettingsWidget(this))
        , stickersStore_(nullptr)
        , liveChatsPage_(new LiveChatHome(this))
        , introduceYourselfSuggestions_(nullptr)
        , needShowIntroduceYourself_(false)
        , settingsTimer_(new QTimer(this))
        , recvMyInfo_(false)
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
        , sidebar_(new Sidebar(_parent))
        , tabWidget_(nullptr)
        , settingsTab_(nullptr)
        , contactsTab_(nullptr)
        , spreadedModeLine_(new LineLayoutSeparator(Utils::scale_value(1), LineLayoutSeparator::Direction::Ver, Styling::getParameters().getColor(Styling::StyleVariable::BASE_BRIGHT),this))
        , updaterButton_(nullptr)
        , NeedShowUnknownsHeader_(false)
        , currentTab_(RECENTS)
        , isManualRecentsMiniMode_(get_gui_settings()->get_value(settings_recents_mini_mode, false))
        , frameCountMode_(FrameCountMode::_1)
        , oneFrameMode_(OneFrameMode::Tab)
        , loaderOverlay_(new LoaderOverlay(this))
        , idInfoSeq_(-1)
        , liveChats_(new LiveChats(this))
    {
        if (isManualRecentsMiniMode_)
            leftPanelState_ = LeftPanelState::picture_only;

        connect(&Utils::InterConnector::instance(), &Utils::InterConnector::showPlaceholder, this, &MainPage::showPlaceholder);
        connect(&Utils::InterConnector::instance(), &Utils::InterConnector::liveChatSelected, this, &MainPage::liveChatSelected);
        connect(&Utils::InterConnector::instance(), &Utils::InterConnector::compactModeChanged, this, &MainPage::compactModeChanged);
        connect(&Utils::InterConnector::instance(), &Utils::InterConnector::contacts, this, &MainPage::contactsClicked);
        connect(&Utils::InterConnector::instance(), &Utils::InterConnector::openDialogOrProfileById, this, &MainPage::openDialogOrProfileById);
        connect(&Utils::InterConnector::instance(), &Utils::InterConnector::loaderOverlayShow, this, &MainPage::showLoaderOverlay);
        connect(&Utils::InterConnector::instance(), &Utils::InterConnector::loaderOverlayHide, this, &MainPage::hideLoaderOverlay);

        sidebar_->hide();

        hideLoaderOverlay();
        connect(loaderOverlay_, &LoaderOverlay::cancelled, this, &MainPage::onIdLoadingCancelled);

        semiWindow_->setCloseWindowInfo({ Utils::CloseWindowInfo::Initiator::Unknown, Utils::CloseWindowInfo::Reason::Keep_Sidebar });

        horizontalLayout_ = Utils::emptyHLayout(this);

        contactsWidget_ = new QWidget();

        if (isManualRecentsMiniMode_)
            contactsWidget_->resize(getRecentMiniModeWidth(), contactsWidget_->height());

        contactsLayout = Utils::emptyVLayout(contactsWidget_);

        moreMenu_ = new ContextMenu(this, 24);
        if (config::get().is_on(config::features::add_contact))
            moreMenu_->addActionWithIcon(qsl(":/header/add_user"), QT_TRANSLATE_NOOP("contact_list", "Add contact"), this, []
        {
            Utils::InterConnector::instance().getMainWindow()->onShowAddContactDialog({ Utils::AddContactDialogs::Initiator::From::ScrChatsTab });
        });

        moreMenu_->addActionWithIcon(qsl(":/add_groupchat"), QT_TRANSLATE_NOOP("contact_list", "Create group"), this, [this]() { createGroupChat(CreateChatSource::dots); });
        moreMenu_->addActionWithIcon(qsl(":/add_channel"), QT_TRANSLATE_NOOP("contact_list", "Create channel"), this, [this]() { createChannel(CreateChatSource::dots); });
        moreMenu_->addSeparator();
        moreMenu_->addActionWithIcon(qsl(":/header/check"), QT_TRANSLATE_NOOP("tab header", "Read all"), this, &MainPage::readAllClicked);

        topWidget_ = new TopPanelWidget(this);

        const auto headerIconSize = QSize(24, 24);

        recentsHeaderButtons_.more_ = new HeaderTitleBarButton(this);

        recentsHeaderButtons_.more_->setDefaultImage(qsl(":/controls/more_icon"), QColor(), headerIconSize);
        Testing::setAccessibleName(recentsHeaderButtons_.more_, qsl("AS headerbutton moreMenu"));
        topWidget_->getRecentsHeader()->addButtonToLeft(recentsHeaderButtons_.more_);
        QObject::connect(recentsHeaderButtons_.more_, &HeaderTitleBarButton::clicked, this, [this]()
        {
            auto p = mapToGlobal(rect().topLeft());
            p.setX(p.x() + Utils::scale_value(8));
            p.setY(p.y() + Utils::scale_value(8));
            moreMenu_->popup(p);
        });

        recentsHeaderButtons_.newContacts_ = new HeaderTitleBarButton(this);
        recentsHeaderButtons_.newContacts_->setDefaultImage(qsl(":/header/new_user"), QColor(), headerIconSize);
        recentsHeaderButtons_.newContacts_->setCustomToolTip(QT_TRANSLATE_NOOP("tab header", "New Contacts"));
        Testing::setAccessibleName(recentsHeaderButtons_.newContacts_, qsl("AS headerbutton newContacts_"));
        recentsHeaderButtons_.newContacts_->setVisibility(Logic::getUnknownsModel()->itemsCount() > 0);
        topWidget_->getRecentsHeader()->addButtonToLeft(recentsHeaderButtons_.newContacts_);
        recentsHeaderButtons_.newContacts_->hide();
        QObject::connect(recentsHeaderButtons_.newContacts_, &HeaderTitleBarButton::clicked, this, [this]() {
            prevSearchInput_ = topWidget_->getSearchWidget()->getText();
            if (!prevSearchInput_.isEmpty())
                contactListWidget_->setSearchMode(false);
            emit Utils::InterConnector::instance().unknownsGoSeeThem();
        });
        QObject::connect(Logic::getUnknownsModel(), &Logic::UnknownsModel::updatedSize, this, [this]()
        {
            const auto count = Logic::getUnknownsModel()->itemsCount();
            recentsHeaderButtons_.newContacts_->setVisibility(count > 0);
            recentsHeaderButtons_.newContacts_->setBadgeText(Utils::getUnreadsBadgeStr(Logic::getUnknownsModel()->totalUnreads()));
            topWidget_->getRecentsHeader()->refresh();
        });


        recentsHeaderButtons_.newEmails_ = new EmailTitlebarButton(this);
        recentsHeaderButtons_.newEmails_->setDefaultImage(qsl(":/header/mail"), QColor(), headerIconSize);
        recentsHeaderButtons_.newEmails_->setCustomToolTip(QT_TRANSLATE_NOOP("tab header", "Emails"));
        topWidget_->getRecentsHeader()->addButtonToLeft(recentsHeaderButtons_.newEmails_);
        recentsHeaderButtons_.newEmails_->hide();
        recentsHeaderButtons_.newEmails_->setVisibility(false);
        connect(MyInfo(), &my_info::received, this, &MainPage::updateNewMailButton);

        Testing::setAccessibleName(topWidget_, qsl("AS myTopWidget_"));
        contactsLayout->addWidget(topWidget_);
        spacerBetweenHeaderAndRecents_->setFixedHeight(Utils::scale_value(8));
        Testing::setAccessibleName(spacerBetweenHeaderAndRecents_, qsl("AS spacerBetweenHeaderAndRecents_"));
        contactsLayout->addWidget(spacerBetweenHeaderAndRecents_);

        if (isManualRecentsMiniMode_)
            spacerBetweenHeaderAndRecents_->hide();

        hideSemiWindow();

        connect(contactListWidget_, &ContactList::createGroupChatClicked, this, [this]() { createGroupChat(CreateChatSource::pencil); });
        connect(contactListWidget_, &ContactList::createChannelClicked, this, [this]() { createChannel(CreateChatSource::pencil); });
        connect(contactListWidget_, &ContactList::addContactClicked, this, &MainPage::addContactClicked);
        connect(contactListWidget_, &ContactList::tabChanged, this, &MainPage::tabChanged);

        connect(&Utils::InterConnector::instance(), &Utils::InterConnector::unknownsGoSeeThem, this, &MainPage::changeCLHeadToUnknownSlot);
        connect(&Utils::InterConnector::instance(), &Utils::InterConnector::unknownsGoBack, this, &MainPage::changeCLHeadToSearchSlot);
        connect(&Utils::InterConnector::instance(), &Utils::InterConnector::addByNick, this, &MainPage::addByNick);

        if (isManualRecentsMiniMode_)
        {
            contactListWidget_->setItemWidth(getRecentMiniModeWidth());
            contactListWidget_->resize(getRecentMiniModeWidth(), contactListWidget_->height());
            contactListWidget_->setPictureOnlyView(true);
            clSpacer_->setFixedWidth(0);
        }

        Testing::setAccessibleName(contactListWidget_, qsl("AS contactListWidget_"));
        contactsLayout->addWidget(contactListWidget_);

        connect(topWidget_, &TopPanelWidget::back, this, &MainPage::recentsTopPanelBack);
        connect(topWidget_, &TopPanelWidget::needSwitchToRecents, this, &MainPage::openRecents);
        connect(topWidget_, &TopPanelWidget::searchCancelled, this, &MainPage::searchCancelled);

        connect(Logic::getContactListModel(), &Logic::ContactListModel::needSwitchToRecents, this, &MainPage::openRecents);
        connect(Logic::getContactListModel(), &Logic::ContactListModel::contact_removed, this, &MainPage::onContactRemoved);

        contactsLayout->addSpacerItem(new QSpacerItem(0, 0, QSizePolicy::Minimum));

        pagesLayout_ = Utils::emptyVLayout();
        Testing::setAccessibleName(settingsHeader_, qsl("AS headerWidget_"));
        pagesLayout_->addWidget(settingsHeader_);
        settingsHeader_->setFixedHeight(Utils::getTopPanelHeight());
        settingsHeader_->setObjectName(qsl("header"));
        connect(settingsHeader_, &Ui::SettingsHeader::backClicked, this, &MainPage::settingsHeaderBack);
        settingsHeader_->hide();

        Testing::setAccessibleName(pages_, qsl("AS pages_"));
        pagesLayout_->addWidget(pages_);

        {
            Testing::setAccessibleName(contactDialog_, qsl("AS contactDialog_"));
            pages_->addWidget(contactDialog_);
            Testing::setAccessibleName(generalSettings_, qsl("AS generalSettings_"));
            pages_->addWidget(generalSettings_);
            Testing::setAccessibleName(liveChatsPage_, qsl("AS liveChatsPage_"));
            pages_->addWidget(liveChatsPage_);
            pages_->push(contactDialog_);
        }
        Testing::setAccessibleName(clSpacer_, qsl("AS clSpacer_"));

        Testing::setAccessibleName(contactsWidget_, qsl("AS recentsWidget_"));
        tabWidget_ = new TabWidget();

        contactsTab_ = new ContactsTab();
        Testing::setAccessibleName(contactsTab_, qsl("AS contactsTab_"));
        indexToTabs_.emplace_back(tabWidget_->addTab(contactsTab_, QT_TRANSLATE_NOOP("tab", "Contacts"), qsl(":/tab/contacts"), qsl(":/tab/contacts_active")), Tabs::Contacts);

        indexToTabs_.emplace_back(tabWidget_->addTab(contactsWidget_, QT_TRANSLATE_NOOP("tab", "Chats"), qsl(":/tab/chats"), qsl(":/tab/chats_active")), Tabs::Recents);

        settingsTab_ = new SettingsTab();
        Testing::setAccessibleName(settingsTab_, qsl("AS settingsTab_"));
        indexToTabs_.emplace_back(tabWidget_->addTab(settingsTab_, QT_TRANSLATE_NOOP("tab", "Settings"), qsl(":/tab/settings"), qsl(":/tab/settings_active")), Tabs::Settings);

        selectTab(Tabs::Recents);

        clHostLayout_->setSpacing(0);
        Testing::setAccessibleName(tabWidget_, qsl("AS tabWidget_"));
        clHostLayout_->addWidget(tabWidget_);

        expandButton_ = new ExpandButton();
        expandButton_->setFixedSize(expandButtonSize());
        expandButton_->hide();
        Testing::setAccessibleName(expandButton_, qsl("AS expandButton_"));
        clHostLayout_->addWidget(expandButton_);

        connect(Ui::GetDispatcher(), &Ui::core_dispatcher::updateReady, this, [this]() {
            expandButton_->setBadgeText(QT_TRANSLATE_NOOP("tab", "NEW"));
            if (!updaterButton_)
            {
                updaterButton_ = new UpdaterButton(this);
                tabWidget_->insertAdditionalWidget(updaterButton_);
            }
        });

        clHostLayout_->setContentsMargins(0, 0, 0, 0);

        clContainer_ = new QWidget;

        auto clContainerLayout = Utils::emptyHLayout();
        clContainerLayout->setSpacing(0);
        clContainerLayout->setContentsMargins(0, 0, 0, 0);
        clContainerLayout->addLayout(clHostLayout_);
        Testing::setAccessibleName(spreadedModeLine_, qsl("AS mp spreadedModeLine_"));
        clContainerLayout->addWidget(spreadedModeLine_);
        spreadedModeLine_->hide();
        clContainer_->setLayout(clContainerLayout);
        clContainer_->setMaximumWidth(getRecentsMaxWidth());
        Testing::setAccessibleName(clContainer_, qsl("AS clContainer_"));
        splitter_->addWidget(clContainer_);
        clContainer_->installEventFilter(this);

        if (isManualRecentsMiniMode_)
        {
            settingsTab_->setCompactMode(true);
            tabWidget_->hideTabbar();
            expandButton_->show();
            clContainer_->setFixedWidth(getRecentMiniModeWidth());
        }

        splitter_->setChildrenCollapsible(false);

        QObject::connect(splitter_, &Splitter::moved, this, [this, recentIdx = splitter_->count()](int desiredPos, int resultPos, int index)
        {
            if (index == recentIdx)
            {
                if (desiredPos < getRecentMiniModeWidth() * 2)
                {
                    isManualRecentsMiniMode_ = true;
                    setLeftPanelState(LeftPanelState::picture_only, false);
                    for_each([width = getRecentMiniModeWidth()](auto w) { w->setFixedWidth(width); }, /*topWidget_,*/ clContainer_);
                }
                else if ((width() > getFrame2MinWith() && !sidebar_->isVisible())
                    || (width() > (getFrame2MinWith() + Sidebar::getDefaultWidth()) && sidebar_->isVisible()))
                {
                    isManualRecentsMiniMode_ = false;

                    setLeftPanelState(LeftPanelState::normal, false);
                    for_each([width = getRecentsMaxWidth()](auto w) { w->setMaximumWidth(width); }, /*topWidget_,*/ clContainer_);
                    for_each([width = getRecentsMinWidth()](auto w) { w->setMinimumWidth(width); }, /*topWidget_,*/ clContainer_);
                }
            }
            saveSplitterState();
        });

        pagesLayout_->setContentsMargins(0, 0, 0, 0);
        pagesContainer_ = new QWidget;
        pagesContainer_->setLayout(pagesLayout_);
        pagesContainer_->setMinimumWidth(getChatMinWidth());
        Testing::setAccessibleName(pagesContainer_, qsl("AS pagesContainer_"));
        splitter_->addWidget(pagesContainer_);

        splitter_->setOpaqueResize(true);
        splitter_->setHandleWidth(splitterHandleWidth());
        splitter_->setMouseTracking(true);

        insertSidebarToSplitter();

        updateSplitterStretchFactors();

        Testing::setAccessibleName(splitter_, qsl("AS splitter_"));
        horizontalLayout_->addWidget(splitter_);

        setFocus();

        connect(contactListWidget_, &ContactList::itemSelected, this, &MainPage::onContactSelected);
        connect(contactListWidget_, &ContactList::itemSelected, this, &MainPage::hideRecentsPopup);
        connect(contactListWidget_, &ContactList::itemSelected, contactDialog_, &ContactDialog::onContactSelected);
        connect(contactDialog_, &ContactDialog::sendMessage, this, &MainPage::onMessageSent);
        connect(contactDialog_, &ContactDialog::clicked, this, &MainPage::hideRecentsPopup);

        connect(contactsTab_->getContactListWidget(), &ContactListWidget::itemSelected, this, &MainPage::onContactSelected);
        connect(contactsTab_->getContactListWidget(), &ContactListWidget::itemSelected, contactDialog_, &ContactDialog::onContactSelected);

        connect(sidebar_, &Sidebar::backButtonClicked, this, &MainPage::sidebarBackClicked);

        connect(&Utils::InterConnector::instance(), &Utils::InterConnector::showSettingsHeader, this, &MainPage::showSettingsHeader);
        connect(&Utils::InterConnector::instance(), &Utils::InterConnector::hideSettingsHeader, this, &MainPage::hideSettingsHeader);
        connect(&Utils::InterConnector::instance(), &Utils::InterConnector::myProfileBack, this, &MainPage::settingsHeaderBack);

        connect(&Utils::InterConnector::instance(), &Utils::InterConnector::closeLastDialog, this, &MainPage::dialogBackClicked);

        connect(&Utils::InterConnector::instance(), &Utils::InterConnector::dialogClosed, this, &MainPage::onDialogClosed);
        connect(Logic::getContactListModel(), &Logic::ContactListModel::selectedContactChanged, this, &MainPage::onSelectedContactChanged);

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

        connect(topWidget_->getSearchWidget(), &SearchWidget::searchBegin, this, &MainPage::searchBegin);
        connect(topWidget_->getSearchWidget(), &SearchWidget::searchEnd, this, &MainPage::openRecents);
        connect(topWidget_->getSearchWidget(), &SearchWidget::inputEmpty, this, &MainPage::onSearchInputEmpty);
        connect(topWidget_->getSearchWidget(), &SearchWidget::searchIconClicked, this, &MainPage::spreadCL);
        connect(topWidget_->getSearchWidget(), &SearchWidget::activeChanged, this, &MainPage::searchActivityChanged);

        if (isManualRecentsMiniMode_)
            topWidget_->setState(LeftPanelState::picture_only);

        contactListWidget_->connectSearchWidget(topWidget_->getSearchWidget());

        connect(&Ui::GetDispatcher()->getVoipController(), &voip_proxy::VoipController::onVoipShowVideoWindow, this, &MainPage::onVoipShowVideoWindow, Qt::DirectConnection);
        connect(&Ui::GetDispatcher()->getVoipController(), &voip_proxy::VoipController::onVoipCallIncoming, this, &MainPage::onVoipCallIncoming, Qt::DirectConnection);
        connect(&Ui::GetDispatcher()->getVoipController(), &voip_proxy::VoipController::onVoipCallIncomingAccepted, this, &MainPage::onVoipCallIncomingAccepted, Qt::DirectConnection);
        connect(&Ui::GetDispatcher()->getVoipController(), &voip_proxy::VoipController::onVoipCallDestroyed, this, &MainPage::onVoipCallDestroyed, Qt::DirectConnection);

        QTimer::singleShot(Ui::period_for_start_stats_settings_ms.count(), this, &MainPage::post_stats_with_settings);
        QObject::connect(settingsTimer_, &QTimer::timeout, this, &MainPage::post_stats_with_settings);
        settingsTimer_->start(Ui::period_for_stats_settings_ms.count());

        connect(Ui::GetDispatcher(), &core_dispatcher::myInfo, this, &MainPage::myInfo, Qt::UniqueConnection);
        connect(Ui::GetDispatcher(), &core_dispatcher::im_created, this, &MainPage::loggedIn);

        connect(Ui::GetDispatcher(), &core_dispatcher::historyUpdate, contactDialog_, &ContactDialog::onContactSelectedToLastMessage);

        connect(Ui::GetDispatcher(), &core_dispatcher::idInfo, this, &MainPage::onIdInfo);

        connect(pages_, &QStackedWidget::currentChanged, this, &MainPage::currentPageChanged);

        connect(&Utils::InterConnector::instance(), &Utils::InterConnector::showStickersStore, this, &MainPage::showStickersStore);

        connect(tabWidget_, &TabWidget::currentChanged, this, &MainPage::tabBarCurrentChanged);
        connect(tabWidget_, &TabWidget::tabBarClicked, this, &MainPage::tabBarClicked);
        connect(tabWidget_->tabBar(), &TabBar::rightClicked, this, &MainPage::tabBarRightClicked);
        connect(expandButton_, &ExpandButton::clicked, this, &MainPage::expandButtonClicked);

        connect(&Utils::InterConnector::instance(), &Utils::InterConnector::closeAnySemitransparentWindow, this, [this](const Utils::CloseWindowInfo& _info) {
            if (leftPanelState_ == LeftPanelState::spreaded)
                hideRecentsPopup();
            if (_info.reason_ != Utils::CloseWindowInfo::Reason::Keep_Sidebar)
                setSidebarVisible(false);
        });

        connect(&Utils::InterConnector::instance(), &Utils::InterConnector::applicationLocked, this, [this]()
        {
            setSidebarVisible(false);
        });

        connect(get_gui_settings(), &Ui::qt_gui_settings::changed, this, [this](const QString& _key) {
            if (_key == ql1s(settings_notify_new_mail_messages))
                updateNewMailButton();
        });

        const auto scale = get_gui_settings()->get_value(settings_scale_coefficient, Utils::getBasicScaleCoefficient());
        const auto splitterScale = get_gui_settings()->get_value(settings_splitter_state_scale, 0.0);
        if (const auto val = QByteArray::fromStdString(get_gui_settings()->get_value<std::string>(settings_splitter_state, std::string())); !val.isEmpty() && scale == splitterScale)
            splitter_->restoreState(val);
        else
            clContainer_->resize(getRecentsDefaultWidth(), clContainer_->height());

        chatUnreadChanged();

        loggedIn();
    }


    MainPage::~MainPage()
    {
        if (videoWindow_)
        {
            delete videoWindow_;
            videoWindow_ = nullptr;
        }
    }

    bool MainPage::isUnknownsOpen() const
    {
        return
            pages_->currentWidget() == contactDialog_ &&
            contactListWidget_->currentTab() == RECENTS &&
            contactListWidget_->isUnknownsOpen();
    }

    bool MainPage::isPictureOnlyView() const
    {
        return clContainer_->width() == getRecentMiniModeWidth();
    }

    void MainPage::setFrameCountMode(FrameCountMode _mode)
    {
        frameCountMode_ = _mode;
        for_each([_mode](auto w) { if (w) w->setFrameCountMode(_mode); }, contactDialog_, sidebar_, stickersStore_);
    }

    void MainPage::setOneFrameMode(OneFrameMode _mode)
    {
        oneFrameMode_ = _mode;
    }

    void MainPage::updateSettingHeader()
    {
        if (frameCountMode_ == FrameCountMode::_1 && oneFrameMode_ == OneFrameMode::Content && getCurrentTab() == Tabs::Settings)
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
        if (getCurrentTab() == Tabs::Settings && settingsHeader_->hasText())
            settingsHeader_->show();
        pagesContainer_->show();
        clContainer_->hide();
        sidebar_->hideAnimated();
        emit Utils::InterConnector::instance().currentPageChanged();
    }

    void MainPage::switchToTabFrame()
    {
        setOneFrameMode(OneFrameMode::Tab);
        clContainer_->show();
        pagesContainer_->hide();
        sidebar_->hideAnimated();
        emit Utils::InterConnector::instance().currentPageChanged();
        emit Utils::InterConnector::instance().stopPttRecord();
    }

    void MainPage::switchToSidebarFrame()
    {
        setOneFrameMode(OneFrameMode::Sidebar);
        insertSidebarToSplitter();
        sidebar_->show();
        pagesContainer_->hide();
        clContainer_->hide();
        emit Utils::InterConnector::instance().currentPageChanged();
        emit Utils::InterConnector::instance().stopPttRecord();
    }

    void MainPage::onIdInfo(const int64_t _seq, const Data::IdInfo& _info)
    {
        if (idInfoSeq_ == _seq)
        {
            const auto id = requestedId_;
            onIdLoadingCancelled();
            if (!_info.isValid())
            {
                QString text;
                if (id.contains(ql1c('@')) && !Utils::isChat(id))
                    text = QT_TRANSLATE_NOOP("toast", "There are no profiles with this email");
                else
                    text = QT_TRANSLATE_NOOP("toast", "There are no profiles or groups with this nickname");

                const auto leftShift = clContainer_->isVisible() ? clContainer_->width() : 0;
                const QPoint pos(leftShift + pagesContainer_->width() / 2, height() - Utils::scale_value(36));
                Ui::ToastManager::instance()->showToast(new Ui::Toast(text, this), pos);
            }
            else
            {
                if (_info.isChatInfo() && !_info.stamp_.isEmpty())
                    Utils::openDialogOrProfile(_info.stamp_, Utils::OpenDOPParam::stamp);
                else
                    Utils::openDialogOrProfile(_info.sn_);
            }
        }
    }

    void MainPage::onIdLoadingCancelled()
    {
        idInfoSeq_ = -1;
        requestedId_.clear();
        hideLoaderOverlay();

        emit Utils::InterConnector::instance().loaderOverlayCancelled();
    }

    void MainPage::updateLoaderOverlayPosition()
    {
        QRect loaderRect(QPoint(clContainer_->isVisible() ? clContainer_->width() : 0, 0), pagesContainer_->size());
        loaderOverlay_->setCenteringRect(loaderRect);
        loaderOverlay_->setFixedSize(size());
    }

    bool MainPage::canSetSearchFocus() const
    {
        return !isSemiWindowVisible() &&
            getCurrentTab() == Tabs::Recents &&
            contactListWidget_->currentTab() == RECENTS;
    }

    void MainPage::setCLWidth(int _val)
    {
        setCLWidthNew(_val);
    }

    void MainPage::setCLWidthNew(int _val)
    {
        auto compact_width = ::Ui::ItemWidth(false, true);
        contactsWidget_->resize(_val, contactsWidget_->height());
        contactListWidget_->setItemWidth(_val);
        contactsTab_->setClWidth(_val);
        contactListWidget_->resize(_val, contactListWidget_->height());
        const bool isCompact = (_val == compact_width);
        contactListWidget_->setPictureOnlyView(isCompact);

        if (leftPanelState_ == LeftPanelState::picture_only && _val == compact_width && splitter_->indexOf(clContainer_) == -1)
        {
            clSpacer_->setFixedWidth(0);
            splitter_->replaceWidgetImpl(0, clContainer_);
            updateSplitterStretchFactors();
            Testing::setAccessibleName(contactsWidget_, qsl("AS contactsWidget_"));
        }
    }

    int MainPage::getCLWidth() const
    {
        return contactListWidget_->width();
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

    bool MainPage::isOneFrameTab() const
    {
        return frameCountMode_ == FrameCountMode::_1 && oneFrameMode_ == OneFrameMode::Tab;
    }

    void MainPage::openDialogOrProfileById(const QString& _id)
    {
        assert(!_id.isEmpty());

        if (fastDropSearchEnabled())
            emit Utils::InterConnector::instance().searchEnd();

        emit Utils::InterConnector::instance().closeAnySemitransparentWindow({ Utils::CloseWindowInfo::Initiator::Unknown, Utils::CloseWindowInfo::Reason::Keep_Sidebar });

        requestedId_ = _id.startsWith(ql1c('@')) ? _id.mid(1) : _id;
        idInfoSeq_ = GetDispatcher()->getIdInfo(requestedId_);
        QTimer::singleShot(getLoaderOverlayDelay().count(), this, [this, id = idInfoSeq_]()
        {
            if (idInfoSeq_ == id)
                showLoaderOverlay();
        });
    }

    FrameCountMode MainPage::getFrameCount() const
    {
        return frameCountMode_;
    }

    void MainPage::closeAndHighlightDialog()
    {
        if (const auto aimid = Logic::getContactListModel()->selectedContact(); !aimid.isEmpty())
        {
            window()->setFocus();

            Logic::getContactListModel()->setCurrent(QString(), -1);
            contactListWidget_->highlightContact(aimid);
        }
    }

    void MainPage::resizeEvent(QResizeEvent* _event)
    {
        if (const int newWidth = width(); width() != _event->oldSize().width())
        {
            if ((!isManualRecentsMiniMode_ && newWidth < getFrame2MinWith())
                || (isManualRecentsMiniMode_ && newWidth < getFrame2MiniModeMinWith()))
            {
                setFrameCountMode(FrameCountMode::_1);

                switch (oneFrameMode_)
                {
                case OneFrameMode::Tab:
                {
                    switchToTabFrame();
                }
                break;
                case OneFrameMode::Content:
                {
                    switchToContentFrame();
                }
                break;
                case OneFrameMode::Sidebar:
                {
                    switchToSidebarFrame();
                }
                break;
                }

                for_each([width = getFrame2MinWith() - 1](auto w) { w->setMaximumWidth(width); }, clContainer_);
                for_each([width = getChatMinWidth()](auto w) { w->setMinimumWidth(width); }, clContainer_);
                sidebar_->setWidth(newWidth);
            }
            else
            {
                sidebar_->setWidth(Sidebar::getDefaultWidth());
                if (!isManualRecentsMiniMode_)
                {
                    for_each([width = getRecentsMaxWidth()](auto w) { w->setMaximumWidth(width); }, clContainer_);
                    for_each([width = getRecentsMinWidth()](auto w) { w->setMinimumWidth(width); }, clContainer_);
                }
                else
                {
                    for_each([width = getRecentMiniModeWidth()](auto w) { w->setFixedWidth(width); }, clContainer_);
                }
                setFrameCountMode(sidebar_->isVisible() && isSideBarInSplitter() ? FrameCountMode::_3 : FrameCountMode::_2);
                if (pages_->currentWidget() == contactDialog_ && getCurrentTab() == Tabs::Settings)
                    selectSettings(settingsTab_->currentSettingsItem());
                pagesContainer_->show();
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

        if (width() != _event->oldSize().width())
        {
            updateSidebarPos(width());
            updateSplitterStretchFactors();
        }

        AttachFilePopup::hidePopup();

        saveSplitterState();
    }

    bool MainPage::eventFilter(QObject* _obj, QEvent* _event)
    {
        if (_obj == clContainer_)
        {
            if (_event->type() == QEvent::Resize)
            {
                QResizeEvent *resizeEvent = static_cast<QResizeEvent*>(_event);
                const auto newWidth = resizeEvent->size().width();
                setCLWidthNew(resizeEvent->size().width());
                updateSplitterStretchFactors();
                saveSplitterState();
                return true;

            }
        }
        return QWidget::eventFilter(_obj, _event);
    }

    bool MainPage::isSuggestVisible() const
    {
        return contactDialog_->isSuggestVisible();
    }

    void MainPage::hideSuggest()
    {
        contactDialog_->hideSuggest();
    }

    Stickers::StickersSuggest* MainPage::getStickersSuggest()
    {
        return contactDialog_->getStickersSuggests();
    }

    void MainPage::showSemiWindow()
    {
        semiWindow_->setFixedSize(size());
        semiWindow_->raise();
        semiWindow_->Show();
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
        return contactListWidget_->isKeyboardFocused();
    }

    void MainPage::stopNavigatingInRecents()
    {
        return contactListWidget_->dropKeyboardFocus();
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

    void MainPage::onProfileSettingsShow(const QString& _uin)
    {
        hideRecentsPopup();
        if (!_uin.isEmpty())
        {
            showSidebar(_uin);
            return;
        }

        if (!profilePage_)
        {
            profilePage_ = new MyProfilePage(this);
            Utils::setDefaultBackground(profilePage_);
            Testing::setAccessibleName(profilePage_, qsl("AS profilePage_"));
            pages_->addWidget(profilePage_);
        }

        profilePage_->initFor(_uin);
        pages_->push(profilePage_);

        if (frameCountMode_ == FrameCountMode::_1 && getCurrentTab() == Tabs::Settings)
            switchToContentFrame();
    }

    void MainPage::onSharedProfileShow(const QString& _uin)
    {
        hideRecentsPopup();
        showSidebar(_uin, false, true);
    }

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
            connect(videoWindow_, &VideoWindow::finished, Utils::InterConnector::instance().getMainWindow(), &MainWindow::exit);
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

    void MainPage::nextChat()
    {
        if (const auto& sel = Logic::getContactListModel()->selectedContact(); !sel.isEmpty())
        {
            if (contactListWidget_->currentTab() == RECENTS && contactListWidget_->isRecentsOpen())
            {
                if (const auto nextContact = Logic::getRecentsModel()->nextAimId(sel); !nextContact.isEmpty())
                {
                    contactListWidget_->setNextSelectWithOffset();
                    emit Logic::getContactListModel()->select(nextContact, -1);
                }
            }
        }
    }

    void MainPage::prevChat()
    {
        if (const auto& sel = Logic::getContactListModel()->selectedContact(); !sel.isEmpty())
        {
            if (contactListWidget_->currentTab() == RECENTS && contactListWidget_->isRecentsOpen())
            {
                if (const auto prevContact = Logic::getRecentsModel()->prevAimId(sel); !prevContact.isEmpty())
                {
                    contactListWidget_->setNextSelectWithOffset();
                    emit Logic::getContactListModel()->select(prevContact, -1);
                }
            }
        }
    }

    void MainPage::onGeneralSettingsShow(int _type)
    {
        if (frameCountMode_ == FrameCountMode::_1 && getCurrentTab() == Tabs::Settings)
            switchToContentFrame();

        generalSettings_->setType(_type);
        pages_->push(generalSettings_);
    }

    void MainPage::clearSearchMembers()
    {
        contactListWidget_->update();
    }

    void MainPage::cancelSelection()
    {
        assert(contactDialog_);
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
        if (frameCountMode_ == FrameCountMode::_1)
        {
            if (_needHighlightInRecents)
            {
                closeAndHighlightDialog();
            }
            else
            {
                window()->setFocus();
                Logic::getContactListModel()->setCurrent(QString(), -1);
            }
            switchToTabFrame();
        }
    }

    void MainPage::sidebarBackClicked()
    {
        if (frameCountMode_ == FrameCountMode::_1)
        {
            if (Logic::getContactListModel()->selectedContact() == sidebar_->currentAimId())
                switchToContentFrame();
            else
                switchToTabFrame();
        }
    }

    ContactDialog* MainPage::getContactDialog() const
    {
        assert(contactDialog_);
        return contactDialog_;
    }

    HistoryControlPage* MainPage::getHistoryPage(const QString& _aimId) const
    {
        return contactDialog_->getHistoryPage(_aimId);
    }

    static bool canOpenSidebarOutsideWithoutMove(int _sidebarWidth)
    {
        const auto availableGeometry = Utils::InterConnector::instance().getMainWindow()->screenGeometry();
        const auto mainWindowGeometry = Utils::InterConnector::instance().getMainWindow()->geometry();

        return (availableGeometry.width() - mainWindowGeometry.topRight().x()) >= _sidebarWidth;
    }

    static bool canOpenSidebarOutsideWithMove(int _sidebarWidth)
    {
        const auto availableGeometry = Utils::InterConnector::instance().getMainWindow()->screenGeometry();
        const auto mainWindowGeometry = Utils::InterConnector::instance().getMainWindow()->geometry();

        return (availableGeometry.width() - mainWindowGeometry.width()) >= _sidebarWidth;
    }

    void MainPage::showSidebar(const QString& _aimId, bool _selectionChanged, bool _sharedProfile)
    {
        updateSidebarPos(width());
        sidebar_->preparePage(_aimId, _selectionChanged, _sharedProfile);
        setSidebarVisible(true);
        if (frameCountMode_ == FrameCountMode::_1)
        {
            clContainer_->hide();
            pagesContainer_->hide();
        }
        setOneFrameMode(OneFrameMode::Sidebar);
        emit Utils::InterConnector::instance().currentPageChanged();
        emit Utils::InterConnector::instance().stopPttRecord();
    }

    void MainPage::showMembersInSidebar(const QString& _aimId)
    {
        showSidebar(_aimId, false);
        sidebar_->showMembers();
    }

    bool MainPage::isSidebarVisible() const
    {
        return sidebar_->isVisible();
    }

    void MainPage::setSidebarVisible(bool _visible)
    {
        setSidebarVisible(Utils::SidebarVisibilityParams(_visible, false));
    }

    void MainPage::setSidebarVisible(const Utils::SidebarVisibilityParams &_params)
    {
        if (!_params.show_ && oneFrameMode_ == OneFrameMode::Sidebar)
        {
            setOneFrameMode(OneFrameMode::Content);
            sidebarBackClicked();
        }

        if (sidebar_->isVisible() == _params.show_)
            return;

        if (_params.show_)
        {
            sidebar_->showAnimated();

            if (frameCountMode_ == FrameCountMode::_2 && isSideBarInSplitter())
                setFrameCountMode(FrameCountMode::_3);
        }
        else
        {
            if (frameCountMode_ == FrameCountMode::_3)
                setFrameCountMode(FrameCountMode::_2);
            sidebar_->hideAnimated();
            splitter_->refresh();
        }

        updateSplitterHandle(width());
    }

    void MainPage::restoreSidebar()
    {
        const auto cont = Logic::getContactListModel()->selectedContact();
        if (isSidebarVisible() && !cont.isEmpty())
            showSidebar(cont);
        else
            setSidebarVisible(false);
    }

    int MainPage::getSidebarWidth() const
    {
        return (frameCountMode_ == FrameCountMode::_3 && isSidebarVisible()) ? sidebar_->width() : 0;
    }

    bool MainPage::isContactDialog() const
    {
        if (!pages_)
            return false;

        return (frameCountMode_ != FrameCountMode::_1 || oneFrameMode_ == OneFrameMode::Content) && pages_->currentWidget() == contactDialog_;
    }

    bool Ui::MainPage::isSearchInDialog() const
    {
        if (!contactListWidget_)
            return false;

        return contactListWidget_->getSearchInDialog();
    }

    void MainPage::onContactSelected(const QString& _contact)
    {
        setOneFrameMode(OneFrameMode::Content);
        if (frameCountMode_ == FrameCountMode::_1)
        {
            pagesContainer_->show();
            clContainer_->hide();
        }
        pages_->poproot();
        settingsHeader_->hide();

        hideRecentsPopup();

        if (auto tab = getCurrentTab(); tab != Tabs::Contacts && tab != Tabs::Recents)
            selectTab(Tabs::Recents);

        if (!_contact.isEmpty() && frameCountMode_ == FrameCountMode::_3 && sidebar_->isVisible() && isSideBarInSplitter())
        {
            emit Utils::InterConnector::instance().closeAnySemitransparentWindow({ Utils::CloseWindowInfo::Initiator::Unknown, Utils::CloseWindowInfo::Reason::Keep_Sidebar });
            showSidebar(_contact, true);
        }
        else
        {
            emit Utils::InterConnector::instance().closeAnySemitransparentWindow({});
        }

        emit Utils::InterConnector::instance().hideSearchDropdown();
        emit Utils::InterConnector::instance().showPlaceholder(Utils::PlaceholdersType::PlaceholdersType_SetExistanseOffIntroduceYourself);
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
            //connect(stickersStore_, &Stickers::Store::active, this, &MainPage::hideRecentsPopup);
            connect(stickersStore_, &Stickers::Store::back, this, [this]()
            {
                settingsHeader_->hide();
                pages_->poproot();
            });
            Testing::setAccessibleName(stickersStore_, qsl("AS stickersStore_"));
            pages_->addWidget(stickersStore_);
            stickersStore_->setBadgeText(Utils::getUnreadsBadgeStr(unreadCounters_.unreadMessages_));
            stickersStore_->setFrameCountMode(frameCountMode_);
        }
        pages_->push(stickersStore_);
        stickersStore_->hideSearch();
        settingsHeader_->hide();
    }

    void MainPage::onSearchEnd()
    {
        if (!isUnknownsOpen())
            openRecents();
    }

    void MainPage::loggedIn()
    {
        connect(Logic::getRecentsModel(), &Logic::RecentsModel::dlgStatesHandled, this, &MainPage::chatUnreadChanged, Qt::UniqueConnection);
        connect(Logic::getRecentsModel(), &Logic::RecentsModel::updated, this, &MainPage::chatUnreadChanged, Qt::UniqueConnection);
        connect(Logic::getUnknownsModel(), &Logic::UnknownsModel::dlgStatesHandled, this, &MainPage::chatUnreadChanged, Qt::UniqueConnection);
        connect(Logic::getUnknownsModel(), &Logic::UnknownsModel::updatedMessages, this, &MainPage::chatUnreadChanged, Qt::UniqueConnection);
        connect(Logic::getUnknownsModel(), &Logic::UnknownsModel::updatedSize, this, &MainPage::chatUnreadChanged, Qt::UniqueConnection);
        connect(Logic::getContactListModel(), &Logic::ContactListModel::contactChanged, this, &MainPage::chatUnreadChanged, Qt::UniqueConnection);
        connect(Logic::getContactListModel(), &Logic::ContactListModel::contact_removed, this, &MainPage::chatUnreadChanged, Qt::UniqueConnection);
    }

    void MainPage::chatUnreadChanged()
    {
        const auto unknownsUnread = Logic::getUnknownsModel()->totalUnreads();
        unreadCounters_.unreadMessages_ = Logic::getRecentsModel()->totalUnreads() + unknownsUnread;
        unreadCounters_.attention_ = Logic::getRecentsModel()->hasAttentionDialogs() || Logic::getUnknownsModel()->hasAttentionDialogs();

        const auto totalUnreadsStr = Utils::getUnreadsBadgeStr(unreadCounters_.unreadMessages_);

        tabWidget_->setBadgeText(getIndexByTab(Tabs::Recents), totalUnreadsStr);
        tabWidget_->setBadgeIcon(getIndexByTab(Tabs::Recents), unreadCounters_.attention_ ? qsl(":/tab/attention") : QString());
        recentsHeaderButtons_.newContacts_->setBadgeText(Utils::getUnreadsBadgeStr(unknownsUnread));

        if (stickersStore_)
            stickersStore_->setBadgeText(totalUnreadsStr);
    }

    void MainPage::expandButtonClicked()
    {
        expandButton_->hide();
        isManualRecentsMiniMode_ = false;
        setLeftPanelState(LeftPanelState::normal, false);
        const auto w = width();
        if (w < getFrame2MinWith())
        {
            setFrameCountMode(FrameCountMode::_1);
            switchToTabFrame();

            for_each([width = getFrame2MinWith() - 1](auto w) { w->setMaximumWidth(width); }, clContainer_);
            for_each([width = getChatMinWidth()](auto w) { w->setMinimumWidth(width); }, clContainer_);
        }
        else
        {
            for_each([width = getRecentsMaxWidth()](auto w) { w->setMaximumWidth(width); }, clContainer_);
            for_each([width = getRecentsMinWidth()](auto w) { w->setMinimumWidth(width); }, clContainer_);

            if (sidebar_->isVisible() && w < getFrame3MinWith())
                setSidebarVisible(false);

        }
        updateSplitterStretchFactors();
    }

    void MainPage::onDialogClosed(const QString&, bool _isCurrent)
    {
        if (_isCurrent && frameCountMode_ == FrameCountMode::_1 && oneFrameMode_ != OneFrameMode::Tab)
            switchToTabFrame();
    }

    void MainPage::onSelectedContactChanged(const QString& _contact)
    {
        if (_contact.isEmpty())
        {
            if (frameCountMode_ == FrameCountMode::_1 && oneFrameMode_ != OneFrameMode::Tab)
                switchToTabFrame();
            else
                setOneFrameMode(OneFrameMode::Tab);
        }
    }

    void MainPage::onContactRemoved(const QString& _contact)
    {
        if (!_contact.isEmpty() && _contact == contactListWidget_->getSearchInDialogContact())
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
        updateLoaderOverlayPosition();
        loaderOverlay_->raise();
        loaderOverlay_->show();
    }

    void MainPage::hideLoaderOverlay()
    {
        loaderOverlay_->hide();
    }

    void MainPage::contactsClicked()
    {
        openRecents();

        SelectContactsWidget contacts(nullptr, Logic::MembersWidgetRegim::CONTACT_LIST_POPUP, QT_TRANSLATE_NOOP("popup_window", "Contacts"), QString(), MainPage::instance());
        Logic::updatePlaceholders();
        contacts.show();

        hideSemiWindow();
    }

    void MainPage::settingsClicked()
    {
        settingsTab_->selectSettings(Utils::CommonSettingsType::CommonSettingsType_General);

        emit Utils::InterConnector::instance().generalSettingsShow((int)Utils::CommonSettingsType::CommonSettingsType_General);
        emit Utils::InterConnector::instance().showSettingsHeader(QT_TRANSLATE_NOOP("main_page", "General settings"));
        selectTab(Tabs::Settings);
    }

    void MainPage::searchBegin()
    {
        emit Utils::InterConnector::instance().hideSearchDropdown();
        contactListWidget_->setSearchMode(true);
    }

    void MainPage::searchEnd()
    {
        if (NeedShowUnknownsHeader_)
            topWidget_->setRegime(TopPanelWidget::Regime::Unknowns);

        topWidget_->getSearchWidget()->clearInput();
        topWidget_->getSearchWidget()->setDefaultPlaceholder();
        clearSearchFocus();

        emit Utils::InterConnector::instance().disableSearchInDialog();

        if (platform::is_linux() && leftPanelState_ != LeftPanelState::picture_only)
            emit Utils::InterConnector::instance().updateTitleButtons();

        contactListWidget_->setSearchMode(false);
        contactListWidget_->setSearchInAllDialogs();
    }

    void MainPage::searchCancelled()
    {
        if (isSearchInDialog())
            GetDispatcher()->post_stats_to_core(core::stats::stats_event_names::chat_search_dialog_cancel, { { "type", "outside" } });

        emit Utils::InterConnector::instance().searchEnd();

        hideRecentsPopup();
    }

    void MainPage::onSearchInputEmpty()
    {
        if (!isSearchInDialog())
        {
            if (topWidget_->getSearchWidget()->hasFocus())
                contactListWidget_->switchToContactListWithHeaders(leftPanelState_ == LeftPanelState::normal ? ContactList::SwichType::Animated : ContactList::SwichType::Simple);

            contactListWidget_->setSearchMode(false);
        }
    }

    void MainPage::onVoipCallIncoming(const std::string& call_id, const std::string& _contact)
    {
        assert(!call_id.empty() && !_contact.empty());
        if (!call_id.empty() && !_contact.empty())
        {
            const std::string full_id = call_id + '#' + _contact;
            auto it = incomingCallWindows_.find(full_id);
            if (incomingCallWindows_.end() == it || !it->second)
            {
                std::shared_ptr<IncomingCallWindow> window(new(std::nothrow) IncomingCallWindow(call_id, _contact));
                assert(!!window);
                if (!!window)
                {
                    window->showFrame();
                    incomingCallWindows_[full_id] = window;
                }
            } else
            {
                std::shared_ptr<IncomingCallWindow> wnd = it->second;
                wnd->showFrame();
            }
        }
        Utils::InterConnector::instance().getMainWindow()->updateMainMenu();
    }

    void MainPage::destroyIncomingCallWindow(const std::string& call_id, const std::string& _contact)
    {
        assert(!call_id.empty() && !_contact.empty());
        if (!call_id.empty() && !_contact.empty())
        {
            const std::string full_id = call_id + '#' + _contact;
            auto it = incomingCallWindows_.find(full_id);
            if (incomingCallWindows_.end() != it)
            {
                auto window = it->second;
                assert(!!window);
                if (!!window)
                    window->hideFrame();
            }
        }
        Utils::InterConnector::instance().getMainWindow()->updateMainMenu();
    }

    void MainPage::onVoipCallIncomingAccepted(const voip_manager::ContactEx& _contactEx)
    {
        //destroy exactly when accept button pressed
        //destroyIncomingCallWindow(_contactEx.contact.call_id, _contactEx.contact.contact);
        Utils::InterConnector::instance().getMainWindow()->closeGallery();
    }

    void MainPage::onVoipCallDestroyed(const voip_manager::ContactEx& _contactEx)
    {
        destroyIncomingCallWindow(_contactEx.contact.call_id, _contactEx.contact.contact);
    }

    void MainPage::showVideoWindow()
    {
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

        Utils::InterConnector::instance().getMainWindow()->updateMainMenu();
    }

    void MainPage::minimizeVideoWindow()
    {
        if (!videoWindow_)
            return;

        videoWindow_->showMinimized();
    }

    void MainPage::closeVideoWindow()
    {
        if (!videoWindow_)
            return;

        videoWindow_->close();

        Utils::InterConnector::instance().getMainWindow()->updateMainMenu();
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
        assert(!!contactListWidget_);
        if (contactListWidget_)
        {
            contactListWidget_->changeTab(RECENTS);

            if (_selectUnread)
            {
                QString aimId = Logic::getRecentsModel()->nextUnreadAimId(Logic::getContactListModel()->selectedContact());
                if (aimId.isEmpty())
                    aimId = Logic::getUnknownsModel()->nextUnreadAimId();
                if (!aimId.isEmpty())
                {
                    contactListWidget_->select(aimId);
                    contactListWidget_->scrollToItem(aimId, QAbstractItemView::ScrollHint::PositionAtCenter);
                }
            }
        }
    }

    void MainPage::selectRecentChat(const QString& _aimId)
    {
        assert(contactListWidget_);
        if (contactListWidget_ && !_aimId.isEmpty())
            contactListWidget_->select(_aimId);
    }

    void MainPage::settingsTabActivate(Utils::CommonSettingsType _item)
    {
        selectTab(Tabs::Settings);
        selectSettings(_item);

        if (frameCountMode_ == FrameCountMode::_1)
            switchToContentFrame();
    }

    void MainPage::onVoipShowVideoWindow(bool _enabled)
    {
        if (!videoWindow_)
        {
            videoWindow_ = new(std::nothrow) VideoWindow();
            connect(videoWindow_, &VideoWindow::finished, Utils::InterConnector::instance().getMainWindow(), &MainWindow::exit);
            Ui::GetDispatcher()->getVoipController().updateActivePeerList();
        }

        if (!!videoWindow_)
        {
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
                {
                    raise();
                }
            }
        }

        Utils::InterConnector::instance().getMainWindow()->updateMainMenu();
    }

    void MainPage::hideInput()
    {
        setSidebarVisible(false);
        contactDialog_->hideInput();
    }

    void MainPage::showPlaceholder(Utils::PlaceholdersType _placeholdersType)
    {
        switch(_placeholdersType)
        {
        case Utils::PlaceholdersType::PlaceholdersType_SetExistanseOnIntroduceYourself:
            needShowIntroduceYourself_ = true;
            break;

        case Utils::PlaceholdersType::PlaceholdersType_SetExistanseOffIntroduceYourself:
            if (needShowIntroduceYourself_)
            {
                if (introduceYourselfSuggestions_)
                {
                    introduceYourselfSuggestions_->hide();
                    pages_->removeWidget(introduceYourselfSuggestions_);
                }

                pages_->poproot();

                needShowIntroduceYourself_ = false;
            }
            break;

        case Utils::PlaceholdersType::PlaceholdersType_IntroduceYourself:
            if (!needShowIntroduceYourself_)
                break;

            pages_->insertWidget(0, showIntroduceYourselfSuggestions(pages_));
            pages_->poproot();
            break;

        default:
            break;
        }
    }

    QWidget* MainPage::showIntroduceYourselfSuggestions(QWidget* _parent)
    {
        if (!introduceYourselfSuggestions_)
        {
            introduceYourselfSuggestions_ = new IntroduceYourself(MyInfo()->aimId(), MyInfo()->friendly(), _parent);
            introduceYourselfSuggestions_->setSizePolicy(QSizePolicy::Policy::Expanding, QSizePolicy::Policy::Expanding);
            introduceYourselfSuggestions_->setStyleSheet(qsl("border: 20px solid red;"));
        }
        return introduceYourselfSuggestions_;
    }

    void MainPage::post_stats_with_settings()
    {
        Utils::getStatsSender()->trySendStats();
    }

    void MainPage::myInfo()
    {
        if (MyInfo()->friendly().isEmpty())
        {
            if (!recvMyInfo_)
            {
                emit Utils::InterConnector::instance().showPlaceholder(Utils::PlaceholdersType::PlaceholdersType_SetExistanseOnIntroduceYourself);
                emit Utils::InterConnector::instance().showPlaceholder(Utils::PlaceholdersType::PlaceholdersType_IntroduceYourself);
            }
        }
        else
        {
            emit Utils::InterConnector::instance().showPlaceholder(Utils::PlaceholdersType::PlaceholdersType_SetExistanseOffIntroduceYourself);
        }
        recvMyInfo_ = true;
    }

    void MainPage::openCreatedGroupChat()
    {
        auto conn = std::make_shared<QMetaObject::Connection>();
        *conn = connect(GetDispatcher(), &core_dispatcher::openChat, this, [this, conn](const QString& _aimId)
        {
            QTimer::singleShot(500, this, [this, id = _aimId]() { contactListWidget_->select(id); });
            disconnect(*conn);
        });
    }

    void MainPage::updateSidebarPos(int _width)
    {
        if (_width >= getFrame3MinWith()
            || (clContainer_->width() != getRecentMiniModeWidth() && _width < getFrame2MinWith())
            || (clContainer_->width() == getRecentMiniModeWidth() && _width < getFrame2MiniModeMinWith()))
        {
            insertSidebarToSplitter();
        }
        else if (isSideBarInSplitter())
        {
            if (sidebar_->isVisible())
            {
                if (pagesContainer_->width() <= getChatMinWidth())
                    takeSidebarFromSplitter();
            }
            else
            {
                if (pagesContainer_->width() < (getChatMinWidth() + Sidebar::getDefaultWidth() + splitterHandleWidth()))
                    takeSidebarFromSplitter();
            }
        }
        else
        {
            if (pagesContainer_->width() >= (getChatMinWidth() + Sidebar::getDefaultWidth() + splitterHandleWidth()))
                insertSidebarToSplitter();
        }

        updateSplitterHandle(_width);
    }

    void MainPage::updateSplitterHandle(int _width)
    {
        if (auto handle = splitter_->handle(1))
        {
            const bool enableHandle = (_width > getFrame2MinWith() && !sidebar_->isVisible())
                || (_width > (getFrame2MinWith() + Sidebar::getDefaultWidth()) && sidebar_->isVisible());
            handle->setDisabled(!enableHandle);
        }
    }

    bool MainPage::isSideBarInSplitter() const
    {
        return splitter_->indexOf(sidebar_) != -1;
    }

    void MainPage::insertSidebarToSplitter()
    {
        if (!isSideBarInSplitter())
        {
            Testing::setAccessibleName(sidebar_, qsl("AS sidebar_"));
            splitter_->addWidget(sidebar_);
            if (auto handle = splitter_->handle(splitter_->count() - 1))
                handle->setDisabled(true);
        }
        sidebar_->setNeedShadow(false);
    }

    void MainPage::takeSidebarFromSplitter()
    {
        sidebar_->setNeedShadow(true);
    }

    void MainPage::popPagesToRoot()
    {
        pages_->poproot();
    }

    void MainPage::liveChatSelected()
    {
        pages_->push(liveChatsPage_);
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
        assert(_newState > LeftPanelState::min && _newState < LeftPanelState::max);

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
                Testing::setAccessibleName(contactsWidget_, qsl("AS contactsWidget_"));
                splitter_->replaceWidgetImpl(0, clContainer_);
            }

            newPanelWidth = ::Ui::ItemWidth(false, false);
            contactListWidget_->setPictureOnlyView(false);
            settingsTab_->setCompactMode(false);
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
                splitter_->replaceWidgetImpl(0, clContainer_);
            clContainer_->setFixedWidth(newPanelWidth);
            topWidget_->getSearchWidget()->clearInput();
            settingsTab_->setCompactMode(true);
            tabWidget_->hideTabbar();
            if (updaterButton_)
                updaterButton_->hide();
            expandButton_->show();
            spacerBetweenHeaderAndRecents_->hide();

            if (getCurrentTab() == Tabs::Recents)
                openRecents();

            if (currentTab_ != RECENTS)
                contactListWidget_->changeTab(RECENTS);

            hideSemiWindow();
        }
        else if (leftPanelState_ == LeftPanelState::spreaded)
        {
            clSpacer_->setFixedWidth(getRecentMiniModeWidth());
            splitter_->replaceWidgetImpl(0, clSpacer_);
            //clHostLayout_->removeWidget(contactsWidget_);

            //contactsLayout->setParent(contactsWidget_);
            newPanelWidth = Utils::scale_value(360);

            showSemiWindow();
            contactListWidget_->setItemWidth(newPanelWidth);
            clContainer_->setParent(this);
            clContainer_->setFixedWidth(newPanelWidth);
            clContainer_->raise();
            clContainer_->show();
            expandButton_->hide();
            spacerBetweenHeaderAndRecents_->show();
        }
        else
        {
            assert(false && "Left Panel state does not exist.");
        }
        contactsTab_->setState(leftPanelState_);
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
            splitter_->setStretchFactor(i, i > 0 ? 1 : 0);
    }

    void MainPage::searchActivityChanged(bool _isActive)
    {
        if (_isActive)
        {
            contactListWidget_->switchToContactListWithHeaders(leftPanelState_ == LeftPanelState::normal ? ContactList::SwichType::Animated : ContactList::SwichType::Simple);
            topWidget_->setRegime(TopPanelWidget::Regime::Search);
        }
    }

    bool MainPage::isVideoWindowActive() const
    {
        return videoWindow_ && videoWindow_->isActiveWindow();
    }

    bool MainPage::isVideoWindowMinimized() const
    {
        return videoWindow_ && videoWindow_->isMinimized();
    }

    void MainPage::setFocusOnInput()
    {
        if (contactDialog_)
            contactDialog_->setFocusOnInputWidget();
    }

    void MainPage::clearSearchFocus()
    {
        topWidget_->getSearchWidget()->clearFocus();
    }

    void MainPage::onSendMessage(const QString& contact)
    {
        if (contactDialog_)
            contactDialog_->onSendMessage(contact);
    }

    void MainPage::startSearchInDialog(const QString& _aimid)
    {
        if (frameCountMode_ == FrameCountMode::_1 && oneFrameMode_ != OneFrameMode::Tab)
            switchToTabFrame();

        selectTab(Tabs::Recents);

        topWidget_->setRegime(TopPanelWidget::Regime::Search);
        topWidget_->getSearchWidget()->clearInput();

        contactListWidget_->setSearchMode(true);

        if (_aimid.isEmpty())
            contactListWidget_->setSearchInAllDialogs();
        else
            contactListWidget_->setSearchInDialog(_aimid);
        contactListWidget_->showSearch();

        setSearchFocus();

        GetDispatcher()->post_stats_to_core(core::stats::stats_event_names::cl_search_dialog);
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
        emit Utils::InterConnector::instance().hideSearchDropdown();
    }

    void MainPage::recentsTopPanelBack()
    {
        if (!prevSearchInput_.isEmpty())
        {
            NeedShowUnknownsHeader_ = false;
            topWidget_->setRegime(TopPanelWidget::Regime::Recents);
            contactListWidget_->switchToRecents();
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

        if (frameCountMode_ == FrameCountMode::_1 && oneFrameMode_ != OneFrameMode::Tab)
            switchToTabFrame();


        SearchWidget* widget = nullptr;
        if (getCurrentTab() == Tabs::Contacts)
        {
            widget = contactsTab_->getSearchWidget();
        }
        else
        {
            widget = topWidget_->getSearchWidget();
        }

        if (widget)
        {
            widget->setFocus();
            widget->setTempPlaceholderText(config::get().is_on(config::features::has_nicknames) ? QT_TRANSLATE_NOOP("add_widget", "Enter nickname") : QT_TRANSLATE_NOOP("add_widget", "Enter email"));
        }
    }

    void MainPage::openRecents()
    {
        searchEnd();
        emit Utils::InterConnector::instance().hideSearchDropdown();
        topWidget_->setRegime(TopPanelWidget::Regime::Recents);
        contactListWidget_->switchToRecents();
    }

    void MainPage::onShowWindowForTesters()
    {
        emit Utils::InterConnector::instance().showDebugSettings();
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
        if (frameCountMode_ == FrameCountMode::_1)
        {
            switchToTabFrame();
            settingsHeader_->setText(QString());
        }
        else
        {
            settingsHeader_->hide();
            pages_->poproot();

            openRecents();

            selectTab(Tabs::Recents);

            emit Utils::InterConnector::instance().headerBack();
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
            emit Utils::InterConnector::instance().hideSearchDropdown();

        emit Utils::InterConnector::instance().currentPageChanged();
    }

    void MainPage::tabBarCurrentChanged(int _index)
    {
        if (fastDropSearchEnabled())
        {
            topWidget_->getSearchWidget()->setNeedClear(true);
            topWidget_->getSearchWidget()->clearInput();
            emit contactsTab_->getContactListWidget()->forceSearchClear(true);
            topWidget_->getSearchWidget()->setNeedClear(false);
        }

        const auto tab = getTabByIndex(_index);
        switch (tab)
        {
        case Tabs::Recents:
        {
            settingsHeader_->hide();
            pages_->setCurrentWidget(contactDialog_);
        }
        break;
        case Tabs::Settings:
        {
            setSidebarVisible(false);
            if (frameCountMode_ != FrameCountMode::_1)
                selectSettings(settingsTab_->currentSettingsItem());
        }
        break;

        case Tabs::Contacts:
        {
            settingsHeader_->hide();
            pages_->setCurrentWidget(contactDialog_);
        }
        break;

        default:
            assert(!"unsupported tab");
            break;
        }

        notifyApplicationWindowActive(tab != Tabs::Settings);

        if (tab != Tabs::Recents && topWidget_->getSearchWidget()->isEmpty())
            openRecents();
    }

    void MainPage::tabBarClicked(int _index)
    {
        const auto tab = getTabByIndex(_index);
        if (tab == Tabs::Recents && getCurrentTab() == tab)
        {
            changeCLHeadToSearchSlot();
            if (unreadCounters_.unreadMessages_ == 0 && !unreadCounters_.attention_)
                return;

            Ui::GetDispatcher()->post_stats_to_core(core::stats::stats_event_names::titlebar_message);

            emit Utils::InterConnector::instance().activateNextUnread();
        }
        else if (tab == Tabs::Recents)
        {
            Ui::GetDispatcher()->post_stats_to_core(core::stats::stats_event_names::chatscr_view);
        }
        else if (tab == Tabs::Settings)
        {
            Ui::GetDispatcher()->post_stats_to_core(core::stats::stats_event_names::settingsscr_view);
        }
    }

    void MainPage::tabBarRightClicked(int _index)
    {
        const auto tab = getTabByIndex(_index);
        if (tab == Tabs::Settings)
        {
            if (Utils::keyboardModifiersCorrespondToKeyComb(QGuiApplication::keyboardModifiers(), Utils::KeyCombination::Ctrl_or_Cmd_AltShift))
                onShowWindowForTesters();
        }
    }

    Tabs MainPage::getTabByIndex(int _index) const
    {
        const auto it = std::find_if(indexToTabs_.begin(), indexToTabs_.end(), [_index](auto x) { return x.first == _index; });
        if (it != indexToTabs_.end())
            return it->second;
        assert(false);
        return Tabs::Recents;
    }

    int MainPage::getIndexByTab(Tabs _tab) const
    {
        const auto it = std::find_if(indexToTabs_.begin(), indexToTabs_.end(), [_tab](auto x) { return x.second == _tab; });
        if (it != indexToTabs_.end())
            return it->first;
        assert(false);
        return 0;
    }

    Tabs MainPage::getCurrentTab() const
    {
        return getTabByIndex(tabWidget_->currentIndex());
    }

    void MainPage::selectSettings(Utils::CommonSettingsType _type)
    {
        settingsTab_->selectSettings(_type);
    }

    void MainPage::selectTab(Tabs _tab)
    {
        tabWidget_->setCurrentIndex(getIndexByTab(_tab));
    }

    bool MainPage::isVideoWindowOn() const
    {
        return videoWindow_ && !videoWindow_->isHidden();
    }

    void MainPage::maximizeVideoWindow()
    {
        if (!videoWindow_)
            return;

        videoWindow_->showMaximized();
    }

    bool MainPage::isVideoWindowMaximized() const
    {
        return videoWindow_ && videoWindow_->isMaximized();
    }

    bool MainPage::isVideoWindowFullscreen() const
    {
        return videoWindow_ && videoWindow_->isInFullscreen();
    }
}
