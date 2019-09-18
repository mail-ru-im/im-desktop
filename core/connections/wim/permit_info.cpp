#include "stdafx.h"
#include "permit_info.h"
#include "wim_packet.h"

using namespace core;
using namespace wim;

permit_info::permit_info()
    : persons_(std::make_shared<core::archive::persons_map>())
{
}

permit_info::~permit_info()
{

}

int32_t permit_info::parse_response_data(const rapidjson::Value& _node_results)
{
    const auto ignores = _node_results.FindMember("ignores");
    const bool has_ignores = ignores != _node_results.MemberEnd() && ignores->value.IsArray();

    const auto blocks = _node_results.FindMember("blocks");
    const bool has_blocks = blocks != _node_results.MemberEnd() && blocks->value.IsArray();

    if (!has_blocks && !has_ignores)
        return wpie_http_parse_response;

    if (has_blocks)
        for (const auto& x : blocks->value.GetArray())
            ignore_aimid_list_.emplace(rapidjson_get_string(x));

    if (has_ignores)
        for (const auto& x : ignores->value.GetArray())
            ignore_aimid_list_.emplace(rapidjson_get_string(x));

    *persons_ = parse_persons(_node_results);

    return 0;
}

const ignorelist_cache& permit_info::get_ignore_list() const
{
    return ignore_aimid_list_;
}
