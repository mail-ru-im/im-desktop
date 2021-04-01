#include "stdafx.h"

#include "../../utils/gui_coll_helper.h"
#include "../../utils/utils.h"
#include "../../../corelib/enumerations.h"

#include "../../my_info.h"

#include "../../cache/emoji/Emoji.h"

#include "../contact_list/ContactListModel.h"

#include "ChatEventInfo.h"
#include "../containers/FriendlyContainer.h"
#include "../containers/PrivacySettingsContainer.h"
#include "statuses/LocalStatuses.h"

using namespace core;

namespace
{
    [[nodiscard]] QString cleanupFriendlyName(QString _name)
    {
        im_assert(!_name.isEmpty());

        _name.remove(ql1s("@uin.icq"), Qt::CaseInsensitive);
        return _name;
    }

    bool isMemberEvent(chat_event_type _type)
    {
        static const auto memberTypes =
        {
            chat_event_type::mchat_add_members,
            chat_event_type::mchat_adm_granted,
            chat_event_type::mchat_adm_revoked,
            chat_event_type::mchat_del_members,
            chat_event_type::mchat_invite,
            chat_event_type::mchat_kicked,
            chat_event_type::mchat_leave,
            chat_event_type::mchat_allowed_to_write,
            chat_event_type::mchat_disallowed_to_write,
            chat_event_type::mchat_waiting_for_approve,
            chat_event_type::mchat_joining_approved,
            chat_event_type::mchat_joining_rejected,
            chat_event_type::mchat_joining_canceled,
        };

        return std::any_of(memberTypes.begin(), memberTypes.end(), [_type](const auto& type) { return type == _type; });
    }

    constexpr int bulletCode() noexcept
    {
        return 0x2023;
    }
}

namespace HistoryControl
{

    ChatEventInfoSptr ChatEventInfo::make(const core::coll_helper& _info, const bool _isOutgoing, const QString& _myAimid, const QString& _aimid)
    {
        im_assert(!_myAimid.isEmpty());

        const auto type = _info.get_value_as_enum<chat_event_type>("type");
        const auto isCaptchaPresent = _info.get_value_as_bool("is_captcha_present", false);
        const auto isChannel = _info.get_value_as_bool("is_channel", false);

        ChatEventInfoSptr eventInfo(new ChatEventInfo(
            type, isCaptchaPresent, _isOutgoing, _myAimid, _aimid, isChannel
        ));

        const auto isGeneric = (type == chat_event_type::generic);
        if (isGeneric)
        {
            eventInfo->setGenericText(
                _info.get<QString>("generic")
            );

            return eventInfo;
        }

        const auto isBuddyReg = (type == chat_event_type::buddy_reg);
        const auto isBuddyFound = (type == chat_event_type::buddy_found);
        const auto isBirthday = (type == chat_event_type::birthday);
        const auto isMessageDeleted = (type == chat_event_type::message_deleted);
        const auto isStranger = (type == chat_event_type::warn_about_stranger || type == chat_event_type::no_longer_stranger);
        if (isBuddyReg || isBuddyFound || isBirthday || isMessageDeleted || isStranger)
        {
            if (isStranger)
                eventInfo->setSender(_aimid);

            return eventInfo;
        }

        eventInfo->setSender(_info.get<QString>("sender"));

        const auto isAddedToBuddyList = (type == chat_event_type::added_to_buddy_list);
        const auto isAvatarModified = (type == chat_event_type::avatar_modified);
        if (isAddedToBuddyList || isAvatarModified)
        {
            return eventInfo;
        }

        const auto isChatNameModified = (type == chat_event_type::chat_name_modified);
        if (isChatNameModified)
        {
            const auto newChatName = _info.get<QString>("chat/new_name");
            im_assert(!newChatName.isEmpty());

            eventInfo->setNewName(newChatName);

            return eventInfo;
        }

        const auto isChatDescriptionModified = (type == chat_event_type::chat_description_modified);
        if (isChatDescriptionModified)
        {
            eventInfo->setNewDescription(_info.get<QString>("chat/new_description"));
            return eventInfo;
        }

        const auto isChatRulesModified = (type == chat_event_type::chat_rules_modified);
        if (isChatRulesModified)
        {
            eventInfo->setNewChatRules(_info.get<QString>("chat/new_rules"));
            return eventInfo;
        }

        const auto isChatStampModified = (type == chat_event_type::chat_stamp_modified);
        if (isChatStampModified)
        {
            eventInfo->setNewChatStamp(_info.get<QString>("chat/new_stamp"));
            return eventInfo;
        }

        const auto isJoinModerationModified = (type == chat_event_type::chat_join_moderation_modified);
        if (isJoinModerationModified)
        {
            eventInfo->setNewJoinModeration(_info.get<bool>("chat/new_join_moderation"));
            return eventInfo;
        }

        const auto isPublicModified = (type == chat_event_type::chat_public_modified);
        if (isPublicModified)
        {
            eventInfo->setNewPublic(_info.get<bool>("chat/new_public"));
            return eventInfo;
        }

        if (isMemberEvent(type))
        {
            if (_info.is_value_exist("mchat/members_aimids"))
            {
                const auto membersAimids = _info.get_value_as_array("mchat/members_aimids");
                eventInfo->setMchatMembersAimIds(*membersAimids);
            }

            eventInfo->setMchatRequestedBy(_info.get<QString>("mchat/requested_by"));

            return eventInfo;
        }

        if (type == chat_event_type::status_reply || type == chat_event_type::custom_status_reply)
        {
            eventInfo->setSenderStatus(_info.get<QString>("stats_reply/sender_status"), _info.get<QString>("stats_reply/sender_status_description"));
            eventInfo->setOwnerStatus(_info.get<QString>("stats_reply/owner_status"), _info.get<QString>("stats_reply/owner_status_description"));

            return eventInfo;
        }

        im_assert(!"unexpected event type");
        return eventInfo;
    }

