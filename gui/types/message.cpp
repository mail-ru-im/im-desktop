#include "stdafx.h"

#include "message.h"
#include "../main_window/history_control/history/Message.h"

#include "../utils/gui_coll_helper.h"
#include "../../corelib/enumerations.h"
#include "../utils/log/log.h"
#include "../utils/UrlParser.h"
#include "../utils/utils.h"

#include "../main_window/history_control/FileSharingInfo.h"
#include "../main_window/history_control/StickerInfo.h"
#include "../main_window/history_control/ChatEventInfo.h"
#include "../main_window/history_control/VoipEventInfo.h"
#include "../main_window/history_control/history/MessageBuilder.h"

#include "../cache/avatars/AvatarStorage.h"
#include "../main_window/contact_list/ContactListModel.h"
#include "../main_window/friendly/FriendlyContainer.h"
#include "../my_info.h"
#include "../main_window/mediatype.h"

namespace
{
    constexpr std::chrono::seconds timeDiffForIndent = std::chrono::minutes(10);
    bool isTimeGapBetween(const Data::MessageBuddy& _first, const Data::MessageBuddy& _second)
    {
        return  std::abs(_first.GetTime() - _second.GetTime()) >= timeDiffForIndent.count();
    }

    bool containsSitePreviewUri(const QString &text, Out QStringRef &uri);
}

namespace Data
{

    void SharedContactData::serialize(core::icollection *_collection) const
    {
        core::coll_helper coll(_collection, false);
        Ui::gui_coll_helper helper(_collection->create_collection(), true);
        helper.set_value_as_qstring("name", name_);
        helper.set_value_as_qstring("phone", phone_);
        helper.set_value_as_qstring("sn", sn_);

        coll.set_value_as_collection("shared_contact", helper.get());
    }

    void SharedContactData::unserialize(core::icollection *_collection)
    {
        Ui::gui_coll_helper helper(_collection, false);
        name_ = helper.get<QString>("name");
        phone_ = helper.get<QString>("phone");
        sn_ = helper.get<QString>("sn");
    }


    DlgState::DlgState()
        : UnreadCount_(0)
        , LastReadMention_(-1)
        , LastMsgId_(-1)
        , YoursLastRead_(-1)
        , TheirsLastRead_(-1)
        , TheirsLastDelivered_(-1)
        , DelUpTo_(-1)
        , FavoriteTime_(-1)
        , Time_(-1)
        , Outgoing_(false)
        , Chat_(false)
        , Visible_(true)
        , Official_(false)
        , Attention_(false)
        , IsLastMessageDelivered_(true)
        , RequestId_(-1)
        , mentionMsgId_(-1)
        , hasMentionMe_(false)
        , unreadMentionsCount_(0)
        , mentionAlert_(false)
        , isSuspicious_(false)
        , isStranger_(false)
        , mediaType_(Ui::MediaType::noMedia)
    {
    }

    MessageBuddy::MessageBuddy()
        : Id_(-1)
        , Prev_(-1)
        , PendingId_(-1)
        , Time_(0)
        , Chat_(false)
        , Unread_(false)
        , Filled_(false)
        , LastId_(-1)
        , Deleted_(false)
        , DeliveredToClient_(false)
        , HasAvatar_(false)
        , HasSenderName_(false)
        , isChainedToPrev_(false)
        , isChainedToNext_(false)
        , IndentBefore_(false)
        , Outgoing_(false)
        , Edited_(false)
        , Type_(core::message_type::base)
    {
    }

    void MessageBuddy::ApplyModification(const MessageBuddy &modification)
    {
        assert(modification.Id_ == Id_);

        EraseEventData();

        if (modification.IsBase())
        {
            SetText(modification.GetText());

            Type_ = core::message_type::base;

            return;
        }

        if (modification.IsChatEvent())
        {
            const auto &chatEventInfo = *modification.GetChatEvent();
            const auto &eventText = chatEventInfo.formatEventText();
            assert(!eventText.isEmpty());

            SetText(eventText);
            SetChatEvent(modification.GetChatEvent());

            Type_ = core::message_type::chat_event;
            //Type_ = core::message_type::base;

            return;
        }

        assert(!"unexpected modification type");
    }

    bool MessageBuddy::IsEmpty() const
    {
        return ((Id_ == -1) && InternalId_.isEmpty());
    }

    bool MessageBuddy::CheckInvariant() const
    {
        if (Outgoing_)
        {
            if (!HasId() && InternalId_.isEmpty())
            {
                return false;
            }
        }
        else
        {
            if (!InternalId_.isEmpty())
            {
                return false;
            }
        }

        return true;
    }

    Logic::preview_type MessageBuddy::GetPreviewableLinkType() const
    {
        assert(Type_ > core::message_type::min);
        assert(Type_ < core::message_type::max);

        if (FileSharing_ || Sticker_ || ChatEvent_ || VoipEvent_)
        {
            return Logic::preview_type::none;
        }

        QStringRef uri;
        if (containsSitePreviewUri(Text_, Out uri))
        {
            return Logic::preview_type::site;
        }

        return Logic::preview_type::none;
    }

