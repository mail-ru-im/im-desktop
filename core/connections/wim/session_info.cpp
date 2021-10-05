#include "stdafx.h"
#include "session_info.h"

#include "../../../corelib/collection_helper.h"
#include "../../../common.shared/json_helper.h"

namespace core::wim
{
    void session_info::serialize(core::coll_helper _coll) const
    {
        _coll.set_value_as_string("os", os_);
        _coll.set_value_as_string("name", client_name_);
        _coll.set_value_as_string("location", location_);
        _coll.set_value_as_string("ip", ip_);
        _coll.set_value_as_string("hash", hash_);
        _coll.set_value_as_int64("started", started_time_);
        _coll.set_value_as_bool("current", is_current_);
    }

    int32_t session_info::unserialize(const rapidjson::Value& _node)
    {
        tools::unserialize_value(_node, "os", os_);
        tools::unserialize_value(_node, "client", client_name_);
        tools::unserialize_value(_node, "location", location_);
        tools::unserialize_value(_node, "ip", ip_);
        tools::unserialize_value(_node, "hash", hash_);
        tools::unserialize_value(_node, "startedTime", started_time_);
        tools::unserialize_value(_node, "current", is_current_);

        return 0;
    }
}