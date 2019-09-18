#include "stdafx.h"

#include "fetch_event_gallery_notify.h"
#include "../wim_im.h"

using namespace core;
using namespace wim;

fetch_event_gallery_notify::fetch_event_gallery_notify(const std::string& _my_aimid)
    : my_aimid_(_my_aimid)
{
}

fetch_event_gallery_notify::~fetch_event_gallery_notify() = default;

const archive::gallery_storage& fetch_event_gallery_notify::get_gallery() const
{
    return gallery_;
}

int32_t fetch_event_gallery_notify::parse(const rapidjson::Value& _node_event_data)
{
    auto iter_sn = _node_event_data.FindMember("sn");
    if (iter_sn == _node_event_data.MemberEnd() || !iter_sn->value.IsString())
        return 1;

    gallery_.set_aimid(iter_sn->value.GetString());
    gallery_.set_my_aimid(my_aimid_);

    return gallery_.unserialize(_node_event_data, false);
}

void fetch_event_gallery_notify::on_im(std::shared_ptr<core::wim::im> _im, std::shared_ptr<auto_callback> _on_complete)
{
    _im->on_event_gallery_notify(this, _on_complete);
}