    bool MessageBuddy::ContainsAnyPreviewableLink() const
    {
        const auto previewLinkType = GetPreviewableLinkType();
        assert(previewLinkType > Logic::preview_type::min);
        assert(previewLinkType < Logic::preview_type::max);

        return (previewLinkType != Logic::preview_type::none);
    }

    bool MessageBuddy::ContainsPreviewableSiteLink() const
    {
        const auto previewLinkType = GetPreviewableLinkType();
        assert(previewLinkType > Logic::preview_type::min);
        assert(previewLinkType < Logic::preview_type::max);

        return (previewLinkType == Logic::preview_type::site);
    }

    bool MessageBuddy::ContainsGif() const
    {
        assert(Type_ > core::message_type::min);
        assert(Type_ < core::message_type::max);

        if (FileSharing_)
        {
            const auto isGif = FileSharing_->getContentType().is_gif();
            return isGif;
        }

        return false;
    }

    bool MessageBuddy::ContainsImage() const
    {
        assert(Type_ > core::message_type::min);
        assert(Type_ < core::message_type::max);

        if (FileSharing_)
        {
            const auto isImage = FileSharing_->getContentType().is_image();
            return isImage;
        }

        return false;
    }

    bool MessageBuddy::ContainsVideo() const
    {
        assert(Type_ > core::message_type::min);
        assert(Type_ < core::message_type::max);

        if (FileSharing_)
        {
            const auto isVideo = FileSharing_->getContentType().is_video();
            return isVideo;
        }

        return false;
    }

    bool MessageBuddy::ContainsMentions() const
    {
        return !Mentions_.empty();
    }

    bool MessageBuddy::GetIndentWith(const MessageBuddy& _buddy, const bool _isMultichat) const
    {
        if (_buddy.IsServiceMessage())
            return false;

        if (_buddy.IsChatEvent())
            return false;

        if (!IsOutgoing() && _buddy.IsOutgoing() && _isMultichat)
            return false;

        if (!isSameDirection(_buddy))
            return true;

        if (!_buddy.IsOutgoing() && !IsOutgoing() && ChatSender_ != _buddy.ChatSender_)
            return true;

        if (isTimeGapWith(_buddy))
            return true;

        return false;
    }

    bool MessageBuddy::isSameDirection(const MessageBuddy& _prevBuddy) const
    {
        if (IsVoipEvent() && _prevBuddy.IsVoipEvent())
        {
            return (IsOutgoingVoip() == _prevBuddy.IsOutgoingVoip());
        }
        else if (IsVoipEvent() && !_prevBuddy.IsVoipEvent())
        {
            return (IsOutgoingVoip() == _prevBuddy.IsOutgoing());
        }
        else if (!IsVoipEvent() && _prevBuddy.IsVoipEvent())
        {
            return (IsOutgoing() == _prevBuddy.IsOutgoingVoip());
        }

        return (IsOutgoing() == _prevBuddy.IsOutgoing());
    }

    bool MessageBuddy::isTimeGapWith(const MessageBuddy& _buddy) const
    {
        return isTimeGapBetween(*this, _buddy);
    }

    bool MessageBuddy::hasSenderNameWith(const MessageBuddy& _prevBuddy, const bool _isMultichat) const
    {
        if (_isMultichat)
        {
            if (IsChatEvent() == _prevBuddy.IsChatEvent())
            {
                if (GetChatSender() != _prevBuddy.GetChatSender())
                    return true;

                return isTimeGapWith(_prevBuddy);
            }

            return !IsChatEvent();
        }

        if (!isSameDirection(_prevBuddy))
            return true;

        if (_prevBuddy.IsChatEvent())
            return true;

        if (_prevBuddy.IsServiceMessage())
            return true;

        return false;
    }

    bool MessageBuddy::hasAvatarWith(const MessageBuddy & _prevBuddy) const
    {
        if (GetChatSender() != _prevBuddy.GetChatSender())
            return true;

        if (isTimeGapWith(_prevBuddy))
            return true;

        return false;
    }

    bool MessageBuddy::IsBase() const
    {
        assert(Type_ > core::message_type::min);
        assert(Type_ < core::message_type::max);

        return (Type_ == core::message_type::base);
    }

    bool MessageBuddy::IsChatEvent() const
    {
        assert(Type_ > core::message_type::min);
        assert(Type_ < core::message_type::max);

        return (Type_ == core::message_type::chat_event);
    }

    bool MessageBuddy::IsDeleted() const
    {
        return Deleted_;
    }

    bool MessageBuddy::IsDeliveredToClient() const
    {
        return DeliveredToClient_;
    }

    bool MessageBuddy::IsDeliveredToServer() const
    {
        return HasId();
    }

    bool MessageBuddy::IsFileSharing() const
    {
        assert(Type_ > core::message_type::min);
        assert(Type_ < core::message_type::max);

        return (Type_ == core::message_type::file_sharing);
    }

    bool MessageBuddy::IsOutgoing() const
    {
        return Outgoing_;
    }

