#include "stdafx.h"

#include "fetch_event_task.h"

#include "../../wim/wim_packet.h"
#include "../../wim/wim_im.h"
#include "../common.shared/json_helper.h"

using namespace core;
using namespace wim;

int32_t fetch_event_task::parse(const rapidjson::Value& _node_event_data)
{
    if (!task_.unserialize(_node_event_data))
        return wpie_error_parse_response;

    return 0;
}

void fetch_event_task::on_im(std::shared_ptr<core::wim::im> _im, std::shared_ptr<auto_callback> _on_complete)
{
    _im->on_event_task(this, _on_complete);
}
