#include "stdafx.h"
#include "../../wim/wim_packet.h"
#include "../../wim/wim_im.h"
#include "../../../smartreply/smartreply_suggest.h"
#include "fetch_event_smartreply_suggests.h"

using namespace core;
using namespace wim;

fetch_event_smartreply_suggest::fetch_event_smartreply_suggest()
    : suggests_(std::make_shared<std::vector<smartreply::suggest>>())
{
}

fetch_event_smartreply_suggest::~fetch_event_smartreply_suggest() = default;

int32_t fetch_event_smartreply_suggest::parse(const rapidjson::Value& _node_event_data)
{
    auto res = smartreply::unserialize_suggests_node(_node_event_data);
    if (!res.empty())
        std::move(res.begin(), res.end(), std::back_inserter(*suggests_));

    return 0;
}

void core::wim::fetch_event_smartreply_suggest::on_im(std::shared_ptr<core::wim::im> _im, std::shared_ptr<auto_callback> _on_complete)
{
    _im->on_event_smartreply_suggests(this, _on_complete);
}