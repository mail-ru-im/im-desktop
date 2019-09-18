#pragma once

#include <unordered_set>
#include "namespaces.h"
#include "MemoryStatsCommon.h"
#include "../corelib/iserializable.h"
#include "stdafx.h"

MEMSTATS_NS_BEGIN

struct RequestInfo: public core::iserializable
{
    RequestedTypes req_types_;

    virtual void serialize(Out core::coll_helper& _coll) const override;
    virtual bool unserialize(const core::coll_helper& _coll) override;

private:
    void clear();
};

struct RequestHandle: public core::iserializable
{
    RequestHandle(RequestId _id)
        : id_(_id)
    {}

    RequestHandle(const RequestHandle &other) = default;
    RequestHandle(RequestHandle &&other) = default;

    bool operator==(const RequestHandle& _other) const
    {
        return id_ == _other.id_;
    }

    bool is_valid() const
    {
        return id_ != INVALID_REQUEST_ID;
    }

/* iserializable */
    virtual void serialize(Out core::coll_helper& _coll) const override;
    virtual bool unserialize(const core::coll_helper& _coll) override;

    RequestId id_ = INVALID_REQUEST_ID;
    RequestInfo info_;
};

struct RequestHandleHash
{
    size_t operator()(const RequestHandle& _handle) const
    {
        return std::hash<int64_t>()(_handle.id_);
    }
};

MEMSTATS_NS_END
