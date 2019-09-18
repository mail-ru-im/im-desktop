#pragma once

#include "namespaces.h"
#include "memory_stats_common.h"
#include "memory_stats_report.h"
#include "memory_stats_request.h"

CORE_MEMORY_STATS_NS_BEGIN

class memory_consumption_reporter
{
public:
    memory_consumption_reporter() = default;
    virtual ~memory_consumption_reporter() = default;

    virtual reports_list generate_instant_reports() const = 0;
};

class memory_consumption_async_reporter
{
public:
    memory_consumption_async_reporter() = default;
    virtual ~memory_consumption_async_reporter() = default;

    virtual void request_report(const request_handle& _req_handle) = 0;
};

CORE_MEMORY_STATS_NS_END
