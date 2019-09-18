#include "stdafx.h"

#include "fetch_event_typing.h"
#include "../wim_im.h"

using namespace core;
using namespace wim;

fetch_event_typing::fetch_event_typing() = default;

fetch_event_typing::~fetch_event_typing() = default;

int32_t fetch_event_typing::parse(const rapidjson::Value& _node_event_data)
{
    if (const auto it = _node_event_data.FindMember("typingStatus"); it != _node_event_data.MemberEnd() && it->value.IsString())
        is_typing_ = rapidjson_get_string_view(it->value) == "typing";

    if (const auto it = _node_event_data.FindMember("aimId"); it != _node_event_data.MemberEnd() && it->value.IsString())
        aimid_ = rapidjson_get_string(it->value);
    else
        return -1;

    if (const auto it_attr = _node_event_data.FindMember("MChat_Attrs"); it_attr != _node_event_data.MemberEnd() && it_attr->value.IsObject())
    {
        if (const auto it = it_attr->value.FindMember("sender"); it != it_attr->value.MemberEnd() && it->value.IsString())
            chatter_aimid_ = rapidjson_get_string(it->value);

        if (const auto itoptions = it_attr->value.FindMember("options"); itoptions != it_attr->value.MemberEnd() && itoptions->value.IsObject())
        {
            if (const auto itname = itoptions->value.FindMember("senderName"); itname != itoptions->value.MemberEnd() && itname->value.IsString())
                chatter_name_ = rapidjson_get_string(itname->value);
        }
    }

    return 0;
}

void fetch_event_typing::on_im(std::shared_ptr<core::wim::im> _im, std::shared_ptr<auto_callback> _on_complete)
{
    _im->on_event_typing(this, _on_complete);
}
