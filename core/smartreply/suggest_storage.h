#pragma once

#include "tools/string_comparator.h"

namespace core
{
    namespace smartreply
    {
        class suggest;

        using suggest_v = std::vector<suggest>;

        class suggest_storage
        {
        public:
            suggest_storage() = default;

            void add_one(suggest&& _suggest);
            void add_pack(const suggest_v& _suggests);

            const suggest_v& get_suggests(const std::string_view _aimid) const;
            void clear_all();
            void clear_for(const std::string_view _aimid);

            void set_changed(const bool _changed) { changed_ = _changed; }
            bool is_changed() const noexcept { return changed_; }

            int32_t unserialize(const rapidjson::Value& _node);
            void serialize(rapidjson::Value& _node, rapidjson_allocator& _a) const;

        private:
            std::map<std::string, suggest_v, core::tools::string_comparator> suggests_;
            bool changed_ = false;
        };
    }
}