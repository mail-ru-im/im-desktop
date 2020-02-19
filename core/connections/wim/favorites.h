#pragma once

#include "friendly/friendly.h"
#include "tools/string_comparator.h"

namespace core
{
    struct icollection;

    namespace wim
    {
        class favorite
        {
            std::string	aimid_;
            int64_t time_ = 0;

            friendly friendly_;

        public:
            favorite();
            favorite(const std::string& _aimid, friendly _friendly, const int64_t _time);

            const std::string& get_aimid() const { return aimid_; }
            const friendly& get_friendly() const { return friendly_; }
            int64_t get_time() const { return time_; }

            int32_t unserialize(const rapidjson::Value& _node);
            void serialize(rapidjson::Value& _node, rapidjson_allocator& _a) const;
        };

        class favorites
        {
            bool changed_ = false;

            std::vector<favorite>	contacts_;
            std::map<std::string, int64_t, tools::string_comparator> index_;

        public:
            size_t size() const noexcept { return contacts_.size(); }

            bool is_changed() const { return changed_; }
            void set_changed(bool _changed) { changed_ = _changed; }

            const std::vector<favorite>& contacts() const { return contacts_; }

            favorites();
            virtual ~favorites();

            void update(favorite& _contact);
            void remove(const std::string_view _aimid);
            bool contains(const std::string_view _aimid) const;
            int64_t get_time(const std::string_view _aimid) const;

            int32_t unserialize(const rapidjson::Value& _node);
            void serialize(rapidjson::Value& _node, rapidjson_allocator& _a);
        };

    }
}