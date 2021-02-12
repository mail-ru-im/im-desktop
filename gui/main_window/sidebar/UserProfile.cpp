#include "stdafx.h"
#include "UserProfile.h"
#include "SidebarUtils.h"
#include "GalleryList.h"
#include "ImageVideoList.h"
#include "LinkList.h"
#include "FilesList.h"
#include "PttList.h"
#include "../MainPage.h"
#include "../MainWindow.h"
#include "../GroupChatOperations.h"
#include "../ReportWidget.h"
#include "../contact_list/TopPanel.h"
#include "../contact_list/RecentsModel.h"
#include "../contact_list/ContactListModel.h"
#include "../contact_list/ChatMembersModel.h"
#include "../contact_list/IgnoreMembersModel.h"
#include "../contact_list/CommonChatsModel.h"
#include "../contact_list/ContactListWidget.h"
#include "../contact_list/SearchWidget.h"
#include "../settings/themes/WallpaperDialog.h"
#include "../containers/FriendlyContainer.h"
#include "../containers/LastseenContainer.h"
#include "../../previewer/toast.h"
#include "../../controls/TransparentScrollBar.h"
#include "../../styles/ThemeParameters.h"
#include "../../utils/utils.h"
#include "../../utils/InterConnector.h"
#include "../../utils/stat_utils.h"
#include "../../utils/gui_coll_helper.h"
#include "../../utils/UrlParser.h"
#include "../../utils/features.h"
#include "../../utils/animations/WidgetAnimations.h"
#include "../../core_dispatcher.h"
#include "utils/PhoneFormatter.h"

namespace
{
    enum widgets
    {
        main = 0,
        gallery = 1,
        common_chats = 2,
    };

    enum gallery_page
    {
        photo_and_video = 0,
        video = 1,
        files = 2,
        links = 3,
        ptt = 4,
    };

    constexpr auto ICON_SIZE = 20;
    constexpr auto BTN_ICON_SIZE = 32;
    constexpr auto AVATAR_SIZE = 52;
    constexpr auto TIMER_INTERVAL = 60000;
    constexpr auto SPACER_MARGIN = 8;
    constexpr auto SMALL_SPACER_HEIGHT = 1;
    constexpr auto TOAST_OFFSET = 10;
    constexpr auto TOP_OFFSET = 10;
    constexpr auto BOTTOM_OFFSET = 20;
    constexpr auto INFO_SPACER_HEIGHT = 8;
    constexpr auto BOTTOM_SPACER = 40;
    constexpr auto POPUP_HOR_OFFSET = 8;
    constexpr auto POPUP_VER_OFFSET = 52;

    QMap<QString, QVariant> makeData(const QString& command)
    {
        QMap<QString, QVariant> result;
        result[qsl("command")] = command;
        return result;
    }

    auto getButtonIconSize() noexcept
    {
        return QSize(BTN_ICON_SIZE, BTN_ICON_SIZE);
    }
}

namespace Ui
{
    UserProfile::UserProfile(QWidget* _parent)
        : SidebarPage(_parent)
        , galleryWidget_(nullptr)
        , report_(nullptr)
        , remove_(nullptr)
        , frameCountMode_(FrameCountMode::_1)
        , currentGalleryPage_(-1)
        , galleryIsEmpty_(false)
        , shortView_(false)
        , replaceFavorites_(true)
        , isInfoPlaceholderVisible_(false)
        , lastseenTimer_(nullptr)
        , seq_(-1)
    {
        init();
    }

    UserProfile::UserProfile(QWidget* _parent, const QString& _phone, const QString& _aimid, const QString& _friendly)
        : SidebarPage(_parent)
        , galleryWidget_(nullptr)
        , report_(nullptr)
        , remove_(nullptr)
        , currentPhone_(_phone)
        , frameCountMode_(FrameCountMode::_1)
        , currentGalleryPage_(-1)
        , galleryIsEmpty_(false)
        , shortView_(true)
        , replaceFavorites_(true)
        , isInfoPlaceholderVisible_(false)
        , lastseenTimer_(nullptr)
        , seq_(-1)
    {
        initShort();
        if (!_aimid.isEmpty())
        {
            info_->setAimIdAndSize(_aimid, Utils::scale_value(AVATAR_SIZE), _friendly);
            info_->makeClickable();
            connect(info_, &AvatarNameInfo::clicked, this, &UserProfile::shortViewAvatarClicked);
        }
        else
        {
            info_->setFrienlyAndSize(_friendly.isEmpty() ? _phone : _friendly, Utils::scale_value(AVATAR_SIZE));
            info_->nameOnly();
        }

        phone_->setText(PhoneFormatter::formatted(_phone), Styling::getParameters().getColor(Styling::StyleVariable::TEXT_PRIMARY));
        controlsWidget_->setVisible(!_aimid.isEmpty());
        openFavorites_->setVisible(false);

        if (!_aimid.isEmpty())
        {
            currentAimId_ = _aimid;
            updateLastSeen();
        }
        switchMessageOrCallMode(MessageOrCallMode::Message);
    }

    UserProfile::~UserProfile()
    {
        Logic::GetLastseenContainer()->unsubscribe(currentAimId_);
    }

    void UserProfile::initFor(const QString& _aimId, SidebarParams _params)
    {
        Utils::ScopedPropertyRollback blocker(this, "updatesEnabled", false);

        const auto newContact = currentAimId_ != _aimId;
        if (newContact)
            Logic::GetLastseenContainer()->unsubscribe(currentAimId_);

        currentAimId_ = _aimId;
        replaceFavorites_ = _params.showFavorites_;
        frameCountMode_ = _params.frameCountMode_;
        loadInfo();

        if (newContact)
        {
            info_->setAimIdAndSize(currentAimId_, Utils::scale_value(AVATAR_SIZE));

            Testing::setAccessibleName(this, qsl("AS Sidebar UserProfile ") + _aimId);
            Testing::setAccessibleName(info_, qsl("AS Sidebar UserProfile AvatarNameInfo ") + _aimId);

            updateMuteState();
            updatePinButton();

            galleryPreview_->setAimId(_aimId);

            const auto& st = Logic::getContactListModel()->getGalleryState(currentAimId_);
            dialogGalleryState(currentAimId_, st);
            if (st.isEmpty())
                Ui::GetDispatcher()->requestDialogGalleryState(currentAimId_);

            changeTab(main);
            area_->ensureVisible(0, 0);

            info_->setInfo(QString());

            setInfoPlaceholderVisible(true);
            about_->hide();
            nick_->hide();
            email_->hide();
            phone_->hide();
            share_->hide();
            generalGroups_->hide();
            controlsWidget_->setEnabled(false);
        }

        updateCloseButton();
        updateControls();
        updateLastSeen();

        Ui::GetDispatcher()->post_stats_to_core(core::stats::stats_event_names::profilescr_view, { { "chat_type", Utils::chatTypeByAimId(currentAimId_) } });

        isActive_ = true;

        emitContentsChanged();
        disableFading();

        elapsedTimer_.start();
    }