    ChatEventInfo::ChatEventInfo(const chat_event_type _type, const bool _isCaptchaPresent, const bool _isOutgoing, const QString& _myAimid, const QString& _aimid, const bool _isChannel)
        : type_(_type)
        , isCaptchaPresent_(_isCaptchaPresent)
        , isOutgoing_(_isOutgoing)
        , isChannel_(_isChannel)
        , myAimid_(_myAimid)
        , aimId_(_aimid)
    {
        im_assert(type_ > chat_event_type::min);
        im_assert(type_ < chat_event_type::max);
        im_assert(!myAimid_.isEmpty());
    }

    QString ChatEventInfo::formatEventTextInternal() const
    {
        switch (type_)
        {
            case chat_event_type::added_to_buddy_list:
                return formatAddedToBuddyListText();

            case chat_event_type::avatar_modified:
                return formatAvatarModifiedText();

            case chat_event_type::birthday:
                return formatBirthdayText();

            case chat_event_type::buddy_reg:
                return formatBuddyReg();

            case chat_event_type::buddy_found:
                return formatBuddyFound();

            case chat_event_type::chat_name_modified:
                return formatChatNameModifiedText();

            case chat_event_type::generic:
                return formatGenericText();

            case chat_event_type::mchat_add_members:
                return formatMchatAddMembersText();

            case chat_event_type::mchat_adm_granted:
                return formatMchatAdmGrantedText();

            case chat_event_type::mchat_adm_revoked:
                return formatMchatAdmRevokedText();

            case chat_event_type::mchat_allowed_to_write:
                return formatMchatAllowedToWrite();

            case chat_event_type::mchat_disallowed_to_write:
                return formatMchatDisallowedToWrite();

            case chat_event_type::mchat_invite:
                return formatMchatInviteText();

            case chat_event_type::mchat_leave:
                return formatMchatLeaveText();

            case chat_event_type::mchat_del_members:
                return formatMchatDelMembersText();

            case chat_event_type::mchat_waiting_for_approve:
                return formatMchatWaitingForApprove();

            case chat_event_type::mchat_joining_approved:
                return formatMchatJoiningApproved();

            case chat_event_type::mchat_joining_rejected:
                return formatMchatJoiningRejected();

            case chat_event_type::mchat_joining_canceled:
                return formatMchatJoiningCanceled();

            case chat_event_type::chat_description_modified:
                return formatChatDescriptionModified();

            case chat_event_type::mchat_kicked:
                return formatMchatKickedText();

            case chat_event_type::message_deleted:
                return formatMessageDeletedText();

            case chat_event_type::chat_rules_modified:
                return formatChatRulesModified();

            case chat_event_type::chat_stamp_modified:
                return formatChatStampModified();

            case chat_event_type::chat_join_moderation_modified:
                return formatJoinModerationModified();

            case chat_event_type::chat_public_modified:
                return formatPublicModified();

            case chat_event_type::warn_about_stranger:
                return formatWarnAboutStranger();

            case chat_event_type::no_longer_stranger:
                return formatNoLongerStranger();

            case chat_event_type::status_reply:
            case chat_event_type::custom_status_reply:
                return formatStatusReply();

            default:
                break;
        }

        im_assert(!"unexpected chat event type");
        return QString();
    }

    QString ChatEventInfo::formatAddedToBuddyListText() const
    {
        im_assert(type_ == chat_event_type::added_to_buddy_list);

        if (isOutgoing_)
            return QT_TRANSLATE_NOOP("chat_event", "You added %1 to contacts").arg(senderFriendly());
        return QT_TRANSLATE_NOOP("chat_event", "%1 added you to contacts").arg(senderFriendly());
    }

    QString ChatEventInfo::formatAvatarModifiedText() const
    {
        im_assert(type_ == chat_event_type::avatar_modified);

        if (isChannel())
        {
            if (isOutgoing())
                return QT_TRANSLATE_NOOP("chat_event", "You changed the channel avatar");

            return QT_TRANSLATE_NOOP("chat_event", "%1 changed the channel avatar").arg(senderOrAdmin());
        }

        if (isOutgoing())
            return QT_TRANSLATE_NOOP("chat_event", "You changed the group avatar");

        return QT_TRANSLATE_NOOP("chat_event", "%1 changed the group avatar").arg(senderOrAdmin());
    }

    QString ChatEventInfo::formatBirthdayText() const
    {
        im_assert(type_ == chat_event_type::birthday);

        constexpr auto birthdayEmojiId = 0x1f381;
        return Emoji::EmojiCode::toQString(Emoji::EmojiCode(birthdayEmojiId)) + QT_TRANSLATE_NOOP("chat_event", " has birthday!");
    }

    QString ChatEventInfo::formatBuddyFound() const
    {
        im_assert(type_ == chat_event_type::buddy_found);

        constexpr auto smileEmojiId = 0x1f642;
        const auto smileEmojiString = Emoji::EmojiCode::toQString(Emoji::EmojiCode(smileEmojiId));
        const QString friendly = Logic::GetFriendlyContainer()->getFriendly(aimId_);

        return QT_TRANSLATE_NOOP("chat_event", "You have recently added %1 to your phone contacts. Write a new message or make a call %2").arg(friendly, smileEmojiString);
    }

    QString ChatEventInfo::formatBuddyReg() const
    {
        im_assert(type_ == chat_event_type::buddy_reg);

        constexpr auto smileEmojiId = 0x1f642;
        const auto smileEmojiString = Emoji::EmojiCode::toQString(Emoji::EmojiCode(smileEmojiId));
        const QString friendly = Logic::GetFriendlyContainer()->getFriendly(aimId_);

        return QT_TRANSLATE_NOOP("chat_event", "So %1 is here now! Write, call %2").arg(friendly, smileEmojiString);
    }

