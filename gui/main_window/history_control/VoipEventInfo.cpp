#include "stdafx.h"

#include "../../utils/gui_coll_helper.h"
#include "../../../corelib/enumerations.h"

#include "../../main_window/containers/FriendlyContainer.h"

#include "VoipEventInfo.h"

namespace HistoryControl
{

    VoipEventInfoSptr VoipEventInfo::Make(const coll_helper &info, const int32_t timestamp)
    {
        assert(timestamp > 0);

        VoipEventInfoSptr result(new VoipEventInfo(timestamp));
        if (result->unserialize(info))
            return result;

        return nullptr;
    }

    VoipEventInfo::VoipEventInfo(const int32_t timestamp)
        : Timestamp_(timestamp)
        , Type_(voip_event_type::invalid)
        , DurationSec_(-1)
        , IsIncomingCall_(false)
        , isVideoCall_(false)
    {
        assert(timestamp > 0);
    }

    void VoipEventInfo::serialize(Out coll_helper & _coll) const
    {
        if (_coll.empty())
            assert(false);
        assert(!"the method should not be used");
    }

    bool VoipEventInfo::unserialize(const coll_helper &_coll)
    {
        assert(Type_ == voip_event_type::invalid);

        Sid_ = _coll.get<QString>("sid");
        Type_ = _coll.get<voip_event_type>("type");

        const auto isValidType = ((Type_ > voip_event_type::min) && (Type_ < voip_event_type::max));
        assert(isValidType);
        if (!isValidType)
            return false;

        ContactAimid_ = _coll.get<QString>("sender_aimid");

        assert(!ContactAimid_.isEmpty());
        if (ContactAimid_.isEmpty())
            return false;

        DurationSec_ = _coll.get<int32_t>("duration_sec", -1);
        assert(DurationSec_ >= -1);

        IsIncomingCall_ = _coll.get<bool>("is_incoming", false);

        if (_coll.is_value_exist("is_video"))
            isVideoCall_ = _coll.get<bool>("is_video", false);

        if (_coll.is_value_exist("conf_members"))
            confMembers_ = Utils::toContainerOfString<decltype(confMembers_)>(_coll.get_value_as_array("conf_members"));

        return true;
    }

    QString VoipEventInfo::getSid() const
    {
        return Sid_;
    }

    QString VoipEventInfo::formatRecentsText() const
    {
        assert(Type_ > voip_event_type::min && Type_ < voip_event_type::max);

        QString result;
        result.reserve(128);

        switch (Type_)
        {
        case core::voip_event_type::missed_call:
            if (IsIncomingCall_)
                result += QT_TRANSLATE_NOOP("chat_event", "Missed call");
            else
                result += QT_TRANSLATE_NOOP("chat_event", "Outgoing call");
            break;

        case core::voip_event_type::call_ended:
        case core::voip_event_type::decline:
            if (IsIncomingCall_)
                result += QT_TRANSLATE_NOOP("chat_event", "Incoming call");
            else
                result += QT_TRANSLATE_NOOP("chat_event", "Outgoing call");
            break;

        case core::voip_event_type::accept:
            return result;

        default:
            assert(!"unexpected event type");
            break;
        }

        if (hasDuration())
            result += u" (" % formatDurationText() % u')';

        return result;
    }

    QString VoipEventInfo::formatItemText() const
    {
        assert(Type_ > voip_event_type::min && Type_ < voip_event_type::max);

        QString result;
        result.reserve(128);

        const auto contactFriendly = Logic::GetFriendlyContainer()->getFriendly(ContactAimid_);
        assert(!contactFriendly.isEmpty());

        switch (Type_)
        {
        case core::voip_event_type::missed_call:
            if (IsIncomingCall_)
                result += confMembers_.empty() ? QT_TRANSLATE_NOOP("chat_event", "You missed call from %1").arg(contactFriendly) : QT_TRANSLATE_NOOP("chat_event", "You missed a group call");
            else
                result += QT_TRANSLATE_NOOP("chat_event", "Your call was missed");
            break;

        case core::voip_event_type::call_ended:
            if (IsIncomingCall_)
                result += confMembers_.empty() ? QT_TRANSLATE_NOOP("chat_event", "Call from %1").arg(contactFriendly) : QT_TRANSLATE_NOOP("chat_event", "Group call");
            else
                result += confMembers_.empty() ? QT_TRANSLATE_NOOP("chat_event", "You called %1").arg(contactFriendly) : QT_TRANSLATE_NOOP("chat_event", "Group call from you");
            break;

        case core::voip_event_type::accept:
            return result;

        case core::voip_event_type::decline:
            if (IsIncomingCall_)
                result += QT_TRANSLATE_NOOP("chat_event", "You declined a call from %1").arg(contactFriendly);
            else
                result += QT_TRANSLATE_NOOP("chat_event", "Your call was declined");
            break;

        default:
            assert(!"unexpected event type");
            break;
        }

        return result;
    }

    QString VoipEventInfo::formatDurationText() const
    {
        const QTime duration = QTime(0, 0).addSecs(DurationSec_);

        if (duration.hour() > 0)
            return qsl("%1:%2:%3")
            .arg(duration.hour(), 2, 10, ql1c('0'))
            .arg(duration.minute(), 2, 10, ql1c('0'))
            .arg(duration.second(), 2, 10, ql1c('0'));

        return qsl("%1:%2")
            .arg(duration.minute(), 2, 10, ql1c('0'))
            .arg(duration.second(), 2, 10, ql1c('0'));
    }

    const QString& VoipEventInfo::getContactAimid() const
    {
        assert(!ContactAimid_.isEmpty());
        return ContactAimid_;
    }

    int32_t VoipEventInfo::getTimestamp() const
    {
        assert(Timestamp_ > 0);
        return Timestamp_;
    }

    bool VoipEventInfo::isClickable() const
    {
#ifdef STRIP_VOIP
        return false;
#endif //STRIP_VOIP
        return isVisible();
    }

    bool VoipEventInfo::isIncomingCall() const
    {
        return IsIncomingCall_;
    }

    bool VoipEventInfo::isVisible() const
    {
        constexpr voip_event_type visTypes[] = { voip_event_type::call_ended, voip_event_type::missed_call , voip_event_type::decline };
        return std::any_of(std::begin(visTypes), std::end(visTypes), [callType = Type_](const auto _visType) { return callType == _visType; });
    }

    bool VoipEventInfo::isMissed() const
    {
        return IsIncomingCall_ && Type_ == voip_event_type::missed_call;
    }

    bool VoipEventInfo::isVideoCall() const
    {
        return isVideoCall_;
    }

    bool VoipEventInfo::hasDuration() const
    {
        return DurationSec_ > 0;
    }

    const std::vector<QString>& VoipEventInfo::getConferenceMembers() const
    {
        return confMembers_;
    }

}