    void UserProfile::setFrameCountMode(FrameCountMode _mode)
    {
        if (frameCountMode_ != _mode)
        {
            frameCountMode_ = _mode;
            updateCloseButton();
            updateControls();
        }
    }

    void UserProfile::close()
    {
        if (lastseenTimer_)
            lastseenTimer_->stop();
        Logic::GetLastseenContainer()->unsubscribe(currentAimId_);

        closeGallery();
        changeTab(main);
        area_->ensureVisible(0, 0);

        isActive_ = false;
    }

    void UserProfile::resizeEvent(QResizeEvent* _event)
    {
        if (!shortView_)
        {
            cl_->setWidthForDelegate(_event->size().width());
            galleryPopup_->setFixedWidth(std::max(0, _event->size().width() - Utils::scale_value(POPUP_HOR_OFFSET * 2)));
        }

        SidebarPage::resizeEvent(_event);
    }

    void UserProfile::init()
    {
        auto layout = Utils::emptyVLayout(nullptr);

        auto areaContainer = new QWidget(this);
        auto areaContainerLayout = Utils::emptyVLayout(areaContainer);

        titleBar_ = new HeaderTitleBar;
        titleBar_->setStyleSheet(ql1s("background-color: %1; border-bottom: %2 solid %3;").arg(Styling::getParameters().getColorHex(Styling::StyleVariable::BASE_GLOBALWHITE),
                                                                                              QString::number(Utils::scale_value(1)) % u"px",
                                                                                              Styling::getParameters().getColorHex(Styling::StyleVariable::BASE_BRIGHT)));
        titleBar_->setTitle(isFavorites() ? QT_TRANSLATE_NOOP("sidebar", "Favorites") : QT_TRANSLATE_NOOP("sidebar", "Information"));
        const auto headerIconSize = QSize(ICON_SIZE, ICON_SIZE);
        titleBar_->setButtonSize(Utils::scale_value(headerIconSize));
        {
            editButton_ = new HeaderTitleBarButton(this);
            editButton_->setDefaultImage(qsl(":/context_menu/edit"), Styling::getParameters().getColor(Styling::StyleVariable::BASE_SECONDARY), headerIconSize);
            titleBar_->addButtonToRight(editButton_);
            connect(editButton_, &HeaderTitleBarButton::clicked, this, &UserProfile::editButtonClicked);
        }
        {
            closeButton_ = new HeaderTitleBarButton(this);
            Testing::setAccessibleName(closeButton_, qsl("AS Sidebar UserProfile closeButton"));
            titleBar_->addButtonToLeft(closeButton_);
            connect(closeButton_, &HeaderTitleBarButton::clicked, this, &UserProfile::closeClicked);
        }

        connect(titleBar_, &HeaderTitleBar::arrowClicked, this, &UserProfile::titleArrowClicked);

        areaContainerLayout->addWidget(titleBar_);
        stackedWidget_ = new QStackedWidget(areaContainer);

        Testing::setAccessibleName(stackedWidget_, qsl("AS Sidebar UserProfile stackedWidget"));
        areaContainerLayout->addWidget(stackedWidget_);

        area_ = CreateScrollAreaAndSetTrScrollBarV(stackedWidget_);
        area_->setWidget(initContent(area_));
        area_->setWidgetResizable(true);
        area_->setFrameStyle(QFrame::NoFrame);
        area_->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
        Utils::transparentBackgroundStylesheet(area_);

        stackedWidget_->insertWidget(main, area_);
        stackedWidget_->insertWidget(gallery, initGallery(stackedWidget_));
        stackedWidget_->insertWidget(common_chats, initCommonChats(stackedWidget_));
        connect(stackedWidget_, &QStackedWidget::currentChanged, this, [this]() { emitContentsChanged(); });

        stackedWidget_->setCurrentIndex(main);

        layout->addWidget(areaContainer);
        setLayout(layout);

        updateCloseButton();
        setMouseTracking(true);

        connect(Logic::getRecentsModel(), &Logic::RecentsModel::favoriteChanged, this, &UserProfile::favoriteChanged);
        connect(Logic::getRecentsModel(), &Logic::RecentsModel::unimportantChanged, this, &UserProfile::unimportantChanged);
        connect(Ui::GetDispatcher(), &Ui::core_dispatcher::dialogGalleryState, this, &UserProfile::dialogGalleryState);
        connect(Ui::GetDispatcher(), &Ui::core_dispatcher::userInfo, this, &UserProfile::userInfo);
        connect(Ui::GetDispatcher(), &Ui::core_dispatcher::recvPermitDeny, this, &UserProfile::recvPermitDeny);
        connect(Ui::GetDispatcher(), &Ui::core_dispatcher::connectionStateChanged, this, &UserProfile::connectionStateChanged);
        connect(Logic::getContactListModel(), &Logic::ContactListModel::contactChanged, this, &UserProfile::contactChanged);
        connect(Logic::GetLastseenContainer(), &Logic::LastseenContainer::lastseenChanged, this, &UserProfile::lastseenChanged);
        connect(Logic::GetFriendlyContainer(), &Logic::FriendlyContainer::friendlyChanged, this, &UserProfile::friendlyChanged);
    }

    void UserProfile::initShort()
    {
        auto layout = Utils::emptyVLayout(this);

        auto w = initContent(this);
        layout->addSpacerItem(new QSpacerItem(0, Utils::scale_value(TOP_OFFSET), QSizePolicy::Preferred, QSizePolicy::Fixed));
        layout->addWidget(w);
        layout->addSpacerItem(new QSpacerItem(0, Utils::scale_value(BOTTOM_OFFSET), QSizePolicy::Preferred, QSizePolicy::Fixed));

        setMouseTracking(true);

        connect(Logic::GetLastseenContainer(), &Logic::LastseenContainer::lastseenChanged, this, &UserProfile::lastseenChanged);
    }

