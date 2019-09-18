#pragma once

#include "stats/memory/memory_consumption_reporter.h"
#include "namespaces.h"

CORE_NS_BEGIN

class gui_memory_consumption_reporter: public memory_stats::memory_consumption_async_reporter
{
public:
    gui_memory_consumption_reporter();
    virtual ~gui_memory_consumption_reporter();

    virtual void request_report(const memory_stats::request_handle& _req_handle) override;
};

CORE_NS_END
