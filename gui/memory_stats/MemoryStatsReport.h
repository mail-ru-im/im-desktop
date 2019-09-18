#pragma once

#include <vector>
#include <list>
#include "stdafx.h"
#include "namespaces.h"
#include "../corelib/iserializable.h"
#include "MemoryStatsCommon.h"

MEMSTATS_NS_BEGIN

class MemoryStatsReport: public core::iserializable
{
public:
    class MemoryStatsSubcategory: public core::iserializable
    {
    public:
        MemoryStatsSubcategory(const NameString_t& _name, MemorySize_t _occupied_memory);
        MemoryStatsSubcategory(const MemoryStatsSubcategory& _other) = default;

        NameString_t getName() const;
        MemorySize_t getOccupiedMemory() const;

    public:
    /* iserializable */
        virtual void serialize(Out core::coll_helper& _coll) const override;
        virtual bool unserialize(const core::coll_helper& _coll) override;

    private:
        void clear();

    private:
        NameString_t name_;
        MemorySize_t occupied_memory_;
    };

    using Subcategories = std::vector<MemoryStatsSubcategory>;

public:
    MemoryStatsReport(const NameString_t& _reportee,
                      MemorySize_t _occupied_memory,
                      StatType _type,
                      const Subcategories& _subcategories = {});

    void addSubcategory(const NameString_t& _name, MemorySize_t _occupied_memory);
    MemorySize_t getOccupiedMemory() const;
    const Subcategories& getSubcategories() const;
    StatType getStatType() const;
    NameString_t getReportee() const;

    static MemoryStatsReport empty_report();

public:
/* iserializable */
    virtual void serialize(Out core::coll_helper& _coll) const override;
    virtual bool unserialize(const core::coll_helper& _coll) override;

private:
    void addSubcategory(const MemoryStatsSubcategory& _subcategory);
    static MemoryStatsSubcategory createSubcategory(const NameString_t& _name, MemorySize_t _occupied_memory);

private:
    void clear();

private:
    NameString_t reportee_;
    MemorySize_t occupied_memory_;
    Subcategories subcategories_;
    StatType type_;
};

using ReportsList = std::list<MemoryStatsReport>;

MEMSTATS_NS_END
