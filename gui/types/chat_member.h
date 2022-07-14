#pragma once

#include "lastseen.h"

class QPixmap;

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
        QDateTime canRemoveTill_;

        bool IsCreator_ = false;
    };

 }

Q_DECLARE_METATYPE(Data::ChatMemberInfo*);