    bool MessageBuddy::IsOutgoingVoip() const
    {
        if (!VoipEvent_)
        {
            return IsOutgoing();
        }

        return !VoipEvent_->isIncomingCall();
    }

    bool MessageBuddy::IsSticker() const
    {
        return (Type_ == core::message_type::sticker);
    }

    bool MessageBuddy::IsPending() const
    {
        return Id_ == -1 && !InternalId_.isEmpty();
    }

    bool MessageBuddy::IsServiceMessage() const
    {
        return (!IsBase() && !IsFileSharing() && !IsSticker() && !IsChatEvent() && !IsVoipEvent());
    }


    bool MessageBuddy::IsVoipEvent() const
    {
        return (VoipEvent_ != nullptr);
    }

    const HistoryControl::ChatEventInfoSptr& MessageBuddy::GetChatEvent() const
    {
        return ChatEvent_;
    }

    const QString& MessageBuddy::GetChatSender() const
    {
        return ChatSender_;
    }

    const QDate& MessageBuddy::GetDate() const
    {
        return Date_;
    }

    const HistoryControl::FileSharingInfoSptr& MessageBuddy::GetFileSharing() const
    {
        return FileSharing_;
    }

    QStringRef MessageBuddy::GetFirstSiteLinkFromText() const
    {
        QStringRef result;
        containsSitePreviewUri(GetText(), Out result);
        return result;
    }

    bool MessageBuddy::GetIndentBefore() const
    {
        return IndentBefore_;
    }

    const HistoryControl::StickerInfoSptr& MessageBuddy::GetSticker() const
    {
        return Sticker_;
    }

    const QString& MessageBuddy::GetText() const
    {
        return Text_;
    }

    qint32 MessageBuddy::GetTime() const
    {
        return Time_;
    }

    qint64 MessageBuddy::GetLastId() const
    {
        assert(LastId_ >= -1);

        return LastId_;
    }

    core::message_type MessageBuddy::GetType() const
    {
        assert(Type_ > core::message_type::min);
        assert(Type_ < core::message_type::max);

        return Type_;
    }

    const common::tools::patch_version& MessageBuddy::GetUpdatePatchVersion() const
    {
        return UpdatePatchVersion_;
    }

    void MessageBuddy::SetUpdatePatchVersion(common::tools::patch_version version)
    {
        UpdatePatchVersion_ = std::move(version);
        Edited_ = !UpdatePatchVersion_.is_empty() || UpdatePatchVersion_.get_offline_version() > 0;
    }

    const HistoryControl::VoipEventInfoSptr& MessageBuddy::GetVoipEvent() const
    {
        return VoipEvent_;
    }

    bool MessageBuddy::HasAvatar() const
    {
        return HasAvatar_;
    }

    bool MessageBuddy::HasSenderName() const
    {
        return HasSenderName_;
    }

    bool MessageBuddy::HasChatSender() const
    {
        return !ChatSender_.isEmpty();
    }

    bool MessageBuddy::isChainedToPrev() const
    {
        return isChainedToPrev_;
    }

    bool MessageBuddy::isChainedToNext() const
    {
        return isChainedToNext_;
    }

    bool MessageBuddy::HasId() const
    {
        return (Id_ != -1);
    }

    bool MessageBuddy::HasText() const
    {
        return !Text_.isEmpty();
    }

    void MessageBuddy::FillFrom(const MessageBuddy &buddy)
    {
        Text_ = buddy.GetText();

        SetLastId(buddy.Id_);
        Unread_ = buddy.Unread_;
        DeliveredToClient_ = buddy.DeliveredToClient_;
        Date_ = buddy.Date_;
        Deleted_ = buddy.Deleted_;
        Edited_ = buddy.Edited_;
        UpdatePatchVersion_ = buddy.UpdatePatchVersion_;

        SetFileSharing(buddy.GetFileSharing());
        SetSticker(buddy.GetSticker());
        SetChatEvent(buddy.GetChatEvent());
        SetVoipEvent(buddy.GetVoipEvent());

        SetHasAvatar(buddy.HasAvatar());
        SetHasSenderName(buddy.HasSenderName());

        setHasChainToPrev(buddy.isChainedToPrev());
        setHasChainToNext(buddy.isChainedToNext());

        Quotes_ = buddy.Quotes_;
        Mentions_ = buddy.Mentions_;
    }

    void MessageBuddy::EraseEventData()
    {
        FileSharing_.reset();
        Sticker_.reset();
        ChatEvent_.reset();
        VoipEvent_.reset();
    }

    void MessageBuddy::SetChatEvent(const HistoryControl::ChatEventInfoSptr& chatEvent)
    {
        assert(!chatEvent || (!Sticker_ && !FileSharing_ && !VoipEvent_));

        ChatEvent_ = chatEvent;
    }

    void MessageBuddy::SetChatSender(const QString& chatSender)
    {
        ChatSender_ = chatSender;
    }

    void MessageBuddy::SetDate(const QDate &date)
    {
        assert(date.isValid());

        Date_ = date;
    }

    void MessageBuddy::SetDeleted(const bool isDeleted)
    {
        Deleted_ = isDeleted;
    }