    QWidget* UserProfile::initContent(QWidget* _parent)
    {
        auto widget = new QWidget(_parent);
        Utils::transparentBackgroundStylesheet(widget);
        Utils::emptyContentsMargins(widget);

        auto layout = Utils::emptyVLayout(widget);
        layout->setAlignment(Qt::AlignTop);

        infoContainer_ = new QWidget(_parent);
        layout->addWidget(infoContainer_);

        auto infoContainerLayout = Utils::emptyVLayout(infoContainer_);

        {
            info_ = addAvatarInfo(widget, infoContainerLayout);
            connect(info_, &AvatarNameInfo::avatarClicked, this, &UserProfile::avatarClicked);
            connect(info_, &AvatarNameInfo::badgeClicked, &Utils::InterConnector::instance(), &Utils::InterConnector::openStatusPicker);

            setInfoPlaceholderVisible(false);

            infoSpacer_ = new QWidget(widget);
            Utils::transparentBackgroundStylesheet(infoSpacer_);
            infoSpacer_->setFixedHeight(INFO_SPACER_HEIGHT);
            infoContainerLayout->addWidget(infoSpacer_);

            {
                controlsWidget_ = new QWidget(widget);
                auto controlsWidgetLayoutV = Utils::emptyVLayout(controlsWidget_);
                auto controlsWidgetLayoutH = Utils::emptyHLayout(nullptr);

                const auto iconSize = getButtonIconSize();
                messageOrCall_ = addColoredButton({}, {}, controlsWidget_, controlsWidgetLayoutH, iconSize, Fading::Off);
                connect(messageOrCall_, &ColoredButton::clicked, this, [this]()
                {
                    if (messageOrCallMode_ == MessageOrCallMode::Message)
                        messageClicked();
                    else
                        audioCallClicked();
                });

                audioCall_ = addColoredButton(qsl(":/sidebar_call"), QString(), controlsWidget_, controlsWidgetLayoutH, iconSize, Fading::Off);
                audioCall_->setMargins(0, 0, audioCall_->getMargins().right(), audioCall_->getMargins().bottom());
                audioCall_->makeRounded();
                connect(audioCall_, &ColoredButton::clicked, this, &UserProfile::audioCallClicked);

                videoCall_ = addColoredButton(qsl(":/sidebar_videocall"), QString(), controlsWidget_, controlsWidgetLayoutH, iconSize, Fading::Off);
                videoCall_->setMargins(0, 0, videoCall_->getMargins().right(), audioCall_->getMargins().bottom());
                videoCall_->makeRounded();
                connect(videoCall_, &ColoredButton::clicked, this, &UserProfile::videoCallClicked);

                controlsWidgetLayoutV->addLayout(controlsWidgetLayoutH);

                if (!shortView_)
                {
                    unblock_ = addColoredButton(qsl(":/unlock_icon"), QT_TRANSLATE_NOOP("sidebar", "Unblock"), controlsWidget_, controlsWidgetLayoutV);
                    connect(unblock_, &ColoredButton::clicked, this, &UserProfile::unblockClicked);
                }

                infoContainerLayout->addWidget(controlsWidget_);
            }

            openFavorites_ = addColoredButton(qsl(":/favorites_icon"), QT_TRANSLATE_NOOP("sidebar", "Open favorites"), controlsWidget_, infoContainerLayout, QSize(ICON_SIZE, ICON_SIZE));
            connect(openFavorites_, &ColoredButton::clicked, this, &UserProfile::messageClicked);

            if (!shortView_)
            {
                about_ = addInfoBlock(QT_TRANSLATE_NOOP("sidebar", "About me"), QString(), widget, infoContainerLayout, 0, Fading::On);
                about_->text_->makeTextField();
                connect(about_->text_, &TextLabel::menuAction, this, &UserProfile::menuAction);

                nick_ = addInfoBlock(QT_TRANSLATE_NOOP("sidebar", "Nickname"), QString(), widget, infoContainerLayout, 0, Fading::On);
                nick_->text_->showButtons();
                nick_->text_->addMenuAction(qsl(":/context_menu/mention"), QT_TRANSLATE_NOOP("context_menu", "Copy nickname"), makeData(qsl("copy_nick")));
                nick_->text_->addMenuAction(qsl(":/context_menu/copy"), QT_TRANSLATE_NOOP("context_menu", "Copy the link to this profile"), makeData(qsl("copy_link")));
                nick_->text_->addMenuAction(qsl(":/context_menu/forward"), QT_TRANSLATE_NOOP("context_menu", "Share profile link"), makeData(qsl("share_link_nick")));

                connect(nick_->text_, &TextLabel::textClicked, this, &UserProfile::nickClicked);
                connect(nick_->text_, &TextLabel::copyClicked, this, &UserProfile::nickCopy);
                connect(nick_->text_, &TextLabel::shareClicked, this, &UserProfile::nickShare);
                connect(nick_->text_, &TextLabel::menuAction, this, &UserProfile::menuAction);

                email_ = addInfoBlock(QT_TRANSLATE_NOOP("sidebar", "Email"), QString(), widget, infoContainerLayout, 0, Fading::On);
                email_->text_->showButtons();
                email_->text_->addMenuAction(qsl(":/context_menu/link"), QT_TRANSLATE_NOOP("context_menu", "Copy email"), makeData(qsl("copy_email")));
                email_->text_->addMenuAction(qsl(":/context_menu/copy"), QT_TRANSLATE_NOOP("context_menu", "Copy the link to this profile"), makeData(qsl("copy_link")));
                email_->text_->addMenuAction(qsl(":/context_menu/forward"), QT_TRANSLATE_NOOP("context_menu", "Share profile link"), makeData(qsl("share_link_email")));

                connect(email_->text_, &TextLabel::textClicked, this, &UserProfile::emailClicked);
                connect(email_->text_, &TextLabel::copyClicked, this, &UserProfile::emailCopy);
                connect(email_->text_, &TextLabel::shareClicked, this, &UserProfile::share);
                connect(email_->text_, &TextLabel::menuAction, this, &UserProfile::menuAction);
            }

            if (shortView_)
                addSpacer(widget, layout);

            phone_ = addInfoBlock(QT_TRANSLATE_NOOP("sidebar", "Phone number"), QString(), widget, infoContainerLayout, 0, Fading::On);
            phone_->text_->showButtons();
            phone_->text_->makeCopyable();
            connect(phone_->text_, &TextLabel::copyClicked, this, &UserProfile::phoneCopy);
            connect(phone_->text_, &TextLabel::shareClicked, this, &UserProfile::sharePhoneClicked);
            connect(phone_->text_, &TextLabel::textClicked, this, &UserProfile::phoneClicked);
            connect(phone_->text_, &TextLabel::menuAction, this, &UserProfile::menuAction);

            if (shortView_)
            {
                addSpacer(widget, infoContainerLayout, Utils::scale_value(SMALL_SPACER_HEIGHT));
                auto label = addLabel(QString(), widget, infoContainerLayout, 0, TextRendering::HorAligment::LEFT, Fading::On);
                label->setText(QT_TRANSLATE_NOOP("sidebar", "Save"), Styling::getParameters().getColor(Styling::StyleVariable::TEXT_PRIMARY));
                label->setCursorForText();
                connect(label, &TextLabel::textClicked, this, [this]() { Q_EMIT saveClicked(QPrivateSignal()); });
            }

            if (shortView_)
                return widget;
        }

        addSpacer(widget, infoContainerLayout);

        {
            share_ = addButton(qsl(":/share_icon"), QT_TRANSLATE_NOOP("sidebar", "Share contact"), widget, infoContainerLayout);
            connect(share_, &SidebarButton::clicked, this, &UserProfile::sharePhoneClicked);

            notifications_ = addCheckbox(qsl(":/notify_icon"), QT_TRANSLATE_NOOP("sidebar", "Notifications"), widget, infoContainerLayout);
            connect(notifications_, &SidebarCheckboxButton::checked, this, &UserProfile::notificationsChecked);
        }

        firstSpacer_ = addSpacer(widget, layout);

        {
            galleryWidget_ = new QWidget(widget);
            auto galleryFader = new Utils::WidgetFader(galleryWidget_);
            galleryFader->setEventDirection(QEvent::Show, QPropertyAnimation::Forward);
            auto galleryLayout = Utils::emptyVLayout(galleryWidget_);

            galleryPhoto_ = addButton(qsl(":/background_icon"), QT_TRANSLATE_NOOP("sidebar", "Photo and video"), galleryWidget_, galleryLayout);
            connect(galleryPhoto_, &SidebarButton::clicked, this, &UserProfile::galleryPhotoClicked);

            galleryPreview_ = addGalleryPrevieWidget(galleryWidget_, galleryLayout);

            galleryVideo_ = addButton(qsl(":/video_icon"), QT_TRANSLATE_NOOP("sidebar", "Video"), galleryWidget_, galleryLayout);
            connect(galleryVideo_, &SidebarButton::clicked, this, &UserProfile::galleryVideoClicked);

            galleryFiles_ = addButton(qsl(":/gallery/file_icon"), QT_TRANSLATE_NOOP("sidebar", "Files"), galleryWidget_, galleryLayout);
            connect(galleryFiles_, &SidebarButton::clicked, this, &UserProfile::galleryFilesClicked);

            galleryLinks_ = addButton(qsl(":/copy_link_icon"), QT_TRANSLATE_NOOP("sidebar", "Links"), galleryWidget_, galleryLayout);
            connect(galleryLinks_, &SidebarButton::clicked, this, &UserProfile::galleryLinksClicked);

            galleryPtt_ = addButton(qsl(":/gallery/micro_icon"), QT_TRANSLATE_NOOP("sidebar", "Voice messages"), galleryWidget_, galleryLayout);
            connect(galleryPtt_, &SidebarButton::clicked, this, &UserProfile::galleryPttClicked);

            layout->addWidget(galleryWidget_);
        }

        secondSpacer_ = addSpacer(widget, layout);

        {
            generalGroups_ = addButton(qsl(":/members_icon"), QT_TRANSLATE_NOOP("sidebar", "Common groups"), widget, layout);
            connect(generalGroups_, &SidebarButton::clicked, this, &UserProfile::commonChatsClicked);

            createGroup_ = addButton(qsl(":/header/add_user"), QT_TRANSLATE_NOOP("sidebar", "Create common group"), widget, layout);
            connect(createGroup_, &SidebarButton::clicked, this, &UserProfile::createGroupClicked);

            pin_ = addButton(qsl(":/pin_chat_icon"), QT_TRANSLATE_NOOP("sidebar", "Pin"), widget, layout);
            connect(pin_, &SidebarButton::clicked, this, &UserProfile::pinClicked);

            theme_ = addButton(qsl(":/colors_icon"), QT_TRANSLATE_NOOP("sidebar", "Wallpapers"), widget, layout);
            connect(theme_, &SidebarButton::clicked, this, &UserProfile::themeClicked);
        }

        addSpacer(widget, layout);

        {
            clearHistory_ = addButton(qsl(":/clear_chat_icon"), QT_TRANSLATE_NOOP("sidebar", "Clear history"), widget, layout);
            connect(clearHistory_, &SidebarButton::clicked, this, &UserProfile::clearHistoryClicked);

            block_ = addButton(qsl(":/ignore_icon"), QT_TRANSLATE_NOOP("sidebar", "Block"), widget, layout);
            connect(block_, &SidebarButton::clicked, this, &UserProfile::blockClicked);

            report_ = addButton(qsl(":/alert_icon"), QT_TRANSLATE_NOOP("sidebar", "Report"), widget, layout);
            connect(report_, &SidebarButton::clicked, this, &UserProfile::reportCliked);

            remove_ = addButton(qsl(":/context_menu/delete"), QT_TRANSLATE_NOOP("sidebar", "Remove"), widget, layout);
            connect(remove_, &SidebarButton::clicked, this, &UserProfile::removeContact);
        }

        layout->addSpacerItem(new QSpacerItem(0, Utils::scale_value(BOTTOM_SPACER), QSizePolicy::Preferred, QSizePolicy::Fixed));

        layout->setSizeConstraint(QLayout::SetMinimumSize);
        return widget;
    }

