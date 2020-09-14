#include "stdafx.h"

#include "lastseen.h"
#include "../../corelib/collection_helper.h"

namespace Data
{
    LastSeen::LastSeen(const core::coll_helper& _coll)
    {
        time_ = {};
        state_ = LastSeenState::active;

        if (_coll.is_value_exist("lastseen"))
        {
            core::coll_helper seen(_coll.get_value_as_collection("lastseen"), false);
            if (seen.is_value_exist("time"))
                time_ = std::make_optional(seen.get_value_as_int64("time"));

            if (seen.is_value_exist("state"))
            {
                const std::string_view str = seen.get_value_as_string("state");
                if (str == "absent")
                    state_ = LastSeenState::absent;
                else if (str == "blocked")
                    state_ = LastSeenState::blocked;
                else if (str == "bot")
                    state_ = LastSeenState::bot;
            }
        }
    }

    QString LastSeen::getStatusString(bool _onlyDate) const
    {
        if (!isValid())
            return QString();
        else if (isOnline())
            return QT_TRANSLATE_NOOP("state", "Online");
        else if (isBlocked())
            return QT_TRANSLATE_NOOP("state", "Blocked");
        else if (isAbsent())
            return QT_TRANSLATE_NOOP("state", "Have never been here");
        else if (isBot())
            return QT_TRANSLATE_NOOP("state", "Bot");

        QString res;

        const auto current = QDateTime::currentDateTime();
        const auto dt = toDateTime();

        const std::chrono::seconds seconds(dt.secsTo(current));

        QString suffix;
        if (seconds < std::chrono::hours(1))
        {
            const auto mins = seconds.count() / 60;

            if (mins <= 0)
            {
                suffix = QT_TRANSLATE_NOOP("date", "just now");
            }
            else
            {
                suffix = Utils::GetTranslator()->getNumberString(
                    mins,
                    QT_TRANSLATE_NOOP3("date", "%1 minute ago", "1"),
                    QT_TRANSLATE_NOOP3("date", "%1 minutes ago", "2"),
                    QT_TRANSLATE_NOOP3("date", "%1 minutes ago", "5"),
                    QT_TRANSLATE_NOOP3("date", "%1 minutes ago", "21")).arg(mins);
            }
        }
        else
        {
            suffix = Utils::GetTranslator()->formatDifferenceToNow(dt);
        }

        if (_onlyDate)
            return suffix;

        if (!suffix.isEmpty())
            res = QT_TRANSLATE_NOOP("date", "Seen ") % suffix;

        return res;
    }

    QDateTime LastSeen::toDateTime() const
    {
        assert(isValid());
        return QDateTime::fromSecsSinceEpoch(*time_);
    }

    const LastSeen& LastSeen::online()
    {
        static const LastSeen seen(0);
        return seen;
    }

    const LastSeen& LastSeen::bot()
    {
        static const LastSeen seen = []()
        {
            LastSeen seen;
            seen.state_ = LastSeenState::bot;
            seen.time_ = {};
            return seen;
        }();
        return seen;
    }
}