    void MessageBuddy::SetFileSharing(const HistoryControl::FileSharingInfoSptr& fileSharing)
    {
        assert(!fileSharing || (!Sticker_ && !ChatEvent_ && !VoipEvent_));

        FileSharing_ = fileSharing;
    }

    void MessageBuddy::SetHasAvatar(const bool hasAvatar)
    {
        HasAvatar_ = hasAvatar;
    }

    void MessageBuddy::SetHasSenderName(const bool _hasName)
    {
        HasSenderName_ = _hasName;
    }

    void MessageBuddy::setHasChainToPrev(const bool _hasChain)
    {
        isChainedToPrev_ = _hasChain;
    }

    void MessageBuddy::setHasChainToNext(const bool _hasChain)
    {
        isChainedToNext_ = _hasChain;
    }

    void MessageBuddy::SetIndentBefore(const bool indentBefore)
    {
        IndentBefore_ = indentBefore;
    }

    void MessageBuddy::SetLastId(const qint64 lastId)
    {
        assert(lastId >= -1);

        LastId_ = lastId;
    }

    void MessageBuddy::SetText(const QString &text)
    {
        Text_ = text;
    }

    void MessageBuddy::SetTime(const qint32 time)
    {
        Time_ = time;
    }

    void MessageBuddy::SetType(const core::message_type type)
    {
        assert(type > core::message_type::min);
        assert(type < core::message_type::max);

        Type_ = type;
    }

    void MessageBuddy::SetVoipEvent(const HistoryControl::VoipEventInfoSptr &voip)
    {
        assert(!voip || (!Sticker_ && !ChatEvent_ && !FileSharing_));

        VoipEvent_ = voip;
    }

    Logic::MessageKey MessageBuddy::ToKey() const
    {
        return Logic::MessageKey(Id_, Prev_, InternalId_, PendingId_, Time_, Type_, IsOutgoing(), GetPreviewableLinkType(), Logic::control_type::ct_message, Date_);
    }

    bool MessageBuddy::IsEdited() const
    {
        return Edited_;
    }

    qint64 MessageBuddy::makePendingId(const QString& _internalId)
    {
        const int pendingPos = _internalId.lastIndexOf(ql1c('-'));
        const auto pendingId = _internalId.rightRef(_internalId.size() - pendingPos - 1);
        return pendingId.toLongLong();
    }

    void MessageBuddy::SetDescription(const QString& _description)
    {
        Desription_ = _description;
    }

    QString MessageBuddy::GetDescription() const
    {
        return Desription_;
    }

    void MessageBuddy::SetUrl(const QString& _url)
    {
        Url_ = _url;
    }

    QString MessageBuddy::GetUrl() const
    {
        return Url_;
    }

    bool MessageBuddy::needUpdateWith(const MessageBuddy &_buddy) const
    {
        const auto snAdded = sharedContact_ && sharedContact_->sn_.isEmpty() && _buddy.sharedContact_ && !_buddy.sharedContact_->sn_.isEmpty();
        const auto needUpdate = GetUpdatePatchVersion() < _buddy.GetUpdatePatchVersion() || Prev_ != _buddy.Prev_ || snAdded;

        return needUpdate;
    }

    void MessageBuddy::SetOutgoing(const bool isOutgoing)
    {
        Outgoing_ = isOutgoing;

        if (!HasId() && Outgoing_)
        {
            assert(!InternalId_.isEmpty());
        }
    }

    void MessageBuddy::SetSticker(const HistoryControl::StickerInfoSptr &sticker)
    {
        assert(!sticker || (!FileSharing_ && !ChatEvent_));

        Sticker_ = sticker;
    }

    const QString& DlgState::GetText() const
    {
        return Text_;
    }

    bool DlgState::HasLastMsgId() const
    {
        assert(LastMsgId_ >= -1);

        return (LastMsgId_ > 0);
    }

    bool DlgState::HasText() const
    {
        return !Text_.isEmpty();
    }

    void DlgState::SetText(QString text)
    {
        Text_ = std::move(text);
    }

    void DlgState::updateTypings(const Logic::TypingFires& _typer, const bool _isTyping)
    {
        for (auto iter = typings_.begin(); iter != typings_.end(); ++iter)
        {
            if (iter->chatterAimId_ == _typer.chatterAimId_)
            {
                if (!_isTyping)
                {
                    typings_.erase(iter);
                }

                return;
            }
        }

        if (_isTyping)
            typings_.push_back(_typer);
    }

    QString DlgState::getTypers() const
    {
        QString res;

        for (const auto& _typer : typings_)
        {
            const QString name = Utils::replaceLine(_typer.chatterName_);

            auto tokens = name.split(ql1c(' '));

            if (!res.isEmpty())
                res += ql1s(", ");

            if (!tokens.isEmpty())
                res += tokens.at(0);
            else
                res += _typer.chatterName_;
        }

        return res;
    }

