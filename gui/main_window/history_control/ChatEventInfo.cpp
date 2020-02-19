#include "stdafx.h"

#include "../../utils/gui_coll_helper.h"
#include "../../../corelib/enumerations.h"

#include "../../my_info.h"

#include "../../cache/emoji/Emoji.h"

#include "../contact_list/ContactListModel.h"

#include "ChatEventInfo.h"
#include "../friendly/FriendlyContainer.h"

using namespace core;

namespace
{
    [[nodiscard]] QString cleanupFriendlyName(QString _name)
    {
        assert(!_name.isEmpty());

        _name.remove(qsl("@uin.icq"), Qt::CaseInsensitive); // use ql1s since qt5.11
        return _name;
    }
}

namespace HistoryControl
{

    ChatEventInfoSptr ChatEventInfo::Make(const core::coll_helper& _info, const bool _isOutgoing, const QString& _myAimid, const QString& _aimid)
    {
        assert(!_myAimid.isEmpty());

        const auto type = _info.get_value_as_enum<chat_event_type>("type");
        const auto isCaptchaPresent = _info.get_value_as_bool("is_captcha_present", false);
        const auto isChannel = _info.get_value_as_bool("is_channel", false);

        ChatEventInfoSptr eventInfo(new ChatEventInfo(
            type, isCaptchaPresent, _isOutgoing, _myAimid, _aimid, isChannel
        ));

        const auto isGeneric = (type == chat_event_type::generic);
        if (isGeneric)
        {
            assert(!_info.is_value_exist("sender_friendly"));

            eventInfo->setGenericText(
                _info.get<QString>("generic")
            );

            return eventInfo;
        }

        const auto isBuddyReg = (type == chat_event_type::buddy_reg);
        const auto isBuddyFound = (type == chat_event_type::buddy_found);
        const auto isBirthday = (type == chat_event_type::birthday);
        const auto isMessageDeleted = (type == chat_event_type::message_deleted);
        if (isBuddyReg || isBuddyFound || isBirthday || isMessageDeleted)
        {
            assert(!_info.is_value_exist("sender_friendly"));
            return eventInfo;
        }

        eventInfo->setSenderInfo(
            _info.get<QString>("sender_aimid"),
            _info.get<QString>("sender_friendly")
        );

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
            assert(!newChatName.isEmpty());

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

        const auto isMchatAddMembers = (type == chat_event_type::mchat_add_members);
        const auto isMchatInvite = (type == chat_event_type::mchat_invite);
        const auto isMchatLeave = (type == chat_event_type::mchat_leave);
        const auto isMchatDelMembers = (type == chat_event_type::mchat_del_members);
        const auto isMchatKicked = (type == chat_event_type::mchat_kicked);
        const auto hasMchatMembers = (isMchatAddMembers || isMchatInvite || isMchatLeave || isMchatDelMembers || isMchatKicked);
        if (hasMchatMembers)
        {
            const auto membersArray = _info.get_value_as_array("mchat/members");
            assert(membersArray);

            eventInfo->setMchatMembers(*membersArray);

            if (_info.is_value_exist("mchat/members_aimids"))
            {
                const auto membersAimids = _info.get_value_as_array("mchat/members_aimids");
                eventInfo->setMchatMembersAimIds(*membersAimids);
            }

            return eventInfo;
        }

        assert(!"unexpected event type");
        return eventInfo;
    }

    ChatEventInfo::ChatEventInfo(const chat_event_type _type, const bool _isCaptchaPresent, const bool _isOutgoing, const QString& _myAimid, const QString& _aimid, const bool _isChannel)
        : Type_(_type)
        , IsCaptchaPresent_(_isCaptchaPresent)
        , IsOutgoing_(_isOutgoing)
        , IsChannel_(_isChannel)
        , MyAimid_(_myAimid)
        , AimId_(_aimid)
    {
        assert(Type_ > chat_event_type::min);
        assert(Type_ < chat_event_type::max);
        assert(!MyAimid_.isEmpty());
    }

    QString ChatEventInfo::formatEventTextInternal() const
    {
        switch (Type_)
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

            case chat_event_type::mchat_invite:
                return formatMchatInviteText();

            case chat_event_type::mchat_leave:
                return formatMchatLeaveText();

            case chat_event_type::mchat_del_members:
                return formatMchatDelMembersText();

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

            default:
                break;
        }

