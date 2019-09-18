#pragma once

#include "../../../types/message.h"
#include "../../../types/chatheads.h"
#include "../../mediatype.h"
#include "../../../utils/utils.h"

namespace core
{
    enum class message_type;
    enum class chat_event_type;
}

namespace HistoryControl
{
    using FileSharingInfoSptr = std::shared_ptr<class FileSharingInfo>;
    using StickerInfoSptr = std::shared_ptr<class StickerInfo>;
    using ChatEventInfoSptr = std::shared_ptr<class ChatEventInfo>;
}

namespace Logic
{
    enum class control_type
    {
        ct_date = 0,
        ct_message = 1,
        ct_new_messages = 2
    };

    enum class preview_type
    {
        min,

        none,
        site,

        max
    };

    class MessageKey
    {
    public:
        static const MessageKey MAX;
        static const MessageKey MIN;

        MessageKey();

        MessageKey(
            const qint64 _id,
            const control_type _control_type);

        MessageKey(
            const qint64 _id,
            const QString& _internal_id);

        MessageKey(
            const qint64 _id,
            const qint64 _prev,
            const QString& _internalId,
            const qint64 _pendingId,
            const qint32 _time,
            const core::message_type _type,
            const bool _outgoing,
            const preview_type _previewType,
            const control_type _control_type,
            const QDate _date);

        bool isEmpty() const noexcept
        {
            return id_ == -1 && prev_ == -1 && internalId_.isEmpty();
        }

        void setEmpty() noexcept
        {
            id_ = -1;
            prev_ = -1;
            internalId_.clear();
        }

        bool isPending() const noexcept
        {
            return id_ == -1 && !internalId_.isEmpty();
        }

        bool operator==(const MessageKey& _other) const noexcept
        {
            const auto idsEqual = (hasId() && id_ == _other.id_);

            const auto internalIdsEqual = (!internalId_.isEmpty() && (internalId_ == _other.internalId_));

            if (idsEqual || internalIdsEqual)
            {
                return controlType_ == _other.controlType_;
            }

            return false;
        }

        bool operator!=(const MessageKey& _other) const noexcept
        {
            return !operator==(_other);
        }

        bool operator<(const MessageKey& _other) const noexcept;
        bool operator>(const MessageKey& _other) const noexcept
        {
            return _other < *this;
        }

        bool operator<=(const MessageKey& _other) const noexcept
        {
            return !(*this > _other);
        }

        bool hasId() const noexcept;

        bool checkInvariant() const noexcept;

        bool isChatEvent() const noexcept;

        bool isOutgoing() const noexcept;

        bool isFileSharing() const noexcept;

        bool isVoipEvent() const noexcept;

        bool isSticker() const noexcept;

        bool isDate() const noexcept;

        bool iNewPlate() const noexcept;

        void setId(const int64_t _id) noexcept;

        qint64 getId() const noexcept;

        void setOutgoing(const bool _isOutgoing) noexcept;

        QString toLogStringShort() const;

        void setControlType(const control_type _controlType) noexcept;
        control_type getControlType() const noexcept;

        core::message_type getType() const noexcept;
        void setType(core::message_type _type) noexcept;

        preview_type getPreviewType() const noexcept;

        qint64 getPrev() const noexcept;

        const QString& getInternalId() const noexcept;

        int getPendingId() const noexcept;

        qint32 getTime() const noexcept;

        QDate getDate() const noexcept;

        MessageKey toPurePending() const;

    private:

        qint64 id_;

        qint64 prev_;

        QString internalId_;

        core::message_type type_;

        control_type controlType_;

        qint32 time_;

        qint64 pendingId_;

        preview_type previewType_;

        bool outgoing_;

        QDate date_;

        bool lessThan(const MessageKey& _rhs) const noexcept;
    };

    using MessageKeyVector = QVector<MessageKey>;

    class Message
    {

    public:

        Message(const QString& _aimId)
            : deleted_(false)
            , aimId_(_aimId)
        {
        }

        bool isBase() const noexcept;

        bool isChatEvent() const noexcept;

        bool isDate() const noexcept;

        bool isDeleted() const noexcept;

        bool isFileSharing() const noexcept;

        bool isOutgoing() const noexcept;

        bool isPending() const noexcept;

        bool isStandalone() const noexcept;

        bool isSticker() const noexcept;

        bool isVoipEvent() const noexcept;

        bool isPreview() const noexcept;

        const HistoryControl::ChatEventInfoSptr& getChatEvent() const;

        const QString& getChatSender() const;

        const HistoryControl::FileSharingInfoSptr& getFileSharing() const;

        const HistoryControl::StickerInfoSptr& getSticker() const;

        const HistoryControl::VoipEventInfoSptr& getVoipEvent() const;

        void setChatEvent(const HistoryControl::ChatEventInfoSptr& _info);

        void setChatSender(const QString& _chatSender);

        void setDeleted(const bool _deleted);

        void setFileSharing(const HistoryControl::FileSharingInfoSptr& info);

        void setSticker(const HistoryControl::StickerInfoSptr& _info);

        void setVoipEvent(const HistoryControl::VoipEventInfoSptr& _info);

        void applyModification(const Data::MessageBuddy& _modification);

        void setBuddy(Data::MessageBuddySptr _buddy);
        const Data::MessageBuddySptr& getBuddy() const;

        const QString& getAimId() const;

        void setKey(const MessageKey& _key);
        const MessageKey& getKey() const;

        const QDate& getDate() const;
        void setDate(const QDate& _date);

        void setChatFriendly(const QString& _friendly);
        const QString& getChatFriendly() const;

    private:

        HistoryControl::FileSharingInfoSptr fileSharing_;

        HistoryControl::StickerInfoSptr sticker_;

        HistoryControl::ChatEventInfoSptr chatEvent_;

        HistoryControl::VoipEventInfoSptr voipEvent_;

        QString chatSender_;

        bool deleted_;

        std::shared_ptr<Data::MessageBuddy> buddy_;

        QString aimId_;

        MessageKey key_;

        QDate date_;

        QString chatFriendly_;

        void EraseEventData();
    };
}
