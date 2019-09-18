#pragma once

#include <unordered_set>
#include "namespaces.h"
#include "memory_stats_common.h"
#include "../corelib/iserializable.h"
#include "stdafx.h"

CORE_MEMORY_STATS_NS_BEGIN

struct request_info: public core::iserializable
{
    requested_types req_types_;

    virtual void serialize(Out core::coll_helper& _coll) const override;
    virtual bool unserialize(const core::coll_helper& _coll) override;

private:
    void clear();
};

struct request_handle: public core::iserializable
{
    request_handle(request_id _id)
        : id_(_id)
    {}

    request_handle(const request_handle &other) = default;
    request_handle(request_handle &&other) = default;

    bool operator==(const request_handle& _other) const
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

    request_id id_ = INVALID_REQUEST_ID;
    request_info info_;
};

struct request_handle_hash
{
    size_t operator()(const request_handle& _handle) const
    {
        return std::hash<int64_t>()(_handle.id_);
    }
};

CORE_MEMORY_STATS_NS_END
