#include "stdafx.h"
#include "memory_stats_report.h"
#include "../corelib/collection_helper.h"
#include "../corelib/collection_helper.h"
#include "../corelib/ifptr.h"
#include "../corelib/core_face.h"

CORE_MEMORY_STATS_NS_BEGIN

// memory_stats_subcategory

memory_stats_report::memory_stats_subcategory::memory_stats_subcategory(const name_string_t &_name, memory_size_t _occupied_memory)
    : name_(_name),
      occupied_memory_(_occupied_memory)
{

}

void memory_stats_report::memory_stats_subcategory::clear()
{
    name_.clear();
    occupied_memory_ = INVALID_SIZE;
}

name_string_t memory_stats_report::memory_stats_subcategory::getName() const
{
    return name_;
}

memory_size_t memory_stats_report::memory_stats_subcategory::getOccupiedMemory() const
{
    return occupied_memory_;
}

void memory_stats_report::memory_stats_subcategory::serialize(core::coll_helper &_coll) const
{
    _coll.set_value_as_string("subcategory_name", name_);
    _coll.set_value_as_int64("occupied_memory", occupied_memory_);
}

bool memory_stats_report::memory_stats_subcategory::unserialize(const core::coll_helper &_coll)
{
    clear();

    name_ = _coll.get_value_as_string("subcategory_name");
    occupied_memory_ = _coll.get_value_as_int64("occupied_memory");

    return true;
}

// end memory_stats_subcategory

memory_stats_report::memory_stats_report(const name_string_t &_reportee,
                                         memory_size_t _occupied_memory,
                                         stat_type _type,
                                         const subcategories& _subcategories)
    : reportee_(_reportee),
      occupied_memory_(_occupied_memory),
      subcategories_(_subcategories),
      type_(_type)
{

}

void memory_stats_report::addSubcategory(const memory_stats_report::memory_stats_subcategory &_subcategory)
{
    subcategories_.push_back(_subcategory);
}

void memory_stats_report::addSubcategory(const name_string_t &_name, memory_size_t _occupied_memory)
{
    subcategories_.push_back(createSubcategory(_name, _occupied_memory));
}

memory_stats_report::memory_stats_subcategory memory_stats_report::createSubcategory(const name_string_t &_name,
                                                                                     memory_size_t _occupied_memory)
{
    return memory_stats_report::memory_stats_subcategory(_name, _occupied_memory);
}

void memory_stats_report::clear()
{
    reportee_.clear();
    subcategories_.clear();
    occupied_memory_ = INVALID_SIZE;
}

void memory_stats_report::serialize(core::coll_helper &_coll) const
{
    _coll.set_value_as_string("reportee", reportee_);
    _coll.set_value_as_int64("total_occupied_memory", occupied_memory_);
    _coll.set_value_as_int("stat_type", static_cast<int>(type_));

    core::ifptr<core::iarray> subcats_array(_coll->create_array());
    subcats_array->reserve(subcategories_.size());

    for (const auto &subcategory : subcategories_)
    {
        core::coll_helper coll_subcat(_coll->create_collection(), false);
        subcategory.serialize(coll_subcat);
        core::ifptr<core::ivalue> subcat_value(_coll->create_value());
        subcat_value->set_as_collection(coll_subcat.get());
        subcats_array->push_back(subcat_value.get());
    }

    _coll.set_value_as_array("subcategories", subcats_array.get());
}

bool memory_stats_report::unserialize(const core::coll_helper &_coll)
{
    clear();

    reportee_ = _coll.get_value_as_string("reportee");
    occupied_memory_ = _coll.get_value_as_int64("total_occupied_memory");
    type_ = static_cast<memory_stats::stat_type>(_coll.get_value_as_int("stat_type"));

    core::iarray* subcategories = _coll.get_value_as_array("subcategories");
    assert(subcategories);
    if (!subcategories)
        return false;

    for (core::iarray::size_type i = 0; i < subcategories->size(); ++i)
    {
        const core::ivalue* subcat_value = subcategories->get_at(i);
        assert(subcat_value);
        if (!subcat_value)
            continue;

        core::icollection* subcat_coll = subcat_value->get_as_collection();
        assert(subcat_coll);
        if (!subcat_coll)
            continue;

        memory_stats_subcategory subcategory("", -1);
        core::coll_helper coll(subcat_coll);
        if (!subcategory.unserialize(coll))
            continue;

        subcategories_.push_back(subcategory);
    }

    return true;
}

memory_size_t memory_stats_report::getOccupiedMemory() const
{
    if (occupied_memory_ != -1 && occupied_memory_)
        return occupied_memory_;

    memory_size_t result = 0;

    for (auto &subcategory: subcategories_)
        result += subcategory.getOccupiedMemory();

    return result;
}

const memory_stats_report::subcategories &memory_stats_report::getSubcategories() const
{
    return subcategories_;
}

stat_type memory_stats_report::getStatType() const
{
    return type_;
}

name_string_t memory_stats_report::getReportee() const
{
    return reportee_;
}

memory_stats_report memory_stats_report::empty_report()
{
    return memory_stats_report("fake", -1, memory_stats::stat_type::invalid);
}


CORE_MEMORY_STATS_NS_END
