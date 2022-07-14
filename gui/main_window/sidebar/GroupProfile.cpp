#include "stdafx.h"
#include "GroupProfile.h"
#include "SidebarUtils.h"
#include "GalleryList.h"
#include "EditNicknameWidget.h"
#include "../MainWindow.h"
#include "../GroupChatOperations.h"
#include "../ReportWidget.h"
#include "../containers/FriendlyContainer.h"
#include "../contact_list/TopPanel.h"
#include "../contact_list/ChatMembersModel.h"
#include "../contact_list/IgnoreMembersModel.h"
#include "../contact_list/ContactListModel.h"
#include "../contact_list/SelectionContactsForGroupChat.h"
#include "../contact_list/ContactListUtils.h"
#include "../contact_list/ContactListWidget.h"
#include "../contact_list/ContactListItemDelegate.h"
#include "../contact_list/SearchWidget.h"
#include "../contact_list/ChatContactsModel.h"
#include "../history_control/ChatEventInfo.h"
#include "../history_control/HistoryControlPage.h"
#include "../settings/themes/WallpaperDialog.h"
#include "../../previewer/toast.h"
#include "../../styles/ThemesContainer.h"
#include "../../controls/DialogButton.h"
#include "../../controls/ContextMenu.h"
#include "../../controls/GeneralDialog.h"
#include "../../utils/stat_utils.h"
#include "../../utils/features.h"
#include "../../utils/animations/WidgetAnimations.h"
#include "../../../common.shared/config/config.h"
#include "../../controls/StartCallButton.h"

namespace
{
    enum widgets
    {
        main = 0,
        settings = 1,
        list = 2,
        gallery = 3,
        thread_info = 4,
    };

    enum list_page
    {
        all = 0,
        pending = 1,
        blocked = 2,
        admins = 3,
        invites = 4,
    };

    enum main_action
    {
        invite,
        unblock,
        join,
        cancel_join,
    };

    constexpr auto ICON_SIZE = 20;
    constexpr auto BTN_ICON_SIZE = 32;
    constexpr auto AVATAR_SIZE = 52;
    constexpr auto SPACER_MARGIN = 8;
    constexpr auto SHORT_MEMBERS_COUNT = 5;
    constexpr auto APPROVE_ALL_BUTTON_HEIGHT = 48;
    constexpr auto INFO_SPACER_HEIGHT = 8;
    constexpr auto BOTTOM_SPACER = 40;
    constexpr auto ABOUT_LINK_SPACER_HEIGHT = 24;
    constexpr auto ABOUT_LINK_RIGHT_OFFSET = 16;
    constexpr auto APPLY_SETTINGS_MARGIN = 24;
    constexpr auto SETTINGS_LEFT_OFFSET = 20;
    constexpr auto SETTINGS_ADD_OFFSET = 4;

    constexpr QSize headerIconSize() noexcept { return { 24, 24 }; }

    QString addToChatText(const std::shared_ptr<Data::ChatInfo>& _info)
    {
        return Data::ChatInfo::isChannel(_info)
            ? QT_TRANSLATE_NOOP("sidebar", "Add to channel")
            : QT_TRANSLATE_NOOP("sidebar", "Add to group");
    }

    QMap<QString, QVariant> makeData(const QString& command, const QString& aimId)
    {
        QMap<QString, QVariant> result;
        result[qsl("command")] = command;
        result[qsl("aimid")] = aimId;
        return result;
    }

    QMap<QString, QVariant> makeCopyData(const QString& _command)
    {
        QMap<QString, QVariant> result;
        result[qsl("command")] = _command;
        return result;
    }

    void setCheckedBlocking(Ui::SidebarCheckboxButton* _checkbox, bool _isChecked)
    {
        if (_checkbox)
        {
            QSignalBlocker sb(_checkbox);
            _checkbox->setChecked(_isChecked);
        }
    }

    void setCheckboxEnabled(Ui::SidebarCheckboxButton* _checkbox, bool _isEnabled)
    {
        if (_checkbox)
            _checkbox->setEnabled(_isEnabled);
    }
}

namespace Ui
{
    GroupProfile::GroupProfile(QWidget* _parent)
        : SidebarPage(_parent)
        , galleryWidget_(nullptr)
        , frameCountMode_(FrameCountMode::_1)
        , shortChatModel_(new Logic::ChatMembersModel(this))
        , listChatModel_(new Logic::ChatMembersModel(this))
        , currentGalleryPage_(MediaContentType::Invalid)
        , currentListPage_(-1)
        , mainAction_(-1)
        , galleryIsEmpty_(false)
        , invalidNick_(false)
        , infoPlaceholderVisible_(false)
    {
        init();
    }

    GroupProfile::~GroupProfile() = default;

    void GroupProfile::initFor(const QString& _aimId, SidebarParams _params)
    {
        isActive_ = true;

        const auto newContact = currentAimId_ != _aimId;
        currentAimId_ = _aimId;

        if (threadsAutoSubscribe_)
        {
            stateAllThreadsAutoSubscribe_ = threadsAutoSubscribe_->isChecked() || Logic::getContactListModel()->areYouSubscribedToThreadInChat(currentAimId_);
            threadsAutoSubscribe_->setChecked(stateAllThreadsAutoSubscribe_);
            connect(threadsAutoSubscribe_, &SidebarCheckboxButton::checked, this, &GroupProfile::threadAllSubscriptionChecked);
        }

        setFrameCountMode(_params.frameCountMode_);
        loadChatInfo();

        if (isThread())
        {
            changeTab(thread_info);
            listChatModel_->setAimId(_aimId);
        }
        else if (newContact)
        {
            info_->setAimIdAndSize(_aimId, Utils::scale_value(AVATAR_SIZE));

            Testing::setAccessibleName(this, qsl("AS Sidebar GroupProfile ") + _aimId);
            Testing::setAccessibleName(info_, qsl("AS Sidebar GroupProfile AvatarNameInfo ") + _aimId);

            galleryPreview_->setAimId(_aimId);
            shortMembersList_->clearCache();

            shortChatModel_->setAimId(_aimId);
            listChatModel_->setAimId(_aimId);

            setInfoPlaceholderVisible(true);

            updatePinButton();
            updateMuteState();

            dialogGalleryState(currentAimId_, Logic::getContactListModel()->getGalleryState(currentAimId_));

            about_->hide();
            rules_->hide();
            link_->hide();
            share_->hide();
            groupStatus_->hide();
            mainActionButton_->setEnabled(false);
            callButton_->hide();
            membersWidget_->hide();

            changeTab(main);
            area_->ensureVisible(0, 0);

            galleryIsEmpty_ = true;
        }

        Ui::GetDispatcher()->post_stats_to_core(core::stats::stats_event_names::profilescr_view, { { "chat_type", Utils::chatTypeByAimId(currentAimId_) } });

        invalidNick_ = false;
        if (callButton_)
            callButton_->setAimId(currentAimId_);

        emitContentsChanged();
        disableFading();

        elapsedTimer_.start();
    }

    void GroupProfile::setFrameCountMode(FrameCountMode _mode)
    {
        if (frameCountMode_ != _mode)
        {
            frameCountMode_ = _mode;
            updateCloseIcon();
            updateChatControls();
        }
    }

    void GroupProfile::close()
    {
        closeGallery();
        changeTab(isThread() ? thread_info : main);

        isActive_ = false;
    }

    void GroupProfile::setMembersVisible(bool _on)
    {
        if (_on)
            allMembersClicked();
        else
            changeTab(isThread() ? thread_info : main);
    }

    bool GroupProfile::isMembersVisible() const
    {
        return (stackedWidget_->currentIndex() == list);
    }

    void GroupProfile::resizeEvent(QResizeEvent* _event)
    {
        if (cl_)
        {
            cl_->setWidthForDelegate(width());
            for (auto w : { membersWidget_, galleryWidget_, buttonsWidget_, aboutLinkToChatBlock_ })
            {
                if (w)
                {
                    w->layout()->invalidate();
                    w->setFixedWidth(width());
                    w->update();
                }
            }

            for (auto w : { members_, threadMembers_ })
            {
                if (w)
                    w->setFixedWidth(width());
            }

            for (auto w : { notifications_, threadSubscription_, threadsAutoSubscribe_ })
            {
                if (w)
                {
                    w->setFixedWidth(width());
                    w->update();
                }
            }
                galleryPopup_->setFixedWidth(std::max(0, width() - GalleryPopup::horOffset() * 2));
        }
        SidebarPage::resizeEvent(_event);
    }

    void GroupProfile::init()
    {
        auto layout = Utils::emptyVLayout(this);

        auto areaContainer = new QWidget(this);
        auto areaContainerLayout = Utils::emptyVLayout(areaContainer);

        titleBar_ = new HeaderTitleBar;
        titleBar_->setStyleSheet(ql1s("background-color: %1; border-bottom: %2 solid %3;").arg(Styling::getParameters().getColorHex(Styling::StyleVariable::BASE_GLOBALWHITE),
            QString::number(Utils::scale_value(1)) % u"px",
            Styling::getParameters().getColorHex(Styling::StyleVariable::BASE_BRIGHT)));
        titleBar_->setTitle(pageTitle(main));
        titleBar_->setButtonSize(Utils::scale_value(headerIconSize()));
        {
            editButton_ = new HeaderTitleBarButton(this);
            Testing::setAccessibleName(editButton_, qsl("AS Sidebar GroupProfile editButton"));
            editButton_->setDefaultImage(qsl(":/context_menu/edit"), Styling::ThemeColorKey{ Styling::StyleVariable::BASE_SECONDARY }, headerIconSize());
            editButton_->setCustomToolTip(QT_TRANSLATE_NOOP("context_menu", "Edit"));
            titleBar_->addButtonToRight(editButton_);
            connect(editButton_, &HeaderTitleBarButton::clicked, this, &GroupProfile::editButtonClicked);
        }
        {
            closeButton_ = new HeaderTitleBarButton(this);
            Testing::setAccessibleName(closeButton_, qsl("AS Sidebar GroupProfile closeButton"));
            titleBar_->addButtonToLeft(closeButton_);
            connect(closeButton_, &HeaderTitleBarButton::clicked, this, &GroupProfile::closeClicked);
        }

        connect(titleBar_, &HeaderTitleBar::arrowClicked, this, &GroupProfile::titleArrowClicked);

        areaContainerLayout->addWidget(titleBar_);
        stackedWidget_ = new QStackedWidget(areaContainer);
        Testing::setAccessibleName(stackedWidget_, qsl("AS Sidebar GroupProfile stackedWidget"));
        areaContainerLayout->addWidget(stackedWidget_);

        delegate_ = new Logic::ContactListItemDelegate(this, Logic::MEMBERS_LIST);
        delegate_->setMembersView();

        threadDelegate_ = new Logic::ContactListItemDelegate(this, Logic::THREAD_SUBSCRIBERS);
        threadDelegate_->setMembersView();

        area_ = CreateScrollAreaAndSetTrScrollBarV(stackedWidget_);
        area_->setWidget(initContent(area_));
        area_->setWidgetResizable(true);
        area_->setFrameStyle(QFrame::NoFrame);
        area_->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
        Utils::transparentBackgroundStylesheet(area_);

        stackedWidget_->insertWidget(main, area_);
        stackedWidget_->insertWidget(settings, initSettings(stackedWidget_));
        stackedWidget_->insertWidget(list, initContactList(stackedWidget_));
        stackedWidget_->insertWidget(gallery, initGallery(stackedWidget_));
        stackedWidget_->insertWidget(thread_info, initThreadInfo(stackedWidget_));
        connect(stackedWidget_, &QStackedWidget::currentChanged, this, [this]() { emitContentsChanged(); });

        if (isThread())
            changeTab(thread_info);
        else
            stackedWidget_->setCurrentIndex(main);

        layout->addWidget(areaContainer);

        updateCloseIcon();
        setMouseTracking(true);

        connect(Ui::GetDispatcher(), &Ui::core_dispatcher::chatInfo, this, &GroupProfile::chatInfo);
        connect(Ui::GetDispatcher(), &Ui::core_dispatcher::chatInfoFailed, this, &GroupProfile::chatInfoFailed);
        connect(Ui::GetDispatcher(), &Ui::core_dispatcher::threadInfoLoaded, this, &GroupProfile::threadInfo);
        connect(Ui::GetDispatcher(), &Ui::core_dispatcher::subscriptionChanged, this, &GroupProfile::loadThreadInfo);
        connect(Ui::GetDispatcher(), &Ui::core_dispatcher::dialogGalleryState, this, &GroupProfile::dialogGalleryState);
        connect(Logic::getRecentsModel(), &Logic::RecentsModel::favoriteChanged, this, &GroupProfile::favoriteChanged);
        connect(Logic::getRecentsModel(), &Logic::RecentsModel::unimportantChanged, this, &GroupProfile::unimportantChanged);
        connect(Ui::GetDispatcher(), &Ui::core_dispatcher::setChatRoleResult, this, &GroupProfile::memberActionResult);
        connect(Ui::GetDispatcher(), &Ui::core_dispatcher::blockMemberResult, this, &GroupProfile::memberActionResult);
        connect(Ui::GetDispatcher(), &Ui::core_dispatcher::inviteCancelResult, this, &GroupProfile::memberActionResult);
        connect(Ui::GetDispatcher(), &Ui::core_dispatcher::pendingListResult, this, &GroupProfile::memberActionResult);
        connect(Ui::GetDispatcher(), &Ui::core_dispatcher::modChatParamsResult, this, &GroupProfile::modChatParamsResult);
        connect(Ui::GetDispatcher(), &Ui::core_dispatcher::suggestGroupNickResult, this, &GroupProfile::suggestGroupNickResult);
        connect(Ui::GetDispatcher(), &Ui::core_dispatcher::modChatParamsResult, this, &GroupProfile::loadChatInfo);
        connect(Ui::GetDispatcher(), &Ui::core_dispatcher::threadAutosubscribe, this, &GroupProfile::onThreadAutosubscribe);
        connect(&Utils::InterConnector::instance(), &Utils::InterConnector::chatEvents, this, &GroupProfile::chatEvents);
        connect(&Utils::InterConnector::instance(), &Utils::InterConnector::updateActiveChatMembersModel, this, &GroupProfile::updateActiveChatMembersModel);
        connect(Logic::getContactListModel(), &Logic::ContactListModel::contactChanged, this, &GroupProfile::onContactChanged);
        connect(Logic::getContactListModel(), &Logic::ContactListModel::yourRoleChanged, this, &GroupProfile::onRoleChanged);
        connect(Logic::getRecentsModel(), &Logic::RecentsModel::favoriteChanged, this, &GroupProfile::favoriteChanged);
        connect(&Styling::getThemesContainer(), &Styling::ThemesContainer::globalThemeChanged, this, &GroupProfile::updatePinButton);
        connect(&Utils::InterConnector::instance(), &Utils::InterConnector::refreshGroupInfo, this, &GroupProfile::refresh, Qt::QueuedConnection);
    }

