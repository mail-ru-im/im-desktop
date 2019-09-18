#include "MemoryStatsResponse.h"
#include "../corelib/collection_helper.h"
#include <iostream>

MEMSTATS_NS_BEGIN

namespace
{
    ReportsList unserialize_reports_array(core::iarray* reports_array);
    void serialize_reports(core::coll_helper& _coll, const ReportsList& _reports);
}

void Response::check_ready(const RequestedTypes& _types)
{
    auto types_left = _types;

    for (const auto& report: reports_)
    {
        types_left.erase(report.getStatType());
    }

    set_ready(types_left.size() == 0);
}

bool Response::is_ready() const
{
    return ready_;
}

void Response::set_ready(bool _ready)
{
    ready_ = _ready;
}

void Response::serialize(core::coll_helper &_coll) const
{
    serialize_reports(_coll, reports_);
}

bool Response::unserialize(const core::coll_helper &_coll)
{
    clear();
    core::iarray* reports_array = _coll.get_value_as_array("reports");
    if (!reports_array)
        return false;

    reports_ = unserialize_reports_array(reports_array);
    return reports_.size() == static_cast<size_t>(reports_array->size());
}

void Response::clear()
{
    reports_.clear();
}

void PartialResponse::serialize(core::coll_helper &_coll) const
{
    serialize_reports(_coll, reports_);
}

bool PartialResponse::unserialize(const core::coll_helper &_coll)
{
    clear();
    core::iarray* reports_array = _coll.get_value_as_array("reports");
    if (!reports_array)
        return false;

    reports_ = unserialize_reports_array(reports_array);
    return reports_.size() == static_cast<size_t>(reports_array->size());
}

void PartialResponse::clear()
{
    reports_.clear();
}

namespace
{
    ReportsList unserialize_reports_array(core::iarray *reports_array)
    {
        ReportsList result;
        assert(reports_array);
        if (!reports_array)
            return {};

        for (core::iarray::size_type i = 0; i < reports_array->size(); ++i)
        {
            core::icollection* report_coll = reports_array->get_at(i)->get_as_collection();
            assert(report_coll);
            if (!report_coll)
                continue;

            auto report = Memory_Stats::MemoryStatsReport::empty_report();
            if (!report.unserialize(core::coll_helper(report_coll, false)))
                continue;

            result.push_back(report);
        }

        return result;
    }

    void serialize_reports(core::coll_helper& _coll, const ReportsList& _reports)
    {
        core::ifptr<core::iarray> reports_array(_coll->create_array());
        reports_array->reserve(_reports.size());

        for (const auto& report: _reports)
        {
            core::ifptr<core::ivalue> report_value(_coll->create_value());

            core::coll_helper coll_report(_coll->create_collection(), true);
            report.serialize(coll_report);

            report_value->set_as_collection(coll_report.get());
            reports_array->push_back(report_value.get());
        }

        _coll.set_value_as_array("reports", reports_array.get());
    }
}

MEMSTATS_NS_END
