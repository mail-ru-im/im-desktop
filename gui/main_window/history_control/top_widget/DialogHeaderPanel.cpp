#include "stdafx.h"

#include "DialogHeaderPanel.h"
#include "OverlayTopChatWidget.h"
#include "main_window/contact_list/ContactListModel.h"
#include "main_window/contact_list/ChatContactsModel.h"
#include "main_window/contact_list/ContactListUtils.h"
#include "main_window/contact_list/SelectionContactsForGroupChat.h"
#include "main_window/contact_list/FavoritesUtils.h"
#include "main_window/contact_list/ServiceContacts.h"
#include "main_window/containers/FriendlyContainer.h"
#include "main_window/containers/LastseenContainer.h"
#include "main_window/proxy/AvatarStorageProxy.h"
#include "main_window/proxy/FriendlyContaInerProxy.h"
#include "main_window/ConnectionWidget.h"
#include "main_window/GroupChatOperations.h"
#include "controls/CustomButton.h"
#include "controls/ContactAvatarWidget.h"
#include "controls/StartCallButton.h"
#include "utils/utils.h"
#include "utils/InterConnector.h"
#include "utils/features.h"
#include "styles/ThemeParameters.h"
#include "types/chat.h"
#include "fonts.h"
#include "core_dispatcher.h"

namespace
{
    constexpr auto getTopIconsSize() noexcept
    {
        return QSize(24, 24);
    }

    constexpr auto getTopButtonsSize() noexcept
    {
        return QSize(24, 24);
    }

    constexpr auto backIconSize() noexcept
    {
        return QSize(20, 20);
    }

    auto getTopButtonsLeftSpace() noexcept
    {
        return Utils::scale_value(12);
    }

    auto getTopButtonsSpacing() noexcept
    {
        return Utils::scale_value(16);
    }

    auto contactNameFont()
    {
        return Fonts::appFontScaled(16, platform::is_apple() ? Fonts::FontWeight::Medium : Fonts::FontWeight::Normal);
    }

    auto threadsFeedContactNameFont()
    {
        return Fonts::appFontScaled(16, Fonts::FontWeight::SemiBold);
    }

    auto contactStatusFont()
    {
        return Fonts::appFontScaled(14, Fonts::FontWeight::Normal);
    }

    int badgeTopOffset() noexcept
    {
        return Utils::scale_value(8);
    }

    int badgeLeftOffset() noexcept
    {
        return Utils::scale_value(28);
    }

    int avatarSize() noexcept
    {
        return Utils::scale_value(32);
    }

    int rightMargin() noexcept
    {
        return Utils::scale_value(16);
    }

    int backIconHorMargin() noexcept
    {
        return Utils::scale_value(12);
    }

    int avatarRightSpacing() noexcept
    {
        return Utils::scale_value(8);
    }

    int nameHeight() noexcept
    {
        return Utils::scale_value(20);
    }

    int statusHeight() noexcept
    {
        return Utils::scale_value(16);
    }

    constexpr std::chrono::milliseconds statusUpdateInterval() { return std::chrono::minutes(1); }
    constexpr auto chatNamePaddingLeft() noexcept { return 16; }
}