    QWidget* GroupProfile::initContent(QWidget* _parent)
    {
        auto widget = new QWidget(_parent);
        Utils::transparentBackgroundStylesheet(widget);
        Utils::emptyContentsMargins(widget);

        auto layout = Utils::emptyVLayout(widget);
        layout->setAlignment(Qt::AlignTop);

        {
            info_ = addAvatarInfo(widget, layout);
            connect(info_, &AvatarNameInfo::avatarClicked, this, &GroupProfile::avatarClicked);

            setInfoPlaceholderVisible(false);

            infoSpacer_ = new QWidget(widget);
            Utils::transparentBackgroundStylesheet(infoSpacer_);
            infoSpacer_->setFixedHeight(INFO_SPACER_HEIGHT);
            layout->addWidget(infoSpacer_);

            groupStatus_ = addLabel(QString(), widget, layout, 0, TextRendering::HorAligment::CENTER, Fading::On);
            groupStatus_->setMargins(groupStatus_->getMargins() + QMargins(0, 0, 0, Utils::scale_value(8)));

            {
                buttonsWidget_ = new QWidget(widget);
                auto buttonsLayout = Utils::emptyHLayout(buttonsWidget_);

                mainActionButton_ = addColoredButton(QString(), QString(), buttonsWidget_, buttonsLayout, QSize(), Fading::Off);
                Testing::setAccessibleName(mainActionButton_, qsl("AS GroupProfile mainActionButton"));
                connect(mainActionButton_, &ColoredButton::clicked, this, &GroupProfile::mainActionClicked);

                callButton_ = new StartCallButton(buttonsWidget_, CallButtonType::Button);
                buttonsLayout->addWidget(callButton_);
                buttonsLayout->setAlignment(callButton_, Qt::AlignTop);
                buttonsLayout->addSpacing(Utils::scale_value(16));

                Testing::setAccessibleName(callButton_, qsl("AS GroupProfile callButton"));

                layout->addWidget(buttonsWidget_);
            }

            about_ = addInfoBlock(QString(), QString(), widget, layout, 0, Fading::On);
            about_->text_->makeTextField();
            connect(about_->text_, &TextLabel::menuAction, this, &GroupProfile::menuAction);
            Testing::setAccessibleName(about_->text_, qsl("AS GroupProfile about"));

            rules_ = addInfoBlock(QT_TRANSLATE_NOOP("siderbar", "Rules"), QString(), widget, layout, 0, Fading::On);
            rules_->text_->makeTextField();
            connect(rules_->text_, &TextLabel::menuAction, this, &GroupProfile::menuAction);
            Testing::setAccessibleName(rules_->text_, qsl("AS GroupProfile rules"));

            link_ = addInfoBlock(QT_TRANSLATE_NOOP("siderbar", "Link"), QString(), widget, layout, 0, Fading::On);
            link_->text_->showButtons();
            connect(link_->text_, &TextLabel::textClicked, this, &GroupProfile::linkClicked);
            connect(link_->text_, &TextLabel::copyClicked, this, &GroupProfile::linkCopy);
            connect(link_->text_, &TextLabel::shareClicked, this, &GroupProfile::linkShare);
            connect(link_->text_, &TextLabel::menuAction, this, &GroupProfile::menuAction);
            Testing::setAccessibleName(link_->text_, qsl("AS GroupProfile link"));
        }

        addSpacer(widget, layout);

        {
            share_ = addButton(qsl(":/share_icon"), QT_TRANSLATE_NOOP("sidebar", "Share group"), widget, layout, qsl("AS Sidebar GroupProfile shareButton"));
            connect(share_, &SidebarButton::clicked, this, &GroupProfile::shareClicked);

            notifications_ = addCheckbox(qsl(":/notify_icon"), QT_TRANSLATE_NOOP("sidebar", "Notifications"), widget, layout);
            Testing::setAccessibleName(notifications_, qsl("AS Sidebar GroupProfile notificationsCheckbox"));
            connect(notifications_, &SidebarCheckboxButton::checked, this, &GroupProfile::notificationsChecked);

            threadsAutoSubscribe_ = addCheckbox(qsl(":/thread_subscribe"), QT_TRANSLATE_NOOP("sidebar", "Subscribe to all threads"), widget, layout);

            settings_ = addButton(qsl(":/settings_icon_sidebar"), QT_TRANSLATE_NOOP("sidebar", "Group settings"), widget, layout, qsl("AS Sidebar GroupProfile settingsButton"));
            connect(settings_, &SidebarButton::clicked, this, &GroupProfile::settingsClicked);
        }

        secondSpacer_ = addSpacer(widget, layout);

        {
            galleryWidget_ = new QWidget(widget);
            auto galleryFader = new Utils::WidgetFader(galleryWidget_);
            galleryFader->setEventDirection(QEvent::Show, QPropertyAnimation::Forward);
            auto galleryLayout = Utils::emptyVLayout(galleryWidget_);

            galleryPhoto_ = addButton(qsl(":/background_icon"), QT_TRANSLATE_NOOP("sidebar", "Photo and video"), galleryWidget_, galleryLayout, qsl("AS Sidebar GroupProfile photoVideoButton"));
            connect(galleryPhoto_, &SidebarButton::clicked, this, &GroupProfile::galleryPhotoClicked);

            galleryPreview_ = addGalleryPrevieWidget(galleryWidget_, galleryLayout);
            Testing::setAccessibleName(galleryPreview_, qsl("AS Sidebar GroupProfile galleryPreview"));

            galleryVideo_ = addButton(qsl(":/video_icon"), QT_TRANSLATE_NOOP("sidebar", "Video"), galleryWidget_, galleryLayout, qsl("AS Sidebar GroupProfile videoButton"));
            connect(galleryVideo_, &SidebarButton::clicked, this, &GroupProfile::galleryVideoClicked);

            galleryFiles_ = addButton(qsl(":/gallery/file_icon"), QT_TRANSLATE_NOOP("sidebar", "Files"), galleryWidget_, galleryLayout, qsl("AS Sidebar GroupProfile filesButton"));
            connect(galleryFiles_, &SidebarButton::clicked, this, &GroupProfile::galleryFilesClicked);

            galleryLinks_ = addButton(qsl(":/copy_link_icon"), QT_TRANSLATE_NOOP("sidebar", "Links"), galleryWidget_, galleryLayout, qsl("AS Sidebar GroupProfile linksButton"));
            connect(galleryLinks_, &SidebarButton::clicked, this, &GroupProfile::galleryLinksClicked);

            galleryPtt_ = addButton(qsl(":/gallery/micro_icon"), QT_TRANSLATE_NOOP("sidebar", "Voice messages"), galleryWidget_, galleryLayout, qsl("AS Sidebar GroupProfile voiceButton"));
            connect(galleryPtt_, &SidebarButton::clicked, this, &GroupProfile::galleryPttClicked);

            layout->addWidget(galleryWidget_);
        }

        thirdSpacer_ = addSpacer(widget, layout);

        {
            membersWidget_ = new QWidget(widget);
            auto membersFader = new Utils::WidgetFader(membersWidget_);
            membersFader->setEventDirection(QEvent::Show, QPropertyAnimation::Forward);
            auto membersLayout = Utils::emptyVLayout(membersWidget_);

            members_ = addMembersPlate(membersWidget_, membersLayout);
            Testing::setAccessibleName(members_, qsl("AS Sidebar GroupProfile membersButton"));
            connect(members_, &MembersPlate::searchClicked, this, &GroupProfile::searchMembersClicked);

            addToChat_ = addButton(qsl(":/add_groupchat"), addToChatText({}), membersWidget_, membersLayout, qsl("AS Sidebar GroupProfile addGroupChatButton"));
            connect(addToChat_, &SidebarButton::clicked, this, &GroupProfile::addToChatClicked);

            pendings_ = addButton(qsl(":/pending_icon"), QT_TRANSLATE_NOOP("sidebar", "Waiting for approval"), membersWidget_, membersLayout, qsl("AS Sidebar GroupProfile WaitingApprovalButton"));
            connect(pendings_, &SidebarButton::clicked, this, &GroupProfile::pendingsClicked);
            connect(&Utils::InterConnector::instance(), &Utils::InterConnector::showPendingMembers, this, &GroupProfile::pendingsClicked);

            yourInvites_ = addButton(qsl(":/pending_icon"), QT_TRANSLATE_NOOP("sidebar", "Your invites"), membersWidget_, membersLayout, qsl("AS GroupProfile yourInvites"));
            connect(yourInvites_, &SidebarButton::clicked, this, &GroupProfile::yourInvitesClicked);

            shortMembersList_ = addMembersWidget(shortChatModel_, delegate_, SHORT_MEMBERS_COUNT, membersWidget_, membersLayout);
            Testing::setAccessibleName(shortMembersList_, qsl("AS GroupProfile shortMembersList"));
            connect(shortMembersList_, &MembersWidget::selected, this, &GroupProfile::contactSelected);
            connect(shortMembersList_, &MembersWidget::removeClicked, this, &GroupProfile::contactRemoved);
            connect(shortMembersList_, &MembersWidget::moreClicked, this, &GroupProfile::contactMenuRequested);
            if (auto scrollArea = qobject_cast<QScrollArea*>(_parent))
                shortMembersList_->setScrollArea(scrollArea);

            admins_ = addButton(qsl(":/admins_icon"), QT_TRANSLATE_NOOP("sidebar", "Admins"), membersWidget_, membersLayout, qsl("AS GroupProfile adminsList"));
            connect(admins_, &SidebarButton::clicked, this, &GroupProfile::adminsClicked);

            allMembers_ = addButton(qsl(":/members_icon"), QT_TRANSLATE_NOOP("sidebar", "All members"), membersWidget_, membersLayout, qsl("AS GroupProfile allMembersList"));
            connect(allMembers_, &SidebarButton::clicked, this, &GroupProfile::allMembersClicked);

            blockedMembers_ = addButton(qsl(":/blocked_users_icon"), QT_TRANSLATE_NOOP("sidebar", "Deleted and blocked"), membersWidget_, membersLayout, qsl("AS GroupProfile blockedMembersList"));
            connect(blockedMembers_, &SidebarButton::clicked, this, &GroupProfile::blockedMembersClicked);

            layout->addWidget(membersWidget_);
        }

        fourthSpacer_ = addSpacer(widget, layout);

        {
            pin_ = addButton(qsl(":/pin_chat_icon"), QT_TRANSLATE_NOOP("sidebar", "Pin"), widget, layout, qsl("AS Sidebar GroupProfile pinButton"));
            connect(pin_, &SidebarButton::clicked, this, &GroupProfile::pinClicked);

            theme_ = addButton(qsl(":/colors_icon"), QT_TRANSLATE_NOOP("sidebar", "Wallpapers"), widget, layout, qsl("AS Sidebar GroupProfile wallpapersButton"));
            connect(theme_, &SidebarButton::clicked, this, &GroupProfile::themeClicked);
        }

        fifthSpacer_ = addSpacer(widget, layout);

        {
            clearHistory_ = addButton(qsl(":/clear_chat_icon"), QT_TRANSLATE_NOOP("sidebar", "Clear history"), widget, layout, qsl("AS Sidebar GroupProfile clearHistoryButton"));
            connect(clearHistory_, &SidebarButton::clicked, this, &GroupProfile::clearHistoryClicked);

            block_ = addButton(qsl(":/ignore_icon"), QT_TRANSLATE_NOOP("sidebar", "Block"), widget, layout, qsl("AS Sidebar GroupProfile blockButton"));
            connect(block_, &SidebarButton::clicked, this, &GroupProfile::blockClicked);

            report_ = addButton(qsl(":/alert_icon"), QT_TRANSLATE_NOOP("sidebar", "Report and block"), widget, layout, qsl("AS Sidebar GroupProfile reportButton"));
            connect(report_, &SidebarButton::clicked, this, &GroupProfile::reportCliked);

            leave_ = addButton(qsl(":/exit_icon"), QT_TRANSLATE_NOOP("sidebar", "Leave and delete"), widget, layout, qsl("AS Sidebar GroupProfile leaveButton"));
            connect(leave_, &SidebarButton::clicked, this, &GroupProfile::leaveClicked);
        }

        layout->addSpacerItem(new QSpacerItem(0, Utils::scale_value(BOTTOM_SPACER), QSizePolicy::Preferred, QSizePolicy::Fixed));

        layout->setSizeConstraint(QLayout::SetMinimumSize);
        return widget;
    }

