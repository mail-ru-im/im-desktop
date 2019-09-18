#include "stdafx.h"

#include "search_result.h"
#include "../main_window/contact_list/contact_profile.h"
#include "../main_window/friendly/FriendlyContainer.h"
#include "../my_info.h"

namespace Data
{
    QString SearchResultContact::getAimId() const
    {
        if (profile_)
            return profile_->get_aimid();

        return QString();
    }

    QString SearchResultContact::getFriendlyName() const
    {
        if (profile_)
            return profile_->get_friendly();

        return QString();
    }

    QString SearchResultContact::getNick() const
    {
        if (profile_)
            return profile_->get_nick_pretty();

        return QString();
    }

    QString SearchResultChat::getAimId() const
    {
        return chatInfo_.AimId_;
    }

    QString SearchResultChat::getFriendlyName() const
    {
        return chatInfo_.Name_;
    }

    QString SearchResultMessage::getAimId() const
    {
        if (message_)
            return message_->AimId_;

        return QString();
    }

    QString SearchResultMessage::getSenderAimId() const
    {
        if (message_)
        {
            if (dialogSearchResult_)
            {
                if (message_->HasChatSender())
                    return message_->GetChatSender();
                else if (message_->IsOutgoing())
                    return Ui::MyInfo()->aimId();
            }

            return message_->AimId_;
        }

        return QString();
    }

    QString SearchResultMessage::getFriendlyName() const
    {
        if (message_)
            return Logic::GetFriendlyContainer()->getFriendly(getSenderAimId());

        return QString();
    }

    qint64 SearchResultMessage::getMessageId() const
    {
        return message_->Id_;
    }

    QString SearchResultContactChatLocal::getFriendlyName() const
    {
        return Logic::GetFriendlyContainer()->getFriendly(getAimId());
    }

    QString SearchResultContactChatLocal::getNick() const
    {
        return Logic::GetFriendlyContainer()->getNick(getAimId());
    }

    QString SearchResultChatMember::getAimId() const
    {
        return info_.AimId_;
    }

    QString SearchResultChatMember::getFriendlyName() const
    {
        return Logic::GetFriendlyContainer()->getFriendly(getAimId());
    }

    QString SearchResultChatMember::getNick() const
    {
        return Logic::GetFriendlyContainer()->getNick(getAimId());
    }

    QString SearchResultChatMember::getRole() const
    {
        return info_.Role_;
    }

    int32_t SearchResultChatMember::getLastseen() const
    {
        return info_.Lastseen_;
    }

    bool SearchResultChatMember::isCreator() const
    {
        return info_.IsCreator_;
    }

    QString SearchResultCommonChat::getAimId() const
    {
        return info_.aimid_;
    }

    QString SearchResultCommonChat::getFriendlyName() const
    {
        return info_.friendly_;
    }
}
