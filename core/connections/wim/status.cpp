#include "stdafx.h"

#include "status.h"

#include "../common.shared/json_helper.h"
#include "../../../corelib/collection_helper.h"

namespace
{
    int64_t to_seconds(core::status::clock_t::time_point _point)
    {
        const auto point_sec = std::chrono::time_point_cast<std::chrono::seconds>(_point);
        return point_sec.time_since_epoch().count();
    }

    core::status::clock_t::time_point from_seconds(int64_t _secs)
    {
        const std::chrono::seconds sec(_secs);
        return core::status::clock_t::time_point(sec);
    }
}

namespace core
{
    void status::serialize(core::coll_helper& _coll) const
    {
        coll_helper coll_st(_coll->create_collection(), true);

        coll_st.set_value_as_string("status", status_);
        coll_st.set_value_as_string("description", description_);
        coll_st.set_value_as_int64("startTime", to_seconds(start_time_));

        if (end_time_)
            coll_st.set_value_as_int64("endTime", to_seconds(*end_time_));

        _coll.set_value_as_collection("status", coll_st.get());
    }

    void status::serialize(rapidjson::Value& _node, rapidjson_allocator& _a) const
    {
        rapidjson::Value status_node(rapidjson::Type::kObjectType);

        status_node.AddMember("status", status_, _a);
        status_node.AddMember("text", description_, _a);
        status_node.AddMember("startTime", to_seconds(start_time_), _a);

        if (end_time_)
            status_node.AddMember("endTime", to_seconds(*end_time_), _a);
        status_node.AddMember("isSending", my_status_sending_, _a);

        _node.AddMember("status", std::move(status_node), _a);
    }

    bool status::unserialize(const core::coll_helper& _coll)
    {
        status_ = _coll.get_value_as_string("status");
        description_ = _coll.get_value_as_string("description", "");
        start_time_ = from_seconds(_coll.get_value_as_int64("startTime"));
        if (_coll.is_value_exist("endTime"))
            end_time_ = from_seconds(_coll.get_value_as_int64("endTime"));
        return true;
    }

    bool status::unserialize(const rapidjson::Value& _node)
    {
        if (!tools::unserialize_value(_node, "status", status_) && !tools::unserialize_value(_node, "media", status_))
            return false;

        tools::unserialize_value(_node, "text", description_);

        if (int64_t secs = 0; tools::unserialize_value(_node, "startTime", secs))
            start_time_ = from_seconds(secs);

        if (int64_t secs = 0; tools::unserialize_value(_node, "endTime", secs))
            end_time_ = from_seconds(secs);

        tools::unserialize_value(_node, "isSending", my_status_sending_);
        return true;
    }

    bool status::is_empty() const
    {
        return status_.empty();
    }
}
