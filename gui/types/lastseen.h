#pragma once

namespace core
{
    class coll_helper;
}

namespace Data
{
    class LastSeen
    {
    public:
        LastSeen() = default;
        LastSeen(const LastSeen &other) = default;
        LastSeen(LastSeen &&other) = default;
        LastSeen(const qint64 _time) : time_(_time) {}
        LastSeen(const core::coll_helper& _coll);

        bool isOnline() const noexcept { return time_ && *time_ == 0 && isActive(); }
        bool isValid() const noexcept { return (time_ && *time_ >= 0) || isBlocked() || isAbsent() || isBot(); }

        enum class LastSeenState
        {
            active,
            absent,
            blocked,
            bot
        };
        bool isActive() const noexcept { return state_ == LastSeenState::active; }
        bool isBlocked() const noexcept { return state_ == LastSeenState::blocked; }
        bool isAbsent() const noexcept { return state_ == LastSeenState::absent; }
        bool isBot() const noexcept { return state_ == LastSeenState::bot; }

        QString getStatusString(bool _onlyDate = false) const;
        QDateTime toDateTime() const;

        LastSeen& operator=(const LastSeen&) = default;
        LastSeen& operator=(LastSeen&&) = default;

        bool operator==(const LastSeen& _other) const
        {
            return _other.time_ == time_ && _other.state_ == state_;
        }

        bool operator!=(const LastSeen& _other) const
        {
            return !(*this == _other);
        }

        static const LastSeen& online();
        static const LastSeen& bot();

    private:
        std::optional<qint64> time_;
        LastSeenState state_ = LastSeenState::active;
    };
}