    QWidget* GroupProfile::initThreadInfo(QWidget* _parent)
    {
        auto widget = new QWidget(_parent);
        Utils::transparentBackgroundStylesheet(widget);
        Utils::emptyContentsMargins(widget);
        auto layout = Utils::emptyVLayout(widget);
        layout->setAlignment(Qt::AlignTop);

        {
            threadSubscription_ = addCheckbox(qsl(":/thread_subscription"), QT_TRANSLATE_NOOP("sidebar", "Subscription to replies"), widget, layout);
            threadSubscription_->setChecked(Logic::getContactListModel()->areYouSubscribedToThread(currentAimId_));
            Testing::setAccessibleName(threadSubscription_, qsl("AS Sidebar ThreadInfo threadSubscriptionCheckbox"));
            connect(threadSubscription_, &SidebarCheckboxButton::checked, this, &GroupProfile::threadSubscriptionChecked);
        }

        addSpacer(widget, layout);
        {
            auto membersWidget = new QWidget(widget);
            auto membersFader = new Utils::WidgetFader(membersWidget);
            membersFader->setEventDirection(QEvent::Show, QPropertyAnimation::Forward);
            auto membersLayout = Utils::emptyVLayout(membersWidget);
            membersLayout->setAlignment(Qt::AlignTop);
            threadMembers_ = addMembersPlate(membersWidget, membersLayout);
            Testing::setAccessibleName(threadMembers_, qsl("AS Sidebar ThreadInfo subscribersButton"));
            connect(threadMembers_, &MembersPlate::searchClicked, this, &GroupProfile::searchMembersClicked);
            threadMembersList_ = addMembersWidget(shortChatModel_, threadDelegate_, SHORT_MEMBERS_COUNT, membersWidget, membersLayout);
            Testing::setAccessibleName(threadMembersList_, qsl("AS ThreadInfo shortMembersList"));
            connect(threadMembersList_, &MembersWidget::selected, this, &GroupProfile::contactSelected);
            if (auto scrollArea = qobject_cast<QScrollArea*>(_parent); scrollArea)
                threadMembersList_->setScrollArea(scrollArea);

            layout->addWidget(membersWidget);
        }

        layout->addSpacerItem(new QSpacerItem(0, Utils::scale_value(BOTTOM_SPACER), QSizePolicy::Preferred, QSizePolicy::Fixed));
        return widget;
    }

    QWidget* GroupProfile::initSettings(QWidget* _parent)
    {
        auto widget = new QWidget(_parent);
        Utils::transparentBackgroundStylesheet(widget);
        Utils::emptyContentsMargins(widget);

        auto layout = Utils::emptyVLayout(widget);
        layout->setAlignment(Qt::AlignTop);

        public_ = addCheckbox(QString(), QT_TRANSLATE_NOOP("groupchats", "Public chat"), widget, layout);
        Testing::setAccessibleName(public_, qsl("AS Sidebar GroupProfile publicChatCheckbox"));
        connect(public_, &SidebarCheckboxButton::checked, this, &GroupProfile::publicClicked);
        connect(public_, &SidebarCheckboxButton::checked, this, &GroupProfile::opposePublicTrustRequired);

        publicLabel_ = addLabel(QT_TRANSLATE_NOOP("groupchats", "Public group can be found in the search"), widget, layout, Utils::scale_value(SETTINGS_ADD_OFFSET), TextRendering::HorAligment::LEFT, Fading::On);

        aboutLinkToChatBlock_ = new QWidget(widget);
        {
            auto aboutLinkToChatLayout = Utils::emptyVLayout(aboutLinkToChatBlock_);

            {
                auto spacer = new QWidget(widget);
                Utils::transparentBackgroundStylesheet(spacer);
                spacer->setFixedHeight(ABOUT_LINK_SPACER_HEIGHT);
                aboutLinkToChatLayout->addWidget(spacer);
            }

            const auto appName = config::get().string(config::values::product_name_full);
            const QString argName = QString::fromUtf8(appName.data(), appName.size());
            aboutLinkToChat_ = addInfoBlock(QT_TRANSLATE_NOOP("groupchats", "The link will open the group in %1").arg(argName), {}, aboutLinkToChatBlock_, aboutLinkToChatLayout, Utils::scale_value(SETTINGS_ADD_OFFSET), Fading::On);
            aboutLinkToChat_->setHeaderLinkColor(Styling::ThemeColorKey{ Styling::StyleVariable::BASE_PRIMARY });
            aboutLinkToChat_->text_->setCursorForText();
            connect(aboutLinkToChat_->text_, &TextLabel::textClicked, this, &GroupProfile::aboutLinkClicked);

            {
                auto spacer = new QWidget(widget);
                Utils::transparentBackgroundStylesheet(spacer);
                spacer->setFixedHeight(INFO_SPACER_HEIGHT);
                aboutLinkToChatLayout->addWidget(spacer);
            }

            makeNewLink_ = addText(QT_TRANSLATE_NOOP("groupchats", "Make new link"), aboutLinkToChatBlock_, aboutLinkToChatLayout, Utils::scale_value(SETTINGS_ADD_OFFSET), -1, Fading::On);
            Testing::setAccessibleName(makeNewLink_, qsl("AS Sidebar GroupProfile makeNewLink"));
            makeNewLink_->setCursorForText();
            makeNewLink_->setColor(Styling::ThemeColorKey{ Styling::StyleVariable::TEXT_PRIMARY });
            connect(makeNewLink_, &TextLabel::textClicked, this, &GroupProfile::makeNewLink);

            {
                auto spacer = new QWidget(widget);
                Utils::transparentBackgroundStylesheet(spacer);
                spacer->setFixedHeight(ABOUT_LINK_SPACER_HEIGHT);
                aboutLinkToChatLayout->addWidget(spacer);
            }

        }
        layout->addWidget(aboutLinkToChatBlock_);

        {
            EditNicknameWidget::FormData initData;
            initData.fixedPart_ = Utils::getDomainUrl() % ql1c('/');
            initData.groupMode_ = true;
            initData.fixedSize_ = false;
            initData.margins_ = QMargins(Utils::scale_value(SETTINGS_LEFT_OFFSET), Utils::scale_value(ABOUT_LINK_SPACER_HEIGHT), Utils::scale_value(ABOUT_LINK_RIGHT_OFFSET), 0);
            editGroupLinkWidget_ = new EditNicknameWidget(widget, initData);
            editGroupLinkWidget_->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Fixed);
            layout->addWidget(editGroupLinkWidget_);
            editGroupLinkWidget_->hide();
        }

        if (Features::isRestrictedFilesEnabled())
        {
            trustRequired_ = addCheckbox({}, QT_TRANSLATE_NOOP("groupchats", "File access restrictions"), widget, layout);
            Testing::setAccessibleName(trustRequired_, qsl("AS Sidebar GroupProfile trustCheckbox"));

            trustRequired_->setCheckValidator([this](bool _desiredValue) { return validateTrustRequiredCheck(_desiredValue); });

            connect(trustRequired_, &SidebarCheckboxButton::clicked, this, &GroupProfile::checkApplyButton);
            connect(trustRequired_, &SidebarCheckboxButton::disabledClicked, this, [this]()
            {
                if (chatInfo_ && chatInfo_->trustRequired_)
                    Utils::showToastOverMainWindow(QT_TRANSLATE_NOOP("sidebar", "Cannot be disabled due to security reasons"), Utils::defaultToastVerOffset());
            });
            connect(trustRequired_, &SidebarCheckboxButton::checked, this, &GroupProfile::opposePublicTrustRequired);

            trustLabel_ = addLabel(QT_TRANSLATE_NOOP("groupchats", "Restrict file access to trusted devices only\nOnly for private groups\n\nThis option cannot be disabled"), widget, layout, Utils::scale_value(SETTINGS_ADD_OFFSET), TextRendering::HorAligment::LEFT, Fading::On);
        }

        joinModeration_ = addCheckbox(QString(), QT_TRANSLATE_NOOP("groupchats", "Join with Approval"), widget, layout);
        Testing::setAccessibleName(joinModeration_, qsl("AS Sidebar GroupProfile joinModerationCheckbox"));
        connect(joinModeration_, &SidebarCheckboxButton::checked, this, &GroupProfile::checkApplyButton);

        approvalLabel_ = addLabel(QT_TRANSLATE_NOOP("groupchats", "New members are waiting for admin approval"), widget, layout, Utils::scale_value(SETTINGS_ADD_OFFSET), TextRendering::HorAligment::LEFT, Fading::On);

        layout->addItem(new QSpacerItem(0, Utils::scale_value(APPLY_SETTINGS_MARGIN), QSizePolicy::Preferred, QSizePolicy::Fixed));

        if (Features::isThreadsForbidEnabled())
        {
            threadsEnabled_ = addCheckbox(QString(), QT_TRANSLATE_NOOP("groupchats", "Enable thread feeds"), widget, layout);
            Testing::setAccessibleName(threadsEnabled_, qsl("AS Sidebar GroupProfile threadsElabledCheckbox"));
            connect(threadsEnabled_, &SidebarCheckboxButton::checked, this, &GroupProfile::checkApplyButton);
            addLabel(QT_TRANSLATE_NOOP("groupchats", "Allow chat members to create threads"), widget, layout, Utils::scale_value(SETTINGS_ADD_OFFSET), TextRendering::HorAligment::LEFT, Fading::On);

            layout->addItem(new QSpacerItem(0, Utils::scale_value(APPLY_SETTINGS_MARGIN), QSizePolicy::Preferred, QSizePolicy::Fixed));
        }

        auto hLayout = Utils::emptyHLayout();
        hLayout->setContentsMargins(Utils::scale_value(SETTINGS_LEFT_OFFSET), 0, 0, 0);
        applyChatSettings_ = new DialogButton(widget, QT_TRANSLATE_NOOP("groupchats", "Apply"));
        hLayout->addWidget(applyChatSettings_);
        hLayout->addSpacerItem(new QSpacerItem(0, 0, QSizePolicy::Expanding));
        layout->addLayout(hLayout);

        connect(editGroupLinkWidget_, &Ui::EditNicknameWidget::ready, this, [this]() { invalidNick_ = false; checkApplyButton(); });
        connect(editGroupLinkWidget_, &Ui::EditNicknameWidget::theSame, this, [this]() { invalidNick_ = true; checkApplyButton(); });
        connect(editGroupLinkWidget_, &Ui::EditNicknameWidget::error, this, [this]() { invalidNick_ = true; checkApplyButton(); });

        connect(applyChatSettings_, &Ui::DialogButton::clicked, this, &GroupProfile::applyChatSettings);

