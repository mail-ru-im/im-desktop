#pragma once

#include "../../corelib/core_face.h"
#include "../../common.shared/patch_version.h"
#include "../../common.shared/message_processing/text_formatting.h"
#include "../types/typing.h"
#include "../main_window/mediatype.h"
#include "reactions.h"
#include "poll.h"

#include "utils/StringComparator.h"

namespace core
{
    class coll_helper;

    enum class message_type;
}

namespace HistoryControl
{
    using FileSharingInfoSptr = std::shared_ptr<class FileSharingInfo>;
    using StickerInfoSptr = std::shared_ptr<class StickerInfo>;
    using ChatEventInfoSptr = std::shared_ptr<class ChatEventInfo>;
    using VoipEventInfoSptr = std::shared_ptr<class VoipEventInfo>;
}

namespace Logic
{
    class MessageKey;
    class CallsModel;
    class CallsSearchModel;

    enum class preview_type;
}

namespace Ui
{
    enum class MediaType;
}

namespace Data
{
    static const auto PRELOAD_MESSAGES_COUNT = 30;
    static const auto MORE_MESSAGES_COUNT = 30;

    struct Quote;
    struct UrlSnippet;

    struct SharedContactData
    {
        QString name_;
        QString phone_;
        QString sn_;

        void serialize(core::icollection* _collection) const;
        void unserialize(core::icollection* _collection);
    };
    using SharedContact = std::optional<SharedContactData>;

    struct GeoData
    {
        QString name_;
        double lat_;
        double long_;

        void serialize(core::icollection* _collection) const;
        void unserialize(core::icollection* _collection);
    };
    using Geo = std::optional<GeoData>;

    struct ButtonData
    {
        enum class style
        {
            BASE = 0,
            PRIMARY = 1,
            ATTENTION = 2,
        };

        QString text_;
        QString url_;
        QString callbackData_;
        style style_ = style::BASE;

        void unserialize(core::icollection* _collection);

        style getStyle() const;
    };


    class FormattedStringView;
    struct FormattedString
    {
    public:
        FormattedString() = default;
        FormattedString(const QString& _string, const core::data::format::string_formatting& _format = {})
            : string_(_string), format_(_format) { }

        bool hasFormatting() const { return !format_.empty(); }
        void replace(QChar _old, QChar _new) { string_.replace(_old, _new); }
        bool isEmpty() const { return string_.isEmpty(); }
        const QString& string() const { return string_; }
        const core::data::format::string_formatting& formatting() const { return format_; }
        core::data::format::string_formatting& formatting() { return format_; }
        int size() const { return string_.size(); }

        bool startsWith(const QString& _prefix) const { return string_.startsWith(_prefix); }
        bool endsWith(const QString& _suffix) const { return string_.endsWith(_suffix); }

        bool startsWith(QChar _ch) const { return string_.startsWith(_ch); }
        bool endsWith(QChar _ch) const { return string_.endsWith(_ch); }

        bool operator!=(const FormattedString& _other);

        void reserve(int _size) { string_.reserve(_size); }

        void chop(int _n);

        FormattedString& operator+=(QChar _ch) { string_ += _ch; return *this; }
        FormattedString& operator+=(const QString& _other) { string_ += _other; return *this; }
        FormattedString& operator+=(const FormattedString& _other);
        FormattedString& operator+=(const FormattedStringView& _other);

        inline void clear() { string_.clear(); format_.clear(); }
        inline void clearFormat() { format_.clear(); }
        void setFormatting(const core::data::format::string_formatting& _formatting);

    protected:
        void fixInvalidRanges();

        QString string_;
        core::data::format::string_formatting format_;
    };

    void serializeFormat(const core::data::format::string_formatting& _format, core::icollection* _collection, std::string_view _name);
    void serializeFormat(const core::data::format::string_formatting& _format, core::coll_helper& _coll, std::string_view _name);

    // TODO-FORMAT-IMPLEMENT
    //! Calls to this function mark places to be replaced in the cource of further development of formatted text
    QString stubFromFormattedString(const FormattedString& _string);

    //! NB: FormattedStringView doesn't take care of keeping any ownership
    class FormattedStringView
    {
    public:
        FormattedStringView() : string_(&emptyString), offset_(0), size_(0) {}

        FormattedStringView(const FormattedString& _string, int _offset = 0, int _size = INT_MAX);

        FormattedStringView& operator=(const FormattedStringView& _other) = default;

        inline FormattedStringView mid(int _offset, int _size = -1) const { return { *string_, _offset + offset_, _size == -1 ? std::max(0, size_ - _offset) : _size }; }

        inline FormattedStringView left(int _size) const { return mid(0, _size); }

