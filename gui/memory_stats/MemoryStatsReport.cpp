#include "MemoryStatsReport.h"

#include "../corelib/collection_helper.h"
#include "../corelib/collection_helper.h"
#include "../corelib/ifptr.h"
#include "../corelib/core_face.h"

MEMSTATS_NS_BEGIN

// memory_stats_subcategory

MemoryStatsReport::MemoryStatsSubcategory::MemoryStatsSubcategory(const NameString_t &_name,
                                                                  MemorySize_t _occupied_memory)
    : name_(_name),
      occupied_memory_(_occupied_memory)
{

}

void MemoryStatsReport::MemoryStatsSubcategory::clear()
{
    name_.clear();
    occupied_memory_ = INVALID_SIZE;
}

NameString_t MemoryStatsReport::MemoryStatsSubcategory::getName() const
{
    return name_;
}

MemorySize_t MemoryStatsReport::MemoryStatsSubcategory::getOccupiedMemory() const
{
    return occupied_memory_;
}

void MemoryStatsReport::MemoryStatsSubcategory::serialize(core::coll_helper &_coll) const
{
    _coll.set_value_as_string("subcategory_name", name_);
    _coll.set_value_as_int64("occupied_memory", occupied_memory_);
}

bool MemoryStatsReport::MemoryStatsSubcategory::unserialize(const core::coll_helper &_coll)
{
    clear();

    name_ = _coll.get_value_as_string("subcategory_name");
    occupied_memory_ = _coll.get_value_as_int64("occupied_memory");

    return true;
}

// end memory_stats_subcategory

MemoryStatsReport::MemoryStatsReport(const NameString_t &_reportee,
                                     MemorySize_t _occupied_memory,
                                     StatType _type,
                                     const Subcategories& _subcategories)
    : reportee_(_reportee),
      occupied_memory_(_occupied_memory),
      subcategories_(_subcategories),
      type_(_type)
{

}

void MemoryStatsReport::addSubcategory(const MemoryStatsReport::MemoryStatsSubcategory &_subcategory)
{
    subcategories_.push_back(_subcategory);
}

void MemoryStatsReport::addSubcategory(const NameString_t &_name, MemorySize_t _occupied_memory)
{
    subcategories_.push_back(createSubcategory(_name, _occupied_memory));
}

MemoryStatsReport::MemoryStatsSubcategory MemoryStatsReport::createSubcategory(const NameString_t &_name,
                                                                                     MemorySize_t _occupied_memory)
{
    return MemoryStatsReport::MemoryStatsSubcategory(_name, _occupied_memory);
}

void MemoryStatsReport::clear()
{
    reportee_.clear();
    subcategories_.clear();
    occupied_memory_ = INVALID_SIZE;
}

void MemoryStatsReport::serialize(core::coll_helper &_coll) const
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

bool MemoryStatsReport::unserialize(const core::coll_helper &_coll)
{
    clear();

    reportee_ = _coll.get_value_as_string("reportee");
    occupied_memory_ = _coll.get_value_as_int64("total_occupied_memory");
    type_ = static_cast<Memory_Stats::StatType>(_coll.get_value_as_int("stat_type"));

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

        MemoryStatsSubcategory subcategory("", -1);
        core::coll_helper coll(subcat_coll);
        if (!subcategory.unserialize(coll))
            continue;

        subcategories_.push_back(subcategory);
    }

    return true;
}

MemorySize_t MemoryStatsReport::getOccupiedMemory() const
{
    if (occupied_memory_ != -1 && occupied_memory_)
        return occupied_memory_;

    MemorySize_t result = 0;

    for (auto &subcategory: subcategories_)
        result += subcategory.getOccupiedMemory();

    return result;
}

const MemoryStatsReport::Subcategories &MemoryStatsReport::getSubcategories() const
{
    return subcategories_;
}

StatType MemoryStatsReport::getStatType() const
{
    return type_;
}

NameString_t MemoryStatsReport::getReportee() const
{
    return reportee_;
}

MemoryStatsReport MemoryStatsReport::empty_report()
{
    return MemoryStatsReport("fake", -1, StatType::Invalid);
}


MEMSTATS_NS_END
