#pragma once
#include "voip/voip3.h"
#include "VoipManagerDefines.h"
#include "voip_memory_reporter.h"

#include <vector>
#include <memory>

namespace core
{
    class core_dispatcher;
    class coll_helper;
}

namespace voip_manager
{
    void addVoipStatistic(std::stringstream& s, const core::coll_helper& params);
    VoipManager *createVoipManager(core::core_dispatcher& dispatcher);
}
