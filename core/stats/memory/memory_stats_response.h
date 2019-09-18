#pragma once

#include <unordered_set>
#include "namespaces.h"
#include "../corelib/iserializable.h"
#include "memory_stats_common.h"
#include "memory_stats_report.h"
#include "stdafx.h"

CORE_MEMORY_STATS_NS_BEGIN

struct partial_response: public core::iserializable
{
    virtual void serialize(Out core::coll_helper& _coll) const override;
    virtual bool unserialize(const core::coll_helper& _coll) override;

    reports_list reports_;

private:
    void clear();
};

struct response: public core::iserializable
{
    void check_ready(const requested_types& _types);
    bool is_ready() const;
    void set_ready(bool _ready);

    virtual void serialize(Out core::coll_helper& _coll) const override;
    virtual bool unserialize(const core::coll_helper& _coll) override;

    reports_list reports_;

private:
    void clear();

private:
    bool ready_ = false;
};


CORE_MEMORY_STATS_NS_END