    void serializeMentions(core::coll_helper& _collection, const Data::MentionMap& _mentions)
    {
        if (!_mentions.empty())
        {
            core::ifptr<core::iarray> mentArray(_collection->create_array());
            mentArray->reserve(_mentions.size());
            for (const auto&[aimid, friendly] : _mentions)
            {
                core::ifptr<core::icollection> mentCollection(_collection->create_collection());
                Ui::gui_coll_helper coll(mentCollection.get(), false);
                coll.set_value_as_qstring("aimid", aimid);
                coll.set_value_as_qstring("friendly", friendly);

                core::ifptr<core::ivalue> val(_collection->create_value());
                val->set_as_collection(mentCollection.get());
                mentArray->push_back(val.get());
            }
            _collection.set_value_as_array("mentions", mentArray.get());
        }
    }

    void serializeQuotes(core::coll_helper& _collection, const Data::QuotesVec& _quotes)
    {
        if (!_quotes.isEmpty())
        {
            core::ifptr<core::iarray> quotesArray(_collection->create_array());
            quotesArray->reserve(_quotes.size());
            for (const auto& q : _quotes)
            {
                core::ifptr<core::icollection> quoteCollection(_collection->create_collection());
                q.serialize(quoteCollection.get());
                core::coll_helper coll(_collection->create_collection(), true);
                core::ifptr<core::ivalue> val(_collection->create_value());
                val->set_as_collection(quoteCollection.get());
                quotesArray->push_back(val.get());
            }
            _collection.set_value_as_array("quotes", quotesArray.get());
        }
    }

    MessagesResult UnserializeMessageBuddies(core::coll_helper* helper, const QString &myAimid)
    {
        assert(!myAimid.isEmpty());

        bool havePending = false;
        QString aimId = QString::fromUtf8(helper->get_value_as_string("contact"));
        MessageBuddies messages;
        MessageBuddies modifications;
        MessageBuddies introMessages;
        qint64 lastMsgId = -1;
        const auto resultExist = helper->is_value_exist("result");
        if (!resultExist || helper->get_value_as_bool("result"))
        {
            const auto theirs_last_delivered = helper->get_value_as_int64("theirs_last_delivered", -1);
            const auto theirs_last_read = helper->get_value_as_int64("theirs_last_read", -1);

            if (helper->is_value_exist("messages"))
            {
                auto msgArray = helper->get_value_as_array("messages");
                unserializeMessages(msgArray, aimId, myAimid, theirs_last_delivered, theirs_last_read, Out messages);
            }

            if (helper->is_value_exist("pending_messages"))
            {
                havePending = true;
                auto msgArray = helper->get_value_as_array("pending_messages");
                unserializeMessages(msgArray, aimId, myAimid, theirs_last_delivered, theirs_last_read, Out messages);
            }

            if (helper->is_value_exist("intro_messages"))
            {
                auto introArray = helper->get_value_as_array("intro_messages");
                unserializeMessages(introArray, aimId, myAimid, theirs_last_delivered, theirs_last_read, Out introMessages);
            }

            if (helper->is_value_exist("modified"))
            {
                auto modificationsArray = helper->get_value_as_array("modified");
                unserializeMessages(modificationsArray, aimId, myAimid, theirs_last_delivered, theirs_last_read, Out modifications);
            }

            if (helper->is_value_exist("last_msg_in_index"))
                lastMsgId = helper->get_value_as_int64("last_msg_in_index");
        }

        return { std::move(aimId), std::move(messages), std::move(introMessages), std::move(modifications), lastMsgId, havePending };
    }