        assert(!"unexpected chat event type");
        return QString();
    }

    QString ChatEventInfo::formatAddedToBuddyListText() const
    {
        assert(Type_ == chat_event_type::added_to_buddy_list);
        assert(!SenderFriendly_.isEmpty());

        if (IsOutgoing_)
            return QT_TRANSLATE_NOOP("chat_event", "You added %1 to contacts").arg(SenderFriendly_);
        return QT_TRANSLATE_NOOP("chat_event", "%1 added you to contacts").arg(SenderFriendly_);
    }

    QString ChatEventInfo::formatAvatarModifiedText() const
    {
        assert(Type_ == chat_event_type::avatar_modified);

        if (IsOutgoing_)
            return QT_TRANSLATE_NOOP("chat_event", "You changed picture of group");

        if (IsChannel_ || Logic::getContactListModel()->isChannel(AimId_))
            return QT_TRANSLATE_NOOP("chat_event", "Channel avatar was changed");

        return QT_TRANSLATE_NOOP("chat_event", "%1 changed picture of group").arg(SenderFriendly_);
    }

    QString ChatEventInfo::formatBirthdayText() const
    {
        assert(Type_ == chat_event_type::birthday);

        constexpr auto birthdayEmojiId = 0x1f381;
        return Emoji::EmojiCode::toQString(Emoji::EmojiCode(birthdayEmojiId)) + QT_TRANSLATE_NOOP("chat_event", " has birthday!");
    }

    QString ChatEventInfo::formatBuddyFound() const
    {
        assert(Type_ == chat_event_type::buddy_found);

        constexpr auto smileEmojiId = 0x1f642;
        const auto smileEmojiString = Emoji::EmojiCode::toQString(Emoji::EmojiCode(smileEmojiId));
        const QString friendly = Logic::GetFriendlyContainer()->getFriendly(AimId_);

        return QT_TRANSLATE_NOOP("chat_event", "You have recently added %1 to your phone contacts. Write a new message or make a call %2").arg(friendly, smileEmojiString);
    }

    QString ChatEventInfo::formatBuddyReg() const
    {
        assert(Type_ == chat_event_type::buddy_reg);

        constexpr auto smileEmojiId = 0x1f642;
        const auto smileEmojiString = Emoji::EmojiCode::toQString(Emoji::EmojiCode(smileEmojiId));
        const QString friendly = Logic::GetFriendlyContainer()->getFriendly(AimId_);

        return QT_TRANSLATE_NOOP("chat_event", "So %1 is here now! Write, call %2").arg(friendly, smileEmojiString);
    }

    QString ChatEventInfo::formatChatNameModifiedText() const
    {
        assert(Type_ == chat_event_type::chat_name_modified);
        assert(!SenderFriendly_.isEmpty());
        assert(!Chat_.NewName_.isEmpty());

        if (IsOutgoing_)
            return QT_TRANSLATE_NOOP("chat_event", "You changed theme to \"%1\"").arg(Chat_.NewName_);

        if (IsChannel_ || Logic::getContactListModel()->isChannel(AimId_))
            return QT_TRANSLATE_NOOP("chat_event", "Theme was changed to \"%1\"").arg(Chat_.NewName_);

        return QT_TRANSLATE_NOOP("chat_event", "%1 changed theme to \"%2\"").arg(SenderFriendly_, Chat_.NewName_);
    }

    QString ChatEventInfo::formatGenericText() const
    {
        assert(Type_ == chat_event_type::generic);
        assert(!Generic_.isEmpty());

        return Generic_;
    }

    QString ChatEventInfo::formatMchatAddMembersText() const
    {
        assert(Type_ == chat_event_type::mchat_add_members);
        assert(!Mchat_.MembersFriendly_.isEmpty());
        assert(!SenderFriendly_.isEmpty());

        if (IsOutgoing_)
            return QT_TRANSLATE_NOOP("chat_event", "You added %1").arg(formatMchatMembersList(false));
        const auto joinedSomeone = !Mchat_.MembersFriendly_.isEmpty() && (SenderFriendly_ == Mchat_.MembersFriendly_.constFirst());
        if (joinedSomeone)
            return QT_TRANSLATE_NOOP("chat_event", "%1 has joined group").arg(SenderFriendly_);

        if (IsChannel_ || Logic::getContactListModel()->isChannel(AimId_))
        {
            if (Mchat_.MembersFriendly_.size() == 1)
                return QT_TRANSLATE_NOOP("chat_event", "%1 was added").arg(formatMchatMembersList(true));

            return QT_TRANSLATE_NOOP("chat_event", "%1 were added").arg(formatMchatMembersList(true));
        }

        return QT_TRANSLATE_NOOP("chat_event", "%1 added %2").arg(SenderFriendly_, formatMchatMembersList(false));
    }

    QString ChatEventInfo::formatChatDescriptionModified() const
    {
        assert(Type_ == chat_event_type::chat_description_modified);

        if (Chat_.NewDescription_.isEmpty())
        {
            if (IsOutgoing_)
                return QT_TRANSLATE_NOOP("chat_event", "You deleted chat description");

            if (IsChannel_ || Logic::getContactListModel()->isChannel(AimId_))
                return QT_TRANSLATE_NOOP("chat_event", "Chat description was deleted");

            return QT_TRANSLATE_NOOP("chat_event", "%1 deleted chat description").arg(SenderFriendly_);
        }
        else
        {
            if (IsOutgoing_)
                return QT_TRANSLATE_NOOP("chat_event", "You changed description to \"%1\"").arg(Chat_.NewDescription_);

            if (IsChannel_ || Logic::getContactListModel()->isChannel(AimId_))
                return QT_TRANSLATE_NOOP("chat_event", "Description was changed to \"%1\"").arg(Chat_.NewDescription_);

            return QT_TRANSLATE_NOOP("chat_event", "%1 changed description to \"%2\"").arg(SenderFriendly_, Chat_.NewDescription_);
        }
    }

    QString ChatEventInfo::formatChatRulesModified() const
    {
        assert(Type_ == chat_event_type::chat_rules_modified);

        if (IsOutgoing_)
            return QT_TRANSLATE_NOOP("chat_event", "You changed chat rules to \"%1\"").arg(Chat_.NewRules_);

        if (IsChannel_ || Logic::getContactListModel()->isChannel(AimId_))
            return QT_TRANSLATE_NOOP("chat_event", "Channel rules were changed to \"%1\"").arg(Chat_.NewRules_);

        return QT_TRANSLATE_NOOP("chat_event", "%1 changed chat rules to \"%2\"").arg(SenderFriendly_, Chat_.NewRules_);
    }

    QString ChatEventInfo::formatChatStampModified() const
    {
        assert(Type_ == chat_event_type::chat_stamp_modified);

        if (IsOutgoing_)
            return QT_TRANSLATE_NOOP("chat_event", "You changed chat link to %1").arg(Utils::getDomainUrl() % ql1c('/') % Chat_.NewStamp_);

        if (IsChannel_ || Logic::getContactListModel()->isChannel(AimId_))
            return QT_TRANSLATE_NOOP("chat_event", "Channel link was changed to %1").arg(Utils::getDomainUrl() % ql1c('/') % Chat_.NewStamp_);

        return QT_TRANSLATE_NOOP("chat_event", "%1 changed chat link to %2").arg(SenderFriendly_, Utils::getDomainUrl() % ql1c('/') % Chat_.NewStamp_);
    }

    QString ChatEventInfo::formatMchatInviteText() const
    {
        assert(Type_ == chat_event_type::mchat_invite);
        assert(!Mchat_.MembersFriendly_.isEmpty());
        assert(!SenderFriendly_.isEmpty());

        const auto joinedMyselfOnly = isMyAimid(SenderAimid_);
        if (IsOutgoing_ && joinedMyselfOnly)
            return QT_TRANSLATE_NOOP("chat_event", "You have joined group");

        if (IsChannel_ || Logic::getContactListModel()->isChannel(AimId_))
        {
            if (Mchat_.MembersFriendly_.size() == 1)
                return QT_TRANSLATE_NOOP("chat_event", "%1 was added").arg(formatMchatMembersList(true));

            return QT_TRANSLATE_NOOP("chat_event", "%1 were added").arg(formatMchatMembersList(true));
        }

        return QT_TRANSLATE_NOOP("chat_event", "%1 added %2").arg(SenderFriendly_, formatMchatMembersList(false));
    }

    QString ChatEventInfo::formatMchatDelMembersText() const
    {
        assert(Type_ == chat_event_type::mchat_del_members);
        assert(!Mchat_.MembersFriendly_.isEmpty());
        assert(!SenderFriendly_.isEmpty());

        if (IsOutgoing_)
            return QT_TRANSLATE_NOOP("chat_event", "You removed %1").arg(formatMchatMembersList(false));

        if (IsChannel_ || Logic::getContactListModel()->isChannel(AimId_))
        {
            if (Mchat_.MembersFriendly_.size() == 1)
                return QT_TRANSLATE_NOOP("chat_event", "%1 was removed").arg(formatMchatMembersList(true));

            return QT_TRANSLATE_NOOP("chat_event", "%1 were removed").arg(formatMchatMembersList(true));
        }

        return QT_TRANSLATE_NOOP("chat_event", "%1 removed %2").arg(SenderFriendly_, formatMchatMembersList(false));
    }

    QString ChatEventInfo::formatMchatKickedText() const
    {
        assert(Type_ == chat_event_type::mchat_kicked);
        assert(!Mchat_.MembersFriendly_.isEmpty());
        assert(!SenderFriendly_.isEmpty());

        if (IsOutgoing_)
            return QT_TRANSLATE_NOOP("chat_event", "You removed %1").arg(formatMchatMembersList(false));

        if (IsChannel_ || Logic::getContactListModel()->isChannel(AimId_))
        {
            if (Mchat_.MembersFriendly_.size() == 1)
                return QT_TRANSLATE_NOOP("chat_event", "%1 was removed").arg(formatMchatMembersList(true));

            return QT_TRANSLATE_NOOP("chat_event", "%1 were removed").arg(formatMchatMembersList(true));
        }

        return QT_TRANSLATE_NOOP("chat_event", "%1 removed %2").arg(SenderFriendly_, formatMchatMembersList(false));
    }

    QString ChatEventInfo::formatMchatLeaveText() const
    {
        assert(Type_ == chat_event_type::mchat_leave);
        assert(!Mchat_.MembersFriendly_.isEmpty());
        assert(!SenderFriendly_.isEmpty());

        if (hasMultipleMembers())
            return QT_TRANSLATE_NOOP3("chat_event", "%1 have left group (this message is visible only to group admins)", "many").arg(formatMchatMembersList(true));

        return QT_TRANSLATE_NOOP3("chat_event", "%1 has left group (this message is visible only to group admins)", "one").arg(formatMchatMembersList(true));
    }

    QString ChatEventInfo::formatMchatMembersList(const bool _activeVoice) const
    {
        assert(!MyAimid_.isEmpty());

        QString result;
        const auto &friendlyMembers = Mchat_.MembersFriendly_;
        if (friendlyMembers.isEmpty())
            return result;

        result.reserve(512);

        const auto you =
            _activeVoice ?
                QT_TRANSLATE_NOOP3("chat_event", "You", "active_voice") :
                QT_TRANSLATE_NOOP3("chat_event", "you", "passive_voice");

        const auto format =
            [this, &you](const QString &name) -> const QString&
            {
                return (isMyAimid(name) ? you : name);
            };


        const auto &first = friendlyMembers.first();

        result += format(first);

        if (friendlyMembers.size() == 1)
            return result;

        for (auto it = std::next(friendlyMembers.begin()), end = std::prev(friendlyMembers.end()); it != end; ++it)
        {
            result += ql1s(", ");
            result += format(*it);
        }

        result += QT_TRANSLATE_NOOP("chat_event", " and ") % format(friendlyMembers.back());
        return result;
    }

    const QString& ChatEventInfo::formatEventText() const
    {
        if (FormattedEventText_.isEmpty())
        {
            FormattedEventText_ = formatEventTextInternal();
        }

        assert(!FormattedEventText_.isEmpty());
        return FormattedEventText_;
    }

    core::chat_event_type ChatEventInfo::eventType() const
    {
        return Type_;
    }

    QString ChatEventInfo::formatMessageDeletedText() const
    {
        return QT_TRANSLATE_NOOP("chat_event", "Message was deleted");
    }

    bool ChatEventInfo::isMyAimid(const QString& _aimId) const
    {
        assert(!_aimId.isEmpty());
        assert(!MyAimid_.isEmpty());

        return (MyAimid_ == _aimId);
    }

    bool ChatEventInfo::hasMultipleMembers() const
    {
        assert(!Mchat_.MembersFriendly_.isEmpty());

        return (Mchat_.MembersFriendly_.size() > 1);
    }

    void ChatEventInfo::setGenericText(QString _text)
    {
        assert(Generic_.isEmpty());
        assert(!_text.isEmpty());

        Generic_ = std::move(_text);
    }

    void ChatEventInfo::setNewChatRules(const QString& _newChatRules)
    {
        assert(Chat_.NewRules_.isEmpty());
        assert(!_newChatRules.isEmpty());

        Chat_.NewRules_ = _newChatRules;
    }

    void ChatEventInfo::setNewChatStamp(const QString& _newChatStamp)
    {
        assert(Chat_.NewStamp_.isEmpty());
        assert(!_newChatStamp.isEmpty());

        Chat_.NewStamp_ = _newChatStamp;
    }

    void ChatEventInfo::setNewDescription(const QString& _newDescription)
    {
        assert(Chat_.NewDescription_.isEmpty());

        Chat_.NewDescription_ = _newDescription;
    }

    void ChatEventInfo::setNewName(const QString& _newName)
    {
        assert(Chat_.NewName_.isEmpty());
        assert(!_newName.isEmpty());

        Chat_.NewName_ = _newName;
    }

    void ChatEventInfo::setSenderInfo(QString _aimid, QString _friendly)
    {
        assert(SenderFriendly_.isEmpty());
        assert(!_friendly.isEmpty());
        assert(SenderAimid_.isEmpty());

        SenderFriendly_ = cleanupFriendlyName(std::move(_friendly));

        if (!_aimid.isEmpty())
            SenderAimid_ = cleanupFriendlyName(std::move(_aimid));
        else
            SenderAimid_ = QString();

        if (!SenderFriendly_.isEmpty() && !isMyAimid(SenderAimid_))
            Mchat_.MembersLinks_[SenderFriendly_] = ql1s("@[") % SenderAimid_ % ql1c(']');
    }

    const QString& ChatEventInfo::getSenderFriendly() const
    {
        return SenderFriendly_;
    }

    bool ChatEventInfo::isOutgoing() const
    {
        return IsOutgoing_;
    }

    bool ChatEventInfo::isCaptchaPresent() const
    {
        return IsCaptchaPresent_;
    }

    const std::map<QString, QString>& ChatEventInfo::getMembersLinks() const
    {
        return Mchat_.MembersLinks_;
    }

    const QVector<QString>& ChatEventInfo::getMembers() const
    {
        return Mchat_.Members_;
    }

    QString ChatEventInfo::getAimId() const
    {
        return AimId_;
    }

    void ChatEventInfo::setMchatMembers(const core::iarray& _members)
    {
        auto &membersFriendly = Mchat_.MembersFriendly_;

        assert(membersFriendly.isEmpty());
        const auto size = _members.size();
        membersFriendly.reserve(size);
        for (auto index = 0; index < size; ++index)
        {
            const auto member = _members.get_at(index);
            assert(member);

            membersFriendly.push_back(cleanupFriendlyName(QString::fromUtf8(member->get_as_string())));
        }

        assert(membersFriendly.size() == _members.size());
    }

    void ChatEventInfo::setMchatMembersAimIds(const core::iarray& _membersAimids)
    {
        if (Mchat_.MembersFriendly_.empty() || Mchat_.MembersFriendly_.size() != _membersAimids.size())
            return;

        const auto size = _membersAimids.size();
        Mchat_.Members_.reserve(size);
        for (auto index = 0; index < size; ++index)
        {
            auto member = QString::fromUtf8(_membersAimids.get_at(index)->get_as_string());
            Mchat_.MembersLinks_[Mchat_.MembersFriendly_[index]] = ql1s("@[") % member % ql1c(']');
            Mchat_.Members_.push_back(std::move(member));
        }
    }
}