    QString ChatEventInfo::formatChatNameModifiedText() const
    {
        im_assert(type_ == chat_event_type::chat_name_modified);
        im_assert(!chat_.newName_.isEmpty());

        if (isChannel())
        {
            if (isOutgoing())
                return QT_TRANSLATE_NOOP("chat_event", "You specified a new channel name - \"%1\"").arg(chat_.newName_);

            return QT_TRANSLATE_NOOP("chat_event", "%1 specified a new channel name - \"%2\"").arg(senderOrAdmin(), chat_.newName_);
        }

        if (isOutgoing())
            return QT_TRANSLATE_NOOP("chat_event", "You specified a new group name - \"%1\"").arg(chat_.newName_);

        return QT_TRANSLATE_NOOP("chat_event", "%1 specified a new group name - \"%2\"").arg(senderOrAdmin(), chat_.newName_);
    }

    QString ChatEventInfo::formatGenericText() const
    {
        im_assert(type_ == chat_event_type::generic);
        im_assert(!generic_.isEmpty());

        return generic_;
    }

    QString ChatEventInfo::formatMchatAddMembersText() const
    {
        im_assert(type_ == chat_event_type::mchat_add_members || type_ == chat_event_type::mchat_invite);

        const auto doorEmoji = Emoji::EmojiCode::toQString(Emoji::EmojiCode(0x1f6aa));
        const auto wavingHandEmoji = Emoji::EmojiCode::toQString(Emoji::EmojiCode(0x1f44b));

        const auto isJoin = mchat_.members_.size() == 1 && sender_ == mchat_.members_.constFirst();
        if (isJoin)
        {
            if (isChannel())
            {
                if (isOutgoing())
                    return QT_TRANSLATE_NOOP("chat_event", "%1 You have subscribed the channel").arg(doorEmoji);

                return QT_TRANSLATE_NOOP("chat_event", "%1 %2 has subscribed the channel").arg(doorEmoji, senderFriendly());
            }

            if (isOutgoing())
                return QT_TRANSLATE_NOOP("chat_event", "%1 You have joined group").arg(doorEmoji);
            else
                return QT_TRANSLATE_NOOP("chat_event", "%1 %2 has joined group").arg(doorEmoji, senderFriendly());
        }

        if (isChannel())
        {
            if (isOutgoing())
                return QT_TRANSLATE_NOOP("chat_event", "%1 You added %2 to the channel").arg(wavingHandEmoji, formatMchatMembersList());

            return QT_TRANSLATE_NOOP("chat_event", "%1 %2 added %3 to the channel").arg(wavingHandEmoji, senderOrAdmin(), formatMchatMembersList());
        }

        if (isOutgoing())
            return QT_TRANSLATE_NOOP("chat_event", "%1 You added %2 to the group").arg(wavingHandEmoji, formatMchatMembersList());

        return QT_TRANSLATE_NOOP("chat_event", "%1 %2 added %3 to the group").arg(wavingHandEmoji, senderOrAdmin(), formatMchatMembersList());
    }

    QString ChatEventInfo::formatChatDescriptionModified() const
    {
        im_assert(type_ == chat_event_type::chat_description_modified);

        if (chat_.newDescription_.isEmpty())
        {
            if (isChannel())
            {
                if (isOutgoing())
                    return QT_TRANSLATE_NOOP("chat_event", "You deleted the channel description");

                return QT_TRANSLATE_NOOP("chat_event", "%1 deleted the channel description").arg(senderOrAdmin());
            }

            if (isOutgoing())
                return QT_TRANSLATE_NOOP("chat_event", "You deleted the group description");

            return QT_TRANSLATE_NOOP("chat_event", "%1 deleted the group description").arg(senderOrAdmin());
        }
        else
        {
            if (isChannel())
            {
                if (isOutgoing())
                    return QT_TRANSLATE_NOOP("chat_event", "You changed the channel description - \"%1\"").arg(chat_.newDescription_);

                return QT_TRANSLATE_NOOP("chat_event", "%1 changed the channel description - \"%2\"").arg(senderOrAdmin(), chat_.newDescription_);
            }

            if (isOutgoing())
                return QT_TRANSLATE_NOOP("chat_event", "You changed the group description - \"%1\"").arg(chat_.newDescription_);

            return QT_TRANSLATE_NOOP("chat_event", "%1 changed the group description - \"%2\"").arg(senderOrAdmin(), chat_.newDescription_);
        }
    }

    QString ChatEventInfo::formatChatRulesModified() const
    {
        im_assert(type_ == chat_event_type::chat_rules_modified);

        if (chat_.newRules_.isEmpty())
        {
            if (isChannel())
            {
                if (isOutgoing())
                    return QT_TRANSLATE_NOOP("chat_event", "You deleted the channel rules");

                return QT_TRANSLATE_NOOP("chat_event", "%1 deleted the channel rules").arg(senderOrAdmin());
            }

            if (isOutgoing())
                return QT_TRANSLATE_NOOP("chat_event", "You deleted the group rules");

            return QT_TRANSLATE_NOOP("chat_event", "%1 deleted the group rules").arg(senderOrAdmin());
        }
        else
        {
            if (isChannel())
            {
                if (isOutgoing())
                    return QT_TRANSLATE_NOOP("chat_event", "You changed the channel rules - \"%1\"").arg(chat_.newRules_);

                return QT_TRANSLATE_NOOP("chat_event", "%1 changed the channel rules - \"%2\"").arg(senderOrAdmin(), chat_.newRules_);
            }

            if (isOutgoing())
                return QT_TRANSLATE_NOOP("chat_event", "You changed the group rules - \"%1\"").arg(chat_.newRules_);

            return QT_TRANSLATE_NOOP("chat_event", "%1 changed the group rules - \"%2\"").arg(senderOrAdmin(), chat_.newRules_);
        }
    }

