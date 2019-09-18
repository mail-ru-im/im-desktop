#pragma once

#include <unordered_set>
#include "namespaces.h"
#include "../corelib/iserializable.h"
#include "MemoryStatsCommon.h"
#include "MemoryStatsReport.h"
#include "stdafx.h"

MEMSTATS_NS_BEGIN

struct PartialResponse: public core::iserializable
{
    virtual void serialize(Out core::coll_helper& _coll) const override;
    virtual bool unserialize(const core::coll_helper& _coll) override;

    ReportsList reports_;

private:
    void clear();
};

struct Response: public core::iserializable
{
    void check_ready(const RequestedTypes& _types);
    bool is_ready() const;
    void set_ready(bool _ready);

    virtual void serialize(Out core::coll_helper& _coll) const override;
    virtual bool unserialize(const core::coll_helper& _coll) override;

    ReportsList reports_;

private:
    void clear();

private:
    bool ready_ = false;
};

MEMSTATS_NS_END