        [[nodiscard]] FormattedStringView trimmed() const;

        bool tryToAppend(const FormattedStringView& _other);

        bool tryToAppend(QChar _ch);

        bool tryToAppend(QStringView _text);

        QStringView string() const;

        bool hasFormatting() const;

        bool isEmpty() const { return size_ == 0 || string_->isEmpty(); }

        inline int size() const { return size_; }

        inline int indexOf(QChar _char) const { return string().indexOf(_char); }

        std::vector<core::data::format::format> getStyles() const;

        FormattedString toFormattedString() const;

    protected:
        [[nodiscard]] core::data::format::format_range cutRangeToFitView(core::data::format::format_range _range) const;

    protected:
        static const FormattedString emptyString;

        const FormattedString* string_ = nullptr;
        int offset_;
        int size_;
    };


    using MentionMap = std::map<QString, QString, Utils::StringComparator>; // uin - friendly
    using FilesPlaceholderMap = std::map<QString, QString, Utils::StringComparator>; // link - placeholder

    void serializeMentions(core::coll_helper& _collection, const Data::MentionMap& _mentions);

    class MessageBuddy
    {
    public:
        MessageBuddy();

        void ApplyModification(const MessageBuddy &modification);

        bool IsEmpty() const;

        bool CheckInvariant() const;

        bool ContainsGif() const;

        bool ContainsImage() const;

        bool ContainsVideo() const;

        bool ContainsMentions() const;

        bool GetIndentWith(const MessageBuddy &buddy, const bool _isMultichat) const;

        bool hasSenderNameWith(const MessageBuddy& _prevBuddy, const bool _isMultichat) const;

        bool hasAvatarWith(const MessageBuddy& _prevBuddy) const;

        bool isSameDirection(const MessageBuddy& _prevBuddy) const;

        bool isTimeGapWith(const MessageBuddy& _buddy) const;

        bool IsBase() const;

        bool IsChatEvent() const;

        bool IsDeleted() const;

        bool IsRestoredPatch() const;

        bool IsDeliveredToClient() const;

        bool IsDeliveredToServer() const;

        bool IsFileSharing() const;

        bool IsOutgoing() const;

        bool isOutgoingPosition() const;

        bool IsOutgoingVoip() const;

        bool IsSticker() const;

        bool IsPending() const;

        bool IsServiceMessage() const;

        bool IsVoipEvent() const;

        const HistoryControl::ChatEventInfoSptr& GetChatEvent() const;

        const QString& GetChatSender() const;

        const QDate& GetDate() const;

        const HistoryControl::FileSharingInfoSptr& GetFileSharing() const;

        bool GetIndentBefore() const;

        const HistoryControl::StickerInfoSptr& GetSticker() const;

        const FormattedString& GetSourceText() const;

        QString GetText() const;

        inline const FormattedString& getFormattedText() const { return Text_; }

        qint32 GetTime() const;

        qint64 GetLastId() const;

        core::message_type GetType() const;

        const common::tools::patch_version& GetUpdatePatchVersion() const;
        void SetUpdatePatchVersion(common::tools::patch_version version);

        const HistoryControl::VoipEventInfoSptr& GetVoipEvent() const;

        bool HasAvatar() const;

        bool HasSenderName() const;

        bool HasChatSender() const;

        bool isChainedToPrev() const;
        bool isChainedToNext() const;

        bool HasId() const;

        bool HasText() const;

        void FillFrom(const MessageBuddy &buddy);

        void EraseEventData();

        void SetChatEvent(const HistoryControl::ChatEventInfoSptr& chatEvent);

        void SetChatSender(const QString& chatSender);

        void SetDate(const QDate &date);

        void SetDeleted(const bool isDeleted);

        void SetRestoredPatch(const bool isRestoredPatch);

        void SetFileSharing(const HistoryControl::FileSharingInfoSptr& fileSharing);

        void SetHasAvatar(const bool hasAvatar);

        void SetHasSenderName(const bool _hasName);

        void setHasChainToPrev(const bool _hasChain);

        void setHasChainToNext(const bool _hasChain);

        void SetIndentBefore(const bool indentBefore);

        void SetLastId(const qint64 lastId);

        void SetOutgoing(const bool isOutgoing);

        void SetSticker(const HistoryControl::StickerInfoSptr &sticker);

        void SetFormatting(const core::data::format::string_formatting& _formatting);

        //void SetText(const QString &text);

        void SetText(const FormattedString& _text);

        void SetTime(const qint32 time);

        void SetType(const core::message_type type);

        void SetVoipEvent(const HistoryControl::VoipEventInfoSptr &voipEvent);

        Logic::MessageKey ToKey() const;

        bool IsEdited() const;

        static qint64 makePendingId(const QString& _internalId);

