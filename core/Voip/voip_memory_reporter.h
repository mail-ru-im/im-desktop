#pragma once

#include "../stats/memory/memory_consumption_reporter.h"

namespace voip_manager
{
    class VoipManager;
}

CORE_MEMORY_STATS_NS_BEGIN

class voip_memory_consumption_reporter: public memory_consumption_reporter
{
public:
    voip_memory_consumption_reporter(std::weak_ptr<voip_manager::VoipManager> voip_manager_);
    virtual ~voip_memory_consumption_reporter() = default;

    virtual reports_list generate_instant_reports() const override;

private:
    std::weak_ptr<voip_manager::VoipManager> _voip_manager;
};

CORE_MEMORY_STATS_NS_END
