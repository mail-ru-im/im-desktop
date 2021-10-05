#include "stdafx.h"
#include "fetch_event_draft.h"
#include "../../../common.shared/json_helper.h"
#include "../wim_im.h"

namespace core::wim
{

int32_t fetch_event_draft::parse(const rapidjson::Value& _node_event_data)
{
    tools::unserialize_value(_node_event_data, "sn", contact_);
    persons_ = parse_persons(_node_event_data);
    draft_.friendly_ = persons_[contact_].friendly_;

    if (const auto it = _node_event_data.FindMember("parentTopic"); it != _node_event_data.MemberEnd())
    {
        if (archive::thread_parent_topic topic; topic.unserialize(it->value))
            parent_topic_ = std::move(topic);
    }

    return draft_.unserialize(_node_event_data, contact_);
}

void fetch_event_draft::on_im(std::shared_ptr<im> _im, std::shared_ptr<auto_callback> _on_complete)
{
    _im->on_event_draft(this, _on_complete);
}

}
