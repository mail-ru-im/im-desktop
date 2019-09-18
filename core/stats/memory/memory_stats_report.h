#pragma once

#include <vector>
#include <list>
#include "stdafx.h"
#include "namespaces.h"
#include "../corelib/iserializable.h"
#include "memory_stats_common.h"

CORE_MEMORY_STATS_NS_BEGIN

class memory_stats_report: public core::iserializable
{
public:
    class memory_stats_subcategory: public core::iserializable
    {
    public:
        memory_stats_subcategory(const name_string_t& _name, memory_size_t _occupied_memory);
        memory_stats_subcategory(const memory_stats_subcategory& _other) = default;

        name_string_t getName() const;
        memory_size_t getOccupiedMemory() const;

    public:
    /* iserializable */
        virtual void serialize(Out core::coll_helper& _coll) const override;
        virtual bool unserialize(const core::coll_helper& _coll) override;

    private:
        void clear();

    private:
        name_string_t name_;
        memory_size_t occupied_memory_;
    };

    using subcategories = std::vector<memory_stats_subcategory>;

public:
    memory_stats_report(const name_string_t& _reportee, memory_size_t _occupied_memory, stat_type _type, const subcategories& _subcategories = {});

    void addSubcategory(const name_string_t& _name, memory_size_t _occupied_memory);
    memory_size_t getOccupiedMemory() const;
    const subcategories& getSubcategories() const;
    stat_type getStatType() const;
    name_string_t getReportee() const;

    static memory_stats_report empty_report();

public:
/* iserializable */
    virtual void serialize(Out core::coll_helper& _coll) const override;
    virtual bool unserialize(const core::coll_helper& _coll) override;

private:
    void addSubcategory(const memory_stats_subcategory& _subcategory);
    static memory_stats_subcategory createSubcategory(const name_string_t& _name, memory_size_t _occupied_memory);

private:
    void clear();

private:
    name_string_t reportee_;
    memory_size_t occupied_memory_;
    subcategories subcategories_;
    stat_type type_;
};

using reports_list = std::list<memory_stats_report>;

CORE_MEMORY_STATS_NS_END
