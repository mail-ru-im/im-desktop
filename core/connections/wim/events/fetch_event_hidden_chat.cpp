#include "stdafx.h"

#include "fetch_event_hidden_chat.h"
#include "../wim_im.h"
#include "../wim_packet.h"
#include "../../../archive/archive_index.h"
#include "../../../tools/json_helper.h"

using namespace core;
using namespace wim;

fetch_event_hidden_chat::fetch_event_hidden_chat() = default;

fetch_event_hidden_chat::~fetch_event_hidden_chat() = default;

int32_t fetch_event_hidden_chat::parse(const rapidjson::Value& _node_event_data)
{
    try
    {
        const auto iter_aimid = _node_event_data.FindMember("aimId");
        const auto iter_last_msg_id = _node_event_data.FindMember("lastMsgId");

        if (
            iter_aimid == _node_event_data.MemberEnd() ||
            iter_last_msg_id == _node_event_data.MemberEnd() ||
            !iter_aimid->value.IsString() ||
            !iter_last_msg_id->value.IsInt64())
            return wpie_error_parse_response;


        aimid_ = rapidjson_get_string(iter_aimid->value);
        last_msg_id_ = iter_last_msg_id->value.GetInt64();
    }
    catch (const std::exception&)
    {
        return wpie_error_parse_response;
    }

    return 0;
}

void fetch_event_hidden_chat::on_im(std::shared_ptr<core::wim::im> _im, std::shared_ptr<auto_callback> _on_complete)
{
    _im->on_event_hidden_chat(this, _on_complete);
}
