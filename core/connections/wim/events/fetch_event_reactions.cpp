#include "stdafx.h"

#include "../wim_im.h"
#include "fetch_event_reactions.h"

namespace core::wim
{

int32_t fetch_event_reactions::parse(const rapidjson::Value& _node_event_data)
{
    return reactions_.unserialize(_node_event_data);
}

void fetch_event_reactions::on_im(std::shared_ptr<im> _im, std::shared_ptr<auto_callback> _on_complete)
{
    _im->on_event_reactions(this, _on_complete);
}

const archive::reactions_data& fetch_event_reactions::get_reactions() const
{
    return reactions_;
}

}
