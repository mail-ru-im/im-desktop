#include "stdafx.h"
#include "fetch_event_trust_status.h"
#include "../common.shared/json_helper.h"
#include "../wim_im.h"
#include "../wim_packet.h"
#include "tools/features.h"

namespace core::wim
{
    fetch_event_trust_status::fetch_event_trust_status()
        : trusted_(features::trust_status_default())
    {
    }

    int32_t fetch_event_trust_status::parse(const rapidjson::Value& _node_event_data)
    {
        if (tools::unserialize_value(_node_event_data, "trusted", trusted_))
            return 0;

        return wpie_error_parse_response;
    }

    void fetch_event_trust_status::on_im(std::shared_ptr<core::wim::im> _im, std::shared_ptr<auto_callback> _on_complete)
    {
        _im->on_event_trust_status(this, _on_complete);
    }
}