#include "Common.h"
#include "contact_list/RecentsModel.h"
#include "contact_list/UnknownsModel.h"

namespace Logic
{
    UnreadCounters getUnreadCounters()
    {
        const auto recentsModel = Logic::getRecentsModel();
        const auto unknownsModel = Logic::getUnknownsModel();
        UnreadCounters counters;
        counters.unknownUnreads_ = unknownsModel ? unknownsModel->totalUnreads() : 0;
        counters.unreadMessages_ = (recentsModel ? recentsModel->totalUnreads() : 0) + counters.unknownUnreads_;
        counters.attention_ = (recentsModel && recentsModel->hasAttentionDialogs()) ||
                             (unknownsModel && unknownsModel->hasAttentionDialogs());
        return counters;
    }
};

