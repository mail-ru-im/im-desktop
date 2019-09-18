#pragma once

#include "stdafx.h"

class QPixmap;

namespace core
{
    class coll_helper;
}

namespace Data
{
    enum ContactType
    {
        MIN = 0,

        BASE = MIN,
        GROUP,
        EMPTY_IGNORE_LIST,

        SERVICE_HEADER,
        DROPDOWN_BUTTON,

        MAX
    };

    class Buddy
    {
    public:
        Buddy()
            : GroupId_(-1)
            , Unreads_(0)
            , OutgoingMsgCount_(0)
            , Is_chat_(false)
            , HasLastSeen_(false)
            , Muted_(false)
            , IsChecked_(false)
            , IsLiveChat_(false)
            , IsOfficial_(false)
            , IsPublic_(false)
            , isChannel_(false)
            , isDeleted_(false)
        {
        }

        bool operator==(const Buddy& other) const
        {
            return AimId_ == other.AimId_;
        }

        void ApplyOther(Buddy* other)
        {
            if (other == nullptr)
                return;

            AimId_ = other->AimId_;
            Friendly_ = other->Friendly_;
            AbContactName_ = other->AbContactName_;
            State_ = other->State_;
            UserType_ = other->UserType_;
            StatusMsg_ = other->StatusMsg_;
            OtherNumber_ = other->OtherNumber_;
            LastSeen_ = other->LastSeen_;
            LastSeenCore_ = other->LastSeenCore_;
            GroupId_ = other->GroupId_;
            Is_chat_ = other->Is_chat_;
            HasLastSeen_ = other->HasLastSeen_;
            Unreads_ = other->Unreads_;
            Muted_ = other->Muted_;
            IsLiveChat_ = other->IsLiveChat_;
            IsOfficial_ = other->IsOfficial_;
            iconId_ = other->iconId_;
            bigIconId_ = other->bigIconId_;
            largeIconId_ = other->largeIconId_;
            OutgoingMsgCount_ = other->OutgoingMsgCount_;
            IsPublic_ = other->IsPublic_;
            isDeleted_ = other->isDeleted_;
            isChannel_ = other->isChannel_;
            // Intentionally ignoring IsChecked_
        }

        QString		AimId_;
        QString		Friendly_;
        QString		AbContactName_;
        QString		State_;
        QString		UserType_;
        QString		StatusMsg_;
        QString		OtherNumber_;
        QDateTime	LastSeen_;
        int32_t         LastSeenCore_;
        int             GroupId_;
        int     	Unreads_;
        int             OutgoingMsgCount_;
        bool            Is_chat_;
        bool            HasLastSeen_;
        bool            Muted_;
        bool            IsChecked_;
        bool            IsLiveChat_;
        bool            IsOfficial_;
        bool            IsPublic_;
        bool            isChannel_;
        bool            isDeleted_;
        QString         iconId_;
        QString         bigIconId_;
        QString         largeIconId_;

        QPixmap     iconPixmap_;
        QPixmap     iconHoverPixmap_;
        QPixmap     iconPressedPixmap_;
    };

    using BuddyPtr = std::shared_ptr<Buddy>;

    class Contact : public Buddy
    {
    public:
        virtual ~Contact();

        virtual const QString& GetDisplayName() const
        {
            return Friendly_.isEmpty() ? AimId_ : Friendly_;
        }

        void ApplyBuddy(BuddyPtr buddy)
        {
            int groupId = GroupId_;
            bool isChat = Is_chat_;
            QString ab = AbContactName_;
            ApplyOther(buddy.get());
            if (buddy->GroupId_ == -1)
                GroupId_ = groupId;
            if (AbContactName_.isEmpty() && !ab.isEmpty())
                AbContactName_ = ab;
            Is_chat_ = isChat;
        }

        virtual ContactType GetType() const
        {
            return contactType_;
        }
        void setType(ContactType _type)
        {
            contactType_ = _type;
        }

        virtual QString GetState() const
        {
            return State_;
        }

        virtual QDateTime GetLastSeen() const
        {
            return LastSeen_;
        }

        virtual int32_t GetLastSeenCore() const
        {
            return LastSeenCore_;
        }

        virtual QString GetOtherNumber() const
        {
            return OtherNumber_;
        }

    protected:
        ContactType contactType_ = BASE;
    };
    using ContactPtr = std::shared_ptr<Contact>;

    class GroupBuddy
    {
    public:
        GroupBuddy()
            : Id_(-1)
            , Added_(false)
            , Removed_(false)
        {
        }

        bool operator==(const GroupBuddy& other) const
        {
            return Id_ == other.Id_;
        }

        void ApplyOther(GroupBuddy* other)
        {
            if (other == nullptr)
                return;

            Id_ = other->Id_;
            Name_ = other->Name_;
            Added_ = other->Added_;
            Removed_ = other->Removed_;
        }

        int Id_;
        QString Name_;

        //used by diff from server
        bool        Added_;
        bool        Removed_;
    };
    using GroupBuddyPtr = std::shared_ptr<GroupBuddy>;

    class Group : public GroupBuddy, public Contact
    {
    public:
        virtual ContactType GetType() const override
        {
            return GROUP;
        }

        void ApplyBuddy(GroupBuddyPtr buddy)
        {
            GroupBuddy::ApplyOther(buddy.get());
            GroupId_ = buddy->Id_;
            AimId_ = QString::number(GroupId_);
        }

        virtual const QString& GetDisplayName() const override
        {
            return Name_;
        }
    };

    struct UserInfo
    {
        QString firstName_;
        QString lastName_;
        QString friendly_;
        QString nick_;
        QString about_;
        QString phone_;
        QString phoneNormalized_;

        int32_t lastseen_ = -1;
        int32_t commonChats_ = 0;

        bool mute_ = false;
        bool official_ = false;
    };

    using GroupPtr = std::shared_ptr<GroupBuddy>;

    using ContactList = QMap<ContactPtr, GroupBuddyPtr>;

    void UnserializeContactList(core::coll_helper* helper, ContactList& cl, QString& type);

    bool UnserializeAvatar(core::coll_helper* helper, QPixmap& _avatar);

    BuddyPtr UnserializePresence(core::coll_helper* helper);

    QString UnserializeActiveDialogHide(core::coll_helper* helper);

    QStringList UnserializeFavorites(core::coll_helper* helper);

    UserInfo UnserializeUserInfo(core::coll_helper* helper);
}

Q_DECLARE_METATYPE(Data::Buddy*);
Q_DECLARE_METATYPE(Data::Contact*);
Q_DECLARE_METATYPE(Data::UserInfo*)
