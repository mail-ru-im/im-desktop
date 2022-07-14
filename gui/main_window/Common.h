#pragma once

namespace Logic
{
    struct UnreadCounters
    {
        qint64 unreadMessages_ = 0;
        qint64 unknownUnreads_ = 0;
        bool attention_ = false;
    };

    UnreadCounters getUnreadCounters();
};