    QString ChatEventInfo::formatChatStampModified() const
    {
        im_assert(type_ == chat_event_type::chat_stamp_modified);

        if (isOutgoing())
            return QT_TRANSLATE_NOOP("chat_event", "You changed the link to %1").arg(Utils::getDomainUrl() % ql1c('/') % chat_.newStamp_);

        return QT_TRANSLATE_NOOP("chat_event", "%1 changed the link to %2").arg(senderOrAdmin(), Utils::getDomainUrl() % ql1c('/') % chat_.newStamp_);
    }

    QString ChatEventInfo::formatJoinModerationModified() const
    {
        im_assert(type_ == chat_event_type::chat_join_moderation_modified);

        if (isOutgoing())
        {
            if (chat_.newJoinModeration_)
                return QT_TRANSLATE_NOOP("chat_event", "You enabled join with approval");
            else
                return QT_TRANSLATE_NOOP("chat_event", "You disabled join with approval");
        }

        if (chat_.newJoinModeration_)
            return QT_TRANSLATE_NOOP("chat_event", "%1 enabled join with approval").arg(senderOrAdmin());
        else
            return QT_TRANSLATE_NOOP("chat_event", "%1 disabled join with approval").arg(senderOrAdmin());
    }

    QString ChatEventInfo::formatPublicModified() const
    {
        im_assert(type_ == chat_event_type::chat_public_modified);

        if (isOutgoing())
        {
            if (chat_.newPublic_)
                return QT_TRANSLATE_NOOP("chat_event", "You made the group public, now it is possible to find it through a search");
            else
                return QT_TRANSLATE_NOOP("chat_event", "You made the group private, now it is impossible to find it through a search");
        }

        if (chat_.newPublic_)
            return QT_TRANSLATE_NOOP("chat_event", "%1 made the group public, now it is possible to find it through a search").arg(senderOrAdmin());
        else
            return QT_TRANSLATE_NOOP("chat_event", "%1 made the group private, now it is impossible to find it through a search").arg(senderOrAdmin());
    }

    QString ChatEventInfo::formatMchatInviteText() const
    {
        im_assert(type_ == chat_event_type::mchat_invite);

        return formatMchatAddMembersText();
    }

    QString ChatEventInfo::formatMchatDelMembersText() const
    {
        im_assert(type_ == chat_event_type::mchat_del_members);

        const auto crossMarkEmoji = Emoji::EmojiCode::toQString(Emoji::EmojiCode(0x274c));

        if (isForMe())
            return QT_TRANSLATE_NOOP("chat_event", "%1 %2 removed you").arg(crossMarkEmoji, senderOrAdmin());

        if (isOutgoing())
            return QT_TRANSLATE_NOOP("chat_event", "%1 You removed %2").arg(crossMarkEmoji, formatMchatMembersList());

        if (isChannel_ || Logic::getContactListModel()->isChannel(aimId_))
        {
            if (mchat_.members_.size() == 1)
                return QT_TRANSLATE_NOOP("chat_event", "%1 was removed").arg(formatMchatMembersList());

            return QT_TRANSLATE_NOOP("chat_event", "%1 were removed").arg(formatMchatMembersList());
        }

        return QT_TRANSLATE_NOOP("chat_event", "%1 %2 removed %3").arg(crossMarkEmoji, senderOrAdmin(), formatMchatMembersList());
    }

    QString ChatEventInfo::formatMchatAdmGrantedText() const
    {
        im_assert(type_ == chat_event_type::mchat_adm_granted);

        if (isForMe())
        {
            if (isChannel())
                return QT_TRANSLATE_NOOP("chat_event", "You were assigned as an administrator of this channel");

            return QT_TRANSLATE_NOOP("chat_event", "You were assigned as an administrator of this group");
        }

        if (isOutgoing())
        {
            if (isChannel())
                return QT_TRANSLATE_NOOP("chat_event", "You assigned %1 administrator of this channel").arg(formatMchatMembersList());

            return QT_TRANSLATE_NOOP("chat_event", "You assigned %1 administrator of this group").arg(formatMchatMembersList());
        }

        if (isChannel())
            return QT_TRANSLATE_NOOP("chat_event", "%1 assigned %2 administrator of this channel").arg(senderOrAdmin(), formatMchatMembersList());

        return QT_TRANSLATE_NOOP("chat_event", "%1 assigned %2 administrator of this group").arg(senderOrAdmin(), formatMchatMembersList());
    }

    QString ChatEventInfo::formatMchatAdmRevokedText() const
    {
        im_assert(type_ == chat_event_type::mchat_adm_revoked);

        if (isForMe())
            return QT_TRANSLATE_NOOP("chat_event", "You are no more an administrator of this group");

        if (isOutgoing())
            return QT_TRANSLATE_NOOP("chat_event", "You removed administrator role from %1").arg(formatMchatMembersList());

        return QT_TRANSLATE_NOOP("chat_event", "%1 removed administrator role from %2").arg(senderOrAdmin(), formatMchatMembersList());
    }

    QString ChatEventInfo::formatMchatAllowedToWrite() const
    {
        im_assert(type_ == chat_event_type::mchat_allowed_to_write);

        if (isForMe())
            return QT_TRANSLATE_NOOP("chat_event", "%1 allowed you to write").arg(senderOrAdmin());

        if (isOutgoing())
            return QT_TRANSLATE_NOOP("chat_event", "You allowed %1 to write").arg(formatMchatMembersList());

        return QT_TRANSLATE_NOOP("chat_event", "%1 allowed %2 to write").arg(senderOrAdmin(), formatMchatMembersList());
    }