namespace Ui
{
    DialogHeaderPanel::DialogHeaderPanel(const QString& _aimId, QWidget* _parent)
        : QWidget(_parent)
        , aimId_(_aimId)
        , youMember_(!Utils::isChat(_aimId))
    {
        setFixedHeight(Utils::getTopPanelHeight());

        const auto isServiceContact = ServiceContacts::isServiceContact(aimId_);
        const auto isThreadsFeedContact = aimId_ == ServiceContacts::getThreadsName();

        auto topLayout = Utils::emptyHLayout(this);
        topLayout->setContentsMargins(0, 0, rightMargin(), 0);

        {
            topWidgetLeftPadding_ = new QWidget(this);
            topWidgetLeftPadding_->setFixedWidth(Utils::scale_value(chatNamePaddingLeft()));
            topWidgetLeftPadding_->setAttribute(Qt::WA_TransparentForMouseEvents);
            topLayout->addWidget(topWidgetLeftPadding_);

            prevChatButtonWidget_ = new QWidget(this);
            prevChatButtonWidget_->setFixedWidth(Utils::scale_value(getTopButtonsSize().width() + (backIconHorMargin() * 2)));
            auto prevButtonLayout = Utils::emptyHLayout(prevChatButtonWidget_);

            auto prevChatButton = new CustomButton(prevChatButtonWidget_, qsl(":/controls/back_icon"), backIconSize());
            Styling::Buttons::setButtonDefaultColors(prevChatButton);
            prevChatButton->setFixedSize(Utils::scale_value(getTopButtonsSize()));
            prevChatButton->setFocusPolicy(Qt::TabFocus);
            prevChatButton->setFocusColor(Styling::getParameters().getPrimaryTabFocusColorKey());
            connect(prevChatButton, &CustomButton::clicked, this, [this, prevChatButton]()
            {
                Q_EMIT switchToPrevDialog(prevChatButton->hasFocus(), QPrivateSignal());
            });

            Testing::setAccessibleName(prevChatButton, qsl("AS HistoryPage backButton"));
            prevButtonLayout->addWidget(prevChatButton);
            Testing::setAccessibleName(prevChatButtonWidget_, qsl("AS HistoryPage backButtonWidget"));
            topLayout->addWidget(prevChatButtonWidget_);
        }

        if (!isThreadsFeedContact)
        {
            avatar_ = new ContactAvatarWidget(this, aimId_, Logic::GetFriendlyContainer()->getFriendly(aimId_), avatarSize(), true);
            avatar_->setStatusTooltipEnabled(!Favorites::isFavorites(aimId_));
            avatar_->SetOffset(Utils::scale_value(1));
            avatar_->SetSmallBadge(true);
            avatar_->setCursor(Qt::PointingHandCursor);
            avatar_->setAvatarProxyFlags(avatarProxyFlags());
            avatar_->setIgnoreClicks(isServiceContact);
            if (isServiceContact)
                avatar_->setCursor(Qt::ArrowCursor);

            connect(avatar_, &ContactAvatarWidget::leftClicked, this, &DialogHeaderPanel::onInfoClicked);

            Testing::setAccessibleName(avatar_, qsl("AS HistoryPage dialogAvatar"));
            topLayout->addWidget(avatar_);

            topLayout->addSpacing(avatarRightSpacing() - (avatar_->width() - avatarSize()));
        }

        {
            contactWidget_ = new QWidget(this);
            contactWidget_->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);

            auto contactWidgetLayout = Utils::emptyVLayout(contactWidget_);
            const auto nameFont = isThreadsFeedContact ? threadsFeedContactNameFont() : contactNameFont();

            contactName_ = new ClickableTextWidget(contactWidget_, nameFont, Styling::ThemeColorKey{ Styling::StyleVariable::TEXT_SOLID });
            contactName_->setFixedHeight(nameHeight());
            contactName_->setCursor(isServiceContact ? Qt::ArrowCursor : Qt::PointingHandCursor);
            contactName_->setContextMenuPolicy(Qt::ContextMenuPolicy::CustomContextMenu);

            if (!isServiceContact)
            {
                connect(contactName_, &ClickableWidget::clicked, this, &DialogHeaderPanel::onInfoClicked);
                connect(contactName_, &ClickableWidget::hoverChanged, this, &DialogHeaderPanel::updateContactNameTooltip);
            }

            Testing::setAccessibleName(contactName_, qsl("AS HistoryPage dialogName"));
            contactWidgetLayout->addWidget(contactName_);
            if (isThreadsFeedContact)
                contactWidgetLayout->setAlignment(contactName_, Qt::AlignHCenter);

            contactStatus_ = new ClickableTextWidget(this, contactStatusFont(), Styling::ThemeColorKey{ Styling::StyleVariable::BASE_PRIMARY });
            contactStatus_->setFixedHeight(statusHeight());
            contactStatus_->setVisible(!isServiceContact);
            Testing::setAccessibleName(contactStatus_, qsl("AS HistoryPage dialogStatus"));
            contactWidgetLayout->addWidget(contactStatus_);

            Testing::setAccessibleName(contactWidget_, qsl("AS HistoryPage dialogWidget"));
            topLayout->addWidget(contactWidget_);
        }

        {
            connectionWidget_ = new ConnectionWidget(this);
            connectionWidget_->setVisible(false);

            Testing::setAccessibleName(connectionWidget_, qsl("AS HistoryPage connectionWidget"));
            topLayout->addWidget(connectionWidget_);
        }

