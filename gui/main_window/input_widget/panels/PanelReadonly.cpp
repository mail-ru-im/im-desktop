#include "stdafx.h"

#include "PanelReadonly.h"

#include "main_window/contact_list/ContactListModel.h"
#include "main_window/contact_list/RecentsModel.h"
#include "main_window/containers/FriendlyContainer.h"
#include "main_window/GroupChatOperations.h"

#include "controls/CustomButton.h"
#include "utils/utils.h"
#include "utils/stat_utils.h"
#include "utils/features.h"
#include "utils/InterConnector.h"
#include "styles/ThemeParameters.h"
#include "fonts.h"
#include "core_dispatcher.h"

namespace
{
    constexpr QSize shareButtonSize() noexcept
    {
        return QSize(32, 32);
    }

    constexpr int cancelIconSize() noexcept
    {
        return 24;
    }

    bool isShareButtonEnabled(const Ui::ReadonlyPanelState _state)
    {
        const std::vector<Ui::ReadonlyPanelState> disabled = { Ui::ReadonlyPanelState::DeleteAndLeave, Ui::ReadonlyPanelState::Unblock, Ui::ReadonlyPanelState::Start };
        return std::none_of(disabled.begin(), disabled.end(), [_state](const auto s) { return s == _state; });
    }
}

namespace Ui
{
    InputPanelReadonly::InputPanelReadonly(QWidget* _parent, const QString& _aimId)
        : QWidget(_parent)
        , aimId_(_aimId)
        , stamp_(Logic::getContactListModel()->getChatStamp(_aimId))
        , isChannel_(Logic::getContactListModel()->isChannel(_aimId))
        , mainButton_(new RoundButton(this))
        , shareButton_(new CustomButton(this, qsl(":/input/share"), shareButtonSize()))
    {
        setFixedHeight(getDefaultInputHeight());

        shareButton_->setFixedSize(Utils::scale_value(shareButtonSize()));
        shareButton_->setCustomToolTip(QT_TRANSLATE_NOOP("tooltips", "Share"));
        updateButtonColors(shareButton_, InputStyleMode::Default);

        auto shareHost = new QWidget(this);
        shareHost->setFixedWidth(getHorMargin());
        auto shareLayout = Utils::emptyHLayout(shareHost);
        shareLayout->addWidget(shareButton_);

        auto rootLayout = Utils::emptyHLayout(this);
        rootLayout->addSpacing(getHorMargin());
        rootLayout->addWidget(mainButton_);
        rootLayout->addWidget(shareHost);

        shareButton_->setFocusPolicy(Qt::TabFocus);
        shareButton_->setFocusColor(focusColorPrimary());

        setTabOrder(mainButton_, shareButton_);

        Testing::setAccessibleName(shareButton_, qsl("AS ChatInput shareButton"));
        Testing::setAccessibleName(mainButton_, qsl("AS ChatInput mainButton"));
        Testing::setAccessibleName(shareHost, qsl("AS ChatInput shareHost"));

        connect(mainButton_, &RoundButton::clicked, this, &InputPanelReadonly::onButtonClicked);
        connect(shareButton_, &CustomButton::clicked, this, &InputPanelReadonly::onShareClicked);

        connect(GetDispatcher(), &core_dispatcher::chatInfo, this, [this](const auto, const auto& _chatInfo){ updateFromChatInfo(_chatInfo); });

        updateStyle(InputStyleMode::Default);
    }