        void SetDescription(const QString& _description);

        void SetDescription(const FormattedString& _description);

        inline void SetDescriptionFormat(const core::data::format::string_formatting& _format) { description_.setFormatting(_format); }

        const FormattedString& GetDescription() const;

        void SetUrl(const QString& _url);
        const QString& GetUrl() const;

        bool needUpdateWith(const MessageBuddy& _buddy) const;

        QString getSender() const;

        void setUnsupported(bool _unsupported);
        bool isUnsupported() const;

        void setHideEdit(bool _hideEdit);
        bool hideEdit() const;

        void serialize(core::icollection* _collection) const;

        QString AimId_;
        QString InternalId_;
        qint64 Id_;
        qint64 Prev_;
        qint64 PendingId_;
        qint32 Time_;
        QVector<Quote> Quotes_;
        MentionMap Mentions_;
        std::vector<UrlSnippet> snippets_;
        FilesPlaceholderMap Files_;

        bool Chat_;
        QString ChatFriendly_;

        SharedContact sharedContact_;
        Poll poll_;
        Geo geo_;
        std::vector<std::vector<ButtonData>> buttons_;
        MessageReactions reactions_;

        //filled by model
        bool Unread_;
        bool Filled_;

    private:
        qint64 LastId_;

        FormattedString Text_;

        QString ChatSender_;

        QDate Date_;

        FormattedString description_;

        QString Url_;

        bool Deleted_;

        bool RestoredPatch_;

        bool DeliveredToClient_;

        bool HasAvatar_;

        bool HasSenderName_;

        bool isChainedToPrev_;

        bool isChainedToNext_;

        bool IndentBefore_;

        bool Outgoing_;

        bool Edited_;

        bool Unsupported_;

        bool HideEdit_;

        core::message_type Type_;

        common::tools::patch_version UpdatePatchVersion_;

        HistoryControl::FileSharingInfoSptr FileSharing_;

        HistoryControl::StickerInfoSptr Sticker_;

        HistoryControl::ChatEventInfoSptr ChatEvent_;

        HistoryControl::VoipEventInfoSptr VoipEvent_;
    };

    using MessageBuddySptr = std::shared_ptr<MessageBuddy>;
    using MessageBuddies = QVector<MessageBuddySptr>;
    using MessageBuddiesSptr = std::shared_ptr<MessageBuddies>;

    typedef std::pair<QString, QString> DlgStateHead;

    class DlgState
    {
    public:
        DlgState();

        bool operator==(const DlgState& other) const
        {
            return AimId_ == other.AimId_;
        }

        const QString& GetText() const;

        bool HasLastMsgId() const;

        bool HasText() const;

        void SetText(QString text);

        void updateTypings(const Logic::TypingFires& _typer, const bool _isTyping);

        QString getTypers() const;

        QString AimId_;
        qint64 UnreadCount_;
        qint64 LastReadMention_;
        qint64 LastMsgId_;
        qint64 YoursLastRead_;
        qint64 TheirsLastRead_;
        qint64 TheirsLastDelivered_;
        qint64 DelUpTo_;
        qint64 PinnedTime_;
        qint64 UnimportantTime_;
        qint32 Time_;
        bool Outgoing_;
        bool Chat_;
        bool Visible_;
        bool Official_;
        bool Attention_;
        bool IsLastMessageDelivered_;
        QString senderAimId_;
        QString LastMessageFriendly_;
        QString senderNick_;
        QString Friendly_;
        qint64 RequestId_;
        qint64 mentionMsgId_;
        QString MailId_;
        QString header_;
        std::optional<QString> infoVersion_;
        std::optional<QString> membersVersion_;

        bool hasMentionMe_;
        int unreadMentionsCount_;
        bool mentionAlert_;

        MessageBuddySptr pinnedMessage_;
        MessageBuddySptr lastMessage_;

        bool isSuspicious_;
        bool isStranger_;
        bool noRecentsUpdate_;

        std::vector<DlgStateHead> heads_;

        std::vector<Logic::TypingFires> typings_;

        Ui::MediaType mediaType_;

    private:
        QString Text_;
    };

    struct Quote
    {
        enum class Type
        {
            text = 1,
            link = 0,
            file_sharing = 2,
            quote = 3,
            other = 100
        };

        FormattedString text_;
        QString senderId_;
        QString chatId_;
        QString senderFriendly_;
        QString chatStamp_;
        QString chatName_;
        QString url_;
        FormattedString description_;
        QString quoterId_;    // if exists, it is an id of the user quoted the message
        qint32 time_;
        qint64 msgId_;
        int32_t setId_;
        int32_t stickerId_;
        bool isForward_;
        MentionMap mentions_;
        SharedContact sharedContact_;
        Poll poll_;
        Geo geo_;
        FilesPlaceholderMap files_;