        {
            buttonsWidget_ = new QWidget(this);

            auto buttonsLayout = Utils::emptyHLayout(buttonsWidget_);
            buttonsLayout->addSpacing(getTopButtonsLeftSpace());
            buttonsLayout->setSpacing(getTopButtonsSpacing());

            const auto btn = [this, iconSize = getTopIconsSize()](QStringView _iconName, const QString& _tooltip, const QString& _accesibleName = {})
            {
                auto button = new CustomButton(buttonsWidget_, _iconName.toString(), iconSize);
                Styling::Buttons::setButtonDefaultColors(button);
                button->setFixedSize(Utils::scale_value(getTopButtonsSize()));
                button->setCustomToolTip(_tooltip);

                if (Testing::isEnabled() && !_accesibleName.isEmpty())
                    Testing::setAccessibleName(button, _accesibleName);

                return button;
            };

            searchButton_ = btn(u":/search_icon", QT_TRANSLATE_NOOP("tooltips", "Search for messages"), qsl("AS HistoryPage searchButton"));
            connect(searchButton_, &CustomButton::clicked, this, &DialogHeaderPanel::searchButtonClicked);
            connect(&Utils::InterConnector::instance(), &Utils::InterConnector::startSearchInDialog, searchButton_, [this](const QString& _searchedDialog)
            {
                if (aimId_ == _searchedDialog && !searchButton_->isActive())
                    searchButton_->setActive(true);
            });
            connect(&Utils::InterConnector::instance(), &Utils::InterConnector::searchClosed, this, &DialogHeaderPanel::deactivateSearchButton);
            connect(&Utils::InterConnector::instance(), &Utils::InterConnector::disableSearchInDialog, this, &DialogHeaderPanel::deactivateSearchButton);
            buttonsLayout->addWidget(searchButton_, 0, Qt::AlignRight);

            if (Features::isExpandedGalleryEnabled())
            {
                galleryButton_ = btn(u":/header/gallery", QT_TRANSLATE_NOOP("tooltips", "Chat gallery"), qsl("AS HistoryPage gallery"));
                connect(galleryButton_,
                    &CustomButton::clicked,
                    &Utils::InterConnector::instance(),
                    [aimId = aimId_]() { Q_EMIT Utils::InterConnector::instance().openDialogGallery(aimId); });
                buttonsLayout->addWidget(galleryButton_, 0, Qt::AlignRight);
            }

            pendingButton_ = btn(u":/pending_icon", QT_TRANSLATE_NOOP("sidebar", "Waiting for approval"), qsl("AS HistoryPage pendingButton"));
            connect(pendingButton_, &CustomButton::clicked, this, &DialogHeaderPanel::pendingButtonClicked);
            buttonsLayout->addWidget(pendingButton_, 0, Qt::AlignRight);
            pendingButton_->hide();

            addMemberButton_ = btn(u":/header/add_user", QT_TRANSLATE_NOOP("tooltips", "Add member"), qsl("AS HistoryPage addMemberButton"));
            connect(addMemberButton_, &CustomButton::clicked, this, &DialogHeaderPanel::addMember);
            buttonsLayout->addWidget(addMemberButton_, 0, Qt::AlignRight);

            callButton_ = new StartCallButton(buttonsWidget_, CallButtonType::Standalone);
            callButton_->setAimId(aimId_);
            Testing::setAccessibleName(callButton_, qsl("AS HistoryPage callButton"));

            buttonsLayout->addWidget(callButton_, 0, Qt::AlignRight);

            auto moreButton = btn(u":/about_icon_small", QT_TRANSLATE_NOOP("tooltips", "Information"), qsl("AS HistoryPage moreButton"));
            connect(moreButton, &CustomButton::clicked, this, &DialogHeaderPanel::onInfoClicked);
            connect(moreButton, &CustomButton::clicked, this, []() { GetDispatcher()->post_stats_to_core(core::stats::stats_event_names::more_button_click); });
            buttonsLayout->addWidget(moreButton, 0, Qt::AlignRight);

            addMemberButton_->setVisible(!isServiceContact && Utils::isChat(aimId_));
            updateCallButtonsVisibility();

            const auto setButtonVisible = [](CustomButton* _btn, bool _visible)
            {
                if (_btn)
                    _btn->setVisible(_visible);
            };

            for (auto btn : { galleryButton_, moreButton })
                setButtonVisible(btn, !isServiceContact);

            Testing::setAccessibleName(buttonsWidget_, qsl("AS HistoryPage buttonsWidget"));
            topLayout->addWidget(buttonsWidget_, 0, Qt::AlignRight);
        }

