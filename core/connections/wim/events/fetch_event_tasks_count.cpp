#include "stdafx.h"

#include "fetch_event_tasks_count.h"
#include "../common.shared/json_helper.h"
#include "../wim_im.h"

namespace core::wim
{
    int32_t fetch_event_tasks_count::parse(const rapidjson::Value& _node_event_data)
    {
        tools::unserialize_value(_node_event_data, "counter", tasks_);
        return 0;
    }

    void fetch_event_tasks_count::on_im(std::shared_ptr<core::wim::im> _im, std::shared_ptr<auto_callback> _on_complete)
    {
        _im->on_event_tasks_count(this, std::move(_on_complete));
    }
}