#pragma once

namespace UserActivity
{
    std::chrono::milliseconds getLastActivityMs();

    std::chrono::milliseconds getLastAppActivityMs();

    void raiseUserActivity();
}