    Data::MessageBuddySptr unserializeMessage(
        core::coll_helper &msgColl,
        const QString &aimId,
        const QString &myAimid,
        const qint64 theirs_last_delivered,
        const qint64 theirs_last_read)
    {
        auto message = std::make_shared<Data::MessageBuddy>();

        message->Id_ = msgColl.get_value_as_int64("id");
        message->InternalId_ = QString::fromUtf8(msgColl.get_value_as_string("internal_id"));
        message->Prev_ = msgColl.get_value_as_int64("prev_id");
        message->AimId_ = aimId;
        message->SetOutgoing(msgColl.get<bool>("outgoing"));
        message->SetDeleted(msgColl.get<bool>("deleted"));
        common::tools::patch_version patch_version;
        patch_version.set_version(std::string_view(msgColl.get_value_as_string("update_patch_version", "")));
        patch_version.set_offline_version(msgColl.get_value_as_int("offline_version"));
        message->SetUpdatePatchVersion(patch_version);

        if (message->IsOutgoing() && (message->Id_ != -1))
            message->Unread_ = (message->Id_ > theirs_last_read);

        if (message->Id_ == -1 && !message->InternalId_.isEmpty())
            message->PendingId_ = Data::MessageBuddy::makePendingId(message->InternalId_);

        const auto timestamp = msgColl.get<int32_t>("time");

        message->SetTime(timestamp);
        if (msgColl->is_value_exist("text"))
            message->SetText(QString::fromUtf8(msgColl.get_value_as_string("text")));
        message->SetDate(QDateTime::fromTime_t(message->GetTime()).date());
        if (msgColl->is_value_exist("description"))
            message->SetDescription(QString::fromUtf8(msgColl.get_value_as_string("description")));
        if (msgColl->is_value_exist("url"))
            message->SetUrl(QString::fromUtf8(msgColl.get_value_as_string("url")));

        __TRACE(
            "delivery",
            "unserialized message\n" <<
            "    id=                    <" << message->Id_ << ">\n" <<
            "    last_delivered=        <" << theirs_last_delivered << ">\n" <<
            "    outgoing=<" << logutils::yn(message->IsOutgoing()) << ">\n" <<
            "    notification_key=<" << message->InternalId_ << ">\n" <<
            "    delivered_to_client=<" << logutils::yn(message->IsDeliveredToClient()) << ">\n" <<
            "    delivered_to_server=<" << logutils::yn(message->Id_ != -1) << ">");

        if (msgColl.is_value_exist("chat"))
        {
            core::coll_helper chat(msgColl.get_value_as_collection("chat"), false);
            if (!chat->empty())
            {
                message->Chat_ = true;
                const QString sender = QString::fromUtf8(chat.get_value_as_string("sender"));
                message->SetChatSender(sender);
                message->ChatFriendly_ = QString::fromUtf8(chat.get_value_as_string("friendly"));
                if (message->ChatFriendly_.isEmpty() && sender != myAimid)
                    message->ChatFriendly_ = sender;
            }
        }

        if (msgColl.is_value_exist("file_sharing"))
        {
            core::coll_helper file_sharing(msgColl.get_value_as_collection("file_sharing"), false);

            message->SetType(core::message_type::file_sharing);
            message->SetFileSharing(std::make_shared<HistoryControl::FileSharingInfo>(file_sharing));
        }

        if (msgColl.is_value_exist("sticker"))
        {
            core::coll_helper sticker(msgColl.get_value_as_collection("sticker"), false);

            message->SetType(core::message_type::sticker);
            message->SetSticker(HistoryControl::StickerInfo::Make(sticker));
        }

        if (msgColl.is_value_exist("voip"))
        {
            core::coll_helper voip(msgColl.get_value_as_collection("voip"), false);

            message->SetType(core::message_type::voip_event);
            message->SetVoipEvent(
                HistoryControl::VoipEventInfo::Make(voip, timestamp)
            );
        }

        if (msgColl.is_value_exist("chat_event"))
        {
            assert(!message->IsChatEvent());

            core::coll_helper chat_event(msgColl.get_value_as_collection("chat_event"), false);

            message->SetType(core::message_type::chat_event);

            message->SetChatEvent(
                HistoryControl::ChatEventInfo::Make(
                    chat_event,
                    message->IsOutgoing(),
                    myAimid
                )
            );
        }

        if (msgColl->is_value_exist("quotes"))
        {
            core::iarray* quotes = msgColl.get_value_as_array("quotes");
            const auto size = quotes->size();
            message->Quotes_.reserve(size);
            for (auto i = 0; i < size; ++i)
            {
                Data::Quote q;
                q.unserialize(quotes->get_at(i)->get_as_collection());
                q.quoterId_ = message->Chat_ ? message->GetChatSender() : message->AimId_;
                message->Quotes_.push_back(std::move(q));
            }
        }

        if (msgColl->is_value_exist("mentions"))
        {
            core::iarray* ment = msgColl.get_value_as_array("mentions");
            for (auto i = 0; i < ment->size(); ++i)
            {
                const auto coll = ment->get_at(i)->get_as_collection();
                core::coll_helper ment_helper(coll, false);
                auto currentAimId = QString::fromUtf8(ment_helper.get_value_as_string("sn"));

                if (!currentAimId.isEmpty())
                {
                    const auto friendly = Logic::GetFriendlyContainer()->getFriendly(currentAimId);
                    message->Mentions_.emplace(std::move(currentAimId), friendly);
                }
            }
        }

        if (msgColl->is_value_exist("snippets"))
        {
            core::iarray* snippets = msgColl.get_value_as_array("snippets");
            const auto size = snippets->size();
            message->snippets_.reserve(size);
            for (auto i = 0; i < size; ++i)
            {
                Data::UrlSnippet s;
                s.unserialize(snippets->get_at(i)->get_as_collection());
                message->snippets_.push_back(std::move(s));
            }
        }

        if (msgColl->is_value_exist("shared_contact"))
        {
            auto contact_coll = msgColl.get_value_as_collection("shared_contact");
            SharedContactData contact;
            contact.unserialize(contact_coll);
            message->sharedContact_ = std::move(contact);
        }

        return message;
    }

