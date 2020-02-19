#include "stdafx.h"

#include "suggest_storage.h"
#include "smartreply_suggest.h"

namespace core
{
    namespace smartreply
    {
        void suggest_storage::add_one(suggest&& _suggest)
        {
            const auto& aimid = _suggest.get_aimid();
            if (aimid.empty())
                return;

            auto& contact_suggests = suggests_[aimid];
            if (!contact_suggests.empty() && contact_suggests.front().get_msgid() != _suggest.get_msgid())
            {
                if (contact_suggests.front().get_msgid() > _suggest.get_msgid())
                    return;

                contact_suggests.clear();
            }

            contact_suggests.push_back(std::move(_suggest));

            set_changed(true);
        }

        void suggest_storage::add_pack(const suggest_v& _suggests)
        {
            if (_suggests.empty())
                return;

            const auto& aimid = _suggests.front().get_aimid();
            if (aimid.empty())
                return;

            auto& contact_suggests = suggests_[aimid];
            if (!contact_suggests.empty())
            {
                if (contact_suggests.front().get_msgid() != _suggests.front().get_msgid())
                {
                    if (contact_suggests.front().get_msgid() > _suggests.front().get_msgid())
                        return;

                    contact_suggests.clear();
                }
                else
                {
                    const auto all_already = std::all_of(_suggests.begin(), _suggests.end(), [&contact_suggests](const auto& s)
                    {
                        return std::any_of(contact_suggests.begin(), contact_suggests.end(), [&s](const auto cs) { return cs.get_type() == s.get_type() && cs.get_data() == s.get_data(); });
                    });

                    if (all_already)
                        return;

                    contact_suggests.erase(
                        std::remove_if(contact_suggests.begin(), contact_suggests.end(), [t = _suggests.front().get_type()](const auto& s) { return s.get_type() == t; }),
                        contact_suggests.end()
                    );
                }
            }

            std::copy(_suggests.begin(), _suggests.end(), std::back_inserter(contact_suggests));

            set_changed(true);
        }

        const suggest_v& suggest_storage::get_suggests(const std::string_view _aimid) const
        {
            if (const auto it = suggests_.find(_aimid); it != suggests_.end())
                return it->second;

            static const suggest_v empty;
            return empty;
        }

        void suggest_storage::clear_all()
        {
            if (!suggests_.empty())
            {
                suggests_.clear();
                set_changed(true);
            }
        }

        void suggest_storage::clear_for(const std::string_view _aimid)
        {
            if (const auto it = suggests_.find(_aimid); it != suggests_.end())
            {
                suggests_.erase(it);
                set_changed(true);
            }
        }

        int32_t suggest_storage::unserialize(const rapidjson::Value& _node)
        {
            const auto iter_suggests = _node.FindMember("instant_suggests");
            if (iter_suggests == _node.MemberEnd() || !iter_suggests->value.IsArray())
                return -1;

            for (const auto& s_node : iter_suggests->value.GetArray())
            {
                for (auto&& s : unserialize_suggests_node(s_node))
                    add_one(std::move(s));
            }

            return 0;
        }

        void suggest_storage::serialize(rapidjson::Value& _node, rapidjson_allocator& _a) const
        {
            rapidjson::Value node_suggests(rapidjson::Type::kArrayType);
            node_suggests.Reserve(suggests_.size(), _a);
            for (const auto& it : suggests_)
            {
                for (const auto& s : it.second)
                {
                    rapidjson::Value suggest_node(rapidjson::Type::kObjectType);
                    s.serialize(suggest_node, _a);
                    node_suggests.PushBack(std::move(suggest_node), _a);
                }
            }

            _node.AddMember("instant_suggests", std::move(node_suggests), _a);
        }
    }
}