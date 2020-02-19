#include "stdafx.h"

#include "../wim_im.h"
#include "fetch_event_poll_update.h"



namespace core::wim
{

int32_t fetch_event_poll_update::parse(const rapidjson::Value& _node_event_data)
{
    poll_.unserialize(_node_event_data);

    return 0;
}

void fetch_event_poll_update::on_im(std::shared_ptr<core::wim::im> _im, std::shared_ptr<core::auto_callback> _on_complete)
{
    _im->on_event_poll_update(this, _on_complete);
}

const archive::poll_data& fetch_event_poll_update::get_poll() const
{
    return poll_;
}

}