    QString ChatEventInfo::formatMchatDisallowedToWrite() const
    {
        im_assert(type_ == chat_event_type::mchat_disallowed_to_write);

        if (isForMe())
            return QT_TRANSLATE_NOOP("chat_event", "%1 banned you to write").arg(senderOrAdmin());

        if (isOutgoing())
            return QT_TRANSLATE_NOOP("chat_event", "You banned %1 to write").arg(formatMchatMembersList());

        return QT_TRANSLATE_NOOP("chat_event", "%1 banned %2 to write").arg(senderOrAdmin(), formatMchatMembersList());
    }

    QString ChatEventInfo::formatMchatWaitingForApprove() const
    {
        im_assert(type_ == chat_event_type::mchat_waiting_for_approve);

        const auto alarmClockEmoji = Emoji::EmojiCode::toQString(Emoji::EmojiCode(0x23f0));
        const auto foldedHandsEmoji = Emoji::EmojiCode::toQString(Emoji::EmojiCode(0x1f64f));

        const auto member = !mchat_.members_.isEmpty() ? mchat_.members_.front() : QString();

        if (isOutgoing())
        {
            if (isMyAimid(member))
                return QT_TRANSLATE_NOOP("chat_event", "%1 Wait for join request approval").arg(alarmClockEmoji);
            else if (isChannel())
                return QT_TRANSLATE_NOOP("chat_event", "%1 You asked to add %2 to the channel").arg(foldedHandsEmoji, formatMchatMembersList());
            else
                return QT_TRANSLATE_NOOP("chat_event", "%1 You asked to add %2 to the group").arg(foldedHandsEmoji, formatMchatMembersList());
        }

        if (sender_ != member)
        {
            if (isChannel())
                return QT_TRANSLATE_NOOP("chat_event", "%1 %2 asked to add %3 to the channel "
                                                       "(this message is visible only to administrators)").arg(foldedHandsEmoji, senderOrAdmin(), formatMchatMembersList());

            return QT_TRANSLATE_NOOP("chat_event", "%1 %2 asked to add %3 to the group").arg(foldedHandsEmoji, senderOrAdmin(), formatMchatMembersList());
        }

        if (isChannel())
            return QT_TRANSLATE_NOOP("chat_event", "%1 %2 is waiting for channel join request approval "
                                                   "(this message is visible only to administrators)").arg(alarmClockEmoji, formatMchatMembersList());

        return QT_TRANSLATE_NOOP("chat_event", "%1 %2 is waiting for group join request approval").arg(alarmClockEmoji, formatMchatMembersList());
    }

    QString ChatEventInfo::formatMchatJoiningApproved() const
    {
        im_assert(type_ == chat_event_type::mchat_joining_approved);

        const auto checkEmoji = Emoji::EmojiCode::toQString(Emoji::EmojiCode(0x2705));

        QString requestedByFriendly;
        if (!mchat_.requestedBy_.isEmpty())
            requestedByFriendly = Logic::GetFriendlyContainer()->getFriendly(mchat_.requestedBy_);

        const auto memberIsMe = isForMe();
        const auto requestedByMe = isMyAimid(mchat_.requestedBy_);

        if (mchat_.requestedBy_.isEmpty())
        {
            if (memberIsMe)
                return QT_TRANSLATE_NOOP("chat_event", "%1 Your join request was approved").arg(checkEmoji);

            if (isOutgoing())
                return QT_TRANSLATE_NOOP("chat_event", "%1 You approved join request from %2").arg(checkEmoji, formatMchatMembersList());

            if (isChannel())
                return QT_TRANSLATE_NOOP("chat_event", "%1 %2 approved join request from %3"
                                         "(this message is visible only to administrators)").arg(checkEmoji, senderOrAdmin(), formatMchatMembersList());

            return QT_TRANSLATE_NOOP("chat_event", "%1 %2 approved join request from %3").arg(checkEmoji, senderOrAdmin(), formatMchatMembersList());
        }

        if (isOutgoing())
        {
            if (isChannel())
                return QT_TRANSLATE_NOOP("chat_event", "%1 You approved join request from %2 to add %3 to the channel").arg(checkEmoji, requestedByFriendly, formatMchatMembersList());

            return QT_TRANSLATE_NOOP("chat_event", "%1 You approved join request from %2 to add %3 to the group").arg(checkEmoji, requestedByFriendly, formatMchatMembersList());
        }

        if (requestedByMe)
        {
            if (isChannel())
                return QT_TRANSLATE_NOOP("chat_event", "%1 Your request to add %2 to the channel was approved").arg(checkEmoji, formatMchatMembersList());

            return QT_TRANSLATE_NOOP("chat_event", "%1 Your request to add %2 to the group was approved").arg(checkEmoji, formatMchatMembersList());
        }

        if (memberIsMe)
        {
            if (isChannel())
                return QT_TRANSLATE_NOOP("chat_event", "%1 Request from %2 to add you to the channel was approved").arg(checkEmoji, requestedByFriendly);

            return QT_TRANSLATE_NOOP("chat_event", "%1 Request from %2 to add you to the group was approved").arg(checkEmoji, requestedByFriendly);
        }

        if (isChannel())
            return QT_TRANSLATE_NOOP("chat_event", "%1 %2 approved join request from %3 to add %4 to the channel "
                                                   "(this message is visible only to administrators)").arg(checkEmoji, senderOrAdmin(), requestedByFriendly, formatMchatMembersList());

        return QT_TRANSLATE_NOOP("chat_event", "%1 %2 approved join request from %3 to add %4 to the group").arg(checkEmoji, senderOrAdmin(), requestedByFriendly, formatMchatMembersList());
    }

