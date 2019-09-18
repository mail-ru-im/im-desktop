#pragma once

#include "../../../types/message.h"
#include "../../../types/chatheads.h"
#include "../../mediatype.h"
#include "../../../utils/utils.h"

namespace hist
{
    class MessageReader : public QObject
    {
        Q_OBJECT

        using MentionsMeVector = std::vector<Data::MessageBuddySptr>;
        using IdsSet = std::set<qint64>;
        struct LastReads
        {
            qint64 text = -1;
            qint64 mention = -1;
        };

    public:
        explicit MessageReader(const QString& _aimId, QObject* _parent = nullptr);
        ~MessageReader();

        void init();

        void setEnabled(bool _value);
        bool isEnabled() const;

        int getMentionsCount() const noexcept;
        Data::MessageBuddySptr getNextUnreadMention() const;

        void onMessageItemRead(const qint64 _messageId, const bool _visible);
        void onReadAllMentionsLess(const qint64 _messageId, const bool _visible);
        void onMessageItemReadVisible(const qint64 _messageId);

        void setDlgState(const Data::DlgState& _dlgState);

    signals:
        void mentionRead(qint64 _messageId, QPrivateSignal) const;

    private:
        void mentionMe(const QString& _contact, const Data::MessageBuddySptr& _mention);
        void sendLastReadMention(qint64 _messageId);
        void sendPartialLastRead(qint64 _messageId);

    private:
        const QString aimId_;
        LastReads lastReads_;
        MentionsMeVector mentions_;
        IdsSet pendingVisibleMessages_;
        bool enabled_ = true;
    };
}
