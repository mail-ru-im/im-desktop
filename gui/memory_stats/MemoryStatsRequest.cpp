#include "MemoryStatsRequest.h"
#include "../corelib/collection_helper.h"

MEMSTATS_NS_BEGIN

void RequestHandle::serialize(core::coll_helper &_coll) const
{
    _coll.set_value_as_int64("request_id", id_);
    core::coll_helper coll_info(_coll->create_collection(), true);
    info_.serialize(coll_info);
    _coll.set_value_as_collection("info", coll_info.get());
}

bool RequestHandle::unserialize(const core::coll_helper &_coll)
{
    id_ = _coll.get_value_as_int64("request_id");
    auto info_coll = _coll.get_value_as_collection("info");

    if (!info_.unserialize(core::coll_helper(info_coll, false)))
        return false;

    return true;
}

void RequestInfo::serialize(core::coll_helper &_coll) const
{
    core::ifptr<core::iarray> req_types_array(_coll->create_array());
    req_types_array->reserve(req_types_.size());

    for (const auto &req_type: req_types_)
    {
        core::ifptr<core::ivalue> subcat_value(_coll->create_value());
        subcat_value->set_as_int(static_cast<int>(req_type));
        req_types_array->push_back(subcat_value.get());
    }

    _coll.set_value_as_array("requested_types", req_types_array.get());
}

bool RequestInfo::unserialize(const core::coll_helper &_coll)
{
    clear();

    core::iarray* requested_types = _coll.get_value_as_array("requested_types");
    assert(requested_types);
    if (!requested_types)
        return false;

    for (core::iarray::size_type i = 0; i < requested_types->size(); ++i)
    {
        req_types_.insert(static_cast<Memory_Stats::StatType>(requested_types->get_at(i)->get_as_int()));
    }

    return true;
}

void RequestInfo::clear()
{
    req_types_.clear();
}

MEMSTATS_NS_END
