#include "stdafx.h"
#include "StatusUtils.h"
#include "../utils/features.h"
#include "../common.shared/config/config.h"
#include "../gui_settings.h"
#include "../main_window/contact_list/StatusListModel.h"
#include "../utils/InterConnector.h"

namespace Statuses
{
    QString getTimeString(std::chrono::seconds _time, TimeMode _mode)
    {
        if (_time.count() == 0 && _mode != TimeMode::AlwaysOn)
            return QString();

        if ((_time.count() == 0 || _time >= std::chrono::hours(24 * 7)) && _mode == TimeMode::AlwaysOn)
            return (_time.count() == 0) ? QT_TRANSLATE_NOOP("status", "Show always") : QT_TRANSLATE_NOOP("status", "Week");

        QString timeString;

        if (_time.count() > 0 && _time < std::chrono::minutes(1))
            timeString += getSecondsString(_time);
        else if (_time < std::chrono::hours(1))
            timeString += getMinutesString(std::chrono::round<std::chrono::minutes>(_time));
        else if (_time < std::chrono::hours(24))
            timeString += getHoursString(std::chrono::round<std::chrono::hours>(_time));
        else if (_time < std::chrono::hours(24 * 7))
            timeString += getDaysString(std::chrono::round<std::chrono::hours>(_time));
        else if (_time < 10 * std::chrono::hours(24 * 7))
            timeString += getWeeksString(std::chrono::round<std::chrono::hours>(_time));
        else
            timeString += QT_TRANSLATE_NOOP("status", "undefined time");

        if (_mode == TimeMode::Left)
            timeString = QT_TRANSLATE_NOOP("status", "%1 left").arg(timeString);
        else if (_mode == TimeMode::Passed)
            timeString = QT_TRANSLATE_NOOP("status", "For %1 already").arg(timeString);

        return timeString;
    }

    const timeV& getTimeList()
    {
        static const timeV times =
                {
                        std::chrono::seconds(0),
                        std::chrono::minutes(10),
                        std::chrono::minutes(30),
                        std::chrono::hours(1),
                        std::chrono::hours(24),
                        std::chrono::hours(24 * 7)
                };

        return times;
    }

    QString getSecondsString(std::chrono::seconds _time)
    {
        if (_time == std::chrono::minutes(1))
            return getMinutesString(std::chrono::round<std::chrono::minutes>(_time));
        return QString::number(_time.count()) % ql1c(' ')
               % Utils::GetTranslator()->getNumberString(
                _time.count(),
                QT_TRANSLATE_NOOP3("status", "second", "1"),
                QT_TRANSLATE_NOOP3("status", "seconds", "2"),
                QT_TRANSLATE_NOOP3("status", "seconds", "5"),
                QT_TRANSLATE_NOOP3("status", "seconds", "21")
        );
    }
    QString getMinutesString(std::chrono::minutes _time)
    {
        if (_time == std::chrono::hours(1))
            return getHoursString(std::chrono::round<std::chrono::hours>(_time));
        return QString::number(_time.count()) % ql1c(' ')
               % Utils::GetTranslator()->getNumberString(
                _time.count(),
                QT_TRANSLATE_NOOP3("status", "minute", "1"),
                QT_TRANSLATE_NOOP3("status", "minutes", "2"),
                QT_TRANSLATE_NOOP3("status", "minutes", "5"),
                QT_TRANSLATE_NOOP3("status", "minutes", "21")
        );
    }
    QString getHoursString(std::chrono::hours _time)
    {
        const auto hours = _time.count();
        if (hours == 24)
            return getDaysString(_time);
        return QString::number(hours) % ql1c(' ')
               % Utils::GetTranslator()->getNumberString(
                hours,
                QT_TRANSLATE_NOOP3("status", "hour", "1"),
                QT_TRANSLATE_NOOP3("status", "hours", "2"),
                QT_TRANSLATE_NOOP3("status", "hours", "5"),
                QT_TRANSLATE_NOOP3("status", "hours", "21")
        );
    }
    QString getDaysString(std::chrono::hours _time)
    {
        const auto days = _time.count() / 24;
        if (days == 7)
            return getWeeksString(_time);
        return QString::number(days) % ql1c(' ')
               % Utils::GetTranslator()->getNumberString(
                days,
                QT_TRANSLATE_NOOP3("status", "day", "1"),
                QT_TRANSLATE_NOOP3("status", "days", "2"),
                QT_TRANSLATE_NOOP3("status", "days", "5"),
                QT_TRANSLATE_NOOP3("status", "days", "21")
        );
    }
    QString getWeeksString(std::chrono::hours _time)
    {
        const auto weeks = _time.count() / 24 / 7;
        return QString::number(weeks) % ql1c(' ')
               % Utils::GetTranslator()->getNumberString(
                weeks,
                QT_TRANSLATE_NOOP3("status", "week", "1"),
                QT_TRANSLATE_NOOP3("status", "weeks", "2"),
                QT_TRANSLATE_NOOP3("status", "weeks", "5"),
                QT_TRANSLATE_NOOP3("status", "weeks", "21")
        );
    }

    bool isStatusEnabled()
    {
        return Features::isStatusEnabled() && Ui::get_gui_settings()->get_value<bool>(settings_allow_statuses, settings_allow_statuses_default());
    }
}