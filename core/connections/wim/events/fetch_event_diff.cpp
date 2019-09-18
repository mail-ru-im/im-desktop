#include "stdafx.h"

#include "fetch_event_diff.h"
#include "../wim_im.h"
#include "../wim_packet.h"
#include "../wim_contactlist_cache.h"
#include "tools/json_helper.h"

using namespace core;
using namespace wim;

fetch_event_diff::fetch_event_diff()
    : diff_(std::make_shared<diffs_map>()),
    persons_(std::make_shared<core::archive::persons_map>())
{
}


fetch_event_diff::~fetch_event_diff() = default;

int32_t fetch_event_diff::parse(const rapidjson::Value& _node_event_data)
{
    try
    {
        if (!_node_event_data.IsArray())
            return wpie_error_parse_response;

        for (const auto& x : _node_event_data.GetArray())
        {
            std::string type;
            tools::unserialize_value(x, "type", type);

            if (const auto it = x.FindMember("data"); it != x.MemberEnd() && it->value.IsArray())
            {
                auto cl = std::make_shared<contactlist>();
                cl->unserialize_from_diff(it->value);
                const auto current_persons = cl->get_persons();
                persons_->insert(current_persons->begin(), current_persons->end());
                diff_->insert(std::make_pair(std::move(type), std::move(cl)));
            }
        }
    }
    catch (const std::exception&)
    {
        return wpie_error_parse_response;
    }

    return 0;
}

void fetch_event_diff::on_im(std::shared_ptr<core::wim::im> _im, std::shared_ptr<auto_callback> _on_complete)
{
    _im->on_event_diff(this, _on_complete);
}
