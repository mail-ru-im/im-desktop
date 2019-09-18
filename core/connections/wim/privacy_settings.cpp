#include "stdafx.h"

#include "privacy_settings.h"
#include "../../tools/json_helper.h"

namespace core::wim
{
    void privacy_group::unserialize(const rapidjson::Value& _node)
    {
        if (std::string_view allow; tools::unserialize_value(_node, "allowTo", allow))
            access_ = from_string(allow);
    }

    void privacy_group::serialize(rapidjson::Value& _rootNode, rapidjson_allocator& _a) const
    {
        if (has_values_set())
        {
            rapidjson::Value node_group(rapidjson::Type::kObjectType);
            node_group.AddMember("allowTo", to_string(access_), _a);
            _rootNode.AddMember(rapidjson::Value(group_name_, _a), std::move(node_group), _a);
        }
    }

    void privacy_group::serialize(core::coll_helper& _root_coll) const
    {
        if (has_values_set())
        {
            _root_coll.set_value_as_string("name", group_name_);
            _root_coll.set_value_as_enum("allowTo", access_);
        }
    }

    bool privacy_group::has_values_set() const noexcept
    {
        return access_ != privacy_access_right::not_set;
    }

    privacy_settings::privacy_settings()
        : groups_({ privacy_group("lastseen"), privacy_group("calls"), privacy_group("groups") })
    {
    }

    void privacy_settings::set_access_value(const std::string_view _group, const privacy_access_right _value)
    {
        auto grp_it = std::find_if(groups_.begin(), groups_.end(), [_group](const auto& _g) { return _g.group_name_ == _group; });
        if (grp_it != groups_.end())
            grp_it->access_ = _value;
    }

    int32_t privacy_settings::unserialize(const rapidjson::Value& _node)
    {
        for (const auto& it : _node.GetObject())
        {
            auto grp_it = std::find_if(groups_.begin(), groups_.end(), [name = rapidjson_get_string_view(it.name)](const auto& _g) { return _g.group_name_ == name; });
            if (grp_it != groups_.end())
                grp_it->unserialize(it.value);
        }
        return 0;
    }

    void privacy_settings::serialize(rapidjson::Value& _rootNode, rapidjson_allocator& _a) const
    {
        for (const auto& g : groups_)
            g.serialize(_rootNode, _a);
    }

    void privacy_settings::serialize(core::coll_helper& _coll) const
    {
        ifptr<iarray> array(_coll->create_array());
        array->reserve((int32_t)groups_.size());

        for (const auto& g : groups_)
        {
            if (g.has_values_set())
            {
                coll_helper coll_grp(_coll->create_collection(), true);
                g.serialize(coll_grp);

                ifptr<ivalue> val(_coll->create_value());
                val->set_as_collection(coll_grp.get());
                array->push_back(val.get());
            }
        }
        _coll.set_value_as_array("privacy_groups", array.get());
    }

    bool privacy_settings::has_values_set() const noexcept
    {
        return std::any_of(groups_.begin(), groups_.end(), [](const auto& _g) { return _g.has_values_set(); });
    }
}