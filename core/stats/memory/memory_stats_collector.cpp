#include "stdafx.h"
#include "memory_stats_collector.h"

CORE_MEMORY_STATS_NS_BEGIN

void memory_stats_collector::register_consumption_reporter(memory_stats_collector::reporter_ptr &&_reporter)
{
    reporters_list_.push_back(std::move(_reporter));
}

void memory_stats_collector::register_async_consumption_reporter(memory_stats_collector::async_reporter_ptr &&_async_reporter)
{
    async_reporters_list_.push_back(std::move(_async_reporter));
}

request_handle memory_stats_collector::request_memory_usage(const requested_types &_types)
{
    static request_id req;

    auto handle = request_handle(++req);
    handle.info_.req_types_ = _types;
    add_request(handle, response());

    for (auto &sync_reporter: reporters_list_)
    {
        memory_stats::partial_response part_response;
        part_response.reports_ = sync_reporter->generate_instant_reports();

        update_response(handle.id_, part_response);
    }

    for (auto &async_reporter: async_reporters_list_)
    {
        async_reporter->request_report(handle);
    }

    return handle;
}

bool memory_stats_collector::update_response(request_id _id, const partial_response &_part_response)
{
    auto it = request_to_response_.find(_id);
    if (it == request_to_response_.end())
        return false;

    it->second.reports_.insert(it->second.reports_.begin(), _part_response.reports_.begin(), _part_response.reports_.end());
    it->second.check_ready(it->first.info_.req_types_);
    return it->second.is_ready();
}

response memory_stats_collector::get_response(request_id _id) const
{
    auto it = request_to_response_.find(_id);
    if (it == request_to_response_.end())
        return response();

    return it->second;
}

void memory_stats_collector::finish_request(request_id _id)
{
    request_to_response_.erase(_id);
}

reports_list memory_stats_collector::generate_instant_reports()
{
    reports_list result;

    for (auto &reporter: reporters_list_)
    {
        assert(reporter);

        const auto& reports = reporter->generate_instant_reports();
        result.insert(result.end(), reports.begin(), reports.end());
    }

    return result;
}

void memory_stats_collector::add_request(const request_handle &_req_handle,
                                         const response &_response)
{
    request_to_response_[_req_handle] = _response;
}


CORE_MEMORY_STATS_NS_END
