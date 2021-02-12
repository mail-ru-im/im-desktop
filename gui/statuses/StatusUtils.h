#pragma once

#include "Status.h"
#include "../utils/utils.h"
#include "../cache/emoji/EmojiCode.h"

namespace Statuses
{
    QString getTimeString(std::chrono::seconds _time, TimeMode _mode = TimeMode::Common);

    QString getSecondsString(std::chrono::seconds _time);
    QString getMinutesString(std::chrono::minutes _time);
    QString getHoursString(std::chrono::hours _time);
    QString getDaysString(std::chrono::hours _time);

    bool isStatusEnabledInConfig();
    bool isStatusEnabled();

    constexpr std::chrono::milliseconds tooltipShowDelay() noexcept { return std::chrono::milliseconds(400); }

    void showToastWithDuration(std::chrono::seconds _time);
}
