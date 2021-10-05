#include "stdafx.h"

#include "fetch_event_thread_update.h"
#include "../wim_im.h"

namespace core::wim
{
    int32_t fetch_event_thread_update::parse(const rapidjson::Value& _node_event_data)
    {
        return update_.unserialize(_node_event_data);
    }

    void fetch_event_thread_update::on_im(std::shared_ptr<core::wim::im> _im, std::shared_ptr<auto_callback> _on_complete)
    {
        _im->on_event_thread_update(this, std::move(_on_complete));
    }
}