    void InputPanelReadonly::setState(const ReadonlyPanelState _state)
    {
        state_ = _state;
        im_assert(state_ != ReadonlyPanelState::Invalid);

        shareButton_->setVisible(isShareButtonEnabled(state_) && !stamp_.isEmpty());

        if (state_ == ReadonlyPanelState::AddToContactList || state_ == ReadonlyPanelState::Start)
        {
            mainButton_->setColors(
                Styling::getParameters().getColor(Styling::StyleVariable::PRIMARY),
                Styling::getParameters().getColor(Styling::StyleVariable::PRIMARY_HOVER),
                Styling::getParameters().getColor(Styling::StyleVariable::PRIMARY_ACTIVE)
            );
        }
        else
        {
            mainButton_->setColors(
                Styling::getParameters().getColor(Styling::StyleVariable::CHAT_PRIMARY),
                Styling::getParameters().getColor(Styling::StyleVariable::CHAT_PRIMARY_HOVER),
                Styling::getParameters().getColor(Styling::StyleVariable::CHAT_PRIMARY_ACTIVE)
            );
        }

        constexpr auto fontSize = platform::is_apple() ? 15 : 16;

        mainButton_->setUpdatesEnabled(false);
        switch (state_)
        {
        case ReadonlyPanelState::AddToContactList:
            mainButton_->setTextColor(Styling::getParameters().getColor(Styling::StyleVariable::BASE_GLOBALWHITE));
            mainButton_->setIcon(qsl(":/input/add"));
            mainButton_->setText(isChannel_ ? QT_TRANSLATE_NOOP("input_widget", "Subscribe") : QT_TRANSLATE_NOOP("input_widget", "Join"), fontSize);
            break;
        case ReadonlyPanelState::EnableNotifications:
            mainButton_->setTextColor(Styling::getParameters().getColor(Styling::StyleVariable::TEXT_PRIMARY));
            mainButton_->setIcon(qsl(":/input/unmute"));
            mainButton_->setText(QT_TRANSLATE_NOOP("input_widget", "Enable notifications"), fontSize);
            break;
        case ReadonlyPanelState::DisableNotifications:
            mainButton_->setTextColor(Styling::getParameters().getColor(Styling::StyleVariable::TEXT_PRIMARY));
            mainButton_->setIcon(qsl(":/input/mute"));
            mainButton_->setText(QT_TRANSLATE_NOOP("input_widget", "Disable notifications"), fontSize);
            break;
        case ReadonlyPanelState::DeleteAndLeave:
            mainButton_->setTextColor(Styling::getParameters().getColor(Styling::StyleVariable::SECONDARY_ATTENTION));
            mainButton_->setIcon(qsl(":/input/delete"));
            mainButton_->setText(QT_TRANSLATE_NOOP("input_widget", "Delete and leave"), fontSize);
            break;
        case ReadonlyPanelState::CancelJoin:
            mainButton_->setTextColor(Styling::getParameters().getColor(Styling::StyleVariable::SECONDARY_ATTENTION));
            mainButton_->setIcon(qsl(":/controls/close_icon"), cancelIconSize());
            mainButton_->setText(QT_TRANSLATE_NOOP("input_widget", "Cancel request"), fontSize);
            break;
        case ReadonlyPanelState::Unblock:
            mainButton_->setTextColor(Styling::getParameters().getColor(Styling::StyleVariable::SECONDARY_ATTENTION));
            mainButton_->setIcon(qsl(":/input/unlock"));
            mainButton_->setText(QT_TRANSLATE_NOOP("input_widget", "Unblock"), fontSize);
            break;
        case ReadonlyPanelState::Start:
            mainButton_->setTextColor(Styling::getParameters().getColor(Styling::StyleVariable::TEXT_SOLID_PERMANENT));
            mainButton_->setIcon(QPixmap());
            mainButton_->setText(QT_TRANSLATE_NOOP("input_widget", "Start"), fontSize);
            break;

        default:
            im_assert(!"invalid state");
            break;
        }
        mainButton_->setUpdatesEnabled(true);
    }

    void InputPanelReadonly::updateStyleImpl(const InputStyleMode _mode)
    {
        updateButtonColors(shareButton_, _mode);
    }

    void InputPanelReadonly::onButtonClicked()
    {
        switch (state_)
        {
        case ReadonlyPanelState::AddToContactList:
            joinClicked();
            break;

        case ReadonlyPanelState::EnableNotifications:
        case ReadonlyPanelState::DisableNotifications:
            notifClicked();
            break;

        case ReadonlyPanelState::DeleteAndLeave:
            leaveClicked();
            break;

        case ReadonlyPanelState::CancelJoin:
            cancelJoinClicked();
            break;

        case ReadonlyPanelState::Unblock:
            unblockClicked();
            break;

        case ReadonlyPanelState::Start:
            startClicked();
            break;

        default:
            im_assert(!"invalid state");
            break;
        }
    }

