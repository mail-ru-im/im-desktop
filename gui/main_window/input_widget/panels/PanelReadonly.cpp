#include "stdafx.h"

#include "PanelReadonly.h"

#include "main_window/contact_list/ContactListModel.h"
#include "main_window/contact_list/RecentsModel.h"
#include "main_window/friendly/FriendlyContainer.h"
#include "main_window/GroupChatOperations.h"

#include "controls/CustomButton.h"
#include "utils/utils.h"
#include "utils/stat_utils.h"
#include "utils/features.h"
#include "styles/ThemeParameters.h"
#include "fonts.h"
#include "core_dispatcher.h"

namespace
{
    QSize shareButtonSize()
    {
        return QSize(32, 32);
    }

    QSize buttonIconSize()
    {
        return QSize(32, 32);
    }

    int textIconSpacing()
    {
        return Utils::scale_value(4);
    }

    bool isShareButtonEnabled(const Ui::ReadonlyPanelState _state)
    {
        return _state != Ui::ReadonlyPanelState::DeleteAndLeave && _state != Ui::ReadonlyPanelState::Unblock;
    }
}

namespace Ui
{
    PanelButton::PanelButton(QWidget* _parent)
        : ClickableWidget(_parent)
    {
        connect(this, &PanelButton::hoverChanged, this, Utils::QOverload<>::of(&PanelButton::update));
        connect(this, &PanelButton::pressChanged, this, Utils::QOverload<>::of(&PanelButton::update));

        setFocusPolicy(Qt::TabFocus);
    }

    void PanelButton::setColors(const QColor& _bgNormal, const QColor& _bgHover, const QColor& _bgActive)
    {
        bgNormal_ = _bgNormal;
        bgHover_ = _bgHover;
        bgActive_ = _bgActive;
        update();
    }

    void PanelButton::setTextColor(const QColor& _color)
    {
        textColor_ = _color;

        if (text_)
            text_->setColor(_color);

        update();
    }

    void PanelButton::setText(const QString& _text)
    {
        if (_text.isEmpty())
        {
            text_.reset();
        }
        else
        {
            text_ = TextRendering::MakeTextUnit(_text, {}, TextRendering::LinksVisible::DONT_SHOW_LINKS, TextRendering::ProcessLineFeeds::REMOVE_LINE_FEEDS);
            text_->init(Fonts::appFontScaled(12, Fonts::FontWeight::Medium), textColor_);
            text_->evaluateDesiredSize();
        }

        update();
    }

    void PanelButton::setIcon(const QString& _iconPath)
    {
        icon_ = Utils::renderSvgScaled(_iconPath, buttonIconSize(), textColor_);
        update();
    }

    void PanelButton::paintEvent(QPaintEvent*)
    {
        QPainter p(this);
        p.setRenderHints(QPainter::Antialiasing | QPainter::SmoothPixmapTransform);

        if (const auto bg = getBgColor(); bg.isValid())
            drawInputBubble(p, rect(), bg, getVerMargin(), getVerMargin());

        const auto hasIcon = !icon_.isNull();
        const auto hasText = !!text_;

        if (!hasIcon && !hasText)
            return;

        const auto r = Utils::scale_bitmap_ratio();
        const auto iconWidth = hasIcon ? icon_.width() / r : 0;
        const auto textWidth = hasText ? text_->desiredWidth() : 0;
        const auto fullWidth = iconWidth + textWidth + ((hasText && hasIcon) ? textIconSpacing() : 0);

        if (hasIcon)
        {
            const auto x = (width() - fullWidth) / 2;
            const auto y = (height() - icon_.height() / r) / 2;
            p.drawPixmap(x, y, icon_);
        }

        if (hasText)
        {
            const auto x = (width() - fullWidth) / 2 + (fullWidth - textWidth);
            const auto y = height() / 2;
            text_->setOffsets(x, y);

            text_->draw(p, TextRendering::VerPosition::MIDDLE);
        }
    }

    void PanelButton::resizeEvent(QResizeEvent* _event)
    {
        bubblePath_ = QPainterPath();
        update();
    }

    void PanelButton::focusInEvent(QFocusEvent* _event)
    {
        update();
    }

    void PanelButton::focusOutEvent(QFocusEvent* _event)
    {
        update();
    }

    QColor PanelButton::getBgColor() const
    {
        QColor bg;
        if ((isPressed() || hasFocus()) && bgActive_.isValid())
            bg = bgActive_;
        else if (isHovered() && bgHover_.isValid())
            bg = bgHover_;
        else
            bg = bgNormal_;

        return bg;
    }

    InputPanelReadonly::InputPanelReadonly(QWidget* _parent)
        : QWidget(_parent)
        , mainButton_(new PanelButton(this))
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

        Testing::setAccessibleName(shareButton_, qsl("AS inputwidget shareButton_"));
        Testing::setAccessibleName(mainButton_, qsl("AS inputwidget mainButton_"));
        Testing::setAccessibleName(shareHost, qsl("AS inputwidget shareHost"));

        connect(mainButton_, &PanelButton::clicked, this, &InputPanelReadonly::onButtonClicked);
        connect(shareButton_, &CustomButton::clicked, this, &InputPanelReadonly::onShareClicked);

        connect(Ui::GetDispatcher(), &Ui::core_dispatcher::chatInfo, this, [this](const auto, const auto& _chatInfo){ updateFromChatInfo(_chatInfo); });

