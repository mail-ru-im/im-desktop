#pragma once

#include <memory>
#include <list>
#include <unordered_map>
#include "memory_consumption_reporter.h"
#include "memory_stats_common.h"
#include "memory_stats_request.h"
#include "memory_stats_response.h"
#include "namespaces.h"

CORE_MEMORY_STATS_NS_BEGIN

class memory_stats_collector
{
public:
    using reporter_ptr = std::unique_ptr<memory_consumption_reporter>;
    using async_reporter_ptr = std::unique_ptr<memory_consumption_async_reporter>;

    using reporters_list = std::list<reporter_ptr>;
    using async_reporters_list = std::list<async_reporter_ptr>;
    using request_to_response = std::unordered_map<request_handle, response, request_handle_hash>;

public:
    memory_stats_collector() = default;
    void register_consumption_reporter(reporter_ptr&& _reporter);
    void register_async_consumption_reporter(async_reporter_ptr&& _async_reporter);

    request_handle request_memory_usage(const requested_types& _types);
    // returns true if the response is now ready
    bool update_response(request_id _id, const partial_response& _part_response);
    response get_response(request_id _id) const;
    void finish_request(request_id _id);

    reports_list generate_instant_reports();

private:
    void add_request(const request_handle& _req_handle, const response& _response);


private:
    reporters_list reporters_list_;
    async_reporters_list async_reporters_list_;
    request_to_response request_to_response_;
};

CORE_MEMORY_STATS_NS_END


