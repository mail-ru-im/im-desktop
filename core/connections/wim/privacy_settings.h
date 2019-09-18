#pragma once

#include "../../../corelib/collection_helper.h"
#include "../../../corelib/enumerations.h"

namespace core::wim
{
    struct privacy_group
    {
        std::string group_name_;
        privacy_access_right access_ = privacy_access_right::not_set;

        privacy_group(const std::string_view _group_name) : group_name_(_group_name) {}

        void unserialize(const rapidjson::Value& _node);
        void serialize(rapidjson::Value& _rootNode, rapidjson_allocator& _a) const;
        void serialize(core::coll_helper& _root_coll) const;

        bool has_values_set() const noexcept;
    };

    struct privacy_settings
    {
        std::vector<privacy_group> groups_;

        privacy_settings();

        void set_access_value(const std::string_view _group, const privacy_access_right _value);

        int32_t unserialize(const rapidjson::Value& _node);
        void serialize(rapidjson::Value& _rootNode, rapidjson_allocator& _a) const;
        void serialize(core::coll_helper& _coll) const;

        bool has_values_set() const noexcept;
    };
}