#include "stdafx.h"
#include "StatusUtils.h"
#include "../utils/features.h"
#include "../common.shared/config/config.h"
#include "../gui_settings.h"
#include "../utils/InterConnector.h"
#include "../previewer/toast.h"

namespace Statuses
{
    QString getTimeString(std::chrono::seconds _time, TimeMode _mode)
    {
        if (_time.count() == 0 && _mode != TimeMode::Passed)
            return QString();

        QString timeString;
        if (_time.count() >= 0)
        {
            if (_time < std::chrono::minutes(1))
                timeString += getSecondsString(_time);
            else if (_time < std::chrono::hours(1))
                timeString += getMinutesString(std::chrono::round<std::chrono::minutes>(_time));
            else if (_time < std::chrono::hours(24))
                timeString += getHoursString(std::chrono::round<std::chrono::hours>(_time));
            else
                timeString += getDaysString(std::chrono::round<std::chrono::hours>(_time));
        }

        if (_mode == TimeMode::Left)
            timeString = QT_TRANSLATE_NOOP("status", "%1 left").arg(timeString);
        else if (_mode == TimeMode::Passed)
            timeString = QT_TRANSLATE_NOOP("status", "For %1 already").arg(timeString);

        return timeString;
    }

    QString getSecondsString(std::chrono::seconds _time)
    {
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
        return QString::number(days) % ql1c(' ')
               % Utils::GetTranslator()->getNumberString(
                days,
                QT_TRANSLATE_NOOP3("status", "day", "1"),
                QT_TRANSLATE_NOOP3("status", "days", "2"),
                QT_TRANSLATE_NOOP3("status", "days", "5"),
                QT_TRANSLATE_NOOP3("status", "days", "21")
        );
    }

    bool isStatusEnabled()
    {
        return Features::isStatusEnabled() && Ui::get_gui_settings()->get_value<bool>(settings_allow_statuses, settings_allow_statuses_default());
    }

    void showToastWithDuration(std::chrono::seconds _time)
    {
        if (std::chrono::seconds::zero() == _time)
            Utils::showTextToastOverContactDialog(QT_TRANSLATE_NOOP("status_popup", "Status is set"));
        else
            Utils::showTextToastOverContactDialog(QT_TRANSLATE_NOOP("status_popup", "Status is set for %1").arg(Statuses::getTimeString(_time)));
    }

    const Status& getBotStatus()
    {
        static const Status botStatus(Emoji::EmojiCode::toQString(Emoji::EmojiCode(0x1F916)), QT_TRANSLATE_NOOP("status_plate", "Bot"));
        return botStatus;
    }
}
