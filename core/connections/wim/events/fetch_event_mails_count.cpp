#include "stdafx.h"

#include "fetch_event_mails_count.h"
#include "../common.shared/json_helper.h"
#include "../wim_im.h"

namespace core::wim
{
    int32_t fetch_event_mails_count::parse(const rapidjson::Value& _node_event_data)
    {
        tools::unserialize_value(_node_event_data, "counter", mails_);
        return 0;
    }

    void fetch_event_mails_count::on_im(std::shared_ptr<core::wim::im> _im, std::shared_ptr<auto_callback> _on_complete)
    {
        _im->on_event_mails_count(this, std::move(_on_complete));
    }
}