#include "stdafx.h"

#include "message.h"
#include "../main_window/history_control/history/Message.h"

#include "../utils/gui_coll_helper.h"
#include "../../corelib/enumerations.h"
#include "../../common.shared/config/config.h"
#include "../utils/log/log.h"
#include "../utils/UrlParser.h"
#include "../utils/utils.h"
#include "../utils/features.h"

#include "../main_window/history_control/FileSharingInfo.h"
#include "../main_window/history_control/StickerInfo.h"
#include "../main_window/history_control/ChatEventInfo.h"
#include "../main_window/history_control/VoipEventInfo.h"
#include "../main_window/history_control/history/MessageBuilder.h"

#include "../cache/avatars/AvatarStorage.h"
#include "../main_window/contact_list/ContactListModel.h"
#include "../main_window/containers/FriendlyContainer.h"
#include "../my_info.h"
#include "../main_window/mediatype.h"
#include "contact.h"

namespace
{
    constexpr std::chrono::seconds timeDiffForIndent = std::chrono::minutes(10);
    bool isTimeGapBetween(const Data::MessageBuddy& _first, const Data::MessageBuddy& _second)
    {
        return std::abs(_first.GetTime() - _second.GetTime()) >= timeDiffForIndent.count();
    }
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

    void GeoData::serialize(core::icollection *_collection) const
    {
        core::coll_helper coll(_collection, false);
        Ui::gui_coll_helper helper(_collection->create_collection(), true);
        helper.set_value_as_qstring("name", name_);
        helper.set_value_as_double("lat", lat_);
        helper.set_value_as_double("long", long_);

        coll.set_value_as_collection("geo", helper.get());
    }

    void GeoData::unserialize(core::icollection *_collection)
    {
        Ui::gui_coll_helper helper(_collection, false);
        name_ = helper.get<QString>("name");
        lat_ = helper.get_value_as_double("lat");
        long_ = helper.get_value_as_double("long");
    }

    void ButtonData::unserialize(core::icollection* _collection)
    {
        Ui::gui_coll_helper helper(_collection, false);
        text_ = helper.get<QString>("text");
        url_ = helper.get<QString>("url");
        callbackData_ = helper.get<QString>("callback_data");

        auto style_str = helper.get<QString>("style");
        if (style_str == u"attention")
            style_ = style::ATTENTION;
        else if (style_str == u"primary")
            style_ = style::PRIMARY;
        else
            style_ = style::BASE;
    }

    ButtonData::style ButtonData::getStyle() const
    {
        return style_;
    }

    DlgState::DlgState()
        : UnreadCount_(0)
        , LastReadMention_(-1)
        , LastMsgId_(-1)
        , YoursLastRead_(-1)
        , TheirsLastRead_(-1)
        , TheirsLastDelivered_(-1)
        , DelUpTo_(-1)
        , PinnedTime_(-1)
        , UnimportantTime_(-1)
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
        , noRecentsUpdate_(false)
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
        , RestoredPatch_(false)
        , DeliveredToClient_(false)
        , HasAvatar_(false)
        , HasSenderName_(false)
        , isChainedToPrev_(false)
        , isChainedToNext_(false)
        , IndentBefore_(false)
        , Outgoing_(false)
        , Edited_(false)
        , Unsupported_(false)
        , HideEdit_(false)
        , Type_(core::message_type::base)
    {
    }

    void MessageBuddy::ApplyModification(const MessageBuddy &modification)
    {
        assert(modification.Id_ == Id_);

        EraseEventData();

        if (modification.IsBase())
        {
            SetText(modification.GetSourceText());

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
                return false;
        }
        else
        {
            if (!InternalId_.isEmpty())
                return false;
        }

        return true;
    }

    bool MessageBuddy::ContainsGif() const
    {
        assert(Type_ > core::message_type::min);
        assert(Type_ < core::message_type::max);

        if (FileSharing_)
            return FileSharing_->getContentType().is_gif();

        return false;
    }

