#pragma once

#include "../../corelib/core_face.h"
#include "../../common.shared/patch_version.h"
#include "../../common.shared/message_processing/text_formatting.h"
#include "../types/typing.h"
#include "../main_window/mediatype.h"
#include "reactions.h"
#include "poll.h"
#include "thread.h"
#include "task.h"

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
    struct ParentTopic;

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

    Q_DECLARE_FLAGS(FormatTypes, core::data::format_type)
    Q_DECLARE_OPERATORS_FOR_FLAGS(FormatTypes)

    struct LinkInfo
    {
        QString url_;

        //! Not empty only for formatted links
        QString displayName_;

        //! Relative to GenericBlock
        QRect rect_;

        bool operator==(const LinkInfo& _other) { return url_ == _other.url_ && displayName_ == _other.displayName_ && rect_ == _other.rect_; }
        bool isEmpty() const { return url_.isEmpty(); }
        bool isFormattedLink() const { return !displayName_.isEmpty(); }
    };

    [[nodiscard]] constexpr QChar singleBackTick() noexcept { return u'`'; }
    [[nodiscard]] constexpr QStringView tripleBackTick() noexcept { return u"```"; }

    class FStringView;
    struct FString
    {
    public:
        class Builder
        {
        public:
            template <typename T>
            Builder& operator%=(const T& _part)
            {
                if (_part.size() == 0)
                    return *this;

                size_ += _part.size();
                parts_.emplace_back(_part);
                return *this;
            }

            template <typename T>
            Builder& operator%=(T&& _part)
            {
                if (_part.size() == 0)
                    return *this;

                size_ += _part.size();
                parts_.emplace_back(std::move(_part));
                return *this;
            }

            FString finalize(bool _mergeMentions = false);
            qsizetype size() const { return size_; }
            bool isEmpty() const { return parts_.empty(); }
        protected:
            std::vector<std::variant<FString, FStringView, QString, QStringView>> parts_;
            qsizetype size_ = 0;
        };
    public:
        FString() = default;
        FString(const QString& _string, const core::data::format& _format = {}, bool _doDebugValidityCheck = true);
        FString(const QString& _string, core::data::range_format&& _format)
            : string_(_string), format_(std::move(_format)) { }
        FString(QString&& _string, core::data::range_format&& _format)
            : string_(std::move(_string)), format_(std::move(_format)) { }
        FString(QString&& _string, core::data::format_type _type, core::data::format_data _data = {})
            : string_(std::move(_string)), format_({ _type, core::data::range{0, static_cast<int>(string_.size())}, _data }) { }

        bool hasFormatting() const { return !format_.empty(); }
        bool containsFormat(core::data::format_type _type) const;
        bool containsAnyFormatExceptFor(FormatTypes _types) const;
        void replace(QChar _old, QChar _new) { string_.replace(_old, _new); }
        bool isEmpty() const { return string_.isEmpty(); }
        bool isTrimmedEmpty() const { return QStringView(string_).trimmed().isEmpty(); }
        const QString& string() const { return string_; }
        const core::data::format& formatting() const { return format_; }
        core::data::format& formatting() { return format_; }
        int size() const { return string_.size(); }
        QChar at(int _pos) const { return string_.at(_pos); }

        template <typename T>
        Builder operator%(const T& _part) const { return (Builder() %= *this) %= _part; }

        bool startsWith(const QString& _prefix) const { return string_.startsWith(_prefix); }
        bool endsWith(const QString& _suffix) const { return string_.endsWith(_suffix); }

        bool startsWith(QChar _ch) const { return string_.startsWith(_ch); }
        bool endsWith(QChar _ch) const { return string_.endsWith(_ch); }

        int indexOf(QChar _ch, int _from = 0) const { return string_.indexOf(_ch, _from); }
        int lastIndexOf(QChar _char) const { return string_.lastIndexOf(_char); }

        bool operator!=(const FString& _other);

        void reserve(int _size) { string_.reserve(_size); }

        void chop(int _n);

        bool contains(const QString& _str) const { return string().contains(_str); }
        bool contains(QChar _ch) const { return string().contains(_ch); }

        FString& operator+=(QChar _ch) { string_ += _ch; return *this; }
        FString& operator+=(const QString& _other) { string_ += _other; return *this; }
        FString& operator+=(QStringView _other);
        FString& operator+=(const FString& _other);
        FString& operator+=(FStringView _other);

        void clear() { string_.clear(); format_.clear(); }
        void clearFormat() { format_.clear(); }
        void setFormatting(const core::data::format& _formatting);
        void addFormat(core::data::format_type _type);
        bool operator==(const FString& _other) const;
        bool operator!=(const FString& _other) const;

        bool isFormatValid() const;

        void removeAllBlockFormats();
        void removeMentionFormats();

    protected:
        QString string_;
        core::data::format format_;
    };

    void serializeFormat(const core::data::format& _format, core::icollection* _collection, std::string_view _name);
    void serializeFormat(const core::data::format& _format, core::coll_helper& _coll, std::string_view _name);

    //! NB: FormattedStringView doesn't take care of keeping any ownership
    class FStringView
    {
    public:
        FStringView() : string_(&emptyString), offset_(0), size_(0) {}
        FStringView(const FString& _string, int _offset = 0, int _size = -1);

        FStringView& operator=(const FStringView& _other) = default;

        bool hasFormatting() const;
        bool isEmpty() const { return size_ == 0 || string_->isEmpty(); }
        int size() const { return size_; }
        std::vector<core::data::range_format> getStyles() const;

        [[nodiscard]] FStringView trimmed() const;
        FStringView mid(int _offset, int _size = -1) const { return { *string_, _offset + offset_, _size == -1 ? std::max(0, size_ - _offset) : _size }; }
        FStringView left(int _size) const { return mid(0, _size); }
        FStringView right(int _size) const { return mid(size() - _size, _size); }
        void chop(int _n) { size_ -= qBound(0, _n, size_); }
        QChar at(int _pos) const { im_assert(_pos < size_);  return string_->at(offset_ + _pos); }
        QChar lastChar() const { return string().back(); }
        int indexOf(QChar _char, qsizetype _from = 0) const { return string().indexOf(_char, _from); }
        int indexOf(QStringView _string, qsizetype _from = 0) const { return string().indexOf(_string, _from); }
        int lastIndexOf(QChar _char) const { return string().lastIndexOf(_char); }
        bool startsWith(QStringView _prefix) const { return string().startsWith(_prefix); }
        bool endsWith(QStringView _prefix) const { return string().endsWith(_prefix); }

        std::vector<FStringView> splitByTypes(FormatTypes _types) const;

        FString toFString() const;

        //! Check if entire view has these format types
        bool isAnyOf(FormatTypes _types) const;

        bool operator==(QStringView _other) const { return string() == _other; }
        bool operator!=(QStringView _other) const { return !operator==(_other); }

        QStringView string() const;
        QString toString() const { return string().toString(); }
        const FString& sourceString() const { return *string_; }
        core::data::range sourceRange() const { return { offset_, size_ }; }

        //! Replace plain string and extend range if it occupied entire string
        FString replaceByString(QStringView _newString, bool _keepMentionFormat = true) const;

        bool tryToAppend(FStringView _other);
        bool tryToAppend(QChar _ch);
        bool tryToAppend(QStringView _text);

    protected:
        [[nodiscard]] core::data::range cutRangeToFitView(core::data::range _range) const;
        [[nodiscard]] core::data::format getFormat() const;

    protected:
        static const FString emptyString;

        const FString* string_ = nullptr;
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

        const FString& GetSourceText() const;

        QString GetText() const;

        const FString& getFormattedText() const { return Text_; }

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

        void SetFormatting(const core::data::format& _formatting);

        void SetText(const FString& _text);

        void SetText(FStringView _text);

        void SetTime(const qint32 time);

        void SetType(const core::message_type type);

        void SetVoipEvent(const HistoryControl::VoipEventInfoSptr &voipEvent);

        Logic::MessageKey ToKey() const;

        bool IsEdited() const;

        static qint64 makePendingId(const QString& _internalId);

        void SetDescription(const QString& _description);

        void SetDescription(const FString& _description);

        void SetDescriptionFormat(const core::data::format& _format) { description_.setFormatting(_format); }

        const FString& GetDescription() const;

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
        Task task_;
        Geo geo_;
        std::vector<std::vector<ButtonData>> buttons_;
        MessageReactions reactions_;
        MessageThreadData threadData_;

        //filled by model
        bool Unread_;
        bool Filled_;

    private:
        qint64 LastId_;

        FString Text_;

        QString ChatSender_;

        QDate Date_;

        FString description_;

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

    struct Draft
    {
        int64_t localTimestamp_ = 0;
        int32_t serverTimestamp_ = 0;
        bool synced_ = false;
        Data::MessageBuddy message_;

        bool isEmpty() const;
        void unserialize(core::coll_helper& helper, const QString& _aimId);
    };

    class DlgState
    {
    public:
        DlgState();

        enum class PinnedServiceItemType
        {
            NonPinnedItem,
            Favorites,
            ScheduledMessages,
            Threads,
            Reminders
        };

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

        PinnedServiceItemType pinnedServiceItemType() const;
        void setPinnedServiceItemType(PinnedServiceItemType _type);

        bool hasDraft() const;
        bool hasParentTopic() const;

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
        qint32 serverTime_;
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

        Draft draft_;
        QString draftText_;
        std::optional<std::shared_ptr<ParentTopic>> parentTopic_;

    private:
        QString Text_;
        PinnedServiceItemType pinnedServiceItemType_;
    };

    enum class ReplaceFilesPolicy
    {
        Keep,
        Replace,
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

        FString text_;
        QString senderId_;
        QString chatId_;
        QString senderFriendly_;
        QString chatStamp_;
        QString chatName_;
        QString url_;
        FString description_;
        QString quoterId_;    // if exists, it is an id of the user quoted the message
        qint32 time_;
        qint64 msgId_;
        int32_t setId_;
        int32_t stickerId_;
        bool isForward_;
        MentionMap mentions_;
        SharedContact sharedContact_;
        Poll poll_;
        Task task_;
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

        bool isEmpty() const { return text_.isEmpty() && !isSticker() && !sharedContact_ && !poll_ && !task_; }
        bool isSticker() const { return setId_ != -1 && stickerId_ != -1; }
        bool isInteractive() const;

        void serialize(core::icollection* _collection, ReplaceFilesPolicy _replacePolicy) const;
        void unserialize(core::icollection* _collection);
    };

    using QuotesVec = QVector<Data::Quote>;

    void serializeQuotes(core::coll_helper& _collection, const Data::QuotesVec& _quotes, ReplaceFilesPolicy _replacePolicy);

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
        const core::coll_helper& _msgColl,
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

QDebug operator<<(QDebug _debug, core::data::format_type _type);

QDebug operator<<(QDebug _debug, Data::FormatTypes _types);

QDebug operator<<(QDebug _debug, const Data::FString& _string);

Q_DECLARE_METATYPE(Data::MessageBuddy);
Q_DECLARE_METATYPE(Data::MessageBuddy*);
Q_DECLARE_METATYPE(Data::DlgState);
Q_DECLARE_METATYPE(Data::CallInfo);
Q_DECLARE_METATYPE(Data::CallInfoPtr);
Q_DECLARE_METATYPE(Data::PatchedMessage);
