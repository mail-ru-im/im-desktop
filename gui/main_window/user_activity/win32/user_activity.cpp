#include "stdafx.h"

#include "../user_activity.h"

namespace UserActivity
{
    typedef std::chrono::time_point<std::chrono::system_clock> timepoint;

    timepoint _lastActivityTime;

    timepoint& lastActivityTime()
    {
        static timepoint tp;

        return tp;
    }

    std::chrono::milliseconds getLastActivityMs()
    {
        LASTINPUTINFO lii;
        lii.cbSize = sizeof(LASTINPUTINFO);

        if (::GetLastInputInfo(&lii))
        {
            return std::chrono::milliseconds(GetTickCount() - lii.dwTime);
        }
        else
        {
            return std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now() - _lastActivityTime);
        }
    }

    std::chrono::milliseconds getLastAppActivityMs()
    {
        return std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now() - _lastActivityTime);
    }

    void raiseUserActivity()
    {
        _lastActivityTime = std::chrono::system_clock::now();
    }
}