    QWidget* UserProfile::initGallery(QWidget* _parent)
    {
        galleryPopup_ = new GalleryPopup();
        connect(galleryPopup_, &GalleryPopup::galleryPhotoClicked, this, &UserProfile::galleryPhotoClicked);
        connect(galleryPopup_, &GalleryPopup::galleryVideoClicked, this, &UserProfile::galleryVideoClicked);
        connect(galleryPopup_, &GalleryPopup::galleryFilesClicked, this, &UserProfile::galleryFilesClicked);
        connect(galleryPopup_, &GalleryPopup::galleryLinksClicked, this, &UserProfile::galleryLinksClicked);
        connect(galleryPopup_, &GalleryPopup::galleryPttClicked, this, &UserProfile::galleryPttClicked);

        auto widget = new QWidget(_parent);
        auto layout = Utils::emptyVLayout(widget);

        gallery_ = new GalleryList(widget, QString());
        layout->addWidget(gallery_);

        return widget;
    }

    QWidget* UserProfile::initCommonChats(QWidget* _parent)
    {
        auto widget = new QWidget(_parent);
        auto contactListLayout = Utils::emptyVLayout(widget);

        searchWidget_ = new SearchWidget(widget);
        searchWidget_->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Preferred);
        contactListLayout->addWidget(searchWidget_);

        contactListLayout->addSpacerItem(new QSpacerItem(0, Utils::scale_value(SPACER_MARGIN), QSizePolicy::Preferred, QSizePolicy::Fixed));

        commonChatsModel_ = new Logic::CommonChatsModel(this);
        commonChatsSearchModel_ = new Logic::CommonChatsSearchModel(this, commonChatsModel_);