        updateStyle(InputStyleMode::Default);
    }

    void InputPanelReadonly::setAimid(const QString& _aimId)
    {
        if (aimId_ != _aimId)
        {
            aimId_ = _aimId;

            if (const auto contact = Logic::getContactListModel()->getContactItem(aimId_))
            {
                stamp_ = contact->get_stamp();
                isChannel_ = contact->is_channel();
            }
            else
            {
                stamp_ = Logic::getContactListModel()->getChatStamp(aimId_);
                isChannel_ = false;
            }
        }
    }

    void InputPanelReadonly::setState(const ReadonlyPanelState _state)
    {
        state_ = _state;
        assert(state_ != ReadonlyPanelState::Invalid);

        shareButton_->setVisible(isShareButtonEnabled(state_) && !stamp_.isEmpty());

        if (state_ == ReadonlyPanelState::AddToContactList)
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

        mainButton_->setUpdatesEnabled(false);
        switch (state_)
        {
        case ReadonlyPanelState::AddToContactList:
            mainButton_->setTextColor(Styling::getParameters().getColor(Styling::StyleVariable::TEXT_SOLID_PERMANENT));
            mainButton_->setIcon(qsl(":/input/add"));
            mainButton_->setText(isChannel_ ? QT_TRANSLATE_NOOP("input_widget", "SUBSCRIBE") : QT_TRANSLATE_NOOP("input_widget", "JOIN"));
            break;
        case ReadonlyPanelState::EnableNotifications:
            mainButton_->setTextColor(Styling::getParameters().getColor(Styling::StyleVariable::PRIMARY));
            mainButton_->setIcon(qsl(":/input/unmute"));
            mainButton_->setText(QT_TRANSLATE_NOOP("input_widget", "ENABLE NOTIFICATIONS"));
            break;
        case ReadonlyPanelState::DisableNotifications:
            mainButton_->setTextColor(Styling::getParameters().getColor(Styling::StyleVariable::BASE_PRIMARY));
            mainButton_->setIcon(qsl(":/input/mute"));
            mainButton_->setText(QT_TRANSLATE_NOOP("input_widget", "DISABLE NOTIFICATIONS"));
            break;
        case ReadonlyPanelState::DeleteAndLeave:
            mainButton_->setTextColor(Styling::getParameters().getColor(Styling::StyleVariable::SECONDARY_ATTENTION));
            mainButton_->setIcon(qsl(":/input/delete"));
            mainButton_->setText(QT_TRANSLATE_NOOP("input_widget", "DELETE AND LEAVE"));
            break;
        case ReadonlyPanelState::Unblock:
            mainButton_->setTextColor(Styling::getParameters().getColor(Styling::StyleVariable::SECONDARY_ATTENTION));
            mainButton_->setIcon(qsl(":/input/unlock"));
            mainButton_->setText(QT_TRANSLATE_NOOP("input_widget", "UNBLOCK"));
            break;

        default:
            assert(!"invalid state");
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

        case ReadonlyPanelState::Unblock:
            unblockClicked();
            break;

        default:
            assert(!"invalid state");
            break;
        }
    }

    void InputPanelReadonly::onShareClicked()
    {
        assert(!stamp_.isEmpty());
        if (!stamp_.isEmpty())
        {
            const QString link = qsl("https://") % Utils::getDomainUrl() % ql1c('/') % stamp_;
            if (const auto count = forwardMessage(link, QString(), QString(), false))
                Ui::GetDispatcher()->post_stats_to_core(core::stats::stats_event_names::sharingscr_choicecontact_action, { { "count", Utils::averageCount(count) } });
        }
    }

    void InputPanelReadonly::joinClicked()
    {
        assert(!stamp_.isEmpty());
        if (!stamp_.isEmpty())
        {
            Logic::getContactListModel()->joinLiveChat(stamp_, true);
            Ui::GetDispatcher()->post_stats_to_core(core::stats::stats_event_names::chatscr_join_action, { {"chat_type", getStatsChatTypeReadonly() } });
        }
    }

    void InputPanelReadonly::notifClicked()
    {
        assert(!aimId_.isEmpty());

        const auto actionMute = state_ == ReadonlyPanelState::DisableNotifications;
        Logic::getRecentsModel()->muteChat(aimId_, actionMute);

        const auto eventName = actionMute ? core::stats::stats_event_names::chatscr_mute_action : core::stats::stats_event_names::chatscr_unmute_action;
        GetDispatcher()->post_stats_to_core(eventName, { {"chat_type", getStatsChatTypeReadonly() } });
    }

    void InputPanelReadonly::unblockClicked()
    {
        const auto confirmed = Utils::GetConfirmationWithTwoButtons(
            QT_TRANSLATE_NOOP("popup_window", "CANCEL"),
            QT_TRANSLATE_NOOP("popup_window", "YES"),
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
            QT_TRANSLATE_NOOP("popup_window", "CANCEL"),
            QT_TRANSLATE_NOOP("popup_window", "YES"),
            QT_TRANSLATE_NOOP("popup_window", "Are you sure you want to leave chat?"),
            Logic::GetFriendlyContainer()->getFriendly(aimId_),
            nullptr);

        if (confirmed)
        {
            Logic::getContactListModel()->removeContactFromCL(aimId_);
            GetDispatcher()->getVoipController().setDecline(aimId_.toUtf8().constData(), false);

            Ui::GetDispatcher()->post_stats_to_core(core::stats::stats_event_names::chatscr_leave_action, { {"chat_type", getStatsChatTypeReadonly() } });
        }
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
        isChannel_ = _chatInfo->DefaultRole_ == qsl("readonly");
        setState(currentState());
    }

    void InputPanelReadonly::setFocusOnButton()
    {
        mainButton_->setFocus(Qt::TabFocusReason);
    }
}