    QString ChatEventInfo::formatMchatJoiningRejected() const
    {
        im_assert(type_ == chat_event_type::mchat_joining_rejected);

        const auto crossMarkEmoji = Emoji::EmojiCode::toQString(Emoji::EmojiCode(0x274c));

        QString requestedByFriendly;
        if (!mchat_.requestedBy_.isEmpty())
            requestedByFriendly = Logic::GetFriendlyContainer()->getFriendly(mchat_.requestedBy_);

        const auto memberIsMe = isForMe();
        const auto requestedByMe = isMyAimid(mchat_.requestedBy_);

        if (mchat_.requestedBy_.isEmpty())
        {
            if (memberIsMe)
                return QT_TRANSLATE_NOOP("chat_event", "%1 Your join request was rejected").arg(crossMarkEmoji);

            if (isOutgoing())
                return QT_TRANSLATE_NOOP("chat_event", "%1 You rejected join request from %2").arg(crossMarkEmoji, formatMchatMembersList());

            if (isChannel())
                return QT_TRANSLATE_NOOP("chat_event", "%1 %2 rejected join request from %3"
                                         "(this message is visible only to administrators)").arg(crossMarkEmoji, senderOrAdmin(), formatMchatMembersList());

            return QT_TRANSLATE_NOOP("chat_event", "%1 %2 rejected join request from %3").arg(crossMarkEmoji, senderOrAdmin(), formatMchatMembersList());
        }

        if (isOutgoing())
        {
            if (isChannel())
                return QT_TRANSLATE_NOOP("chat_event", "%1 You rejected join request from %2 to add %3 to the channel").arg(crossMarkEmoji, requestedByFriendly, formatMchatMembersList());

            return QT_TRANSLATE_NOOP("chat_event", "%1 You rejected join request from %2 to add %3 to the group").arg(crossMarkEmoji, requestedByFriendly, formatMchatMembersList());
        }

        if (requestedByMe)
        {
            if (isChannel())
                return QT_TRANSLATE_NOOP("chat_event", "%1 Your request to add %2 to the channel was rejected").arg(crossMarkEmoji, formatMchatMembersList());

            return QT_TRANSLATE_NOOP("chat_event", "%1 Your request to add %2 to the group was rejected").arg(crossMarkEmoji, formatMchatMembersList());
        }

        if (memberIsMe)
        {
            if (isChannel())
                return QT_TRANSLATE_NOOP("chat_event", "%1 Request from %2 to add you to the channel was rejected").arg(crossMarkEmoji, requestedByFriendly);

            return QT_TRANSLATE_NOOP("chat_event", "%1 Request from %2 to add you to the group was rejected").arg(crossMarkEmoji, requestedByFriendly);
        }

        if (isChannel())
            return QT_TRANSLATE_NOOP("chat_event", "%1 %2 rejected join request from %3 to add %4 to the channel "
                                                   "(this message is visible only to administrators)").arg(crossMarkEmoji, senderOrAdmin(), requestedByFriendly, formatMchatMembersList());

        return QT_TRANSLATE_NOOP("chat_event", "%1 %2 rejected join request from %3 to add %4 to the group").arg(crossMarkEmoji, senderOrAdmin(), requestedByFriendly, formatMchatMembersList());
    }

    QString ChatEventInfo::formatMchatJoiningCanceled() const
    {
        im_assert(type_ == chat_event_type::mchat_joining_canceled);

        const auto crossMarkEmoji = Emoji::EmojiCode::toQString(Emoji::EmojiCode(0x274c));

        if (isOutgoing())
            return QT_TRANSLATE_NOOP("chat_event", "%1 You have canceled your join request").arg(crossMarkEmoji);

        QString name;
        if (!mchat_.members_.isEmpty())
            name = Logic::GetFriendlyContainer()->getFriendly(mchat_.members_.front());

        return QT_TRANSLATE_NOOP("chat_event", "%1 %2 has canceled join request").arg(crossMarkEmoji, name);
    }

    QString ChatEventInfo::formatMchatKickedText() const
    {
        im_assert(type_ == chat_event_type::mchat_kicked);

        const auto crossMarkEmoji = Emoji::EmojiCode::toQString(Emoji::EmojiCode(0x274c));

        if (isForMe())
            return QT_TRANSLATE_NOOP("chat_event", "%1 %2 removed you").arg(crossMarkEmoji, senderOrAdmin());

        if (isOutgoing())
            return QT_TRANSLATE_NOOP("chat_event", "%1 You removed %2").arg(crossMarkEmoji, formatMchatMembersList());

        if (isChannel_ || Logic::getContactListModel()->isChannel(aimId_))
        {
            if (mchat_.members_.size() == 1)
                return QT_TRANSLATE_NOOP("chat_event", "%1 was removed").arg(formatMchatMembersList());

            return QT_TRANSLATE_NOOP("chat_event", "%1 were removed").arg(formatMchatMembersList());
        }

        return QT_TRANSLATE_NOOP("chat_event", "%1 %2 removed %3").arg(crossMarkEmoji, senderOrAdmin(), formatMchatMembersList());
    }

    QString ChatEventInfo::formatMchatLeaveText() const
    {
        im_assert(type_ == chat_event_type::mchat_leave);

        const auto manWalkingEmoji = Emoji::EmojiCode::toQString(Emoji::EmojiCode(0x1f6b6, 0x200d, 0x2642, 0xfe0f));

        if (isOutgoing())
        {
            if (isChannel())
                return QT_TRANSLATE_NOOP("chat_event", "%1 You have left channel").arg(manWalkingEmoji);
            else
                return QT_TRANSLATE_NOOP("chat_event", "%1 You have left group").arg(manWalkingEmoji);
        }

        if (hasMultipleMembers())
            return QT_TRANSLATE_NOOP3("chat_event", "%1 %2 have left group (this message is visible only to administrators)", "many").arg(manWalkingEmoji, formatMchatMembersList());

        return QT_TRANSLATE_NOOP3("chat_event", "%1 %2 has left group (this message is visible only to administrators)", "one").arg(manWalkingEmoji, formatMchatMembersList());
    }

