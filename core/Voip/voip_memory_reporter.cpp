#include "stdafx.h"
#include "voip_memory_reporter.h"
#include "VoipManager.h"

CORE_MEMORY_STATS_NS_BEGIN

namespace
{
    const name_string_t Name = "voip_initialization";
}

voip_memory_consumption_reporter::voip_memory_consumption_reporter(std::weak_ptr<voip_manager::VoipManager> voip_manager_)
    : _voip_manager(voip_manager_)
{

}

reports_list voip_memory_consumption_reporter::generate_instant_reports() const
{
    auto voip_mgr = _voip_manager.lock();
    if (!voip_mgr)
        return {};

    memory_stats_report report(Name, voip_mgr->get_voip_initialization_memory(), stat_type::voip_initialization);
    return { report };
}


CORE_MEMORY_STATS_NS_END
