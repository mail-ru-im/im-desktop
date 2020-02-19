#include "stdafx.h"
#include "contact.h"

#include "../../corelib/collection_helper.h"
#include "utils/PhoneFormatter.h"


namespace Data
{

    Contact::~Contact()
    {
    }

    void UnserializeContactList(core::coll_helper* helper, ContactList& cl, QString& type)
    {
        type = QString();
        if (helper->is_value_exist("type"))
            type = QString::fromUtf8(helper->get_value_as_string("type"));

        core::iarray* groups = helper->get_value_as_array("groups");
        const auto groupsSize = groups->size();
        for (core::iarray::size_type igroups = 0; igroups < groupsSize; ++igroups)
        {
            core::coll_helper group_coll(groups->get_at(igroups)->get_as_collection(), false);
            auto group = std::make_shared<GroupBuddy>();
            group->Id_ = group_coll.get_value_as_int("group_id");
            group->Name_ = QString::fromUtf8(group_coll.get_value_as_string("group_name"));
            group->Added_ = group_coll.get_value_as_bool("added");
            group->Removed_ = group_coll.get_value_as_bool("removed");
            core::iarray* contacts = group_coll.get_value_as_array("contacts");
            const auto contactsSize = contacts->size();
            for (core::iarray::size_type icontacts = 0; icontacts < contactsSize; ++icontacts)
            {
                core::coll_helper value(contacts->get_at(icontacts)->get_as_collection(), false);
                qlonglong lastSeen = value.get_value_as_int("lastseen");
                auto contact = std::make_shared<Contact>();
                contact->AimId_ = QString::fromUtf8(value.get_value_as_string("aimId"));
                contact->Friendly_ = QString::fromUtf8(value.get_value_as_string("friendly"));
                contact->AbContactName_ = QString::fromUtf8(value.get_value_as_string("abContactName"));
                contact->UserType_ = QString::fromUtf8(value.get_value_as_string("userType"));
                contact->StatusMsg_ = QString::fromUtf8(value.get_value_as_string("statusMsg"));
                contact->OtherNumber_ = QString::fromUtf8(value.get_value_as_string("otherNumber"));
                contact->HasLastSeen_ = lastSeen != -1;
                contact->LastSeenCore_ = lastSeen;
                contact->LastSeen_ = lastSeen > 0 ? QDateTime::fromTime_t(uint(lastSeen)) : QDateTime();
                contact->Is_chat_ = value.get_value_as_bool("is_chat");
                contact->GroupId_ = group->Id_;
                contact->Muted_ = value.get_value_as_bool("mute");
                contact->IsLiveChat_ = value.get_value_as_bool("livechat");
                contact->IsOfficial_ = value.get_value_as_bool("official");
                contact->IsPublic_ = value.get_value_as_bool("public");
                contact->isChannel_ = value.get_value_as_bool("channel");
                contact->iconId_ = QString::fromUtf8(value.get_value_as_string("iconId"));
                contact->bigIconId_ = QString::fromUtf8(value.get_value_as_string("bigIconId"));
                contact->largeIconId_ = QString::fromUtf8(value.get_value_as_string("largeIconId"));
                contact->OutgoingMsgCount_ = value.get_value_as_int("outgoingCount");
                contact->isDeleted_ = value.get_value_as_bool("deleted");

                cl.insert(contact, group);
            }
        }
    }

    bool UnserializeAvatar(core::coll_helper* helper, QPixmap& _avatar)
    {
        if (helper->get_value_as_bool("result"))
        {
            core::istream* stream = helper->get_value_as_stream("avatar");
            if (stream)
            {
                uint32_t size = stream->size();

                _avatar.loadFromData(stream->read(size), size);

                stream->reset();
            }
            return true;
        }

        return false;
    }

    BuddyPtr UnserializePresence(core::coll_helper* helper)
    {
        auto result = std::make_shared<Buddy>();

        result->AimId_ = QString::fromUtf8(helper->get_value_as_string("aimid"));
        result->Friendly_ = QString::fromUtf8(helper->get_value_as_string("friendly"));
        result->AbContactName_ = QString::fromUtf8(helper->get_value_as_string("abContactName"));
        result->UserType_ = QString::fromUtf8(helper->get_value_as_string("userType"));
        result->StatusMsg_ = QString::fromUtf8(helper->get_value_as_string("statusMsg"));
        result->OtherNumber_ = QString::fromUtf8(helper->get_value_as_string("otherNumber"));
        qlonglong lastSeen = helper->get_value_as_int("lastseen");
        result->HasLastSeen_ = lastSeen != -1;
        result->LastSeenCore_ = lastSeen;
        result->LastSeen_ = lastSeen > 0 ? QDateTime::fromTime_t(uint(lastSeen)) : QDateTime();
        result->Is_chat_ = helper->get_value_as_bool("is_chat");
        result->Muted_ = helper->get_value_as_bool("mute");
        result->IsLiveChat_ = helper->get_value_as_bool("livechat");
        result->IsOfficial_ = helper->get_value_as_bool("official");
        result->iconId_ = QString::fromUtf8(helper->get_value_as_string("iconId"));
        result->bigIconId_ = QString::fromUtf8(helper->get_value_as_string("bigIconId"));
        result->largeIconId_ = QString::fromUtf8(helper->get_value_as_string("largeIconId"));
        result->OutgoingMsgCount_ = helper->get_value_as_int("outgoingCount");
        result->isDeleted_ = helper->get_value_as_bool("deleted");

        return result;
    }

    QString UnserializeActiveDialogHide(core::coll_helper* helper)
    {
        return QString::fromUtf8(helper->get_value_as_string("contact"));
    }

    QStringList UnserializeFavorites(core::coll_helper* helper)
    {
        core::iarray* contacts = helper->get_value_as_array("favorites");
        QStringList result;
        const auto size = contacts->size();
        result.reserve(size);
        for (core::iarray::size_type i = 0; i < size; ++i)
        {
            core::coll_helper value(contacts->get_at(i)->get_as_collection(), false);
            result.push_back(QString::fromUtf8(value.get_value_as_string("aimId")));
        }

        return result;
    }

    UserInfo UnserializeUserInfo(core::coll_helper* helper)
    {
        UserInfo info;

        auto error = helper->is_value_exist("error") ? helper->get_value_as_int("error") : 0;
        if (error == 0)
        {
            info.firstName_ = QString::fromUtf8(helper->get_value_as_string("firstName"));
            info.lastName_ = QString::fromUtf8(helper->get_value_as_string("lastName"));
            info.friendly_ = QString::fromUtf8(helper->get_value_as_string("friendly"));
            info.nick_ = QString::fromUtf8(helper->get_value_as_string("nick"));
            info.about_ = QString::fromUtf8(helper->get_value_as_string("about"));
            info.phone_ = QString::fromUtf8(helper->get_value_as_string("phone"));
            info.phoneNormalized_ = PhoneFormatter::formatted(info.phone_);
            info.lastseen_ = helper->get_value_as_int("lastseen");
            info.commonChats_ = helper->get_value_as_int("commonChats");
            info.mute_ = helper->get_value_as_bool("mute");
            info.official_ = helper->get_value_as_bool("official");
        }

        return info;
    }
}
