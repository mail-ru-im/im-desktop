#include "stdafx.h"

#include "fetch_event_appsdata.h"

#include "../wim_im.h"

using namespace core;
using namespace wim;

fetch_event_appsdata::fetch_event_appsdata() = default;

fetch_event_appsdata::~fetch_event_appsdata() = default;

int32_t fetch_event_appsdata::parse(const rapidjson::Value& _node_event_data)
{
    const auto iter_appsdata = _node_event_data.FindMember("appsData");
    if (iter_appsdata != _node_event_data.MemberEnd() && iter_appsdata->value.IsObject())
    {
        const auto iter_store = iter_appsdata->value.FindMember("store");
        if (iter_store != iter_appsdata->value.MemberEnd() && iter_store->value.IsObject())
            refresh_stickers_ = true;
    }

    return 0;
}

void fetch_event_appsdata::on_im(std::shared_ptr<core::wim::im> _im, std::shared_ptr<auto_callback> _on_complete)
{
    _im->on_event_appsdata(this, _on_complete);
}

bool fetch_event_appsdata::is_refresh_stickers() const
{
    return refresh_stickers_;
}
