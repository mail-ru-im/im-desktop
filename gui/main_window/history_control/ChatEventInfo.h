#pragma once

namespace core
{
    class coll_helper;

    struct iarray;

    enum class chat_event_type;
}

namespace HistoryControl
{

    typedef std::shared_ptr<class ChatEventInfo> ChatEventInfoSptr;

    class ChatEventInfo
    {
    public:
        static ChatEventInfoSptr make(
            const core::coll_helper& _info,
            const bool _isOutgoing,
            const QString& _myAimid,
            const QString& _aimid);

        const QString& formatEventText() const;
        core::chat_event_type eventType() const;
        QString senderFriendly() const;

        bool isChannel() const;
        bool isOutgoing() const;
        bool isCaptchaPresent() const;
        bool isForMe() const;

        const std::map<QString, QString>& getMembersLinks() const;
        const QVector<QString>& getMembers() const;
        const QString& getAimId() const noexcept { return aimId_; }
        const QString& getSender() const noexcept { return sender_; }

    private:
        ChatEventInfo(const core::chat_event_type _type, const bool _isCaptchaPresent, const bool _isOutgoing, const QString& _myAimid, const QString& _aimid, const bool _is_channel);

        QString formatEventTextInternal() const;
        QString formatAddedToBuddyListText() const;
        QString formatAvatarModifiedText() const;
        QString formatBirthdayText() const;
        QString formatBuddyFound() const;
        QString formatBuddyReg() const;
        QString formatChatNameModifiedText() const;
        QString formatGenericText() const;
        QString formatMchatAddMembersText() const;
        QString formatMchatDelMembersText() const;
        QString formatMchatAdmGrantedText() const;
        QString formatMchatAdmRevokedText() const;
        QString formatMchatAllowedToWrite() const;
        QString formatMchatDisallowedToWrite() const;
        QString formatMchatWaitingForApprove() const;
        QString formatMchatJoiningApproved() const;
        QString formatMchatJoiningRejected() const;
        QString formatMchatJoiningCanceled() const;
        QString formatChatDescriptionModified() const;
        QString formatChatRulesModified() const;
        QString formatChatStampModified() const;
        QString formatJoinModerationModified() const;
        QString formatPublicModified() const;
        QString formatMchatInviteText() const;
        QString formatMchatKickedText() const;
        QString formatMchatLeaveText() const;
        QString formatMchatMembersList() const;
        QString formatMessageDeletedText() const;
        QString formatWarnAboutStranger() const;
        QString formatNoLongerStranger() const;

        bool isMyAimid(const QString& _aimId) const;
        bool hasMultipleMembers() const;
        void setGenericText(QString _text);
        void setNewChatRules(const QString& _newChatRules);
        void setNewChatStamp(const QString& _newChatStamp);
        void setNewDescription(const QString& _newDescription);
        void setNewName(const QString& _newName);
        void setNewJoinModeration(bool _newJoinModeration);
        void setNewPublic(bool _newPublic);
        void setSender(QString _aimid);
        void setMchatMembersAimIds(const core::iarray& _membersAimids);
        void setMchatRequestedBy(const QString& _requestedBy);
        QString senderOrAdmin() const;

        const core::chat_event_type type_;
        const bool isCaptchaPresent_;
        const bool isOutgoing_;
        const bool isChannel_;
        const QString myAimid_;
        mutable QString formattedEventText_;
        QString sender_;
        QString generic_;
        QString aimId_;

        struct
        {
            QVector<QString> members_;
            std::map<QString, QString> membersLinks_;
            QString requestedBy_;
        } mchat_;

        struct
        {
            QString newName_;
            QString newDescription_;
            QString newRules_;
            QString newStamp_;
            bool newJoinModeration_ = false;
            bool newPublic_ = false;
        } chat_;
    };

}
