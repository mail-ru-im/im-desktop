#include "stdafx.h"
#include "json_helper.h"

namespace core::tools
{
    void sort_json_keys_by_name(rapidjson::Value& _node)
    {
        auto members_comparator = [](const auto& _lhs, const auto& _rhs) noexcept
        {
            return rapidjson_get_string_view(_lhs.name) < rapidjson_get_string_view(_rhs.name);
        };

        std::sort(_node.MemberBegin(), _node.MemberEnd(), members_comparator);

        for (auto& member : _node.GetObject())
        {
            if (member.value.IsObject())
            {
                sort_json_keys_by_name(member.value);
            }
            else if (member.value.IsArray())
            {
                for (auto& array_member : member.value.GetArray())
                {
                    if (array_member.IsObject())
                        sort_json_keys_by_name(array_member);
                }
            }
        }
    }
}