        overlayTopChatWidget_ = new OverlayTopChatWidget(this);
        overlayPendings_ = new OverlayTopChatWidget(this);
        overlayPendings_->hide();

        setPrevChatButtonVisible(false);
        setContactStatusClickable(false);
        updateName();

        if (!isServiceContact && Features::isExpandedGalleryEnabled())
        {
            connect(GetDispatcher(), &core_dispatcher::dialogGalleryState, this, &DialogHeaderPanel::dialogGalleryState);
            dialogGalleryState(aimId_, Logic::getContactListModel()->getGalleryState(aimId_));

            requestDialogGallery();
        }

        connect(GetDispatcher(), &core_dispatcher::connectionStateChanged, this, &DialogHeaderPanel::connectionStateChanged);
        if (GetDispatcher()->getConnectionState() != ConnectionState::stateOnline)
            connectionStateChanged(GetDispatcher()->getConnectionState());

        connect(Logic::GetFriendlyContainer(), &Logic::FriendlyContainer::friendlyChanged, this, [this](const QString& _aimId)
        {
            if (_aimId == aimId_)
                updateName();
        });
        connect(Logic::GetLastseenContainer(), &Logic::LastseenContainer::lastseenChanged, this, [this](const QString& _aimId)
        {
            if (_aimId == aimId_)
                initStatus();
        });
        connect(&Utils::InterConnector::instance(), &Utils::InterConnector::changeBackButtonVisibility, this, &DialogHeaderPanel::onChangeBackButtonVisibility);
    }

    void DialogHeaderPanel::deactivateSearchButton()
    {
        searchButton_->setActive(false);
    }

    void DialogHeaderPanel::updateCallButtonsVisibility()
    {
#ifdef STRIP_VOIP
        bool isVisible = false;
#else
        bool isVisible =
            GetDispatcher()->getConnectionState() == ConnectionState::stateOnline
            && !Logic::getContactListModel()->isChannel(aimId_)
            && !Logic::GetLastseenContainer()->isBot(aimId_)
            && aimId_ != MyInfo()->aimId()
            && !Logic::getContactListModel()->isDeleted(aimId_)
            && !ServiceContacts::isServiceContact(aimId_);

        if (isVisible && chatMembersCount_ < 2 && Utils::isChat(aimId_))
            isVisible = false;
#endif
        callButton_->setVisible(isVisible);
    }

    void DialogHeaderPanel::setPrevChatButtonVisible(bool _visible, bool _saveState)
    {
        if (_saveState)
            backButtonWasVisible_ = prevChatButtonWidget_->isVisible() || _visible;
        prevChatButtonWidget_->setVisible(_visible);
        topWidgetLeftPadding_->setVisible(!_visible);
    }

    void DialogHeaderPanel::setContactStatusClickable(bool _isEnabled)
    {
        if (_isEnabled)
        {
            contactStatus_->setCursor(Qt::PointingHandCursor);
            connect(contactStatus_, &ClickableTextWidget::clicked, this, &DialogHeaderPanel::onStatusClicked, Qt::UniqueConnection);
        }
        else
        {
            contactStatus_->setCursor(Qt::ArrowCursor);
            disconnect(contactStatus_, &ClickableTextWidget::clicked, this, &DialogHeaderPanel::onStatusClicked);
        }
    }

    void DialogHeaderPanel::setOverlayTopWidgetVisible(bool _visible)
    {
        overlayTopChatWidget_->setVisible(_visible);
    }

    void DialogHeaderPanel::updatePendingButtonPosition()
    {
        overlayPendings_->resize(size());
        const auto horOffset = mapFromGlobal(pendingButton_->mapToGlobal(pendingButton_->rect().topRight())).x() - pendingButton_->width() / 2;
        overlayPendings_->setPosition({ horOffset, badgeTopOffset() });
    }

    void DialogHeaderPanel::updateFromChatInfo(const std::shared_ptr<Data::ChatInfo>& _info)
    {
        if (!_info)
        {
            const auto text = Logic::getContactListModel()->isChannel(aimId_)
                ? QT_TRANSLATE_NOOP("groupchats", "You are not a subscriber of this channel")
                : QT_TRANSLATE_NOOP("groupchats", "You are not a member of this chat");
            contactStatus_->setText(text);

            setContactStatusClickable(false);
            addMemberButton_->hide();

            youMember_ = false;
            updateGalleryButtonVisibility();
            return;
        }

        if (_info->AimId_ != aimId_)
            return;

        const auto youMember = Data::ChatInfo::areYouMember(_info);
        const auto youAdminModer = Data::ChatInfo::areYouAdminOrModer(_info);

        chatMembersCount_ = youMember ? _info->MembersCount_ : 0;

        const auto prevYouMember = std::exchange(youMember_, youMember || youAdminModer);
        if (!prevYouMember && youMember_)
            requestDialogGallery();

        if (youAdminModer)
        {
            const auto pendingCount = _info->PendingCount_;
            addMemberButton_->setVisible(pendingCount == 0);
            pendingButton_->setVisible(pendingCount != 0);
            overlayPendings_->setVisible(pendingCount != 0);
            overlayPendings_->setBadgeText(Utils::getUnreadsBadgeStr(pendingCount));

            updatePendingButtonPosition();
        }
        else
        {
            pendingButton_->hide();
            overlayPendings_->hide();
            addMemberButton_->setVisible(youMember);
        }

        updateGalleryButtonVisibility();
        updateCallButtonsVisibility();

        contactName_->setText(_info->Name_);
        contactStatus_->setText(Utils::getMembersString(_info->MembersCount_, _info->isChannel()));
        contactStatus_->show();
        setContactStatusClickable(!_info->Members_.isEmpty() && youMember);

        if (youMember)
            Q_EMIT needUpdateMembers(QPrivateSignal());
    }

    void DialogHeaderPanel::setUnreadBadgeText(const QString& _text)
    {
        overlayTopChatWidget_->setBadgeText(_text);
    }

    void DialogHeaderPanel::suspendConnectionWidget()
    {
        connectionWidget_->suspend();
    }

    void DialogHeaderPanel::resumeConnectionWidget()
    {
        connectionWidget_->resume();
    }

    void DialogHeaderPanel::initStatus()
    {
        const auto startTimer = [this]()
        {
            if (!contactStatusTimer_)
            {
                contactStatusTimer_ = new QTimer(this);
                contactStatusTimer_->setInterval(statusUpdateInterval());
                connect(contactStatusTimer_, &QTimer::timeout, this, &DialogHeaderPanel::initStatus);
            }
            if (!contactStatusTimer_->isActive())
                contactStatusTimer_->start();
        };

        const auto stopTimer = [this]()
        {
            if (contactStatusTimer_)
                contactStatusTimer_->stop();
        };

        if (Favorites::isFavorites(aimId_))
        {
            stopTimer();
            contactStatus_->hide();
        }
        else if (Utils::isChat(aimId_))
        {
            stopTimer();
        }
        else
        {
            const auto lastSeen = Logic::GetLastseenContainer()->getLastSeen(aimId_, Logic::LastSeenRequestType::Subscribe);

            if (!lastSeen.isValid() || lastSeen.isOnline() || lastSeen.isBlocked() || lastSeen.isAbsent() || lastSeen.isBot())
                stopTimer();
            else // offline
                startTimer();

            const auto statusText = lastSeen.getStatusString();
            if (!statusText.isEmpty())
            {
                contactStatus_->setText(statusText);
                contactStatus_->setColor(Styling::ThemeColorKey{ lastSeen.isOnline() ? Styling::StyleVariable::TEXT_PRIMARY : Styling::StyleVariable::BASE_PRIMARY });
            }
            contactStatus_->setVisible(!statusText.isEmpty());

            if (avatar_)
                avatar_->update();
        }
    }

    void DialogHeaderPanel::updateName()
    {
        contactName_->setText(Logic::getFriendlyContainerProxy(friendlyProxyFlags()).getFriendly(aimId_));
    }

    void DialogHeaderPanel::resizeEvent(QResizeEvent* _e)
    {
        overlayTopChatWidget_->resize(size());
        overlayTopChatWidget_->setPosition({ badgeLeftOffset(), badgeTopOffset() });

        updatePendingButtonPosition();
        updateCallButtonsVisibility();
    }

    void DialogHeaderPanel::onInfoClicked()
    {
        const auto needHide =
            Utils::InterConnector::instance().isSidebarVisible() &&
            Utils::InterConnector::instance().getSidebarAimid() == aimId_;

        if (needHide)
            Utils::InterConnector::instance().setSidebarVisible(false);
        else if (Utils::isChat(aimId_))
            Utils::InterConnector::instance().showSidebar(aimId_);
        else
            Q_EMIT Utils::InterConnector::instance().profileSettingsShow(aimId_);
    }

    void DialogHeaderPanel::onStatusClicked()
    {
        Utils::InterConnector::instance().showMembersInSidebar(aimId_);
    }

    void DialogHeaderPanel::updateContactNameTooltip(bool _show)
    {
        if (Features::longPathTooltipsAllowed())
        {
            const auto name = Logic::GetFriendlyContainer()->getFriendly(aimId_);
            contactName_->setTooltipText(_show && contactName_->isElided() ? name : QString());
        }
    }

    void DialogHeaderPanel::searchButtonClicked()
    {
        if (!searchButton_->isActive())
            Q_EMIT Utils::InterConnector::instance().startSearchInDialog(aimId_);
        else
            Q_EMIT Utils::InterConnector::instance().searchEnd();
    }

    void DialogHeaderPanel::pendingButtonClicked()
    {
        if (!Utils::InterConnector::instance().isSidebarVisible())
            Utils::InterConnector::instance().showSidebar(aimId_);

        Q_EMIT Utils::InterConnector::instance().showPendingMembers();
    }

    void DialogHeaderPanel::addMember()
    {
        if (!Utils::isChat(aimId_))
            return;

        Logic::ChatContactsModel membersModel;
        membersModel.loadChatContacts(aimId_);

        auto selectMembersDialog = new SelectContactsWidget(&membersModel, Logic::MembersWidgetRegim::SELECT_MEMBERS,
            QT_TRANSLATE_NOOP("groupchats", "Add to chat"), QT_TRANSLATE_NOOP("popup_window", "Done"), this);

        connect(&membersModel, &Logic::ChatContactsModel::dataChanged, selectMembersDialog, &SelectContactsWidget::UpdateMembers);
        connect(this, &DialogHeaderPanel::needUpdateMembers, &membersModel, [&membersModel, aimId = aimId_]() { membersModel.loadChatContacts(aimId); });

        const auto guard = QPointer(this);
        const auto res = selectMembersDialog->show();
        if (!guard)
        {
            Logic::getContactListModel()->removeTemporaryContactsFromModel();
            return;
        }

        if (res == QDialog::Accepted)
        {
            postAddChatMembersFromCLModelToCore(aimId_);
            GetDispatcher()->post_stats_to_core(core::stats::stats_event_names::chats_add_member_dialog);
        }
        else
        {
            Logic::getContactListModel()->clearCheckedItems();
            Logic::getContactListModel()->removeTemporaryContactsFromModel();
        }

        selectMembersDialog->deleteLater();
    }

    void DialogHeaderPanel::connectionStateChanged(ConnectionState _state)
    {
        const auto isOnline = _state == Ui::ConnectionState::stateOnline;
        contactWidget_->setVisible(isOnline);
        connectionWidget_->setVisible(!isOnline);

        updateCallButtonsVisibility();
    }

    void DialogHeaderPanel::dialogGalleryState(const QString& _aimId, const Data::DialogGalleryState& _state)
    {
        if (_aimId == aimId_)
        {
            hasGallery_ = !_state.isEmpty();
            updateGalleryButtonVisibility();
        }
    }

    int32_t DialogHeaderPanel::friendlyProxyFlags() const
    {
        return Logic::FriendlyContainerProxy::ReplaceFavorites | Logic::FriendlyContainerProxy::ReplaceService;
    }

    int32_t DialogHeaderPanel::avatarProxyFlags() const
    {
        return Logic::AvatarStorageProxy::ReplaceFavorites | Logic::AvatarStorageProxy::ReplaceService;
    }

    void DialogHeaderPanel::updateGalleryButtonVisibility()
    {
        if (galleryButton_)
            galleryButton_->setVisible(youMember_ && hasGallery_);
    }

    void DialogHeaderPanel::requestDialogGallery()
    {
        GetDispatcher()->getDialogGallery(aimId_, { ql1s("image"), ql1s("video") }, 0, 0, 1, false);
    }

    void DialogHeaderPanel::onChangeBackButtonVisibility(const QString& _aimId, bool _isVisible)
    {
        if (aimId_ == _aimId)
        {
            const auto isVisible = _isVisible && backButtonWasVisible_;
            setPrevChatButtonVisible(isVisible, false);
            setOverlayTopWidgetVisible(isVisible);
        }
    }
}
