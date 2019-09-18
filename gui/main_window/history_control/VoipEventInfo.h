#pragma once

#include "../../../corelib/iserializable.h"

namespace core
{
    struct icollection;

    enum class voip_event_type;
}

namespace HistoryControl
{

    using namespace core;

    typedef std::shared_ptr<class VoipEventInfo> VoipEventInfoSptr;

    class VoipEventInfo : public core::iserializable
    {
    public:
        static VoipEventInfoSptr Make(const coll_helper &info, const int32_t timestamp);

        void serialize(Out coll_helper &_coll) const override;
        bool unserialize(const coll_helper &_coll) override;

        QString formatRecentsText() const;
        QString formatItemText() const;
        QString formatDurationText() const;

        const QString& getContactAimid() const;

        int32_t getTimestamp() const;

        bool isClickable() const;
        bool isIncomingCall() const;
        bool isVisible() const;
        bool isMissed() const;
        bool isVideoCall() const;
        bool hasDuration() const;

        const std::vector<QString>& getConferenceMembers() const;

    private:
        VoipEventInfo(const int32_t timestamp);

        QString ContactAimid_;
        int32_t Timestamp_;
        voip_event_type Type_;
        int32_t DurationSec_;
        bool IsIncomingCall_;
        bool isVideoCall_;
        std::vector<QString> confMembers_;
    };

}