        return widget;
    }

    QWidget* GroupProfile::initContactList(QWidget* _parent)
    {
        auto widget = new QWidget(_parent);
        auto contactListLayout = Utils::emptyVLayout(widget);

        searchWidget_ = new SearchWidget(widget);
        searchWidget_->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Preferred);
        Testing::setAccessibleName(searchWidget_, qsl("AS GroupProfile search"));
        contactListLayout->addWidget(searchWidget_);

        contactListLayout->addSpacerItem(new QSpacerItem(0, Utils::scale_value(SPACER_MARGIN), QSizePolicy::Preferred, QSizePolicy::Fixed));

        cl_ = new Ui::ContactListWidget(widget, Logic::MembersWidgetRegim::MEMBERS_LIST, listChatModel_);
        cl_->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Expanding);
        cl_->setContentsMargins(0, 0, 0, 0);
        cl_->setClDelegate(delegate_);
        cl_->connectSearchWidget(searchWidget_);
        Testing::setAccessibleName(cl_, qsl("AS GroupProfile contactList"));

        connect(cl_, &ContactListWidget::selected, this, &GroupProfile::contactSelected);
        connect(cl_, &ContactListWidget::removeClicked, this, &GroupProfile::contactRemoved);
        connect(cl_, &ContactListWidget::moreClicked, this, &GroupProfile::contactMenuRequested);
        connect(cl_, &ContactListWidget::approve, this, &GroupProfile::contactMenuApproved);

        contactListLayout->addWidget(cl_);

        approveAll_ = addButton(qsl(":/waiting_for_accept_icon"), QT_TRANSLATE_NOOP("sidebar", "Approve All"), widget, contactListLayout);
        approveAll_->setFixedHeight(Utils::scale_value(APPROVE_ALL_BUTTON_HEIGHT));
        connect(approveAll_, &SidebarButton::clicked, this, &GroupProfile::approveAllClicked);

        return widget;
    }

    QWidget* GroupProfile::initGallery(QWidget* _parent)
    {
        galleryPopup_ = new GalleryPopup();
        connect(galleryPopup_, &GalleryPopup::galleryPhotoClicked, this, &GroupProfile::galleryPhotoClicked);
        connect(galleryPopup_, &GalleryPopup::galleryVideoClicked, this, &GroupProfile::galleryVideoClicked);
        connect(galleryPopup_, &GalleryPopup::galleryFilesClicked, this, &GroupProfile::galleryFilesClicked);
        connect(galleryPopup_, &GalleryPopup::galleryLinksClicked, this, &GroupProfile::galleryLinksClicked);
        connect(galleryPopup_, &GalleryPopup::galleryPttClicked, this, &GroupProfile::galleryPttClicked);

        auto widget = new QWidget(_parent);
        auto layout = Utils::emptyVLayout(widget);

        gallery_ = new GalleryList(widget, QString());
        layout->addWidget(gallery_);

        return widget;
    }

    void GroupProfile::initTexts()
    {
        const auto chan = isChannel();

        share_->setText(chan ? QT_TRANSLATE_NOOP("sidebar", "Share channel") : QT_TRANSLATE_NOOP("sidebar", "Share group"));
        settings_->setText(chan ? QT_TRANSLATE_NOOP("sidebar", "Channel settings") : QT_TRANSLATE_NOOP("sidebar", "Group settings"));
        addToChat_->setText(addToChatText(chatInfo_));
        public_->setText(chan ? QT_TRANSLATE_NOOP("groupchats", "Public channel") : QT_TRANSLATE_NOOP("sidebar", "Public group"));
        const auto appName = config::get().string(config::values::product_name_full);
        const QString argName = QString::fromUtf8(appName.data(), appName.size());
        aboutLinkToChat_->setHeaderText(QString(chan ? QT_TRANSLATE_NOOP("groupchats", "The link will open the channel in %1") : QT_TRANSLATE_NOOP("groupchats", "The link will open the group in %1")).arg(argName));
        aboutLinkToChat_->setHeaderLinkColor(Styling::ThemeColorKey{ Styling::StyleVariable::BASE_PRIMARY });
        allMembers_->setText(chan ? QT_TRANSLATE_NOOP("sidebar", "All subscribers") : QT_TRANSLATE_NOOP("sidebar", "All members"));
        publicLabel_->setText(chan ? QT_TRANSLATE_NOOP("groupchats", "The public channel can be found in the search and its link will be available to all members") : QT_TRANSLATE_NOOP("groupchats", "The public group can be found in the search and its link will be available to all members"));
        approvalLabel_->setText(chan ? QT_TRANSLATE_NOOP("groupchats", "New subscribers are waiting for admin approval") : QT_TRANSLATE_NOOP("groupchats", "New members are waiting for admin approval"));
    }

    QString GroupProfile::pageTitle(int _pageIndex) const
    {
        switch (_pageIndex)
        {
        case main:
        case thread_info:
            return QT_TRANSLATE_NOOP("sidebar", "Information");
        case settings:
            return isChannel() ? QT_TRANSLATE_NOOP("sidebar", "Channel settings") : QT_TRANSLATE_NOOP("sidebar", "Group settings");
        case list:
            return isChannel() || isThread() ? QT_TRANSLATE_NOOP("sidebar", "Subscribers") : QT_TRANSLATE_NOOP("sidebar", "Members");
        case gallery:
            return getGalleryTitle(currentGalleryPage_);
        }
        return QString();
    }

    void GroupProfile::updateCloseIcon()
    {
        QString iconPath;
        QString tooltipText;
        if (frameCountMode_ == FrameCountMode::_1 || stackedWidget_->currentIndex() != main)
        {
            iconPath = qsl(":/controls/back_icon");
            tooltipText = QT_TRANSLATE_NOOP("sidebar", "Back");
        }
        else
        {
            iconPath = qsl(":/controls/close_icon");
            tooltipText = QT_TRANSLATE_NOOP("sidebar", "Close");
        }

        closeButton_->setDefaultImage(iconPath, Styling::ColorParameter{}, headerIconSize());
        closeButton_->setCustomToolTip(tooltipText);
        Styling::Buttons::setButtonDefaultColors(closeButton_);
    }

    void GroupProfile::updatePinButton()
    {
        const auto isPinned = Logic::getRecentsModel()->isFavorite(currentAimId_);
        pin_->setIcon(Utils::StyledPixmap::scaled(isPinned ? qsl(":/unpin_chat_icon") : qsl(":/pin_chat_icon"), QSize(ICON_SIZE, ICON_SIZE), Styling::ThemeColorKey{ Styling::StyleVariable::BASE_SECONDARY }));
        pin_->setText(isPinned ? QT_TRANSLATE_NOOP("sidebar", "Unpin") : QT_TRANSLATE_NOOP("sidebar", "Pin"));
        pin_->setVisible(!Logic::getRecentsModel()->isUnimportant(currentAimId_));
    }

    void GroupProfile::loadChatInfo()
    {
        if (isThread())
            loadThreadInfo();
        else
            loadGroupInfo();
    }

    void GroupProfile::loadGroupInfo()
    {
        Ui::gui_coll_helper collection(Ui::GetDispatcher()->create_collection(), true);
        collection.set_value_as_qstring("aimid", currentAimId_);

        if (currentAimId_.isEmpty() || Logic::getContactListModel()->areYouNotAMember(currentAimId_))
        {
            const auto stamp = Logic::getContactListModel()->getChatStamp(currentAimId_);
            if (stamp.isEmpty())
                return;

            collection.set_value_as_qstring("stamp", stamp);
        }

        collection.set_value_as_bool("threadsAutoSubscribe", stateAllThreadsAutoSubscribe_);

        collection.set_value_as_int("limit", SHORT_MEMBERS_COUNT);
        Ui::GetDispatcher()->post_message_to_core("chats/info/get", collection.get());
    }

    void GroupProfile::loadThreadInfo(bool _subscription)
    {
        if (!_subscription && threadMembers_ && threadMembers_->getMembersCount() == 1)
            threadInfo({}, {});

        Ui::gui_coll_helper collection(Ui::GetDispatcher()->create_collection(), true);
        collection.set_value_as_qstring("thread_id", currentAimId_);
        Ui::GetDispatcher()->post_message_to_core("threads/get", collection.get());
    }

    void GroupProfile::onThreadAutosubscribe(int _error)
    {
        if (_error == 0)
            loadGroupInfo();
    }

    void GroupProfile::closeGallery()
    {
        gallery_->markClosed();
        Ui::GetDispatcher()->cancelGalleryHolesDownloading(currentAimId_);
    }

    void GroupProfile::changeTab(int _tab)
    {
        if (cl_)
            Q_EMIT cl_->forceSearchClear(!isThread());

        titleBar_->setTitle(pageTitle(_tab));
        titleBar_->setArrowVisible(_tab == gallery && galleryPopup_->itemsCount() > 1);

        const auto prevTab = stackedWidget_->currentIndex();
        if (_tab == settings)
        {
            editGroupLinkWidget_->setVisible(chatInfo_ && chatInfo_->Public_);
            if (chatInfo_)
                editGroupLinkWidget_->setNick(chatInfo_->Stamp_);
            editGroupLinkWidget_->clearHint();
            aboutLinkToChatBlock_->setVisible(chatInfo_ && !chatInfo_->Public_);

            updateSettingsCheckboxes();

            applyChatSettings_->setEnabled(false);
        }
        else if (isThread() && prevTab == list && _tab == thread_info)
        {
            loadChatInfo();
        }

        stackedWidget_->setCurrentIndex(_tab);
        searchWidget_->setNeedClear(prevTab == list && _tab != list && !isThread());
        searchWidget_->searchCompleted();

        if (isThread() && _tab == list)
            searchWidget_->clearInput();

        updateCloseIcon();
        updateRegim();

        const auto canEdit = chatInfo_ && (Data::ChatInfo::areYouAdminOrModer(chatInfo_) || !Data::ChatInfo::isChatControlled(chatInfo_));
        editButton_->setVisible((_tab == main) && canEdit);
        editButton_->setVisibility((_tab == main) && canEdit);
        area_->ensureVisible(0, 0);
    }

    void GroupProfile::changeGalleryPage(MediaContentType _page)
    {
        titleBar_->setTitle(getGalleryTitle(_page));
        gallery_->openContentFor(currentAimId_, _page);
        currentGalleryPage_ = _page;

        emitContentsChanged();
    }

    void GroupProfile::updateRegim()
    {
        if (const auto isThreadInfo = (stackedWidget_->currentIndex() == thread_info); !chatInfo_ || isThreadInfo)
        {
            if (isThreadInfo)
                delegate_->setRegim(Logic::MembersWidgetRegim::CONTACT_LIST);
            return;
        }

        if (stackedWidget_->currentIndex() == list)
        {
            switch (currentListPage_)
            {
            case pending:
                delegate_->setRegim(Logic::MembersWidgetRegim::PENDING_MEMBERS);
                return;

            case invites:
                delegate_->setRegim(Logic::MembersWidgetRegim::YOUR_INVITES_LIST);
                return;

            case blocked:
                delegate_->setRegim(Data::ChatInfo::areYouAdminOrModer(chatInfo_) ? Logic::MembersWidgetRegim::MEMBERS_LIST : Logic::MembersWidgetRegim::CONTACT_LIST);
                return;

            default:
                break;
            }
        }

        delegate_->setRegim(Data::ChatInfo::areYouAdminOrModer(chatInfo_) ? Logic::MembersWidgetRegim::ADMIN_MEMBERS : Logic::MembersWidgetRegim::THREAD_SUBSCRIBERS);
    }

    void GroupProfile::updateChatControls()
    {
        if (!chatInfo_)
            return;

        std::unique_ptr<Utils::ScopedPropertyRollback> blocker;
        if (isVisible())
            blocker = std::make_unique<Utils::ScopedPropertyRollback>(this, "updatesEnabled", false);

        const auto chatInCL = Logic::getContactListModel()->hasContact(currentAimId_);
        const auto isIgnored = Logic::getIgnoreModel()->contains(currentAimId_);
        const auto isUnimportant = Logic::getRecentsModel()->isUnimportant(currentAimId_);
        const auto youBlocked = chatInfo_->YouBlocked_;
        const auto youPending = chatInfo_->YouPending_;
        const auto youNotMember = chatInfo_->YourRole_.isEmpty();
        const auto justAMember = !isIgnored && !youBlocked && !youNotMember && !youPending;
        const auto chatIsChannel = isChannel();
        const auto adminOrModer = Data::ChatInfo::areYouAdminOrModer(chatInfo_);
        const auto canEdit = adminOrModer || !Data::ChatInfo::isChatControlled(chatInfo_);
        const auto isNewsFeed = Logic::getContactListModel()->isFeed(currentAimId_);
        const auto canLeaveAndReport = !isNewsFeed || (isNewsFeed && adminOrModer);

        const auto makeIcon = [](const auto& _path, const QSize& _iconSize = QSize(BTN_ICON_SIZE, BTN_ICON_SIZE))
        {
            return Utils::StyledPixmap::scaled(_path, _iconSize, Styling::ThemeColorKey{ Styling::StyleVariable::BASE_GLOBALWHITE });
        };

        groupStatus_->hide();
        mainActionButton_->setEnabled(true);
        if (isIgnored)
        {
            mainAction_ = unblock;
            mainActionButton_->setIcon(makeIcon(qsl(":/unlock_icon")));
            mainActionButton_->setText(QT_TRANSLATE_NOOP("sidebar", "Unblock"));
            mainActionButton_->show();
            callButton_->hide();
        }
        else if (youBlocked)
        {
            if (chatIsChannel)
                groupStatus_->setText(QT_TRANSLATE_NOOP("sidebar", "You can't read messages in this channel because you are blocked"));
            else
                groupStatus_->setText(QT_TRANSLATE_NOOP("sidebar", "You can't read or write messages in this group because you are blocked"));
            groupStatus_->show();
            mainActionButton_->hide();
            callButton_->hide();
        }
        else if (youPending)
        {
            mainAction_ = cancel_join;
            if (chatIsChannel)
                groupStatus_->setText(QT_TRANSLATE_NOOP("sidebar", "Wait for subscription request approval"));
            else
                groupStatus_->setText(QT_TRANSLATE_NOOP("sidebar", "Wait for join request approval"));
            groupStatus_->show();
            mainActionButton_->setIcon(makeIcon(qsl(":/controls/close_icon"), QSize(ICON_SIZE, ICON_SIZE)));
            mainActionButton_->setText(QT_TRANSLATE_NOOP("input_widget", "Cancel request"));
            mainActionButton_->show();
            callButton_->hide();
        }
        else if (youNotMember)
        {
            const auto youRejected = [this]()
            {
                const auto state = Logic::getRecentsModel()->getDlgState(currentAimId_);
                if (auto msg = state.lastMessage_; msg && msg->IsChatEvent())
                    if (auto event = msg->GetChatEvent(); event && event->isForMe())
                        return event->eventType() == core::chat_event_type::mchat_joining_rejected;
                return false;
            }();

            if (youRejected)
            {
                if (chatIsChannel)
                    groupStatus_->setText(QT_TRANSLATE_NOOP("sidebar", "Your subscription request was rejected"));
                else
                    groupStatus_->setText(QT_TRANSLATE_NOOP("sidebar", "Your join request was rejected"));
                groupStatus_->show();
            }

            if (!youRejected && !chatInfo_->Stamp_.isEmpty())
            {
                mainAction_ = join;
                mainActionButton_->setIcon(makeIcon(qsl(":/input/add")));
                mainActionButton_->setText(chatIsChannel ? QT_TRANSLATE_NOOP("sidebar", "Subscribe") : QT_TRANSLATE_NOOP("sidebar", "Join"));
                mainActionButton_->show();
            }
            else
            {
                mainActionButton_->hide();
            }
            callButton_->hide();
        }
        else
        {
            mainAction_ = invite;
            mainActionButton_->setIcon(makeIcon(qsl(":/add_user_icon"), QSize(ICON_SIZE, ICON_SIZE)));
            mainActionButton_->setText(addToChatText(chatInfo_));
            mainActionButton_->show();
            if (chatIsChannel || chatInfo_->MembersCount_ < 2)
                callButton_->hide();
            else
                callButton_->show();
        }

        const auto linkVisible = chatInfo_->Public_ || adminOrModer;
        link_->setVisible(linkVisible);
        if (linkVisible)
        {
            link_->text_->clearMenuActions();

            const auto share = chatIsChannel ? QT_TRANSLATE_NOOP("context_menu", "Share channel link") : QT_TRANSLATE_NOOP("context_menu", "Share group link");
            const auto copy = chatIsChannel ? QT_TRANSLATE_NOOP("context_menu", "Copy channel link") : QT_TRANSLATE_NOOP("context_menu", "Copy group link");
            link_->text_->addMenuAction(qsl(":/context_menu/forward"), share, makeCopyData(qsl("share")));
            link_->text_->addMenuAction(qsl(":/context_menu/copy"), copy, makeCopyData(qsl("copy_link")));
        }

        setInfoPlaceholderVisible(false);
        dialogGalleryState(currentAimId_, Logic::getContactListModel()->getGalleryState(currentAimId_));

        share_->setVisible(chatInfo_->Public_ || adminOrModer);
        notifications_->setVisible(chatInCL || (!isIgnored && !youNotMember));
        notifications_->setEnabled(true);
        threadsAutoSubscribe_->setVisible(Features::isThreadsEnabled());
        threadsAutoSubscribe_->setEnabled(true);
        settings_->setVisible(adminOrModer);
        membersWidget_->setVisible(justAMember && (!chatIsChannel || adminOrModer));
        about_->setHeaderText(chatIsChannel ? QT_TRANSLATE_NOOP("siderbar", "About the channel") : QT_TRANSLATE_NOOP("siderbar", "About the group"));
        about_->setVisible(!chatInfo_->About_.isEmpty());
        rules_->setVisible(!chatInfo_->Rules_.isEmpty());
        pin_->setVisible(!youPending && !youNotMember && !isUnimportant);
        theme_->setVisible(chatInCL || (!youPending && !youNotMember));

        const auto canClearHistory = canLeaveAndReport && (chatInCL || (!youPending && !youNotMember));
        const auto canBlock = canLeaveAndReport && (chatInCL || youBlocked || justAMember);
        fifthSpacer_->setVisible(canClearHistory);
        clearHistory_->setVisible(canClearHistory);
        block_->setVisible(canBlock);
        report_->setVisible(canBlock && Features::isReportMessagesEnabled());
        leave_->setVisible(canBlock);

        editButton_->setVisible(stackedWidget_->currentIndex() == main && canEdit);
        editButton_->setVisibility(stackedWidget_->currentIndex() == main && canEdit);
        titleBar_->setTitle(pageTitle(stackedWidget_->currentIndex()));

        if (blocker)
            blocker->rollback();

        emitContentsChanged();
    }

    void GroupProfile::blockUser(const QString& aimId, ActionType _action)
    {
        auto model = stackedWidget_->currentIndex() == list ? listChatModel_ : shortChatModel_;
        if (!model->contains(aimId))
            return;

        bool confirmed = false, removeMessages = false;
        if (_action == ActionType::POSITIVE)
        {
            auto w = new Ui::BlockAndDeleteWidget(nullptr, Logic::GetFriendlyContainer()->getFriendly(aimId), currentAimId_);
            GeneralDialog generalDialog(w, Utils::InterConnector::instance().getMainWindow());
            generalDialog.addLabel(QT_TRANSLATE_NOOP("block_and_delete", "Delete?"));
            generalDialog.addButtonsPair(QT_TRANSLATE_NOOP("block_and_delete", "Cancel"), QT_TRANSLATE_NOOP("block_and_delete", "Yes"));
            confirmed = generalDialog.execute();
            removeMessages = w->needToRemoveMessages();
        }
        else
        {
            confirmed = Utils::GetConfirmationWithTwoButtons(
                QT_TRANSLATE_NOOP("popup_window", "Cancel"),
                QT_TRANSLATE_NOOP("popup_window", "Yes"),
                QT_TRANSLATE_NOOP("popup_window", "Are you sure you want to unblock user?"),
                Logic::GetFriendlyContainer()->getFriendly(aimId),
                nullptr);
        }

        if (confirmed)
        {
            Ui::gui_coll_helper collection(Ui::GetDispatcher()->create_collection(), true);
            collection.set_value_as_qstring("aimid", currentAimId_);
            collection.set_value_as_qstring("contact", aimId);
            collection.set_value_as_bool("block", _action == ActionType::POSITIVE);
            collection.set_value_as_bool("remove_messages", removeMessages);
            Ui::GetDispatcher()->post_message_to_core("chats/block", collection.get());
        }

        if (_action == ActionType::NEGATIVE)
        {
            if (chatInfo_ && chatInfo_->BlockedCount_ == 1)
                changeTab(isThread() ? thread_info : main);

            model->loadBlocked();
        }
    }

    void GroupProfile::readOnly(const QString& _aimId, ActionType _action)
    {
        auto model = stackedWidget_->currentIndex() == list ? listChatModel_ : shortChatModel_;
        if (!model->contains(_aimId))
            return;

        const auto action = _action == ActionType::POSITIVE ? Utils::ROAction::Ban : Utils::ROAction::Allow;
        const auto confirmed = Utils::GetConfirmationWithTwoButtons(
            QT_TRANSLATE_NOOP("popup_window", "Cancel"),
            QT_TRANSLATE_NOOP("popup_window", "Yes"),
            Utils::getReadOnlyString(action, isChannel()),
            Logic::GetFriendlyContainer()->getFriendly(_aimId),
            nullptr);

        if (confirmed)
        {
            Ui::gui_coll_helper collection(Ui::GetDispatcher()->create_collection(), true);
            collection.set_value_as_qstring("aimid", currentAimId_);
            collection.set_value_as_qstring("contact", _aimId);
            collection.set_value_as_qstring("role", _action == ActionType::POSITIVE ? u"readonly" : u"member");
            Ui::GetDispatcher()->post_message_to_core("chats/role/set", collection.get());
        }
    }

    void GroupProfile::changeRole(const QString& aimId, ActionType _action)
    {
        auto model = stackedWidget_->currentIndex() == list ? listChatModel_ : shortChatModel_;
        if (!model->contains(aimId))
            return;

        const auto confirmed = Utils::GetConfirmationWithTwoButtons(
            QT_TRANSLATE_NOOP("popup_window", "Cancel"),
            QT_TRANSLATE_NOOP("popup_window", "Yes"),
            _action == ActionType::POSITIVE ? QT_TRANSLATE_NOOP("popup_window", "Are you sure you want to make user admin in this chat?") : QT_TRANSLATE_NOOP("popup_window", "Are you sure you want to revoke admin role?"),
            Logic::GetFriendlyContainer()->getFriendly(aimId),
            nullptr);

        if (confirmed)
        {
            Ui::gui_coll_helper collection(Ui::GetDispatcher()->create_collection(), true);
            collection.set_value_as_qstring("aimid", currentAimId_);
            collection.set_value_as_qstring("contact", aimId);
            collection.set_value_as_qstring("role", _action == ActionType::POSITIVE ? u"moder" : (isChannel() ? u"readonly" : u"member"));
            Ui::GetDispatcher()->post_message_to_core("chats/role/set", collection.get());
        }
    }

    void GroupProfile::removeInvite(const QString& _aimId)
    {
        Ui::gui_coll_helper collection(Ui::GetDispatcher()->create_collection(), true);
        collection.set_value_as_qstring("contact", _aimId);
        collection.set_value_as_qstring("chat", currentAimId_);
        Ui::GetDispatcher()->post_message_to_core("livechat/invite/cancel", collection.get());
    }

    void GroupProfile::deleteUser(const QString& _aimId)
    {
        const auto confirmed = Utils::GetConfirmationWithTwoButtons(
            QT_TRANSLATE_NOOP("popup_window", "Cancel"),
            QT_TRANSLATE_NOOP("popup_window", "Yes"),
            QT_TRANSLATE_NOOP("popup_window", "Are you sure you want to delete user?"),
            Logic::GetFriendlyContainer()->getFriendly(_aimId),
            nullptr);

        if (confirmed)
        {
            Ui::gui_coll_helper collection(Ui::GetDispatcher()->create_collection(), true);
            collection.set_value_as_qstring("aimid", currentAimId_);
            core::ifptr<core::iarray> arr(collection->create_array());
            arr->push_back(collection.create_qstring_value(_aimId).get());
            collection.set_value_as_array("members", arr.get());
            Ui::GetDispatcher()->post_message_to_core("remove_members", collection.get());
        }
    }

    void GroupProfile::requestNickSuggest()
    {
        Ui::gui_coll_helper collection(Ui::GetDispatcher()->create_collection(), true);
        collection.set_value_as_qstring("sn", currentAimId_);
        collection.set_value_as_bool("public", public_->isChecked());
        Ui::GetDispatcher()->post_message_to_core("suggest_group_nick", collection.get());
    }

    void GroupProfile::chatInfo(qint64, const std::shared_ptr<Data::ChatInfo>& _chatInfo, const int _requestMembersLimit)
    {
        if (!isActive() || _chatInfo->AimId_ != currentAimId_)
            return;

        chatInfo_ = _chatInfo;

        enableFading();
        shortMembersList_->clearCache();
        shortChatModel_->updateInfo(chatInfo_);

        const auto timeLag = (std::chrono::milliseconds(elapsedTimer_.elapsed()) - kLoadDelay);
        if (timeLag.count() > 0 && timeLag < kShowDelay)
            QTimer::singleShot(kShowDelay, this, &GroupProfile::refresh);
        else
            refresh();
    }

    void GroupProfile::refresh()
    {
        initTexts();
        info_->setInfo(Utils::getMembersString(chatInfo_->MembersCount_, chatInfo_->isChannel()));
        info_->tryClearSelection(QPoint(-1, -1));

        if (!chatInfo_->About_.isEmpty())
        {
            about_->setText(chatInfo_->About_);
            about_->disableCommandsInText();
        }

        if (!chatInfo_->Rules_.isEmpty())
        {
            rules_->setText(chatInfo_->Rules_);
            rules_->disableCommandsInText();
        }

        link_->setText(Utils::getDomainUrl() % u'/' % chatInfo_->Stamp_, Styling::ThemeColorKey{ Styling::StyleVariable::TEXT_PRIMARY });
        link_->setVisible(chatInfo_->Public_ || Data::ChatInfo::areYouAdminOrModer(chatInfo_));

        share_->setVisible(chatInfo_->Public_ || Data::ChatInfo::areYouAdminOrModer(chatInfo_));

        members_->setMembersCount(chatInfo_->MembersCount_, chatInfo_->isChannel());
        allMembers_->setCounter(chatInfo_->MembersCount_);
        approveAll_->setCounter(chatInfo_->PendingCount_, false);

        const auto canEdit = Data::ChatInfo::areYouAdminOrModer(chatInfo_) || !Data::ChatInfo::isChatControlled(chatInfo_);

        settings_->setVisible(Data::ChatInfo::areYouAdminOrModer(chatInfo_));
        editButton_->setVisible(canEdit && stackedWidget_->currentIndex() == main);
        editButton_->setVisibility(canEdit && stackedWidget_->currentIndex() == main);

        if (Data::ChatInfo::areYouAdminOrModer(chatInfo_))
        {
            pendings_->setCounter(chatInfo_->PendingCount_);
            blockedMembers_->setCounter(chatInfo_->BlockedCount_);
        }
        else
        {
            pendings_->hide();
            blockedMembers_->hide();
        }

        if (Features::areYourInvitesButtonVisible())
            yourInvites_->setCounter(chatInfo_->YourInvitesCount_);
        else
            yourInvites_->hide();

        if (isChannel())
        {
            admins_->setCounter(chatInfo_->AdminsCount_, false);
            admins_->setVisible(Data::ChatInfo::areYouAdminOrModer(chatInfo_));
        }
        else
        {
            admins_->setCounter(chatInfo_->AdminsCount_);
        }

        updateSettingsCheckboxes();

        aboutLinkToChat_->setText(Utils::getDomainUrl() % ql1c('/') % chatInfo_->Stamp_);
        editGroupLinkWidget_->setNick(chatInfo_->Stamp_);

        editGroupLinkWidget_->setVisible(chatInfo_->Public_);
        editGroupLinkWidget_->clearHint();
        aboutLinkToChatBlock_->setVisible(!chatInfo_->Public_);

        checkApplyButton();

        updateRegim();
        updateChatControls();

        QTimer::singleShot(kShowDuration, this, &GroupProfile::disableFading);
    }

    void GroupProfile::enableFading()
    {
        Utils::WidgetFader::setEffectEnabled(this, true);
    }

    void GroupProfile::disableFading()
    {
        Utils::WidgetFader::setEffectEnabled(this, false);
    }

    void GroupProfile::scrollToTop()
    {
        area_->ensureVisible(0, 0);
    }

    void GroupProfile::chatInfoFailed(qint64 _seq, core::group_chat_info_errors _errorCode, const QString& _aimId)
    {
        disableFading();
        if (!isActive() || _aimId != currentAimId_)
            return;

        if (_errorCode == core::group_chat_info_errors::not_in_chat || _errorCode == core::group_chat_info_errors::blocked)
        {
            if (chatInfo_)
            {
                chatInfo_->YourRole_.clear();
            }
            else
            {
                chatInfo_ = std::make_shared<Data::ChatInfo>();
                chatInfo_->Controlled_ = true;
            }

            const auto text = Logic::getContactListModel()->isChannel(_aimId)
                              ? QT_TRANSLATE_NOOP("groupchats", "You are not a subscriber of this channel")
                              : QT_TRANSLATE_NOOP("groupchats", "You are not a member of this chat");
            info_->setInfo(text);

            if (_errorCode == core::group_chat_info_errors::blocked)
                chatInfo_->YouBlocked_ = true;
            updateChatControls();
        }
        else
        {
            hideControls();
        }
    }

    void GroupProfile::threadInfo(qint64, const Data::ThreadInfo& _threadInfo)
    {
        if (_threadInfo.threadId_ != currentAimId_)
            return;

        const QSignalBlocker blocker(threadSubscription_);
        threadSubscription_->setChecked(_threadInfo.areYouSubscriber_);

        enableFading();
        threadMembersList_->clearCache();
        shortChatModel_->updateInfo(&_threadInfo);
        threadMembers_->setMembersCount(_threadInfo.subscribersCount_, true);
        threadMembers_->setVisible(_threadInfo.subscribersCount_ > 0);
        if (cl_)
            Testing::setAccessibleName(cl_, qsl("AS ThreadInfo contactList"));
    }

    void GroupProfile::dialogGalleryState(const QString& _aimId, const Data::DialogGalleryState& _state)
    {
        if (!isActive() || _aimId != currentAimId_)
            return;

        galleryIsEmpty_ = _state.isEmpty();

        galleryPhoto_->setCounter(countForType(_state, MediaContentType::ImageVideo));
        galleryVideo_->setCounter(countForType(_state, MediaContentType::Video));
        galleryFiles_->setCounter(countForType(_state, MediaContentType::Files));
        galleryLinks_->setCounter(countForType(_state, MediaContentType::Links));
        galleryPtt_->setCounter(countForType(_state, MediaContentType::Voice));
        galleryPopup_->setCounters(_state);

        updateGalleryVisibility();

        if (stackedWidget_->currentIndex() == gallery)
        {
            if (countForType(_state, currentGalleryPage_) == 0)
            {
                closeGallery();
                changeTab(isThread() ? thread_info : main);
            }
            else
            {
                titleBar_->setArrowVisible(galleryPopup_->itemsCount() > 1);
            }
        }
    }

    void GroupProfile::favoriteChanged(const QString& _aimid)
    {
        if (isActive() && _aimid == currentAimId_)
            updatePinButton();
    }

    void GroupProfile::unimportantChanged(const QString& _aimid)
    {
        if (isActive() && _aimid == currentAimId_)
            updatePinButton();
    }

    void GroupProfile::modChatParamsResult(int _error)
    {
        applyChatSettings_->setEnabled(_error != 0);
    }

    void GroupProfile::suggestGroupNickResult(const QString& _nick)
    {
        if (!chatInfo_)
            return;

        auto nick = _nick.isEmpty() ? chatInfo_->Stamp_ : _nick;
        aboutLinkToChat_->setText(Utils::getDomainUrl() % u'/' % nick);
        editGroupLinkWidget_->setNick(nick);

        checkApplyButton();
    }

    void GroupProfile::share()
    {
        if (!chatInfo_)
            return;

        forwardMessage(u"https://" % Utils::getDomainUrl() % u'/' % chatInfo_->Stamp_, QString(), QString(), false);
    }

    void GroupProfile::shareClicked()
    {
        share();
        GetDispatcher()->post_stats_to_core(core::stats::stats_event_names::profilescr_sharecontact_action, { { "chat_type", Utils::chatTypeByAimId(currentAimId_) } });
    }

    void GroupProfile::copy(const QString& _text)
    {
        QApplication::clipboard()->setText(_text);
    }

    void GroupProfile::notificationsChecked(bool _checked)
    {
        Logic::getRecentsModel()->muteChat(currentAimId_, !_checked);
        GetDispatcher()->post_stats_to_core(_checked ? core::stats::stats_event_names::unmute : core::stats::stats_event_names::mute_sidebar);
        GetDispatcher()->post_stats_to_core(core::stats::stats_event_names::profilescr_notifications_event, { { "chat_type", Utils::chatTypeByAimId(currentAimId_) }, { "do" , _checked ? "enable" : "mute" } });
    }

    void GroupProfile::threadSubscriptionChecked(bool _checked)
    {
        Ui::GetDispatcher()->updateThreadSubscription(currentAimId_, _checked);
    }

    void GroupProfile::threadAllSubscriptionChecked(bool _checked)
    {
        stateAllThreadsAutoSubscribe_ = _checked && Logic::getContactListModel()->areYouSubscribedToThreadInChat(currentAimId_);
        Ui::gui_coll_helper collection(Ui::GetDispatcher()->create_collection(), true);
        collection.set_value_as_qstring("chat_id", currentAimId_);
        collection.set_value_as_bool("thread_auto_subscribe", stateAllThreadsAutoSubscribe_);
        Ui::GetDispatcher()->post_message_to_core("thread/autosubscribe", collection.get());
    }

    void GroupProfile::settingsClicked()
    {
        checkApplyButton();
        changeTab(settings);
        GetDispatcher()->post_stats_to_core(core::stats::stats_event_names::profilescr_settings_event, { { "chat_type", Utils::chatTypeByAimId(currentAimId_) } });
    }

    void GroupProfile::galleryPhotoClicked()
    {
        switchToGallery(MediaContentType::ImageVideo);
    }

    void GroupProfile::galleryVideoClicked()
    {
        switchToGallery(MediaContentType::Video);
    }

    void GroupProfile::galleryFilesClicked()
    {
        switchToGallery(MediaContentType::Files);
    }

    void GroupProfile::galleryLinksClicked()
    {
        switchToGallery(MediaContentType::Links);
    }

    void GroupProfile::galleryPttClicked()
    {
        switchToGallery(MediaContentType::Voice);
    }

    void GroupProfile::searchMembersClicked()
    {
        currentListPage_ = all;
        changeTab(list);
        if (isThread())
            listChatModel_->loadThreadSubscribers();
        else
            listChatModel_->loadAllMembers();
        approveAll_->hide();
        searchWidget_->setFocus();
    }

    void GroupProfile::addToChatClicked()
    {
        Logic::ChatContactsModel membersModel;
        membersModel.loadChatContacts(currentAimId_);

        SelectContactsWidget selectMembersDialog(&membersModel, Logic::MembersWidgetRegim::SELECT_MEMBERS,
                addToChatText(chatInfo_),
                QT_TRANSLATE_NOOP("popup_window", "Done"), this);

        if (selectMembersDialog.show() == QDialog::Accepted)
        {
            GetDispatcher()->post_stats_to_core(core::stats::stats_event_names::adtochatscr_adding_action, { { "counter", Utils::averageCount(Logic::getContactListModel()->getCheckedContacts().size()) }, {"from", "profile" } });
            postAddChatMembersFromCLModelToCore(currentAimId_);
            GetDispatcher()->post_stats_to_core(core::stats::stats_event_names::chats_add_member_sidebar);
        }
        else
        {
            Logic::getContactListModel()->clearCheckedItems();
            Logic::getContactListModel()->removeTemporaryContactsFromModel();
        }

        GetDispatcher()->post_stats_to_core(core::stats::stats_event_names::profilescr_adding_action, { { "chat_type", Utils::chatTypeByAimId(currentAimId_) }, {"from", "head" } });
    }

    void GroupProfile::pendingsClicked()
    {
        currentListPage_ = pending;
        changeTab(list);
        titleBar_->setTitle(QT_TRANSLATE_NOOP("sidebar", "Waiting for approval"));
        listChatModel_->loadPending();
        approveAll_->show();
        searchWidget_->setFocus();
    }

    void GroupProfile::yourInvitesClicked()
    {
        currentListPage_ = invites;
        changeTab(list);
        titleBar_->setTitle(QT_TRANSLATE_NOOP("sidebar", "Your invites"));
        listChatModel_->loadYourInvites();
        approveAll_->hide();
        searchWidget_->setFocus();
    }

    void GroupProfile::allMembersClicked()
    {
        currentListPage_ = all;
        changeTab(list);
        titleBar_->setTitle(isChannel() || isThread() ? QT_TRANSLATE_NOOP("sidebar", "Subscribers") : QT_TRANSLATE_NOOP("sidebar", "Members"));
        if (isThread())
            listChatModel_->loadThreadSubscribers();
        else
            listChatModel_->loadAllMembers();
        approveAll_->hide();
        searchWidget_->setFocus();
        GetDispatcher()->post_stats_to_core(core::stats::stats_event_names::profilescr_members_action, { { "chat_type", Utils::chatTypeByAimId(currentAimId_) } });
    }

    void GroupProfile::adminsClicked()
    {
        currentListPage_ = admins;
        changeTab(list);
        titleBar_->setTitle(QT_TRANSLATE_NOOP("sidebar", "Admins"));
        listChatModel_->loadAdmins();
        approveAll_->hide();
        searchWidget_->setFocus();
    }

    void GroupProfile::blockedMembersClicked()
    {
        currentListPage_ = blocked;
        changeTab(list);
        titleBar_->setTitle(QT_TRANSLATE_NOOP("sidebar", "Deleted and blocked"));
        listChatModel_->loadBlocked();
        approveAll_->hide();
        searchWidget_->setFocus();
        GetDispatcher()->post_stats_to_core(core::stats::stats_event_names::profilescr_blocked_action, { { "chat_type", Utils::chatTypeByAimId(currentAimId_) } });
    }

    void GroupProfile::pinClicked()
    {
        GetDispatcher()->pinContact(currentAimId_, !Logic::getRecentsModel()->isFavorite(currentAimId_));
        Q_EMIT Utils::InterConnector::instance().closeAnySemitransparentWindow({ Utils::CloseWindowInfo::Initiator::Unknown, Utils::CloseWindowInfo::Reason::Keep_Sidebar });
    }

    void GroupProfile::themeClicked()
    {
        showWallpaperSelectionDialog(currentAimId_);
    }

    void GroupProfile::clearHistoryClicked()
    {
        closeGallery();

        const auto confirmed = Utils::GetConfirmationWithTwoButtons(
            QT_TRANSLATE_NOOP("popup_window", "Cancel"),
            QT_TRANSLATE_NOOP("popup_window", "Yes"),
            isChannel() ? QT_TRANSLATE_NOOP("popup_window", "Are you sure you want to clear history?") : QT_TRANSLATE_NOOP("popup_window", "Are you sure you want to erase chat history?"),
            Logic::GetFriendlyContainer()->getFriendly(currentAimId_),
            nullptr);

        if (confirmed)
        {
            eraseHistory(currentAimId_);
            GetDispatcher()->post_stats_to_core(core::stats::stats_event_names::profilescr_blockbuttons_action, { { "do", "spam" },{ "chat_type", Utils::chatTypeByAimId(currentAimId_) } });
        }
    }

    void GroupProfile::blockClicked()
    {
        closeGallery();

        if (Logic::getContactListModel()->ignoreContactWithConfirm(currentAimId_))
        {
            GetDispatcher()->post_stats_to_core(core::stats::stats_event_names::ignore_contact, { { "From", "Sidebar" } });
            Utils::InterConnector::instance().setSidebarVisible(false);
            GetDispatcher()->post_stats_to_core(core::stats::stats_event_names::profilescr_blockbuttons_action, { { "do", "block" },{ "chat_type", Utils::chatTypeByAimId(currentAimId_) } });
        }
    }

    void GroupProfile::reportCliked()
    {
        closeGallery();

        if (Ui::ReportContact(currentAimId_, Logic::GetFriendlyContainer()->getFriendly(currentAimId_)))
        {
            Logic::getContactListModel()->ignoreContact(currentAimId_, true);
            GetDispatcher()->post_stats_to_core(core::stats::stats_event_names::spam_contact, { { "From", "Sidebar" } });
            GetDispatcher()->post_stats_to_core(core::stats::stats_event_names::profilescr_blockbuttons_action, { { "do", "spam" },{ "chat_type", Utils::chatTypeByAimId(currentAimId_) } });
        }
    }

    void GroupProfile::leaveClicked()
    {
        closeGallery();

        const auto text = isChannel()
            ? QT_TRANSLATE_NOOP("popup_window", "Are you sure you want to erase history and leave channel?")
            : QT_TRANSLATE_NOOP("popup_window", "Are you sure you want to erase history and leave chat?");

        const auto confirmed = Utils::GetConfirmationWithTwoButtons(
            QT_TRANSLATE_NOOP("popup_window", "Cancel"),
            QT_TRANSLATE_NOOP("popup_window", "Leave"),
            text,
            QT_TRANSLATE_NOOP("sidebar", "Leave and delete"),
            nullptr);

        if (confirmed)
        {
            Logic::getContactListModel()->removeContactFromCL(currentAimId_);
            GetDispatcher()->post_stats_to_core(core::stats::stats_event_names::delete_contact, { { "From", "Sidebar" } });
        }
    }

    void GroupProfile::linkClicked()
    {
        if (!chatInfo_)
            return;

        copy(u"https://" % Utils::getDomainUrl() % u'/' % chatInfo_->Stamp_);
        Utils::showToastOverMainWindow(QT_TRANSLATE_NOOP("sidebar", "Link copied"), Utils::defaultToastVerOffset());
        Ui::GetDispatcher()->post_stats_to_core(core::stats::stats_event_names::profilescr_groupurl_action, { { "chat_type", Utils::chatTypeByAimId(currentAimId_) }, { "do", "copy_url" } });
    }

    void GroupProfile::aboutLinkClicked()
    {
        if (applyChatSettings_->isEnabled())
            return;

        copy(u"https://" % aboutLinkToChat_->text_->getText());
        Utils::showToastOverMainWindow(QT_TRANSLATE_NOOP("sidebar", "Link copied"), Utils::defaultToastVerOffset());
    }

    void GroupProfile::linkCopy()
    {
        if (!chatInfo_)
            return;

        copy(u"https://" % Utils::getDomainUrl() % u'/' % chatInfo_->Stamp_);
        Utils::showToastOverMainWindow(QT_TRANSLATE_NOOP("sidebar", "Link copied"), Utils::defaultToastVerOffset());
        Ui::GetDispatcher()->post_stats_to_core(core::stats::stats_event_names::profilescr_groupurl_action, { { "chat_type", Utils::chatTypeByAimId(currentAimId_) }, { "do", "copy_url" } });
    }

    void GroupProfile::linkShare()
    {
        share();
        Ui::GetDispatcher()->post_stats_to_core(core::stats::stats_event_names::profilescr_groupurl_action, { { "chat_type", Utils::chatTypeByAimId(currentAimId_) }, { "do", "share" } });
    }

    void GroupProfile::mainActionClicked()
    {
        if (mainAction_ == invite)
        {
            addToChatClicked();
        }
        else if (mainAction_ == join)
        {
            if (chatInfo_)
                Logic::getContactListModel()->joinLiveChat(chatInfo_->Stamp_, true);
        }
        else if (mainAction_ == cancel_join)
        {
            Utils::showCancelGroupJoinDialog(currentAimId_);
        }
        else if (mainAction_ == unblock)
        {
            const auto confirmed = Utils::GetConfirmationWithTwoButtons(
                QT_TRANSLATE_NOOP("popup_window", "Cancel"),
                QT_TRANSLATE_NOOP("popup_window", "Yes"),
                QT_TRANSLATE_NOOP("popup_window", "Are you sure you want to delete user from ignore list?"),
                Logic::GetFriendlyContainer()->getFriendly(currentAimId_),
                nullptr);

            if (confirmed)
                Logic::getContactListModel()->ignoreContact(currentAimId_, false);

            GetDispatcher()->post_stats_to_core(core::stats::stats_event_names::profilescr_unlock_action, { { "chat_type", Utils::chatTypeByAimId(currentAimId_) } });
        }
    }

    void GroupProfile::makeNewLink()
    {
        if (!chatInfo_)
            return;

        if (Utils::getDomainUrl() % ql1c('/') % chatInfo_->Stamp_ == aboutLinkToChat_->text_->getText())
        {
            const QString text = isChannel() ? QT_TRANSLATE_NOOP("popup_window", "If you generate a new link to the channel the old one will stop working") : QT_TRANSLATE_NOOP("popup_window", "If you generate a new link to the group the old one will stop working");
            auto confirm = Utils::GetConfirmationWithTwoButtons(
                QT_TRANSLATE_NOOP("popup_window", "Cancel"),
                QT_TRANSLATE_NOOP("popup_window", "Generate"),
                text,
                QT_TRANSLATE_NOOP("popup_window", "The old link will stop working"),
                nullptr);

            if (!confirm)
                return;
        }

        requestNickSuggest();
    }

    void GroupProfile::contactSelected(const QString& _aimid)
    {
        SidebarParams params;
        params.showFavorites_ = false;

        Utils::InterConnector::instance().showSidebarWithParams(_aimid, params);

        GetDispatcher()->post_stats_to_core(core::stats::stats_event_names::profile_open, { { "From", "Members_List" } });
        GetDispatcher()->post_stats_to_core(core::stats::stats_event_names::profilescr_chattomembers_action, { { "chat_type", Utils::chatTypeByAimId(currentAimId_) } });
    }

    void GroupProfile::contactRemoved(const QString& _aimid)
    {
        const auto isList = stackedWidget_->currentIndex() == list;
        if (Data::ChatInfo::areYouAdminOrModer(chatInfo_))
        {
            const auto action = (isList && currentListPage_ == blocked) ? ActionType::NEGATIVE : ActionType::POSITIVE;
            blockUser(_aimid, action);
        }
        else
        {
            if (isList && currentListPage_ == invites)
                removeInvite(_aimid);
            else
                deleteUser(_aimid);
        }
    }

    void GroupProfile::contactMenuRequested(const QString& _aimid)
    {
        auto menu = new ContextMenu(this);
        auto model = stackedWidget_->currentIndex() == list ? listChatModel_ : shortChatModel_;
        if (model->contains(_aimid))
        {
            if (MyInfo()->aimId() == _aimid)
            {
                menu->addActionWithIcon(
                    qsl(":/exit_icon"),
                    QT_TRANSLATE_NOOP("sidebar", "Leave and delete"),
                    makeData(qsl("leave"), _aimid));
            }
            else
            {
                const auto role = model->getMemberItem(_aimid)->getRole();
                if (role != u"admin" && Data::ChatInfo::areYouAdmin(chatInfo_))
                {
                    if (role == u"moder")
                        menu->addActionWithIcon(
                            qsl(":/context_menu/admin_off"),
                            QT_TRANSLATE_NOOP("sidebar", "Revoke admin role"),
                            makeData(qsl("revoke_admin"), _aimid));
                    else
                        menu->addActionWithIcon(
                            qsl(":/context_menu/admin"),
                            QT_TRANSLATE_NOOP("sidebar", "Assign admin role"),
                            makeData(qsl("make_admin"), _aimid));
                }

                menu->addActionWithIcon(
                    qsl(":/context_menu/profile"),
                    QT_TRANSLATE_NOOP("sidebar", "Profile"),
                    makeData(qsl("profile"), _aimid));

                if (Data::ChatInfo::areYouAdminOrModer(chatInfo_))
                {
                    if (role == u"member")
                        menu->addActionWithIcon(
                            qsl(":/context_menu/readonly"),
                            QT_TRANSLATE_NOOP("sidebar", "Ban to write"),
                            makeData(qsl("readonly"), _aimid));
                    else if (role == u"readonly")
                        menu->addActionWithIcon(
                            qsl(":/context_menu/readonly_off"),
                            QT_TRANSLATE_NOOP("sidebar", "Allow to write"),
                            makeData(qsl("revoke_readonly"), _aimid));
                }

                if (role != u"admin" && role != u"moder")
                    menu->addActionWithIcon(
                        qsl(":/context_menu/block"),
                        QT_TRANSLATE_NOOP("sidebar", "Delete"),
                        makeData(qsl("block"), _aimid));

                if (Features::clRemoveContactsAllowed())
                {
                    menu->addSeparator();
                    menu->addActionWithIcon(
                        qsl(":/context_menu/spam"),
                        QT_TRANSLATE_NOOP("sidebar", "Report"),
                        makeData(qsl("spam"), _aimid));
                }
            }
        }
        menu->showAtLeft(true);
        connect(menu, &ContextMenu::triggered, this, &GroupProfile::memberMenu, Qt::QueuedConnection);
        connect(menu, &ContextMenu::triggered, menu, &ContextMenu::deleteLater, Qt::QueuedConnection);
        connect(menu, &ContextMenu::aboutToHide, menu, &ContextMenu::deleteLater, Qt::QueuedConnection);
        menu->popup(QCursor::pos());
    }

    void GroupProfile::contactMenuApproved(const QString& _aimid, bool _approve)
    {
        Ui::GetDispatcher()->resolveChatPendings(currentAimId_, { _aimid }, _approve);

        if (chatInfo_ && chatInfo_->PendingCount_ == 1)
            changeTab(isThread() ? thread_info : main);
    }

    void GroupProfile::closeClicked()
    {
        if (stackedWidget_->currentIndex() == thread_info)
        {
            Utils::InterConnector::instance().navigateBackFromThreadInfo(currentAimId_);
        }
        else if (stackedWidget_->currentIndex() == main)
        {
            Utils::InterConnector::instance().setSidebarVisible(false);
        }
        else
        {
            closeGallery();
            changeTab(isThread() ? thread_info : main);
        }
    }

    void GroupProfile::editButtonClicked()
    {
        if (!chatInfo_)
            return;

        QString name = chatInfo_->Name_;
        QString desription = chatInfo_->About_;
        QString rules = chatInfo_->Rules_;

        auto avatar = editGroup(currentAimId_, name, desription, rules);
        if (name != chatInfo_->Name_)
        {
            Ui::gui_coll_helper collection(Ui::GetDispatcher()->create_collection(), true);
            collection.set_value_as_qstring("aimid", currentAimId_);
            collection.set_value_as_qstring("name", name);
            Ui::GetDispatcher()->post_message_to_core("chats/mod/name", collection.get());
        }

        if (desription != chatInfo_->About_)
        {
            Ui::gui_coll_helper collection(Ui::GetDispatcher()->create_collection(), true);
            collection.set_value_as_qstring("aimid", currentAimId_);
            collection.set_value_as_qstring("about", desription);
            Ui::GetDispatcher()->post_message_to_core("chats/mod/about", collection.get());
        }

        if (rules != chatInfo_->Rules_)
        {
            Ui::gui_coll_helper collection(Ui::GetDispatcher()->create_collection(), true);
            collection.set_value_as_qstring("aimid", currentAimId_);
            collection.set_value_as_qstring("rules", rules);
            Ui::GetDispatcher()->post_message_to_core("chats/mod/rules", collection.get());
        }

        if (!avatar.isNull())
        {
            const auto byteArray = avatarToByteArray(avatar);

            Ui::gui_coll_helper helper(GetDispatcher()->create_collection(), true);

            core::ifptr<core::istream> data_stream(helper->create_stream());
            if (!byteArray.isEmpty())
                data_stream->write((const uint8_t*)byteArray.data(), (uint32_t)byteArray.size());

            helper.set_value_as_stream("avatar", data_stream.get());
            helper.set_value_as_qstring("aimid", currentAimId_);

            GetDispatcher()->post_message_to_core("set_avatar", helper.get());
        }
        refresh();
    }

    void GroupProfile::approveAllClicked()
    {
        //!! todo: use new method for approve all
        const auto& members = listChatModel_->getMembers();
        std::vector<QString> contacts;
        contacts.reserve(members.size());
        for (const auto& m : members)
            contacts.push_back(m.AimId_);
        Ui::GetDispatcher()->resolveChatPendings(currentAimId_, contacts, true);

        closeGallery();
        changeTab(isThread() ? thread_info : main);
    }

    void GroupProfile::memberMenu(QAction* action)
    {
        const auto params = action->data().toMap();
        const auto command = params[qsl("command")].toString();
        const auto aimId = params[qsl("aimid")].toString();

        if (command == u"profile")
        {
            Utils::InterConnector::instance().showSidebar(aimId);
        }
        else if (command == u"spam")
        {
            const auto friendly = Logic::GetFriendlyContainer()->getFriendly(aimId);
            if (Ui::ReportContact(aimId, friendly))
            {
                Logic::getContactListModel()->removeContactFromCL(aimId);
                GetDispatcher()->post_stats_to_core(core::stats::stats_event_names::spam_contact, { { "From", "MembersList_Menu" } });
            }
        }
        else if (command == u"block")
        {
            blockUser(aimId, ActionType::POSITIVE);
        }
        else if (command == u"readonly")
        {
            readOnly(aimId, ActionType::POSITIVE);
        }
        else if (command == u"revoke_readonly")
        {
            readOnly(aimId, ActionType::NEGATIVE);
        }
        else if (command == u"make_admin")
        {
            changeRole(aimId, ActionType::POSITIVE);
        }
        else if (command == u"revoke_admin")
        {
            changeRole(aimId, ActionType::NEGATIVE);
        }
        else if (command == u"leave")
        {
            leaveClicked();
        }
    }

    void GroupProfile::menuAction(QAction* action)
    {
        const auto params = action->data().toMap();
        const auto command = params[qsl("command")].toString();

        if (command == u"share")
        {
            share();
        }
        else if (command == u"copy_link")
        {
            linkCopy();
        }
        else if (command == u"copy")
        {
            const auto text = params[qsl("text")].toString();
            copy(text);
            Utils::showCopiedToast();
        }
    }

    void GroupProfile::memberActionResult()
    {
        if (!isActive())
            return;

        if (stackedWidget_->currentIndex() == list)
        {
            switch (currentListPage_)
            {
            case all:
                if (isThread())
                    listChatModel_->loadThreadSubscribers();
                else
                    listChatModel_->loadAllMembers();
                break;

            case pending:
                listChatModel_->loadPending();
                break;

            case blocked:
                listChatModel_->loadBlocked();
                break;

            case admins:
                listChatModel_->loadAdmins();
                break;

            case invites:
                listChatModel_->loadYourInvites();
                break;
            }
        }
        else
        {
            loadChatInfo();
        }
    }

    void GroupProfile::chatEvents(const QString& _aimid)
    {
        if (!isActive() || currentAimId_ != _aimid)
            return;

        memberActionResult();
    }

    void GroupProfile::onContactChanged(const QString& _aimid)
    {
        if (!isActive() || currentAimId_ != _aimid)
            return;

        updateMuteState();
    }

    void GroupProfile::onRoleChanged(const QString& _aimId)
    {
        if (!isActive() || currentAimId_ != _aimId)
            return;

        if (stackedWidget_->currentIndex() == gallery)
        {
            closeGallery();
            changeTab(isThread() ? thread_info : main);
        }
    }

    void GroupProfile::updateMuteState()
    {
        setCheckedBlocking(notifications_, !Logic::getContactListModel()->isMuted(currentAimId_));
    }

    void GroupProfile::avatarClicked()
    {
        GetDispatcher()->post_stats_to_core(core::stats::stats_event_names::profilescr_avatar_click, { { "chat_type", Utils::chatTypeByAimId(currentAimId_) } });
        Utils::InterConnector::instance().getMainWindow()->openAvatar(currentAimId_);
    }

    void GroupProfile::titleArrowClicked()
    {
        galleryPopup_->move(mapToGlobal({ GalleryPopup::horOffset(), GalleryPopup::verOffset() }));
        galleryPopup_->show();
    }

    void GroupProfile::applyChatSettings()
    {
        if (!chatInfo_)
            return;

        Ui::gui_coll_helper collection(Ui::GetDispatcher()->create_collection(), true);
        if (public_->isChecked() != chatInfo_->Public_)
            collection.set_value_as_bool("public", public_->isChecked());

        if (chatInfo_->Stamp_ != editGroupLinkWidget_->getNick() && !invalidNick_)
        {
            auto nick = editGroupLinkWidget_->getNick();
            collection.set_value_as_qstring("stamp", nick);

            aboutLinkToChat_->setText(Utils::getDomainUrl() % ql1c('/') % nick);

            GetDispatcher()->post_stats_to_core(public_->isChecked() ? core::stats::stats_event_names::profilescr_groupname_action : core::stats::stats_event_names::profilescr_newgroupurl_action, { { "chat_type", Utils::chatTypeByAimId(currentAimId_) } });
        }

        if (joinModeration_->isChecked() != chatInfo_->ApprovedJoin_)
            collection.set_value_as_bool("approved", joinModeration_->isChecked());

        if (Features::isThreadsForbidEnabled() && threadsEnabled_->isChecked() != chatInfo_->ThreadsEnabed_)
            collection.set_value_as_bool("threadsEnabled", threadsEnabled_->isChecked());

        if (trustRequired_ && trustRequired_->isChecked() != chatInfo_->trustRequired_)
            collection.set_value_as_bool("trustRequired", trustRequired_->isChecked());

        if (!collection->empty())
        {
            collection.set_value_as_qstring("aimid", currentAimId_);
            GetDispatcher()->post_message_to_core("chats/mod/params", collection.get());
        }

        Q_EMIT Utils::InterConnector::instance().setFocusOnInput(currentAimId_);

        editGroupLinkWidget_->clearHint();
    }

    void GroupProfile::checkApplyButton()
    {
        if (!chatInfo_)
            return;

        editGroupLinkWidget_->setVisible(public_->isChecked());
        aboutLinkToChatBlock_->setVisible(!public_->isChecked());

        const auto paramsChanged =
            public_->isChecked() != chatInfo_->Public_ ||
            chatInfo_->Stamp_ != editGroupLinkWidget_->getNick() ||
            joinModeration_->isChecked() != chatInfo_->ApprovedJoin_ ||
            (trustRequired_ && trustRequired_->isChecked() != chatInfo_->trustRequired_) ||
            (Features::isThreadsForbidEnabled() && threadsEnabled_->isChecked() != chatInfo_->ThreadsEnabed_);

        applyChatSettings_->setEnabled(!invalidNick_ && paramsChanged);

        if (invalidNick_)
        {
            aboutLinkToChat_->setTextLinkColor(Styling::ThemeColorKey{ Styling::StyleVariable::TEXT_SOLID });
            aboutLinkToChat_->text_->setCursorForText();
        }
        else
        {
            aboutLinkToChat_->setTextLinkColor(Styling::ThemeColorKey{ Styling::StyleVariable::TEXT_PRIMARY });
            aboutLinkToChat_->text_->unsetCursorForText();
        }
    }

    void GroupProfile::publicClicked()
    {
        if (!chatInfo_)
            return;

        if (!public_->isChecked())
        {
            if (chatInfo_->Public_)
            {
                const auto text = isChannel()
                                  ? QT_TRANSLATE_NOOP("popup_window", "If you change the channel type to private, the channel link will change and the old one will stop working")
                                  : QT_TRANSLATE_NOOP("popup_window", "If you change the group type to private, the group link will change and the old one will stop working");
                const auto confText = isChannel() ? QT_TRANSLATE_NOOP("popup_window", "Make private channel") : QT_TRANSLATE_NOOP("popup_window", "Make private");
                const auto confirm = Utils::GetConfirmationWithTwoButtons(
                    QT_TRANSLATE_NOOP("popup_window", "Cancel"),
                    confText,
                    text,
                    QT_TRANSLATE_NOOP("popup_window", "Public link will stop working"),
                    nullptr);

                if (!confirm)
                {
                    setCheckedBlocking(public_, true);
                    return;
                }

                requestNickSuggest();
            }
            else
            {
                aboutLinkToChat_->setText(Utils::getDomainUrl() % ql1c('/') % chatInfo_->Stamp_);
                editGroupLinkWidget_->setNick(chatInfo_->Stamp_);
            }
        }
        else
        {
            if (chatInfo_->Public_)
            {
                aboutLinkToChat_->setText(Utils::getDomainUrl() % ql1c('/') % chatInfo_->Stamp_);
                editGroupLinkWidget_->setNick(chatInfo_->Stamp_);
            }
            else
            {
                editGroupLinkWidget_->setNick(QString());

                requestNickSuggest();
            }
        }

        checkApplyButton();
    }

    void GroupProfile::opposePublicTrustRequired()
    {
        if (!chatInfo_ || !trustRequired_)
            return;

        const auto turnedOnPublic = public_->isChecked() && !chatInfo_->Public_;
        const auto turnedOnTrustReq = trustRequired_->isChecked() && !chatInfo_->trustRequired_;

        if (turnedOnPublic != turnedOnTrustReq)
            return;

        if (sender() == public_)
            trustRequired_->setChecked(false);
        else
            public_->setChecked(false);
    }

    void GroupProfile::hideControls()
    {
        setInfoPlaceholderVisible(true);

        {
            QSignalBlocker b(notifications_);
            notifications_->setChecked(false);
        }
        notifications_->setEnabled(false);
        {
            QSignalBlocker b(threadsAutoSubscribe_);
            threadsAutoSubscribe_->setChecked(false);
        }
        threadsAutoSubscribe_->setEnabled(false);

        mainActionButton_->setVisible(false);
        callButton_->setVisible(false);
        share_->setVisible(false);
        link_->setVisible(false);
        settings_->setVisible(false);
        secondSpacer_->setVisible(false);
        membersWidget_->setVisible(false);
        about_->setVisible(false);
        rules_->setVisible(false);
        pin_->setVisible(false);
        theme_->setVisible(false);
        fifthSpacer_->setVisible(false);
        clearHistory_->setVisible(false);
        block_->setVisible(false);
        report_->setVisible(false);
        leave_->setVisible(false);
        infoSpacer_->setVisible(false);
        editButton_->setVisible(false);
        editButton_->setVisibility(false);
    }

    void GroupProfile::updateActiveChatMembersModel(const QString& _aimId)
    {
        // if active tab with not fully loaded list of members - reset the list
        if (isActive() && currentAimId_ == _aimId
            && stackedWidget_ && stackedWidget_->currentIndex() == list
            && listChatModel_ && !listChatModel_->isFullMembersListLoaded())
        {
            listChatModel_->setSearchPattern(searchWidget_->getText());
        }
    }

    void GroupProfile::switchToGallery(MediaContentType _type)
    {
        galleryPopup_->hide();
        changeGalleryPage(_type);
        changeTab(gallery);
    }

    bool GroupProfile::validateTrustRequiredCheck(bool _desiredValue)
    {
        if (_desiredValue)
        {
            const auto warn = Emoji::EmojiCode::toQString(Emoji::EmojiCode(0x26a0));
            const auto text = QT_TRANSLATE_NOOP("popup_window", "Opening or downloading files will be possible from trusted device only\n\n%1 It will be impossible to disable this setting!");
            return Utils::GetConfirmationWithTwoButtons(
                QT_TRANSLATE_NOOP("popup_window", "Cancel"),
                QT_TRANSLATE_NOOP("popup_window", "OK"),
                text.arg(warn),
                QT_TRANSLATE_NOOP("popup_window", "File access will be restricted"),
                nullptr);
        }

        return _desiredValue;
    }

    void GroupProfile::setInfoPlaceholderVisible(bool _isVisible)
    {
        info_->setEnabled(!_isVisible);
        infoPlaceholderVisible_ = _isVisible;
        updateGalleryVisibility();
    }

    void GroupProfile::updateGalleryVisibility()
    {
        if (!galleryWidget_)
            return;

        if (infoPlaceholderVisible_ || !chatInfo_)
        {
            galleryWidget_->hide();
            thirdSpacer_->hide();
            fourthSpacer_->hide();
        }
        else
        {
            const bool notAMember = !chatInfo_ || chatInfo_->YouPending_ || chatInfo_->YourRole_.isEmpty();
            const auto visible = !galleryIsEmpty_ && !notAMember;
            galleryWidget_->setVisible(visible);
            thirdSpacer_->setVisible(visible);
            fourthSpacer_->setVisible(Logic::getIgnoreModel()->contains(currentAimId_) && !chatInfo_->YouBlocked_ && !notAMember && (!isChannel() || Data::ChatInfo::areYouAdminOrModer(chatInfo_)));
        }
    }

    void GroupProfile::updateSettingsCheckboxes()
    {
        const auto isPublic = chatInfo_ && chatInfo_->Public_;
        const auto isTrustRequired = chatInfo_ && chatInfo_->trustRequired_;
        const auto isThreadsEnabled = chatInfo_ && chatInfo_->ThreadsEnabed_;
        const auto isThreadSubscribe = chatInfo_ && chatInfo_->threadsAutoSubscribe_;

        setCheckedBlocking(public_, isPublic);
        setCheckedBlocking(joinModeration_, chatInfo_ && chatInfo_->ApprovedJoin_);
        setCheckedBlocking(trustRequired_, isTrustRequired);
        setCheckedBlocking(threadsEnabled_, isThreadsEnabled);
        setCheckedBlocking(threadsAutoSubscribe_, isThreadSubscribe);

        setCheckboxEnabled(public_, !isTrustRequired);
        setCheckboxEnabled(trustRequired_, !isPublic && !isTrustRequired);
    }

    bool GroupProfile::isThread() const
    {
        return Logic::getContactListModel()->isThread(currentAimId_);
    }

    bool GroupProfile::isChannel() const
    {
        return Data::ChatInfo::isChannel(chatInfo_);
    }

    QString GroupProfile::getSelectedText() const
    {
        return info_->getSelectedText() % about_->getSelectedText() % rules_->getSelectedText();
    }

    void GroupProfile::mousePressEvent(QMouseEvent* _event)
    {
        auto pos = area_->widget()->mapFromParent(_event->pos());
        pos.ry() -= titleBar_->height();
        info_->tryClearSelection(pos);
        about_->text_->tryClearSelection(pos);
        rules_->text_->tryClearSelection(pos);

        QWidget::mousePressEvent(_event);
    }

    void GroupProfile::showEvent(QShowEvent* _event)
    {
        if (searchWidget_&& stackedWidget_ && stackedWidget_->currentIndex() == list && !GeneralDialog::isActive())
            searchWidget_->setFocus();

        SidebarPage::showEvent(_event);
    }
}