    QString ChatEventInfo::formatMchatMembersList() const
    {
        QString result;
        const auto& members = mchat_.members_;
        if (members.isEmpty())
            return result;

        result.reserve(512);

        for (const auto& m : members)
        {
            if (isMyAimid(m))
                result += QT_TRANSLATE_NOOP("chat_event", "you");
            else
                result += Logic::GetFriendlyContainer()->getFriendly(m);
            result += ql1s(", ");
        }
        result.chop(2);
        return result;
    }

    const QString& ChatEventInfo::formatEventText() const
    {
        if (formattedEventText_.isEmpty())
            formattedEventText_ = formatEventTextInternal();

        im_assert(!formattedEventText_.isEmpty());
        return formattedEventText_;
    }

    QString ChatEventInfo::formatRecentsEventText() const
    {
        if (type_ == chat_event_type::status_reply || type_ == chat_event_type::custom_status_reply)
            return formatStatusReplyRecents();

        return formatEventTextInternal();
    }

    core::chat_event_type ChatEventInfo::eventType() const
    {
        return type_;
    }

    QString ChatEventInfo::formatMessageDeletedText() const
    {
        return QT_TRANSLATE_NOOP("chat_event", "Message was deleted");
    }

    QString ChatEventInfo::formatWarnAboutStranger() const
    {
        const auto friendly = Logic::GetFriendlyContainer()->getFriendly(aimId_);
        const auto handEmoji = Emoji::EmojiCode::toQString(Emoji::EmojiCode(0xd83d, 0xdc4b));
        const auto callSettings = Logic::GetPrivacySettingsContainer()->getCachedValue(u"calls");
        const auto groupSettings = Logic::GetPrivacySettingsContainer()->getCachedValue(u"groups");
        const auto canCall = (callSettings == privacy_access_right::my_contacts);
        const auto canAddToGroup = (groupSettings == privacy_access_right::my_contacts);

        QString result;
        result += QT_TRANSLATE_NOOP("chat_event", "%1 Send the user a message or press \"OK\" button to allow %2 to");
        if (canCall || canAddToGroup)
            result += u':';

        if (canCall)
        {
            result += QChar::LineFeed;
            result += QT_TRANSLATE_NOOP("chat_event", " %3 call you");
        }
        if (canAddToGroup)
        {
            result += QChar::LineFeed;
            result += QT_TRANSLATE_NOOP("chat_event", " %3 add you to groups");
        }

        if (canCall || canAddToGroup)
        {
            result += QChar::LineFeed;
            result += ql1s(" %3");
        }

        result += QT_TRANSLATE_NOOP("chat_event", " see if you read his messages");

        if (canCall || canAddToGroup)
            result = result.arg(handEmoji, friendly, QChar(bulletCode()));
        else
            result = result.arg(handEmoji, friendly);

        return result;
    }

    QString ChatEventInfo::formatNoLongerStranger() const
    {
        const auto friendly = Logic::GetFriendlyContainer()->getFriendly(aimId_);
        const auto checkEmoji = Emoji::EmojiCode::toQString(Emoji::EmojiCode(0x2705));
        const auto callSettings = Logic::GetPrivacySettingsContainer()->getCachedValue(u"calls");
        const auto groupSettings = Logic::GetPrivacySettingsContainer()->getCachedValue(u"groups");
        const auto canCall = (callSettings == privacy_access_right::my_contacts);
        const auto canAddToGroup = (groupSettings == privacy_access_right::my_contacts);

        QString result = QT_TRANSLATE_NOOP("chat_event", "%1 From now on %2 is able to");
        if (canCall || canAddToGroup)
            result += u':';

        if (canCall)
        {
            result += QChar::LineFeed;
            result += QT_TRANSLATE_NOOP("chat_event", " %3 call you");
        }
        if (canAddToGroup)
        {
            result += QChar::LineFeed;
            result += QT_TRANSLATE_NOOP("chat_event", " %3 add you to groups");
        }

        if (canCall || canAddToGroup)
        {
            result += QChar::LineFeed;
            result += ql1s(" %3");
        }

        result += QT_TRANSLATE_NOOP("chat_event", " see if you read his messages");

        if (canCall || canAddToGroup)
            result = result.arg(checkEmoji, friendly, QChar(bulletCode()));
        else
            result = result.arg(checkEmoji, friendly);

        return result;
    }

    QString ChatEventInfo::formatStatusReply() const
    {
        QString ownerStatusDescription = statusReply_.ownerStatusDescription_;
        if (ownerStatusDescription.isEmpty())
            ownerStatusDescription = Statuses::LocalStatuses::instance()->getStatus(statusReply_.ownerStatus_).description_;

        auto senderStatusDescription = statusReply_.senderStatusDescription_;
        if (senderStatusDescription.isEmpty())
            senderStatusDescription = Statuses::LocalStatuses::instance()->getStatus(statusReply_.senderStatus_).description_;

        const auto pointingDownEmojiStr = Emoji::EmojiCode::toQString(Emoji::EmojiCode(0x1f447));

        if (isOutgoing())
        {
            return QT_TRANSLATE_NOOP("chat_event", "You replied to status\n"
                                                                     "%1 %2\n"
                                                                     "%3\n"
                                                                     "%4 %5").arg(statusReply_.ownerStatus_, ownerStatusDescription,
                                                                                  pointingDownEmojiStr,
                                                                                  statusReply_.senderStatus_, senderStatusDescription);
        }

        return QT_TRANSLATE_NOOP("chat_event", "%1 replied to status\n"
                                                                 "%2 %3\n"
                                                                 "%4\n"
                                                                 "%5 %6").arg(senderFriendly(),
                                                                              statusReply_.ownerStatus_, ownerStatusDescription,
                                                                              pointingDownEmojiStr,
                                                                              statusReply_.senderStatus_, senderStatusDescription);
    }