    bool MessageBuddy::ContainsImage() const
    {
        assert(Type_ > core::message_type::min);
        assert(Type_ < core::message_type::max);

        if (FileSharing_)
            return FileSharing_->getContentType().is_image();

        return false;
    }

    bool MessageBuddy::ContainsVideo() const
    {
        assert(Type_ > core::message_type::min);
        assert(Type_ < core::message_type::max);

        if (FileSharing_)
            return FileSharing_->getContentType().is_video();

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

    bool MessageBuddy::IsRestoredPatch() const
    {
        return RestoredPatch_;
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

    bool MessageBuddy::isOutgoingPosition() const
    {
        return IsOutgoing() && !Logic::getContactListModel()->isChannel(AimId_);
    }

    bool MessageBuddy::IsOutgoingVoip() const
    {
        if (!VoipEvent_)
            return IsOutgoing();

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

    bool MessageBuddy::GetIndentBefore() const
    {
        return IndentBefore_;
    }

    const HistoryControl::StickerInfoSptr& MessageBuddy::GetSticker() const
    {
        return Sticker_;
    }

    const QString& MessageBuddy::GetSourceText() const
    {
        return Text_;
    }

    QString MessageBuddy::GetText() const
    {
        return (Sticker_ ? QT_TRANSLATE_NOOP("message", "Sticker") : Text_.trimmed());
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
        Text_ = buddy.GetSourceText();

        SetLastId(buddy.Id_);
        Unread_ = buddy.Unread_;
        DeliveredToClient_ = buddy.DeliveredToClient_;
        Date_ = buddy.Date_;
        Deleted_ = buddy.Deleted_;
        RestoredPatch_ = buddy.RestoredPatch_;
        Edited_ = buddy.Edited_;
        Unsupported_ = buddy.Unsupported_;
        HideEdit_ = buddy.HideEdit_;
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

    void MessageBuddy::SetRestoredPatch(const bool isRestoredPatch)
    {
        RestoredPatch_ = isRestoredPatch;
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
        return Logic::MessageKey(Id_, Prev_, InternalId_, PendingId_, Time_, Type_, IsOutgoing(), Logic::control_type::ct_message, Date_);
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

    const QString& MessageBuddy::GetDescription() const
    {
        return Desription_;
    }

    void MessageBuddy::SetUrl(const QString& _url)
    {
        Url_ = _url;
    }

    const QString& MessageBuddy::GetUrl() const
    {
        return Url_;
    }

    bool MessageBuddy::needUpdateWith(const MessageBuddy &_buddy) const
    {
        const auto snAdded = sharedContact_ && sharedContact_->sn_.isEmpty() && _buddy.sharedContact_ && !_buddy.sharedContact_->sn_.isEmpty();
        const auto pollIdAdded = poll_ && poll_->id_.isEmpty() && _buddy.poll_ && !_buddy.poll_->id_.isEmpty();
        const auto needUpdate = GetUpdatePatchVersion() < _buddy.GetUpdatePatchVersion() || Prev_ != _buddy.Prev_ || snAdded || pollIdAdded;

        return needUpdate;
    }

    QString MessageBuddy::getSender() const
    {
        return (Chat_ && HasChatSender()) ? normalizeAimId(ChatSender_) : AimId_;
    }

    void MessageBuddy::setUnsupported(bool _unsupported)
    {
        Unsupported_ = _unsupported;
    }

    bool MessageBuddy::isUnsupported() const
    {
        return Unsupported_;
    }

    void MessageBuddy::setHideEdit(bool _hideEdit)
    {
        HideEdit_ = _hideEdit;
    }

    bool MessageBuddy::hideEdit() const
    {
        return HideEdit_;
    }

    void MessageBuddy::SetOutgoing(const bool isOutgoing)
    {
        Outgoing_ = isOutgoing;
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
        auto isChatterAimId = [&_typer](const auto& _x) { return _x.chatterAimId_ == _typer.chatterAimId_; };
        if (_isTyping)
        {
            if (std::none_of(typings_.begin(), typings_.end(), isChatterAimId))
                typings_.push_back(_typer);
        }
        else
        {
            const auto it = std::find_if(typings_.begin(), typings_.end(), isChatterAimId);
            if (it != typings_.end())
                typings_.erase(it);
        }
    }

    QString DlgState::getTypers() const
    {
        QString res;
        const auto joiner = ql1s(", ");
        for (const auto& _typer : typings_)
        {
            const QString name = Utils::replaceLine(_typer.chatterName_);
            if (const auto idx = name.indexOf(u' '); idx != -1)
                res += name.leftRef(idx);
            else
                res += _typer.chatterName_;
            res += joiner;
        }

        if (!res.isEmpty())
            res.chop(joiner.size());
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
        core::coll_helper& _msgColl,
        const QString& _aimId,
        const QString& _myAimid,
        const qint64 _theirs_last_delivered,
        const qint64 _theirs_last_read)
    {
        auto message = std::make_shared<Data::MessageBuddy>();

        message->Id_ = _msgColl.get_value_as_int64("id");
        message->InternalId_ = QString::fromUtf8(_msgColl.get_value_as_string("internal_id"));
        message->Prev_ = _msgColl.get_value_as_int64("prev_id");
        message->AimId_ = _aimId;
        message->SetOutgoing(_msgColl.get<bool>("outgoing"));
        message->SetDeleted(_msgColl.get<bool>("deleted"));
        message->SetRestoredPatch(_msgColl.get<bool>("restored_patch"));
        common::tools::patch_version patch_version;
        patch_version.set_version(std::string_view(_msgColl.get_value_as_string("update_patch_version", "")));
        patch_version.set_offline_version(_msgColl.get_value_as_int("offline_version"));
        message->SetUpdatePatchVersion(patch_version);

        if (message->IsOutgoing() && (message->Id_ != -1))
            message->Unread_ = (message->Id_ > _theirs_last_read);

        if (message->Id_ == -1 && !message->InternalId_.isEmpty())
            message->PendingId_ = Data::MessageBuddy::makePendingId(message->InternalId_);

        const auto timestamp = _msgColl.get<int32_t>("time");

        message->SetTime(timestamp);
        if (_msgColl->is_value_exist("text"))
            message->SetText(QString::fromUtf8(_msgColl.get_value_as_string("text")));
        message->SetDate(QDateTime::fromSecsSinceEpoch(message->GetTime()).date());
        if (_msgColl->is_value_exist("description"))
            message->SetDescription(QString::fromUtf8(_msgColl.get_value_as_string("description")));
        if (_msgColl->is_value_exist("url"))
            message->SetUrl(QString::fromUtf8(_msgColl.get_value_as_string("url")));

        __TRACE(
            "delivery",
            "unserialized message\n" <<
            "    id=                    <" << message->Id_ << ">\n" <<
            "    last_delivered=        <" << _theirs_last_delivered << ">\n" <<
            "    outgoing=<" << logutils::yn(message->IsOutgoing()) << ">\n" <<
            "    notification_key=<" << message->InternalId_ << ">\n" <<
            "    delivered_to_client=<" << logutils::yn(message->IsDeliveredToClient()) << ">\n" <<
            "    delivered_to_server=<" << logutils::yn(message->Id_ != -1) << ">");

        if (_msgColl.is_value_exist("chat"))
        {
            core::coll_helper chat(_msgColl.get_value_as_collection("chat"), false);
            if (!chat->empty())
            {
                message->Chat_ = true;
                const QString sender = QString::fromUtf8(chat.get_value_as_string("sender"));
                message->SetChatSender(sender);
                message->ChatFriendly_ = QString::fromUtf8(chat.get_value_as_string("friendly"));
                if (message->ChatFriendly_.isEmpty() && sender != _myAimid)
                    message->ChatFriendly_ = sender;
            }
        }

        if (_msgColl->is_value_exist("unsupported") && _msgColl.get_value_as_bool("unsupported"))
        {
            const auto url = Features::updateAppUrl();
            auto text = QT_TRANSLATE_NOOP("message", "The message is not supported on your version of ICQ. Update the app to see the message");
            message->SetText(url.isEmpty() ? text : text % QChar::Space % url);
            message->SetUrl(QString());
            message->SetDescription(QString());
            message->setUnsupported(true);
            return message;
        }

        if (_msgColl.is_value_exist("file_sharing"))
        {
            core::coll_helper file_sharing(_msgColl.get_value_as_collection("file_sharing"), false);

            message->SetType(core::message_type::file_sharing);
            message->SetFileSharing(std::make_shared<HistoryControl::FileSharingInfo>(file_sharing));
        }

        if (_msgColl.is_value_exist("sticker"))
        {
            core::coll_helper sticker(_msgColl.get_value_as_collection("sticker"), false);

            message->SetType(core::message_type::sticker);
            message->SetSticker(HistoryControl::StickerInfo::Make(sticker));
        }

        if (_msgColl.is_value_exist("voip"))
        {
            core::coll_helper voip(_msgColl.get_value_as_collection("voip"), false);

            message->SetType(core::message_type::voip_event);
            message->SetVoipEvent(
                HistoryControl::VoipEventInfo::Make(voip, timestamp)
            );
        }

        if (_msgColl.is_value_exist("chat_event"))
        {
            assert(!message->IsChatEvent());

            core::coll_helper chat_event(_msgColl.get_value_as_collection("chat_event"), false);

            message->SetType(core::message_type::chat_event);

            message->SetChatEvent(
                HistoryControl::ChatEventInfo::make(
                    chat_event,
                    message->IsOutgoing(),
                    _myAimid,
                    message->AimId_
                )
            );
        }

        if (_msgColl->is_value_exist("quotes"))
        {
            core::iarray* quotes = _msgColl.get_value_as_array("quotes");
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

        if (_msgColl->is_value_exist("mentions"))
        {
            core::iarray* ment = _msgColl.get_value_as_array("mentions");
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

        if (_msgColl->is_value_exist("snippets"))
        {
            core::iarray* snippets = _msgColl.get_value_as_array("snippets");
            const auto size = snippets->size();
            message->snippets_.reserve(size);
            for (auto i = 0; i < size; ++i)
            {
                Data::UrlSnippet s;
                s.unserialize(snippets->get_at(i)->get_as_collection());
                message->snippets_.push_back(std::move(s));
            }
        }

        if (_msgColl->is_value_exist("shared_contact"))
        {
            auto contact_coll = _msgColl.get_value_as_collection("shared_contact");
            SharedContactData contact;
            contact.unserialize(contact_coll);
            message->sharedContact_ = std::move(contact);
        }

        if (_msgColl->is_value_exist("geo"))
        {
            auto geo_coll = _msgColl.get_value_as_collection("geo");
            GeoData geo;
            geo.unserialize(geo_coll);
            message->geo_ = std::move(geo);
        }

        if (_msgColl->is_value_exist("poll"))
        {
            auto poll_coll = _msgColl.get_value_as_collection("poll");
            PollData poll;
            poll.unserialize(poll_coll);
            message->poll_ = std::move(poll);
        }

        if (_msgColl->is_value_exist("reactions"))
        {
            auto reactions_coll = _msgColl.get_value_as_collection("reactions");
            MessageReactionsData reactions;
            reactions.unserialize(reactions_coll);
            message->reactions_ = std::move(reactions);
        }

        if (_msgColl->is_value_exist("buttons"))
        {
            core::iarray* buttons = _msgColl.get_value_as_array("buttons");
            const auto size = buttons->size();
            message->buttons_.reserve(size);
            for (auto i = 0; i < size; ++i)
            {
                std::vector<ButtonData> btns;
                core::iarray* line = (const_cast<core::ivalue*>(buttons->get_at(i)))->get_as_array();
                for (int j = 0; j < line->size(); ++j)
                {
                    Data::ButtonData data;
                    data.unserialize(line->get_at(j)->get_as_collection());
                    btns.push_back(std::move(data));
                }
                message->buttons_.push_back(std::move(btns));
            }
        }

        if (_msgColl->is_value_exist("hide_edit"))
            message->setHideEdit(_msgColl.get_value_as_bool("hide_edit"));

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
        state.noRecentsUpdate_ = helper->get<bool>("no_recents_update");

        if (helper->is_value_exist("pinned_time"))
            state.PinnedTime_ = helper->get<int64_t>("pinned_time");

        if (helper->is_value_exist("unimportant_time"))
            state.UnimportantTime_ = helper->get<int64_t>("unimportant_time");

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

        if (helper->is_value_exist("members_version"))
            state.membersVersion_ = helper->get<QString>("members_version");

        if (helper->is_value_exist("info_version"))
            state.infoVersion_ = helper->get<QString>("info_version");

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

        if (geo_)
            geo_->serialize(_collection);

        if (poll_)
            poll_->serialize(_collection);
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
            if (coll->is_value_exist("chatName") && !std::string_view(coll.get_value_as_string("chatName")).empty() || Logic::getContactListModel()->contains(chatId_))
            {
                if (const auto name = Logic::GetFriendlyContainer()->getFriendly(chatId_); !name.isEmpty() && name != chatId_)
                    chatName_ = name;
            }
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

        if (coll.is_value_exist("geo"))
        {
            auto geo_coll = coll.get_value_as_collection("geo");
            GeoData geo;
            geo.unserialize(geo_coll);
            geo_ = std::move(geo);
        }

        if (coll.is_value_exist("poll"))
        {
            auto poll_coll = coll.get_value_as_collection("poll");
            PollData poll;
            poll.unserialize(poll_coll);
            poll_ = std::move(poll);
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

    void PatchedMessage::unserialize(core::coll_helper& helper)
    {
        aimId_ = helper.get<QString>("contact");
        const auto msgArray = helper.get_value_as_array("msg_ids");
        const int32_t size = msgArray->size();
        msgIds_.reserve(size);
        for (int32_t i = 0; i < size; ++i)
            msgIds_.push_back(msgArray->get_at(i)->get_as_int64());
    }
}

namespace Data
{
    void unserializeMessages(
        core::iarray* _msgArray,
        const QString& _aimId,
        const QString& _myAimid,
        const qint64 _theirs_last_delivered,
        const qint64 _theirs_last_read,
        Out Data::MessageBuddies &messages)
    {
        assert(!_aimId.isEmpty());
        assert(!_myAimid.isEmpty());
        assert(_msgArray);

        __TRACE(
            "delivery",
            "unserializing messages collection\n" <<
            "    size=<" << _msgArray->size() << ">\n" <<
            "    last_delivered=<" << _theirs_last_delivered << ">");

        const auto size = _msgArray->size();
        messages.reserve(messages.size() + size);
        for (int32_t i = 0; i < size; ++i)
        {
            core::coll_helper value(
                _msgArray->get_at(i)->get_as_collection(),
                false
            );

            messages.push_back(Data::unserializeMessage(value, _aimId, _myAimid, _theirs_last_delivered, _theirs_last_read));
        }
    }

    CallInfo::CallInfo()
        : count_(1)
    {
    }

    CallInfo::CallInfo(const QString& _aimid, MessageBuddySptr _message)
        : count_(1)
    {
        AimId_ = _aimid;
        Messages_.emplace_back(std::move(_message));

        if (auto voip = GetVoipEvent())
        {
            VoipSid_ = voip->getSid();
            Members_ = voip->getConferenceMembers();
        }

        if (std::find(Members_.begin(), Members_.end(), _aimid) == Members_.end())
            Members_.push_back(_aimid);

        if (!isSingleItem())
            Members_.erase(std::remove(Members_.begin(), Members_.end(), Ui::MyInfo()->aimId()), Members_.end());

        std::sort(Members_.begin(), Members_.end());
    }

    void CallInfo::addMessages(const std::vector<MessageBuddySptr>& _messages)
    {
        Messages_.insert(Messages_.begin(), _messages.begin(), _messages.end());
        std::sort(Messages_.begin(), Messages_.end(), [](const auto& iter1, const auto& iter2) { return iter1->Time_ < iter2->Time_; });
    }

    void CallInfo::mergeMembers(const std::vector<QString>& _members)
    {
        if (Members_ != _members)
        {
            Members_.insert(Members_.end(), _members.begin(), _members.end());
            std::sort(Members_.begin(), Members_.end());
            Members_.erase(std::unique(Members_.begin(), Members_.end()), Members_.end());
        }
    }

    void CallInfo::calcCount()
    {
        if (isSingleItem())
        {
            count_ = Messages_.size();
        }
        else
        {
            std::map<QString, int> counts;
            for (const auto& m : Messages_)
            {
                const auto& aimid = m->AimId_;
                if (std::find_if(counts.begin(), counts.end(), [aimid](const auto& iter) { return iter.first == aimid; }) == counts.end())
                    counts[aimid] = 1;
                else
                    ++counts[aimid];
            }

            count_ = 0;
            for (const auto& c : counts)
                count_ = std::max(count_, c.second);
        }
    }

    QString CallInfo::getFriendly(bool _withNumbers) const
    {
        QString result;

        for (const auto& id : Members_)
        {
            result += Logic::GetFriendlyContainer()->getFriendly(id);
            result += ql1s(", ");
        }

        result = result.mid(0, result.size() - 2);

        if (count_ > 1 && _withNumbers)
            result += qsl(" (%1)").arg(count_);

        return result;
    }

    const QString& CallInfo::getAimId() const
    {
        return AimId_;
    }

    QString CallInfo::getServiceAimid() const
    {
        return ServiceAimid_;
    }

    QString CallInfo::getButtonsText() const
    {
        return ButtonsText_;
    }

    HistoryControl::VoipEventInfoSptr CallInfo::GetVoipEvent() const
    {
        if (Messages_.empty() || !Messages_.front())
            return nullptr;

        return Messages_.front()->GetVoipEvent();
    }

    const std::vector<QString>& CallInfo::getHightlights() const
    {
        return Highlights_;
    }

    const std::vector<MessageBuddySptr>& CallInfo::getMessages() const
    {
        return Messages_;
    }

    const std::vector<QString>& CallInfo::getMembers() const
    {
        return Members_;
    }

    qint32 CallInfo::time() const
    {
        if (Messages_.empty())
            return -1;

        return Messages_.begin()->get()->Time_;
    }

    bool CallInfo::isSingleItem() const
    {
        return Members_.size() == 1;
    }

    bool CallInfo::isOutgoing() const
    {
        if (Messages_.empty())
            return false;

        return Messages_.begin()->get()->IsOutgoing();
    }

    bool CallInfo::isService() const
    {
        return !ServiceAimid_.isEmpty();
    }

    bool CallInfo::isSpaceItem() const
    {
        return isService() && ServiceAimid_ == u"~space~";
    }

    bool CallInfo::isVideo() const
    {
        auto voip = GetVoipEvent();
        if (voip)
            return voip->isVideoCall();

        return false;
    }

    bool CallInfo::isMissed() const
    {
        auto voip = GetVoipEvent();
        return voip && voip->isMissed();
    }

    CallInfo UnserializeCallInfo(core::coll_helper* helper)
    {
        auto aimid = helper->get<QString>("aimid");
        core::coll_helper message(helper->get_value_as_collection("message"), false);
        return CallInfo(aimid, unserializeMessage(message, aimid, Ui::MyInfo()->aimId(), -1, -1));
    }

    CallInfoVec UnserializeCallLog(core::coll_helper* helper)
    {
        CallInfoVec result;

        auto calls = helper->get_value_as_array("calls");
        if (calls)
        {
            const auto size = calls->size();
            result.reserve(size);
            for (int32_t i = 0; i < size; ++i)
            {
                core::coll_helper value(
                    calls->get_at(i)->get_as_collection(),
                    false
                );

                result.push_back(UnserializeCallInfo(&value));
            }
        }

        return result;
    }
}
