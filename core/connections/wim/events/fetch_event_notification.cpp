#include "stdafx.h"

#include "fetch_event_notification.h"
#include "../wim_im.h"
#include "../wim_packet.h"
#include "../../../tools/json_helper.h"

using namespace core;
using namespace wim;

static constexpr std::string_view active_dialogs_value() noexcept
{
    return "sent";
}

fetch_event_notification::fetch_event_notification() = default;


fetch_event_notification::~fetch_event_notification() = default;

int32_t fetch_event_notification::parse(const rapidjson::Value& _node_event_data)
{
    if (const auto iter_fields = _node_event_data.FindMember("fields"); iter_fields != _node_event_data.MemberEnd() && iter_fields->value.IsArray())
    {
        for (const auto& field : iter_fields->value.GetArray())
        {
            if(!mailbox_storage::unserialize(field, changes_)) // if not mailbox notification check active dialogs notification
            {
                if (const auto active_dialogs = field.FindMember("activeDialogs"); active_dialogs != field.MemberEnd() && active_dialogs->value.IsString())
                {
                    if (rapidjson_get_string_view(active_dialogs->value) == active_dialogs_value())
                        active_dialogs_sent_ = true;
                }
            }
        }
    }
    return 0;
}

void fetch_event_notification::on_im(std::shared_ptr<core::wim::im> _im, std::shared_ptr<auto_callback> _on_complete)
{
    _im->on_event_notification(this, _on_complete);
}