    ServerMessagesIds UnserializeServerMessagesIds(const core::coll_helper& helper)
    {
        auto aimId = QString::fromUtf8(helper.get_value_as_string("contact"));
        if (helper.is_value_exist("result") && !helper.get_value_as_bool("result"))
        {
            return {};
        }

        QVector<qint64> ids;
        if (core::iarray* msgArray = helper.get_value_as_array("ids"); msgArray)
        {
            const int32_t size = msgArray->size();
            ids.reserve(size);
            for (int32_t i = 0; i < size; ++i)
                ids.push_back(msgArray->get_at(i)->get_as_int64());
        }

        MessageBuddies deleted;
        if (helper.is_value_exist("deleted"))
        {
            auto deletedArray = helper.get_value_as_array("deleted");
            const auto myAimid = helper.get<QString>("my_aimid");
            const auto theirs_last_delivered = helper.get_value_as_int64("theirs_last_delivered", -1);
            const auto theirs_last_read = helper.get_value_as_int64("theirs_last_read", -1);
            unserializeMessages(deletedArray, aimId, myAimid, theirs_last_delivered, theirs_last_read, Out deleted);
        }

        MessageBuddies modifications;
        if (helper.is_value_exist("modified"))
        {
            auto modificationsArray = helper.get_value_as_array("modified");
            const auto myAimid = helper.get<QString>("my_aimid");
            const auto theirs_last_delivered = helper.get_value_as_int64("theirs_last_delivered", -1);
            const auto theirs_last_read = helper.get_value_as_int64("theirs_last_read", -1);
            unserializeMessages(modificationsArray, aimId, myAimid, theirs_last_delivered, theirs_last_read, Out modifications);
        }

        return { std::move(aimId), std::move(ids), std::move(deleted), std::move(modifications) };
    }

    DlgState UnserializeDlgState(core::coll_helper* helper, const QString &myAimId)
    {
        DlgState state;
        state.AimId_ = helper->get<QString>("contact");
        state.UnreadCount_ = helper->get<int64_t>("unreads");
        state.LastReadMention_ = helper->get<int64_t>("last_read_mention");
        state.LastMsgId_ = helper->get<int64_t>("last_msg_id");
        state.YoursLastRead_ = helper->get<int64_t>("yours_last_read");
        state.TheirsLastRead_ = helper->get<int64_t>("theirs_last_read");
        state.TheirsLastDelivered_ = helper->get<int64_t>("theirs_last_delivered");
        state.DelUpTo_ = helper->get<int64_t>("del_up_to");
        state.Visible_ = helper->get<bool>("visible");
        state.Friendly_ = helper->get<QString>("friendly");
        state.Chat_ = helper->get<bool>("is_chat");
        state.Official_ = helper->get<bool>("official");
        state.Attention_ = helper->get<bool>("attention");
        state.unreadMentionsCount_ = helper->get<int32_t>("unread_mention_count");
        state.isSuspicious_ = helper->get<bool>("suspicious");
        state.isStranger_ = helper->get<bool>("stranger");

        if (helper->is_value_exist("favorite_time"))
            state.FavoriteTime_ = helper->get<int64_t>("favorite_time");

        if (helper->is_value_exist("message"))
        {
            core::coll_helper value(helper->get_value_as_collection("message"), false);

            auto messageBuddy = Data::unserializeMessage(value, state.AimId_, myAimId, state.TheirsLastDelivered_, state.TheirsLastRead_);

            state.hasMentionMe_ = (messageBuddy->Mentions_.find(myAimId) != messageBuddy->Mentions_.end());

            const auto serializeMessage = helper->get<bool>("serialize_message");
            if (serializeMessage)
            {
                auto recentsText = hist::MessageBuilder::formatRecentsText(*messageBuddy);
                state.mediaType_ = recentsText.mediaType_;
                state.SetText(std::move(recentsText.text_));

                state.mentionMsgId_ = messageBuddy->Id_;
                state.IsLastMessageDelivered_ = messageBuddy->HasId();
            }

            state.senderAimId_ = messageBuddy->GetChatSender();
            state.senderNick_ = messageBuddy->ChatFriendly_;
            state.Time_ = value.get<int32_t>("time");
            state.Outgoing_ = value.get<bool>("outgoing");
            state.lastMessage_ = std::move(messageBuddy);
        }

        if (helper->is_value_exist("pinned"))
        {
            core::coll_helper value(helper->get_value_as_collection("pinned"), false);
            state.pinnedMessage_ = Data::unserializeMessage(value, state.AimId_, myAimId, state.TheirsLastDelivered_, state.TheirsLastRead_);
        }

        if (helper->is_value_exist("heads"))
        {
            const auto heads_array = helper->get_value_as_array("heads");

            state.heads_.reserve(heads_array->size());

            for (int32_t i = 0; i < heads_array->size(); ++i)
            {
                const auto val_head = heads_array->get_at(i);

                core::coll_helper coll_head(val_head->get_as_collection(), false);

                QString headAimid = QString::fromUtf8(coll_head.get_value_as_string("aimid"));
                QString headFriendly = Logic::GetFriendlyContainer()->getFriendly(headAimid);

                state.heads_.emplace_back(std::move(headAimid), std::move(headFriendly));
            }
        }

        return state;
    }

    bool Quote::isInteractive() const
    {
        return !isForward_ || Logic::getContactListModel()->contains(chatId_);
    }

