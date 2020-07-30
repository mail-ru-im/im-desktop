#include "stdafx.h"

#include "fetch_event_imstate.h"
#include "../wim_im.h"
#include "../wim_packet.h"
#include "../../../tools/json_helper.h"

using namespace core;
using namespace wim;

fetch_event_imstate::fetch_event_imstate() = default;

fetch_event_imstate::~fetch_event_imstate() = default;

const imstate_list& fetch_event_imstate::get_states() const
{
    return states_;
}

int32_t fetch_event_imstate::parse(const rapidjson::Value& _node_event_data)
{
    const auto iter_imstates = _node_event_data.FindMember("imStates");
    if  (iter_imstates == _node_event_data.MemberEnd() || !iter_imstates->value.IsArray())
        return wpie_error_parse_response;

    states_.reserve(iter_imstates->value.Size());

    for (const auto& st : iter_imstates->value.GetArray())
    {
        imstate ustate;

        if (std::string req_id; tools::unserialize_value(st, "sendReqId", req_id))
            ustate.set_request_id(std::move(req_id));
        else
            continue;

        if (std::string msg_id; tools::unserialize_value(st, "msgId", msg_id))
            ustate.set_msg_id(std::move(msg_id));
        else
            continue;

        if (std::string_view state; tools::unserialize_value(st, "state", state))
        {
            if (state == "failed")
                ustate.set_state(imstate_sent_state::failed);
            else if (state == "sent")
                ustate.set_state(imstate_sent_state::sent);
            else if (state == "delivered")
                ustate.set_state(imstate_sent_state::delivered);
        }
        else
        {
            continue;
        }

        if (int64_t msg_id = 0; tools::unserialize_value(st, "histMsgId", msg_id))
            ustate.set_hist_msg_id(msg_id);

        if (int64_t msg_id = 0; tools::unserialize_value(st, "beforeHistMsgId", msg_id))
            ustate.set_before_hist_msg_id(msg_id);

        if (int error = 0; tools::unserialize_value(st, "errorCode", error))
            ustate.set_error_code(error);

        states_.push_back(std::move(ustate));
    }

    return 0;
}

void fetch_event_imstate::on_im(std::shared_ptr<core::wim::im> _im, std::shared_ptr<auto_callback> _on_complete)
{
    _im->on_event_imstate(this, _on_complete);
}
