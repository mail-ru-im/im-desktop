#pragma once

#include "message.h"
#include "lastseen.h"

class QPixmap;

namespace core
{
    class coll_helper;
}

namespace Data
{
    class ChatMemberInfo
    {
    public:

        bool operator==(const ChatMemberInfo& other) const
        {
            return AimId_ == other.AimId_;
        }

        QString AimId_;
        QString Role_;
        LastSeen Lastseen_;

        bool IsCreator_ = false;
    };

    class ChatInfo
    {
    public:
        QString AimId_;
        QString Name_;
        QString Location_;
        QString About_;
        QString Rules_;
        QString YourRole_;
        QString Owner_;
        QString MembersVersion_;
        QString InfoVersion_;
        QString Stamp_;
        QString Creator_;
        QString DefaultRole_;

        int32_t CreateTime_ = -1;
        int32_t AdminsCount_ = -1;
        int32_t MembersCount_ = - 1;
        int32_t FriendsCount = -1;
        int32_t BlockedCount_ = -1;
        int32_t PendingCount_ = -1;

        bool YouBlocked_ = false;
        bool YouPending_ = false;
        bool Public_ = false;
        bool ApprovedJoin_ = false;
        bool Live_ = false;
        bool Controlled_ = false;
        bool YouMember_ = false;

        QVector<ChatMemberInfo> Members_;

        bool isChannel() const { return DefaultRole_ == u"readonly"; }
    };

    QVector<ChatMemberInfo> UnserializeChatMembers(core::coll_helper* helper, const QString& _creator = QString());
    ChatInfo UnserializeChatInfo(core::coll_helper* helper);

    class ChatMembersPage
    {
    public:
        QString AimId_;
        QString Cursor_;

        QVector<ChatMemberInfo> Members_;
    };

    ChatMembersPage UnserializeChatMembersPage(core::coll_helper* _helper);

    class ChatContacts
    {
    public:
        QString AimId_;
        QString Cursor_;

        QVector<QString> Members_;
    };

    ChatContacts UnserializeChatContacts(core::coll_helper* _helper);

    struct ChatResult
    {
        QVector<ChatInfo> chats;
        QString newTag;
        bool restart = false;
        bool finished = false;
    };

    ChatResult UnserializeChatHome(core::coll_helper* helper);

    struct DialogGalleryEntry
    {
        qint64 msg_id_ = -1;
        qint64 seq_ = -1;
        QString url_;
        QString type_;
        QString sender_;
        bool outgoing_ = false;
        time_t time_ = 0;
        QString action_;
        QString caption_;
    };

    DialogGalleryEntry UnserializeGalleryEntry(core::coll_helper* _helper);

    struct DialogGalleryState
    {
        bool isEmpty() const { return images_count_ == 0 && videos_count == 0 && files_count == 0 && links_count == 0 && ptt_count == 0; }

        int images_count_ = 0;
        int videos_count = 0;
        int files_count = 0;
        int links_count = 0;
        int ptt_count = 0;
        int audio_count = 0;
    };

    DialogGalleryState UnserializeDialogGalleryState(core::coll_helper* _helper);

    struct CommonChatInfo
    {
        bool operator==(const CommonChatInfo& other) const
        {
            return aimid_ == other.aimid_;
        }

        QString aimid_;
        QString friendly_;
        int32_t membersCount_ = 0;
    };
}

Q_DECLARE_METATYPE(Data::ChatMemberInfo*);
Q_DECLARE_METATYPE(Data::ChatInfo);
Q_DECLARE_METATYPE(QVector<Data::ChatInfo>);
Q_DECLARE_METATYPE(QVector<Data::ChatMemberInfo>);
Q_DECLARE_METATYPE(Data::DialogGalleryEntry);
Q_DECLARE_METATYPE(QVector<Data::DialogGalleryEntry>);
Q_DECLARE_METATYPE(Data::DialogGalleryState);
Q_DECLARE_METATYPE(Data::CommonChatInfo);