    void Quote::serialize(core::icollection* _collection) const
    {
        Ui::gui_coll_helper coll(_collection, false);
        coll.set_value_as_qstring("text", text_);
        coll.set_value_as_qstring("sender", senderId_);
        coll.set_value_as_qstring("chatId", chatId_);
        coll.set_value_as_qstring("senderFriendly", senderFriendly_);
        coll.set_value_as_int("time", time_);
        coll.set_value_as_int64("msg", msgId_);
        coll.set_value_as_bool("forward", isForward_);
        coll.set_value_as_int("setId", setId_);
        coll.set_value_as_int("stickerId", stickerId_);
        coll.set_value_as_qstring("stamp", chatStamp_);
        coll.set_value_as_qstring("chatName", chatName_);
        coll.set_value_as_qstring("url", url_);
        coll.set_value_as_qstring("description", description_);

        if (sharedContact_)
            sharedContact_->serialize(_collection);
    }

    void Quote::unserialize(core::icollection* _collection)
    {
        Ui::gui_coll_helper coll(_collection, false);
        if (coll->is_value_exist("text"))
            text_ = QString::fromUtf8(coll.get_value_as_string("text"));

        if (coll->is_value_exist("sender"))
            senderId_ = QString::fromUtf8(coll.get_value_as_string("sender"));

        if (coll->is_value_exist("chatId"))
            chatId_ = QString::fromUtf8(coll.get_value_as_string("chatId"));

        if (coll->is_value_exist("time"))
            time_ = coll.get_value_as_int("time");

        if (coll->is_value_exist("msg"))
            msgId_ = coll.get_value_as_int64("msg");

        if (!senderId_.isEmpty())
            senderFriendly_ = Logic::GetFriendlyContainer()->getFriendly(senderId_);

        if (coll->is_value_exist("forward"))
            isForward_ = coll.get_value_as_bool("forward");

        if (coll->is_value_exist("setId"))
            setId_ = coll.get_value_as_int("setId");

        if (coll->is_value_exist("stickerId"))
            stickerId_ = coll.get_value_as_int("stickerId");

        if (coll->is_value_exist("stamp"))
            chatStamp_ = QString::fromUtf8(coll.get_value_as_string("stamp"));

        if (coll->is_value_exist("url"))
            url_ = QString::fromUtf8(coll.get_value_as_string("url"));

        if (coll->is_value_exist("description"))
            description_ = QString::fromUtf8(coll.get_value_as_string("description"));

        if (!chatId_.isEmpty() && Utils::isChat(chatId_))
        {
            if (const auto name = Logic::GetFriendlyContainer()->getFriendly(chatId_); !name.isEmpty() && name != chatId_)
                chatName_ = name;
            else if (coll->is_value_exist("chatName"))
                chatName_ = QString::fromUtf8(coll.get_value_as_string("chatName"));
        }

        if (senderId_ == Ui::MyInfo()->aimId())
            senderFriendly_ = Ui::MyInfo()->friendly();

        if (coll.is_value_exist("shared_contact"))
        {
            auto contact_coll = coll.get_value_as_collection("shared_contact");
            SharedContactData contact;
            contact.unserialize(contact_coll);
            sharedContact_ = std::move(contact);
        }
    }

    void UrlSnippet::unserialize(core::icollection * _collection)
    {
        Ui::gui_coll_helper coll(_collection, false);
        const auto getString = [&coll](const auto& _name, auto& _field)
        {
            if (coll->is_value_exist(_name))
                _field = QString::fromUtf8(coll.get_value_as_string(_name));
        };

        getString("url", url_);
        getString("content_type", contentType_);
        getString("preview_url", previewUrl_);
        getString("title", title_);
        getString("description", description_);

        if (coll->is_value_exist("preview_width"))
            previewSize_.setWidth(coll.get_value_as_int("preview_width"));
        if (coll->is_value_exist("preview_height"))
            previewSize_.setHeight(coll.get_value_as_int("preview_height"));
    }
}

namespace Data
{
    void unserializeMessages(
        core::iarray* msgArray,
        const QString &aimId,
        const QString &myAimid,
        const qint64 theirs_last_delivered,
        const qint64 theirs_last_read,
        Out Data::MessageBuddies &messages)
    {
        assert(!aimId.isEmpty());
        assert(!myAimid.isEmpty());
        assert(msgArray);

        __TRACE(
            "delivery",
            "unserializing messages collection\n" <<
            "    size=<" << msgArray->size() << ">\n" <<
            "    last_delivered=<" << theirs_last_delivered << ">");

        const auto size = msgArray->size();
        messages.reserve(messages.size() + size);
        for (int32_t i = 0; i < size; ++i)
        {
            core::coll_helper value(
                msgArray->get_at(i)->get_as_collection(),
                false
            );

            messages.push_back(Data::unserializeMessage(value, aimId, myAimid, theirs_last_delivered, theirs_last_read));
        }
    }
}

namespace
{
    bool containsSitePreviewUri(const QString &text, Out QStringRef &uri)
    {
        Out uri = QStringRef();

        static const QRegularExpression space(
            qsl("\\s|\\n|\\r"),
            QRegularExpression::UseUnicodePropertiesOption | QRegularExpression::OptimizeOnFirstUsageOption
        );

        const auto parts = text.splitRef(space, QString::SkipEmptyParts);

        for (const auto &part : parts)
        {
            Utils::UrlParser parser;

            parser.process(part);

            if (parser.hasUrl())
            {
                uri = part;
                return true;
            }
        }

        return false;
    }
}