        cl_ = new Ui::ContactListWidget(widget, Logic::COMMON_CHATS, nullptr, commonChatsSearchModel_, commonChatsModel_);
        cl_->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Expanding);
        cl_->setContentsMargins(0, 0, 0, 0);
        cl_->connectSearchWidget(searchWidget_);

        connect(cl_, &ContactListWidget::selected, this, &UserProfile::chatSelected);

        contactListLayout->addWidget(cl_);

        return widget;
    }

    void UserProfile::updateCloseButton()
    {
        QString iconPath;
        if (frameCountMode_ != FrameCountMode::_1 && prevAimId_.isEmpty() && stackedWidget_->currentIndex() == main)
            iconPath = qsl(":/controls/close_icon");
        else
            iconPath = qsl(":/controls/back_icon");

        closeButton_->setDefaultImage(iconPath, Styling::getParameters().getColor(Styling::StyleVariable::BASE_SECONDARY));
        closeButton_->setHoverImage(iconPath, Styling::getParameters().getColor(Styling::StyleVariable::BASE_SECONDARY_HOVER));
    }

    void UserProfile::updatePinButton()
    {
        const auto isPinned = Logic::getRecentsModel()->isFavorite(currentAimId_);
        pin_->setIcon(Utils::renderSvgScaled(isPinned ? qsl(":/unpin_chat_icon") : qsl(":/pin_chat_icon"), QSize(ICON_SIZE, ICON_SIZE), Styling::getParameters().getColor(Styling::StyleVariable::BASE_SECONDARY)));
        pin_->setText(isPinned ? QT_TRANSLATE_NOOP("sidebar", "Unpin") : QT_TRANSLATE_NOOP("sidebar", "Pin"));
        pin_->setVisible(!Logic::getRecentsModel()->isUnimportant(currentAimId_));
    }

    void UserProfile::changeTab(int _tab)
    {
        Q_EMIT cl_->forceSearchClear(true);

        switch (_tab)
        {
            case main:
                titleBar_->setTitle(isFavorites() ? QT_TRANSLATE_NOOP("sidebar", "Favorites") : QT_TRANSLATE_NOOP("sidebar", "Information"));
                titleBar_->setArrowVisible(false);
                break;

            case gallery:
                titleBar_->setArrowVisible(true);
                break;

            case common_chats:
                titleBar_->setTitle(QT_TRANSLATE_NOOP("sidebar", "Common groups"));
                titleBar_->setArrowVisible(false);
                break;
        }

        stackedWidget_->setCurrentIndex(_tab);
        updateCloseButton();

        const auto haveContact = Logic::getContactListModel()->getContactItem(currentAimId_) != nullptr;
        const auto itsMe = (currentAimId_ == Ui::MyInfo()->aimId());
        const auto canEdit = !itsMe && haveContact && (!Logic::getRecentsModel()->isSuspicious(currentAimId_) && !Logic::getRecentsModel()->isStranger(currentAimId_));

        editButton_->setVisible(_tab == main && canEdit);
        editButton_->setVisibility(_tab == main && canEdit);
    }

    void UserProfile::changeGalleryPage(int _page)
    {
        switch (_page)
        {
            case photo_and_video:
                titleBar_->setTitle(QT_TRANSLATE_NOOP("sidebar", "Photo and video"));
                gallery_->setTypes({ ql1s("image"), ql1s("video") });
                gallery_->setContentWidget(new ImageVideoList(gallery_, MediaContentWidget::ImageVideo, "Photo+Video"));
                break;

            case video:
                titleBar_->setTitle(QT_TRANSLATE_NOOP("sidebar", "Video"));
                gallery_->setType(ql1s("video"));
                gallery_->setContentWidget(new ImageVideoList(gallery_, MediaContentWidget::Video, "Video"));
                break;

            case files:
                titleBar_->setTitle(QT_TRANSLATE_NOOP("sidebar", "Files"));
                gallery_->setTypes({ ql1s("file"), ql1s("audio") });
                gallery_->setContentWidget(new FilesList(gallery_));
                break;

            case links:
                titleBar_->setTitle(QT_TRANSLATE_NOOP("sidebar", "Links"));
                gallery_->setType(ql1s("link"));
                gallery_->setContentWidget(new LinkList(gallery_));
                break;

            case ptt:
                titleBar_->setTitle(QT_TRANSLATE_NOOP("sidebar", "Voice messages"));
                gallery_->setType(ql1s("ptt"));
                gallery_->setContentWidget(new PttList(gallery_));
                break;

            default:
                assert(false);
        }

        gallery_->initFor(currentAimId_);
        currentGalleryPage_ = _page;

        emitContentsChanged();
    }

    void UserProfile::closeGallery()
    {
        gallery_->markClosed();
        Ui::GetDispatcher()->cancelGalleryHolesDownloading(currentAimId_);
    }

    void UserProfile::loadInfo()
    {
        Ui::GetDispatcher()->getUserInfo(currentAimId_);
    }

    void UserProfile::updateControls()
    {
        setUpdatesEnabled(false);

        if (!currentAimId_.isEmpty())
        {
            const auto& st = Logic::getContactListModel()->getGalleryState(currentAimId_);
            dialogGalleryState(currentAimId_, st);
            if (st.isEmpty())
                Ui::GetDispatcher()->requestDialogGalleryState(currentAimId_);
        }

        const auto selected = Logic::getContactListModel()->selectedContact() == currentAimId_;
        const auto isIgnored = Logic::getIgnoreModel()->contains(currentAimId_);
        const auto isUnimportant = Logic::getRecentsModel()->isUnimportant(currentAimId_);
        const auto itsMe = (currentAimId_ == Ui::MyInfo()->aimId());
        const auto inCL = Logic::getContactListModel()->hasContact(currentAimId_);
        const auto isBot = Logic::GetLastseenContainer()->isBot(currentAimId_);
        const auto canEdit = !itsMe && inCL && (!Logic::getRecentsModel()->isSuspicious(currentAimId_) && !Logic::getRecentsModel()->isStranger(currentAimId_));

        const auto isAuioCallVisible = !isBot && (!selected || frameCountMode_ == FrameCountMode::_1);
        switchMessageOrCallMode((isBot || isAuioCallVisible) ? MessageOrCallMode::Message : MessageOrCallMode::Call);

        infoContainer_->setVisible(!isFavorites());

        if (itsMe)
        {
            audioCall_->setVisible(false);
            videoCall_->setVisible(false);
            messageOrCall_->setVisible(false);
            openFavorites_->setVisible(true);
        }
        else
        {
            messageOrCall_->setVisible(true);
            audioCall_->setVisible(isAuioCallVisible);
            videoCall_->setVisible(!isBot);
            openFavorites_->setVisible(itsMe && !isFavorites());
        }
        unblock_->setVisible(isIgnored);
        block_->setVisible(!isIgnored && !itsMe);
        report_->setVisible(!itsMe && Features::clRemoveContactsAllowed());
        remove_->setVisible(inCL && Features::clRemoveContactsAllowed() && !itsMe);
        notifications_->setVisible(!isIgnored && !itsMe);
        createGroup_->setVisible(!itsMe);
        editButton_->setVisible(stackedWidget_->currentIndex() == main && canEdit);
        editButton_->setVisibility(stackedWidget_->currentIndex() == main && canEdit);

        pin_->setVisible(!isUnimportant);

        setUpdatesEnabled(true);

        emitContentsChanged();
    }

    void UserProfile::updateStatus(const Data::LastSeen& _lastseen)
    {
        QString status;
        bool online = false;

        if (Ui::GetDispatcher()->getConnectionState() == Ui::ConnectionState::stateOnline)
        {
            const auto& seen = currentAimId_ == Ui::MyInfo()->aimId() ? Data::LastSeen::online() : _lastseen;
            online = seen.isOnline();
            status = seen.getStatusString();
        }

        info_->setInfo(status, Styling::getParameters().getColor(online ? Styling::StyleVariable::TEXT_PRIMARY : Styling::StyleVariable::BASE_PRIMARY));
    }

    QString UserProfile::getShareLink() const
    {
        const QString res = u"https://" % Utils::getDomainUrl() % u'/';

        if (Features::isNicksEnabled() && !userInfo_.nick_.isEmpty())
        {
            return res % userInfo_.nick_;
        }
        else if (Utils::isUin(currentAimId_))
        {
            return res % currentAimId_;
        }
        else
        {
            Utils::UrlParser p;
            p.process(currentAimId_);
            if (p.hasUrl() && p.getUrl().is_email())
                return res % currentAimId_;
        }

        return QString();
    }

    bool UserProfile::isFavorites() const
    {
        return replaceFavorites_ && currentAimId_ == MyInfo()->aimId();
    }

    void Ui::UserProfile::switchMessageOrCallMode(MessageOrCallMode _mode)
    {
        messageOrCallMode_ = _mode;

        const auto makeIcon = [](const auto& _path)
        {
            return Utils::renderSvgScaled(_path, getButtonIconSize(), Styling::getParameters().getColor(Styling::StyleVariable::BASE_GLOBALWHITE));
        };

        if (_mode == MessageOrCallMode::Message)
        {
            messageOrCall_->setIcon(makeIcon(qsl(":/chat_icon")));
            messageOrCall_->setText(QT_TRANSLATE_NOOP("sidebar", "Message"));
        }
        else
        {
            messageOrCall_->setIcon(makeIcon(qsl(":/sidebar_call")));
            messageOrCall_->setText(QT_TRANSLATE_NOOP("sidebar", "Call"));
        }
    }

    void UserProfile::editButtonClicked()
    {
        const auto name = Logic::GetFriendlyContainer()->getFriendly(currentAimId_);
        const auto newName = editName(name);
        if (name != newName)
            Logic::getContactListModel()->renameContact(currentAimId_, newName);
    }

    void UserProfile::closeClicked()
    {
        if (stackedWidget_->currentIndex() != main)
        {
            closeGallery();
            changeTab(main);
            return;
        }

        if (prevAimId_.isEmpty() || prevAimId_ != Logic::getContactListModel()->selectedContact())
            Utils::InterConnector::instance().setSidebarVisible(false);
        else
            Utils::InterConnector::instance().showSidebar(prevAimId_);
    }

    void UserProfile::avatarClicked()
    {
        GetDispatcher()->post_stats_to_core(core::stats::stats_event_names::profilescr_avatar_click, { { "chat_type", Utils::chatTypeByAimId(currentAimId_) }});
        Utils::InterConnector::instance().getMainWindow()->openAvatar(currentAimId_);
    }

    void UserProfile::share()
    {
        const auto link = getShareLink();
        if (link.isEmpty())
            return;

        if (auto count = forwardMessage(link, QString(), QString(), false))
            Ui::GetDispatcher()->post_stats_to_core(core::stats::stats_event_names::sharingscr_choicecontact_action, { { "count", Utils::averageCount(count) } });
    }

    void UserProfile::shareClicked()
    {
        share();
        GetDispatcher()->post_stats_to_core(core::stats::stats_event_names::profilescr_sharecontact_action, { { "chat_type", Utils::chatTypeByAimId(currentAimId_) } });
    }

    void UserProfile::copy(const QString& _text)
    {
        QApplication::clipboard()->setText(_text);
    }

    void UserProfile::sharePhoneClicked()
    {
        const auto& phone = shortView_ ? currentPhone_ : userInfo_.phoneNormalized_;
        const auto& name = shortView_ ? info_->getFriendly() : userInfo_.friendly_;
        sharePhone(name, phone, currentAimId_);
        Ui::GetDispatcher()->post_stats_to_core(core::stats::stats_event_names::profilescr_mobilenumber_action, { { "do", "share" } });
    }

    void UserProfile::notificationsChecked(bool _checked)
    {
        Logic::getRecentsModel()->muteChat(currentAimId_, !_checked);
        GetDispatcher()->post_stats_to_core(_checked ? core::stats::stats_event_names::unmute : core::stats::stats_event_names::mute_sidebar);
        GetDispatcher()->post_stats_to_core(core::stats::stats_event_names::profilescr_notifications_event, { { "chat_type", Utils::chatTypeByAimId(currentAimId_) }, { "do" , _checked ? "enable" : "mute" } });
    }

    void UserProfile::galleryPhotoClicked()
    {
        galleryPopup_->hide();
        changeTab(gallery);
        changeGalleryPage(photo_and_video);
    }

    void UserProfile::galleryVideoClicked()
    {
        galleryPopup_->hide();
        changeTab(gallery);
        changeGalleryPage(video);
    }

    void UserProfile::galleryFilesClicked()
    {
        galleryPopup_->hide();
        changeTab(gallery);
        changeGalleryPage(files);
    }

    void UserProfile::galleryLinksClicked()
    {
        galleryPopup_->hide();
        changeTab(gallery);
        changeGalleryPage(links);
    }

    void UserProfile::galleryPttClicked()
    {
        galleryPopup_->hide();
        changeTab(gallery);
        changeGalleryPage(ptt);
    }

    void UserProfile::createGroupClicked()
    {
        createGroupChat({ currentAimId_ }, CreateChatSource::profile);
        Ui::GetDispatcher()->post_stats_to_core(core::stats::stats_event_names::profilescr_newgroup_action);
    }

    void UserProfile::commonChatsClicked()
    {
        changeTab(common_chats);
        commonChatsModel_->load(currentAimId_);
        Ui::GetDispatcher()->post_stats_to_core(core::stats::stats_event_names::profilescr_groups_action);
    }

    void UserProfile::pinClicked()
    {
        GetDispatcher()->pinContact(currentAimId_, !Logic::getRecentsModel()->isFavorite(currentAimId_));
        Q_EMIT Utils::InterConnector::instance().closeAnySemitransparentWindow({ Utils::CloseWindowInfo::Initiator::Unknown, Utils::CloseWindowInfo::Reason::Keep_Sidebar });
    }

    void UserProfile::themeClicked()
    {
        showWallpaperSelectionDialog(currentAimId_);
        Ui::GetDispatcher()->post_stats_to_core(core::stats::stats_event_names::profilescr_wallpaper_action, { { "chat_type", Utils::chatTypeByAimId(currentAimId_) } });
    }

    void UserProfile::clearHistoryClicked()
    {
        closeGallery();

        const auto confirmed = Utils::GetConfirmationWithTwoButtons(
                QT_TRANSLATE_NOOP("popup_window", "Cancel"),
                QT_TRANSLATE_NOOP("popup_window", "Yes"),
                QT_TRANSLATE_NOOP("popup_window", "Are you sure you want to erase chat history?"),
                Logic::GetFriendlyContainer()->getFriendly(currentAimId_),
                nullptr);

        if (confirmed)
            eraseHistory(currentAimId_);
    }

    void UserProfile::blockClicked()
    {
        closeGallery();

        if (Logic::getContactListModel()->ignoreContactWithConfirm(currentAimId_))
        {
            GetDispatcher()->post_stats_to_core(core::stats::stats_event_names::ignore_contact, { { "From", "Sidebar" } });
            Utils::InterConnector::instance().setSidebarVisible(false);
            GetDispatcher()->post_stats_to_core(core::stats::stats_event_names::profilescr_blockbuttons_action, { { "type", "block" },{ "chat_type", Utils::chatTypeByAimId(currentAimId_) } });
        }
    }

    void UserProfile::reportCliked()
    {
        closeGallery();

        if (Ui::ReportContact(currentAimId_, Logic::GetFriendlyContainer()->getFriendly(currentAimId_)))
        {
            Logic::getContactListModel()->ignoreContact(currentAimId_, true);
            GetDispatcher()->post_stats_to_core(core::stats::stats_event_names::spam_contact, { { "From", "Sidebar" } });
            GetDispatcher()->post_stats_to_core(core::stats::stats_event_names::profilescr_blockbuttons_action, { { "type", "spam" },{ "chat_type", Utils::chatTypeByAimId(currentAimId_) } });
        }
    }

    void UserProfile::removeContact()
    {
        const QString text = QT_TRANSLATE_NOOP("popup_window", "Remove %1 from your contacts?").arg(Logic::GetFriendlyContainer()->getFriendly(currentAimId_));

        const auto confirm = Utils::GetConfirmationWithTwoButtons(
                QT_TRANSLATE_NOOP("popup_window", "Cancel"),
                QT_TRANSLATE_NOOP("popup_window", "Yes"),
                text,
                QT_TRANSLATE_NOOP("popup_window", "Remove contact"),
                nullptr
        );

        if (confirm)
        {
            Logic::getRecentsModel()->hideChat(currentAimId_);
            Logic::getContactListModel()->removeContactFromCL(currentAimId_);
        }
    }

    void UserProfile::favoriteChanged(const QString& _aimid)
    {
        if (_aimid == currentAimId_)
            updatePinButton();
    }

    void UserProfile::unimportantChanged(const QString& _aimid)
    {
        if (_aimid == currentAimId_)
            updatePinButton();
    }

    void UserProfile::dialogGalleryState(const QString& _aimId, const Data::DialogGalleryState& _state)
    {
        if (_aimId != currentAimId_)
            return;

        galleryIsEmpty_ = _state.isEmpty();

        galleryPhoto_->setCounter(_state.images_count_ + _state.videos_count);
        galleryVideo_->setCounter(_state.videos_count);
        galleryFiles_->setCounter(_state.files_count);
        galleryLinks_->setCounter(_state.links_count);
        galleryPtt_->setCounter(_state.ptt_count);
        galleryPopup_->setCounters(_state.images_count_ + _state.videos_count, _state.videos_count, _state.files_count, _state.links_count, _state.ptt_count);

        updateGalleryVisibility();

        if (stackedWidget_->currentIndex() == gallery)
        {
            switch (currentGalleryPage_)
            {
                case photo_and_video:
                    if (_state.images_count_ + _state.videos_count != 0)
                        return;
                    break;

                case video:
                    if (_state.videos_count != 0)
                        return;
                    break;

                case files:
                    if (_state.files_count != 0)
                        return;
                    break;

                case links:
                    if (_state.links_count != 0)
                        return;
                    break;

                case ptt:
                    if (_state.ptt_count != 0)
                        return;
                    break;
            }

            closeGallery();
            changeTab(main);
        }
    }

    void UserProfile::userInfo(const int64_t, const QString& _aimid, const Data::UserInfo& _info)
    {
        if (_aimid != currentAimId_)
            return;

        if (_info.isEmpty_)
        {
            setInfoPlaceholderVisible(true);
            hideControls();
            return;
        }

        userInfo_ = _info;
        enableFading();
        const auto timeLag = (std::chrono::milliseconds(elapsedTimer_.elapsed()) - kLoadDelay);
        if (timeLag.count() > 0 && timeLag < kShowDelay)
            QTimer::singleShot(kShowDelay, this, &UserProfile::refresh);
        else
            refresh();
    }

    void UserProfile::refresh()
    {
        setInfoPlaceholderVisible(false);
        controlsWidget_->setEnabled(true);
        about_->setVisible(!userInfo_.about_.isEmpty());
        about_->setText(userInfo_.about_);

        if (Utils::isValidEmailAddress(currentAimId_))
        {
            nick_->hide();
            email_->show();
            email_->setText(currentAimId_, Styling::getParameters().getColor(Styling::StyleVariable::TEXT_PRIMARY));
        }
        else
        {
            email_->hide();
            nick_->setVisible(!userInfo_.nick_.isEmpty() && Features::isNicksEnabled());
            nick_->setText(Utils::makeNick(userInfo_.nick_), Styling::getParameters().getColor(Styling::StyleVariable::TEXT_PRIMARY));
        }


        phone_->setVisible(!userInfo_.phoneNormalized_.isEmpty());
        phone_->setText(userInfo_.phoneNormalized_, Styling::getParameters().getColor(Styling::StyleVariable::TEXT_PRIMARY));
        share_->setVisible(!userInfo_.phoneNormalized_.isEmpty());
        const auto isIgnored = Logic::getIgnoreModel()->contains(currentAimId_);

        generalGroups_->setVisible(true);
        generalGroups_->setCounter(userInfo_.commonChats_);
        theme_->setVisible(true);
        clearHistory_->setVisible(true);
        notifications_->setEnabled(true);

        updateLastSeen();
        updateControls();

        QTimer::singleShot(kShowDuration, this, &UserProfile::disableFading);
    }

    void UserProfile::enableFading()
    {
        Utils::WidgetFader::setEffectEnabled(this, true);
    }

    void UserProfile::disableFading()
    {
        Utils::WidgetFader::setEffectEnabled(this, false);
    }

    void UserProfile::scrollToTop()
    {
        area_->ensureVisible(0, 0);
    }

    void UserProfile::menuAction(QAction* action)
    {
        const auto params = action->data().toMap();
        const auto command = params[qsl("command")].toString();

        if (command == u"copy")
        {
            const auto text = params[qsl("text")].toString();
            copy(text);
            Utils::showToastOverMainWindow(QT_TRANSLATE_NOOP("toast", "Copied to clipboard"), Utils::scale_value(TOAST_OFFSET));
        }
        else if (command == u"copy_nick")
        {
            nickClicked();
        }
        else if (command == u"copy_link")
        {
            if (auto link = getShareLink(); !link.isEmpty())
            {
                copy(link);
                Utils::showToastOverMainWindow(link % QChar::LineFeed % QT_TRANSLATE_NOOP("toast", "Link copied"), Utils::scale_value(TOAST_OFFSET));
            }
        }
        else if (command == u"share_link_nick")
        {
            nickShare();
        }
        else if (command == u"copy_email")
        {
            emailClicked();
        }
        else if (command == u"share_link_email")
        {
            share();
        }
    }

    void UserProfile::messageClicked()
    {
        const auto selectedContact = Logic::getContactListModel()->selectedContact();
        if (selectedContact != currentAimId_)
                Q_EMIT Utils::InterConnector::instance().addPageToDialogHistory(selectedContact);

        Logic::GetFriendlyContainer()->updateFriendly(currentAimId_);
        Logic::getContactListModel()->setCurrent(currentAimId_, -1, true);
        GetDispatcher()->post_stats_to_core(core::stats::stats_event_names::open_chat, { { "From", "Profile" } });
        GetDispatcher()->post_stats_to_core(core::stats::stats_event_names::profilescr_message_action);
    }

    void UserProfile::audioCallClicked()
    {
        doVoipCall(currentAimId_, CallType::Audio, CallPlace::Profile);
    }

    void UserProfile::videoCallClicked()
    {
        doVoipCall(currentAimId_, CallType::Video, CallPlace::Profile);
    }

    void UserProfile::unblockClicked()
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

    void UserProfile::updateLastSeen()
    {
        if (!currentAimId_.isEmpty())
        {
            const auto lastseen = Logic::GetLastseenContainer()->getLastSeen(currentAimId_, Logic::LastSeenRequestType::Subscribe);
            updateStatus(lastseen);

            if (!lastseen.isBot() && !lastseen.isAbsent() && !lastseen.isBlocked())
            {
                if (!lastseenTimer_)
                {
                    lastseenTimer_ = new QTimer(this);
                    lastseenTimer_->setInterval(TIMER_INTERVAL);
                    connect(lastseenTimer_, &QTimer::timeout, this, &UserProfile::updateLastSeen);
                }
                lastseenTimer_->start();
            }
        }
    }

    void UserProfile::updateNick()
    {
        const auto nick = Utils::makeNick(Logic::GetFriendlyContainer()->getNick(currentAimId_));
        if (!nick.isEmpty() && !currentAimId_.contains(u'@'))
        {
            nick_->setVisible(Features::isNicksEnabled());
            nick_->setText(nick, Styling::getParameters().getColor(Styling::StyleVariable::TEXT_PRIMARY));
        }
        else
        {
            nick_->setVisible(false);
        }
    }

    void UserProfile::connectionStateChanged()
    {
        updateLastSeen();
        if (Ui::GetDispatcher()->getConnectionState() == Ui::ConnectionState::stateOnline && !shortView_ && !currentAimId_.isEmpty())
            loadInfo();
    }

    void UserProfile::shortViewAvatarClicked()
    {
        const auto selectedContact = Logic::getContactListModel()->selectedContact();
        if (selectedContact != currentAimId_)
                Q_EMIT Utils::InterConnector::instance().addPageToDialogHistory(selectedContact);

        Logic::getContactListModel()->setCurrent(currentAimId_, -1, true);
        Utils::InterConnector::instance().showSidebar(currentAimId_);
    }

    void UserProfile::chatSelected(const QString& _aimid)
    {
        const auto selectedContact = Logic::getContactListModel()->selectedContact();
        if (selectedContact != _aimid)
                Q_EMIT Utils::InterConnector::instance().addPageToDialogHistory(selectedContact);

        Logic::getContactListModel()->setCurrent(_aimid, -1, true);
    }

    void UserProfile::recvPermitDeny(bool)
    {
        if (Utils::InterConnector::instance().isSidebarVisible())
            updateControls();
    }

    void UserProfile::emailClicked()
    {
        copy(currentAimId_);
        Utils::showToastOverMainWindow(QT_TRANSLATE_NOOP("sidebar", "Email copied"), Utils::scale_value(TOAST_OFFSET));
    }

    void UserProfile::emailCopy(const QString& _email)
    {
        copy(_email);
        Utils::showToastOverMainWindow(QT_TRANSLATE_NOOP("sidebar", "Email copied"), Utils::scale_value(TOAST_OFFSET));
    }

    void UserProfile::nickCopy(const QString& _nick)
    {
        copy(_nick);
        Utils::showToastOverMainWindow(QT_TRANSLATE_NOOP("sidebar", "Nickname copied"), Utils::scale_value(TOAST_OFFSET));
        Ui::GetDispatcher()->post_stats_to_core(core::stats::stats_event_names::profilescr_groupurl_action, { { "chat_type", "chat" }, { "do", "copy_nick" } });
    }

    void UserProfile::nickShare()
    {
        share();
        Ui::GetDispatcher()->post_stats_to_core(core::stats::stats_event_names::profilescr_groupurl_action, { { "chat_type", "chat" }, { "do", "share" } });
    }

    void UserProfile::nickClicked()
    {
        nickCopy(Utils::makeNick(userInfo_.nick_));
    }

    void UserProfile::phoneCopy(const QString& _phone)
    {
        copy(_phone);
        Utils::showToastOverMainWindow(QT_TRANSLATE_NOOP("sidebar", "Phone number copied"), Utils::scale_value(TOAST_OFFSET));
        Ui::GetDispatcher()->post_stats_to_core(core::stats::stats_event_names::profilescr_mobilenumber_action, { { "do", "copy" } });
    }

    void UserProfile::phoneClicked()
    {
        phoneCopy(shortView_ ? currentPhone_ : userInfo_.phoneNormalized_);
    }

    void UserProfile::contactChanged(const QString& _aimid)
    {
        if (currentAimId_ == _aimid)
        {
            updateLastSeen();
            updateMuteState();
        }
    }

    void UserProfile::titleArrowClicked()
    {
        galleryPopup_->move(mapToGlobal(QPoint(Utils::scale_value(POPUP_HOR_OFFSET), Utils::scale_value(POPUP_VER_OFFSET))));
        galleryPopup_->show();
    }

    void UserProfile::lastseenChanged(const QString& _aimid)
    {
        if (_aimid == currentAimId_)
            updateLastSeen();
    }

    void UserProfile::updateMuteState()
    {
        QSignalBlocker b(notifications_);
        notifications_->setChecked(!Logic::getContactListModel()->isMuted(currentAimId_));
    }

    void UserProfile::friendlyChanged(const QString& _aimid, const QString& _friendly)
    {
        Q_UNUSED(_friendly)

        if (currentAimId_ != _aimid)
            return;

        updateNick();
    }

    void UserProfile::hideControls()
    {
        setInfoPlaceholderVisible(true);

        notifications_->setEnabled(false);
        notifications_->setChecked(false);

        controlsWidget_->setVisible(false);
        nick_->setVisible(false);
        email_->setVisible(false);
        phone_->setVisible(false);
        about_->setVisible(false);
        share_->setVisible(false);
        about_->setVisible(false);
        pin_->setVisible(false);
        theme_->setVisible(false);
        clearHistory_->setVisible(false);
        block_->setVisible(false);
        report_->setVisible(false);
        editButton_->setVisible(false);
        editButton_->setVisibility(false);
        generalGroups_->setVisible(false);
        createGroup_->setVisible(false);
        remove_->setVisible(false);
    }

    void UserProfile::setInfoPlaceholderVisible(bool _isVisible)
    {
        info_->setEnabled(!_isVisible);
        isInfoPlaceholderVisible_ = _isVisible;
        updateGalleryVisibility();
    }

    void UserProfile::updateGalleryVisibility()
    {
        if (!galleryWidget_)
            return;

        if (isInfoPlaceholderVisible_)
        {
            galleryWidget_->hide();
            secondSpacer_->hide();
        }
        else
        {
            galleryWidget_->setVisible(!galleryIsEmpty_);
            secondSpacer_->setVisible(!galleryIsEmpty_);
        }
    }

    QString UserProfile::getSelectedText() const
    {
        return about_->getSelectedText();
    }
}