    void InputPanelReadonly::onShareClicked()
    {
        im_assert(!stamp_.isEmpty());
        if (!stamp_.isEmpty())
        {
            const QString link = u"https://" % Utils::getDomainUrl() % u'/' % stamp_;
            if (const auto count = forwardMessage(link, QString(), QString(), false))
                Ui::GetDispatcher()->post_stats_to_core(core::stats::stats_event_names::sharingscr_choicecontact_action, { { "count", Utils::averageCount(count) } });
        }
    }

    void InputPanelReadonly::joinClicked()
    {
        im_assert(!stamp_.isEmpty());
        if (!stamp_.isEmpty())
            Logic::getContactListModel()->joinLiveChat(stamp_, true);
    }

    void InputPanelReadonly::notifClicked()
    {
        im_assert(!aimId_.isEmpty());

        const auto actionMute = state_ == ReadonlyPanelState::DisableNotifications;
        Logic::getRecentsModel()->muteChat(aimId_, actionMute);

        const auto eventName = actionMute ? core::stats::stats_event_names::chatscr_mute_action : core::stats::stats_event_names::chatscr_unmute_action;
        GetDispatcher()->post_stats_to_core(eventName, { {"chat_type", getStatsChatTypeReadonly() } });
    }

    void InputPanelReadonly::unblockClicked()
    {
        const auto confirmed = Utils::GetConfirmationWithTwoButtons(
            QT_TRANSLATE_NOOP("popup_window", "Cancel"),
            QT_TRANSLATE_NOOP("popup_window", "Yes"),
            QT_TRANSLATE_NOOP("popup_window", "Are you sure you want to delete user from ignore list?"),
            Logic::GetFriendlyContainer()->getFriendly(aimId_),
            nullptr);

        if (confirmed)
        {
            Logic::getContactListModel()->ignoreContact(aimId_, false);
            Ui::GetDispatcher()->post_stats_to_core(core::stats::stats_event_names::chatscr_unblockuser_action, { {"chat_type", getStatsChatTypeReadonly() } });
        }
    }

    void InputPanelReadonly::leaveClicked()
    {
        auto confirmed = Utils::GetConfirmationWithTwoButtons(
            QT_TRANSLATE_NOOP("popup_window", "Cancel"),
            QT_TRANSLATE_NOOP("popup_window", "Yes"),
            QT_TRANSLATE_NOOP("popup_window", "Are you sure you want to leave chat?"),
            Logic::GetFriendlyContainer()->getFriendly(aimId_),
            nullptr);

        if (confirmed)
        {
            Logic::getContactListModel()->removeContactFromCL(aimId_);
            Ui::GetDispatcher()->post_stats_to_core(core::stats::stats_event_names::chatscr_leave_action, { {"chat_type", getStatsChatTypeReadonly() } });
        }
    }

    void InputPanelReadonly::cancelJoinClicked()
    {
        Utils::showCancelGroupJoinDialog(aimId_);
    }

    void InputPanelReadonly::startClicked()
    {
        Q_EMIT Utils::InterConnector::instance().startBot();
    }

    std::string InputPanelReadonly::getStatsChatTypeReadonly() const
    {
        return Utils::isChat(aimId_) ? (isChannel_ ? "channel" : "group") : "chat";
    }

    void InputPanelReadonly::updateFromChatInfo(const std::shared_ptr<Data::ChatInfo>& _chatInfo)
    {
        if (_chatInfo->AimId_ != aimId_)
            return;

        stamp_ = _chatInfo->Stamp_;
        isChannel_ = _chatInfo->isChannel();
        setState(currentState());
    }

    void InputPanelReadonly::setFocusOnButton()
    {
        mainButton_->setFocus(Qt::TabFocusReason);
    }
}