    QString ChatEventInfo::formatStatusReplyRecents() const
    {
        auto senderStatusDescription = statusReply_.senderStatusDescription_;
        if (senderStatusDescription.isEmpty())
            senderStatusDescription = Statuses::LocalStatuses::instance()->getStatus(statusReply_.senderStatus_).description_;

        if (senderStatusDescription.isEmpty())
            senderStatusDescription = Statuses::Status::emptyDescription();

        if (isOutgoing())
            return QT_TRANSLATE_NOOP("chat_event", "You replied to status: %1 %2").arg(statusReply_.senderStatus_, senderStatusDescription);

        return QT_TRANSLATE_NOOP("chat_event", "Replied to status: %1 %2").arg(statusReply_.senderStatus_, senderStatusDescription);
    }

    bool ChatEventInfo::isMyAimid(const QString& _aimId) const
    {
        im_assert(!myAimid_.isEmpty());

        return (myAimid_ == _aimId);
    }

    bool ChatEventInfo::hasMultipleMembers() const
    {
        im_assert(!mchat_.members_.isEmpty());

        return (mchat_.members_.size() > 1);
    }

    void ChatEventInfo::setGenericText(QString _text)
    {
        im_assert(generic_.isEmpty());
        im_assert(!_text.isEmpty());

        generic_ = std::move(_text);
    }

    void ChatEventInfo::setNewChatRules(const QString& _newChatRules)
    {
        im_assert(chat_.newRules_.isEmpty());

        chat_.newRules_ = _newChatRules;
    }

    void ChatEventInfo::setNewChatStamp(const QString& _newChatStamp)
    {
        im_assert(chat_.newStamp_.isEmpty());
        im_assert(!_newChatStamp.isEmpty());

        chat_.newStamp_ = _newChatStamp;
    }

    void ChatEventInfo::setNewDescription(const QString& _newDescription)
    {
        im_assert(chat_.newDescription_.isEmpty());

        chat_.newDescription_ = _newDescription;
    }

    void ChatEventInfo::setNewName(const QString& _newName)
    {
        im_assert(chat_.newName_.isEmpty());
        im_assert(!_newName.isEmpty());

        chat_.newName_ = _newName;
    }

    void ChatEventInfo::setNewJoinModeration(bool _newJoinModeration)
    {
        chat_.newJoinModeration_ = _newJoinModeration;
    }

    void ChatEventInfo::setNewPublic(bool _newPublic)
    {
        chat_.newPublic_ = _newPublic;
    }

    void ChatEventInfo::setSender(QString _aimid)
    {
        im_assert(sender_.isEmpty());

        if (!_aimid.isEmpty())
            sender_ = cleanupFriendlyName(std::move(_aimid));
        else
            sender_ = QString();

        if (!isMyAimid(sender_))
            mchat_.membersLinks_[senderFriendly()] = u"@[" % sender_ % u']';
    }

    QString ChatEventInfo::senderFriendly() const
    {
        return Logic::GetFriendlyContainer()->getFriendly(sender_);
    }

    bool ChatEventInfo::isChannel() const
    {
        return isChannel_ || Logic::getContactListModel()->isChannel(aimId_);
    }

    bool ChatEventInfo::isOutgoing() const
    {
        return isMyAimid(sender_);
    }

    bool ChatEventInfo::isCaptchaPresent() const
    {
        return isCaptchaPresent_;
    }

    bool ChatEventInfo::isForMe() const
    {
        return mchat_.members_.size() == 1 && isMyAimid(mchat_.members_.front());
    }

    const std::map<QString, QString>& ChatEventInfo::getMembersLinks() const
    {
        return mchat_.membersLinks_;
    }

    const QVector<QString>& ChatEventInfo::getMembers() const
    {
        return mchat_.members_;
    }

    void ChatEventInfo::setMchatMembersAimIds(const core::iarray& _membersAimids)
    {
        const auto size = _membersAimids.size();
        mchat_.members_.reserve(size);
        for (auto index = 0; index < size; ++index)
        {
            auto member = QString::fromUtf8(_membersAimids.get_at(index)->get_as_string());
            const auto friendly = Logic::GetFriendlyContainer()->getFriendly(member);
            mchat_.membersLinks_[friendly] = u"@[" % member % u']';
            mchat_.members_.push_back(std::move(member));
        }
    }

    void ChatEventInfo::setMchatRequestedBy(const QString& _requestedBy)
    {
        mchat_.requestedBy_ = _requestedBy;

        if (!_requestedBy.isEmpty())
            mchat_.membersLinks_[Logic::GetFriendlyContainer()->getFriendly(_requestedBy)] = u"@[" % _requestedBy % u']';
    }

    QString ChatEventInfo::senderOrAdmin() const
    {
        return sender_ == aimId_ ? QT_TRANSLATE_NOOP("chat_event", "Administrator") : senderFriendly();
    }

    void ChatEventInfo::setSenderStatus(const QString& _status, const QString& _description)
    {
        statusReply_.senderStatus_ = _status;
        statusReply_.senderStatusDescription_ = _description;
    }

    void ChatEventInfo::setOwnerStatus(const QString& _status, const QString& _description)
    {
        statusReply_.ownerStatus_ = _status;
        statusReply_.ownerStatusDescription_ = _description;
    }
}