        //gui only values
        bool isFirstQuote_;
        bool isLastQuote_;
        bool isSelectable_;

        Quote::Type type_;
        Ui::MediaType mediaType_;

        bool hasReply_;

        Quote()
            : time_(-1)
            , msgId_(-1)
            , setId_(-1)
            , stickerId_(-1)
            , isForward_(false)
            , isFirstQuote_(false)
            , isLastQuote_(false)
            , isSelectable_(false)
            , type_(Quote::Type::text)
            , mediaType_(Ui::MediaType::noMedia)
            , hasReply_(true)
        {
        }

        bool isEmpty() const { return text_.isEmpty() && !isSticker() && !sharedContact_ && !poll_; }
        bool isSticker() const { return setId_ != -1 && stickerId_ != -1; }
        bool isInteractive() const;

        void serialize(core::icollection* _collection) const;
        void unserialize(core::icollection* _collection);
    };

    using QuotesVec = QVector<Data::Quote>;

    void serializeQuotes(core::coll_helper& _collection, const Data::QuotesVec& _quotes);

    struct UrlSnippet
    {
        QString url_;
        QString contentType_;

        QString previewUrl_;
        QSize previewSize_;

        QString title_;
        QString description_;

        void unserialize(core::icollection* _collection);
    };

    struct MessagesResult
    {
        QString aimId;
        MessageBuddies messages;
        MessageBuddies introMessages;
        MessageBuddies modifications;
        qint64 lastMsgId;
        bool havePending;
    };

    MessagesResult UnserializeMessageBuddies(core::coll_helper* helper, const QString &myAimid);

    void unserializeMessages(
        core::iarray* _msgArray,
        const QString& _aimId,
        const QString& _myAimid,
        const qint64 _theirs_last_delivered,
        const qint64 _theirs_last_read,
        Out Data::MessageBuddies& _messages);


    Data::MessageBuddySptr unserializeMessage(
        core::coll_helper& _msgColl,
        const QString& _aimId,
        const QString& _myAimid,
        const qint64 _theirs_last_delivered,
        const qint64 _theirs_last_read);

    struct ServerMessagesIds
    {
        QString AimId_;

        QVector<qint64> AllIds_;

        Data::MessageBuddies Deleted_;

        Data::MessageBuddies Modifications_;
    };
    ServerMessagesIds UnserializeServerMessagesIds(const core::coll_helper& helper);

    DlgState UnserializeDlgState(core::coll_helper* helper, const QString &myAimId);

    struct CallInfo
    {
    public:
        CallInfo();
        CallInfo(const QString& _aimid, MessageBuddySptr _message);

        void addMessages(const std::vector<MessageBuddySptr>& _messages);
        void mergeMembers(const std::vector<QString>& _members);
        void calcCount();

        QString getFriendly() const;
        QString getServiceAimid() const;
        QString getButtonsText() const;

        const std::vector<QString>& getHightlights() const;
        const QString& getAimId() const;
        const std::vector<MessageBuddySptr>& getMessages() const;
        const std::vector<QString>& getMembers() const;

        qint32 time() const;
        int count() const;

        bool isSingleItem() const;
        bool isOutgoing() const;
        bool isService() const;
        bool isSpaceItem() const;
        bool isVideo() const;
        bool isMissed() const;

    private:
        HistoryControl::VoipEventInfoSptr GetVoipEvent() const;

    private:
        friend class Logic::CallsModel;
        friend class Logic::CallsSearchModel;

        QString VoipSid_;
        QString AimId_;
        std::vector<MessageBuddySptr> Messages_;
        std::vector<QString> Members_;

        QString ButtonsText_;
        QString ServiceAimid_;

        //for search
        std::vector<QString> Highlights_;
        int count_;
    };

    using CallInfoVec = std::vector<Data::CallInfo>;
    using CallInfoPtr = std::shared_ptr<Data::CallInfo>;

    CallInfo UnserializeCallInfo(core::coll_helper* helper);
    CallInfoVec UnserializeCallLog(core::coll_helper* helper);

    struct PatchedMessage
    {
        QString aimId_;
        std::vector<qint64> msgIds_;

        void unserialize(core::coll_helper& helper);
    };
}

Q_DECLARE_METATYPE(Data::MessageBuddy);
Q_DECLARE_METATYPE(Data::MessageBuddy*);
Q_DECLARE_METATYPE(Data::DlgState);
Q_DECLARE_METATYPE(Data::CallInfo);
Q_DECLARE_METATYPE(Data::CallInfoPtr);
Q_DECLARE_METATYPE(Data::PatchedMessage);
